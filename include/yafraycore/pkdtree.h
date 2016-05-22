#ifndef Y_PKDTREE_H
#define Y_PKDTREE_H

#include <yafray_config.h>

#include <utilities/y_alloc.h>
#include <core_api/bound.h>
#include <algorithm>
#include <vector>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>

__BEGIN_YAFRAY

namespace kdtree {

#define KD_MAX_STACK 64
#define NON_REC_LOOKUP 1

template <class T>
struct kdNode
{
	void createLeaf(const T *d)
	{
		flags = 3;
		data = d;
	}
	void createInterior(int axis, PFLOAT d)
	{
		division = d;
		flags = (flags & ~3) | axis;
	}
	PFLOAT 	SplitPos() const { return division; }
	int 	SplitAxis() const { return flags & 3; }
	int 	nPrimitives() const { return flags >> 2; }
	bool 	IsLeaf() const { return (flags & 3) == 3; }
	u_int32	getRightChild() const { return (flags >> 2); }
	void 	setRightChild(u_int32 i) { flags = (flags&3) | (i << 2); }
	union
	{
		PFLOAT division;
		const T *data;
	};
	u_int32	flags;

	friend class boost::serialization::access;
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(flags);
		if(IsLeaf()) ar & BOOST_SERIALIZATION_NVP(data);
		else ar & BOOST_SERIALIZATION_NVP(division);
	}
};

template<class NodeData> struct CompareNode
{
	CompareNode(int a) { axis = a; }
	int axis;
	bool operator()(const NodeData *d1,	const NodeData *d2) const
	{
		return d1->pos[axis] == d2->pos[axis] ? (d1 < d2) : d1->pos[axis] < d2->pos[axis];
	}
};

template <class T>
class pointKdTree
{
	public:
		pointKdTree() {};
		pointKdTree(const std::vector<T> &dat, const std::string &mapName, int numThreads=1);
		~pointKdTree(){ if(nodes) y_free(nodes); }
		template<class LookupProc> void lookup(const point3d_t &p, const LookupProc &proc, PFLOAT &maxDistSquared) const;
		double lookupStat()const{ return double(Y_PROCS)/double(Y_LOOKUPS); } //!< ratio of photons tested per lookup call
	protected:
		template<class LookupProc> void recursiveLookup(const point3d_t &p, const LookupProc &proc, PFLOAT &maxDistSquared, int nodeNum) const;
		struct KdStack
		{
			const kdNode<T> *node; //!< pointer to far child
			PFLOAT s; 		//!< the split val of parent node
			int axis; 		//!< the split axis of parent node
		};
		void buildTree(u_int32 start, u_int32 end, bound_t &nodeBound, const T **prims);
		void buildTreeWorker(u_int32 start, u_int32 end, bound_t &nodeBound, const T **prims, int level, uint32_t & localNextFreeNode, kdNode<T> * localNodes);
		kdNode<T> *nodes;
		u_int32 nElements, nextFreeNode;
		bound_t treeBound;
		mutable unsigned int Y_LOOKUPS, Y_PROCS;
		int maxLevelThreads = 0;  //max level where we will launch threads. We will try to launch at least as many threads as scene threads parameter
		std::mutex mutx;

		friend class boost::serialization::access;
		template<class Archive> void save(Archive & ar, const unsigned int version) const
		{
			ar & BOOST_SERIALIZATION_NVP(nElements);
			ar & BOOST_SERIALIZATION_NVP(nextFreeNode);
			ar & BOOST_SERIALIZATION_NVP(treeBound);
			ar & BOOST_SERIALIZATION_NVP(Y_LOOKUPS);
			ar & BOOST_SERIALIZATION_NVP(Y_PROCS);
			ar & boost::serialization::make_array(nodes, nextFreeNode);
		}
		
		friend class boost::serialization::access;
		template<class Archive> void load(Archive & ar, const unsigned int version)
		{
			ar & BOOST_SERIALIZATION_NVP(nElements);
			ar & BOOST_SERIALIZATION_NVP(nextFreeNode);
			ar & BOOST_SERIALIZATION_NVP(treeBound);
			ar & BOOST_SERIALIZATION_NVP(Y_LOOKUPS);
			ar & BOOST_SERIALIZATION_NVP(Y_PROCS);	
			nodes = (kdNode<T> *)y_memalign(64, 4*nElements*sizeof(kdNode<T>)); //actually we could allocate one less...2n-1
			ar & boost::serialization::make_array(nodes, nextFreeNode);
		}
		
		BOOST_SERIALIZATION_SPLIT_MEMBER()
};

template<class T>
pointKdTree<T>::pointKdTree(const std::vector<T> &dat, const std::string &mapName, int numThreads)
{
	Y_LOOKUPS=0; Y_PROCS=0;
	nextFreeNode = 0;
	nElements = dat.size();
	
	if(nElements == 0)
	{
		Y_ERROR << "pointKdTree: " << mapName << " empty vector!" << yendl;
		return;
	}
	
	nodes = (kdNode<T> *)y_memalign(64, 4*nElements*sizeof(kdNode<T>)); //actually we could allocate one less...2n-1
	
	const T **elements = new const T*[nElements];
	
	for(u_int32 i=0; i<nElements; ++i) elements[i] = &dat[i];
	
	treeBound.set(dat[0].pos, dat[0].pos);
	
	for(u_int32 i=1; i<nElements; ++i) treeBound.include(dat[i].pos);
	
	maxLevelThreads = (int) std::ceil(std::log2((float) numThreads)); //in how many pkdtree levels we will spawn threads, so we create at least as many threads as scene threads parameter (or more)
	int realThreads = (int) pow(2.f, maxLevelThreads); //real amount of threads we will create during pkdtree creation depending on the maximum level where we will generate threads
	
	Y_INFO << "pointKdTree: Starting " << mapName << " recusive tree build for " << nElements << " elements [using " << realThreads << " threads]" << yendl;

	buildTree(0, nElements, treeBound, elements);
	
	Y_VERBOSE << "pointKdTree: " << mapName << " tree built." << yendl;

	delete[] elements;
}

template<class T>
void pointKdTree<T>::buildTree(u_int32 start, u_int32 end, bound_t &nodeBound, const T **prims)
{
	buildTreeWorker(start, end, nodeBound, prims, 0, nextFreeNode, nodes);
}

template<class T>
void pointKdTree<T>::buildTreeWorker(u_int32 start, u_int32 end, bound_t &nodeBound, const T **prims, int level, uint32_t & localNextFreeNode, kdNode<T> * localNodes)
{
	++level;
	if(end - start == 1)
	{
		localNodes[localNextFreeNode].createLeaf(prims[start]);
		localNextFreeNode++;
		--level;
		return;
	}
	int splitAxis = nodeBound.largestAxis();
	int splitEl = (start+end)/2;
	std::nth_element(&prims[start], &prims[splitEl],
					&prims[end], CompareNode<T>(splitAxis));
	u_int32 curNode = localNextFreeNode;
	PFLOAT splitPos = prims[splitEl]->pos[splitAxis];
	localNodes[curNode].createInterior(splitAxis, splitPos);
	++localNextFreeNode;
	bound_t boundL = nodeBound, boundR = nodeBound;
	switch(splitAxis){
		case 0: boundL.setMaxX(splitPos); boundR.setMinX(splitPos); break;
		case 1: boundL.setMaxY(splitPos); boundR.setMinY(splitPos); break;
		case 2: boundL.setMaxZ(splitPos); boundR.setMinZ(splitPos); break;
	}
	
	if (level <= maxLevelThreads)  //launch threads for the first "x" levels to try to match (at least) the scene threads parameter
	{
		//<< recurse below child >>
		uint32_t nextFreeNode1 = 0;
		kdNode<T> * nodes1 = (kdNode<T> *)y_memalign(64, 4 * (splitEl - start)*sizeof(kdNode<T>));
		std::thread * belowWorker = new std::thread(&pointKdTree<T>::buildTreeWorker, this, start, splitEl, std::ref(boundL), prims, level, std::ref(nextFreeNode1), nodes1);

		//<< recurse above child >>
		uint32_t nextFreeNode2 = 0;
		kdNode<T> * nodes2 = (kdNode<T> *)y_memalign(64, 4 * (end - splitEl)*sizeof(kdNode<T>));
		std::thread * aboveWorker = new std::thread(&pointKdTree<T>::buildTreeWorker, this, splitEl, end, std::ref(boundR), prims, level, std::ref(nextFreeNode2), nodes2);

		belowWorker->join();
		aboveWorker->join();
		delete belowWorker;
		delete aboveWorker;

		if (nodes1)
		{
			for (uint32_t i = 0; i < nextFreeNode1; ++i)
			{
				localNodes[i + localNextFreeNode] = nodes1[i];
				if (!localNodes[i + localNextFreeNode].IsLeaf())
				{
					uint32_t right_child = localNodes[i + localNextFreeNode].getRightChild();
					localNodes[i + localNextFreeNode].setRightChild(right_child + localNextFreeNode);
				}
			}
			y_free(nodes1);
		}

		if (nodes2)
		{
			for (uint32_t i = 0; i < nextFreeNode2; ++i)
			{
				localNodes[i + localNextFreeNode + nextFreeNode1] = nodes2[i];
				if (!localNodes[i + localNextFreeNode + nextFreeNode1].IsLeaf())
				{
					uint32_t right_child = localNodes[i + localNextFreeNode + nextFreeNode1].getRightChild();
					localNodes[i + localNextFreeNode + nextFreeNode1].setRightChild(right_child + localNextFreeNode + nextFreeNode1);
				}
			}
			y_free(nodes2);
		}

		localNodes[curNode].setRightChild (localNextFreeNode + nextFreeNode1);
		localNextFreeNode = localNextFreeNode + nextFreeNode1 + nextFreeNode2;
	}
	else  //for the rest of the levels in the tree, don't launch more threads, do normal "sequential" operation
	{
		//<< recurse below child >>
		buildTreeWorker(start, splitEl, boundL, prims, level, localNextFreeNode, localNodes);
		//<< recurse above child >>
		localNodes[curNode].setRightChild (localNextFreeNode);
		buildTreeWorker(splitEl, end, boundR, prims, level, localNextFreeNode, localNodes);
	}
	--level;
}


template<class T> template<class LookupProc> 
void pointKdTree<T>::lookup(const point3d_t &p, const LookupProc &proc, PFLOAT &maxDistSquared) const
{
#if NON_REC_LOOKUP > 0
	++Y_LOOKUPS;
	KdStack stack[KD_MAX_STACK];
	const kdNode<T> *farChild, *currNode = nodes;
	
	int stackPtr = 1;
	stack[stackPtr].node = nullptr; // "nowhere", termination flag
	
	while (true)
	{
		while( !currNode->IsLeaf() )
		{
			int axis = currNode->SplitAxis();
			PFLOAT splitVal = currNode->SplitPos();
			
			if( p[axis] <= splitVal ) //need traverse left first
			{
				farChild = &nodes[currNode->getRightChild()];
				currNode++;
			}
			else //need traverse right child first
			{
				farChild = currNode+1;
				currNode = &nodes[currNode->getRightChild()];
			}
			++stackPtr;
			stack[stackPtr].node = farChild;
			stack[stackPtr].axis = axis;
			stack[stackPtr].s = splitVal;
		}

		// Hand leaf-data kd-tree to processing function
		vector3d_t v = currNode->data->pos - p;
		PFLOAT dist2 = v.lengthSqr();

		if (dist2 < maxDistSquared)
		{
			++Y_PROCS;
			proc(currNode->data, dist2, maxDistSquared);
		}
		
		if(!stack[stackPtr].node) return; // stack empty, done.
		//radius probably lowered so we may pop additional elements:
		int axis = stack[stackPtr].axis;
		dist2 = p[axis] - stack[stackPtr].s;
		dist2 *= dist2;

		while(dist2 > maxDistSquared)
		{
			--stackPtr;
			if(!stack[stackPtr].node) return;// stack empty, done.
			axis = stack[stackPtr].axis;
			dist2 = p[axis] - stack[stackPtr].s;
			dist2 *= dist2;
		}
		currNode = stack[stackPtr].node;
		--stackPtr;
	}
#else
	recursiveLookup(p, proc, maxDistSquared, 0);
	++Y_LOOKUPS;
	if(Y_LOOKUPS == 159999)
	{
		Y_VERBOSE << "pointKd-Tree:average photons tested per lookup:" << double(Y_PROCS)/double(Y_LOOKUPS) << yendl;
	}
#endif
}

template<class T> template<class LookupProc> 
void pointKdTree<T>::recursiveLookup(const point3d_t &p, const LookupProc &proc, PFLOAT &maxDistSquared, int nodeNum) const
{
	const kdNode<T> *currNode = &nodes[nodeNum];
	if(currNode->IsLeaf())
	{
		vector3d_t v = currNode->data->pos - p;
		PFLOAT dist2 = v.lengthSqr();
		if (dist2 < maxDistSquared)
			proc(currNode->data, dist2, maxDistSquared);
			++Y_PROCS;
		return;
	}
	int axis = currNode->SplitAxis();
	PFLOAT dist2 = p[axis] - currNode->SplitPos();
	dist2 *= dist2;
	if(p[axis] <= currNode->SplitPos())
	{
		recursiveLookup(p, proc, maxDistSquared, nodeNum+1);
		if (dist2 < maxDistSquared)
			recursiveLookup(p, proc, maxDistSquared, currNode->getRightChild());
	}
	else
	{
		recursiveLookup(p, proc, maxDistSquared, currNode->getRightChild());
		if (dist2 < maxDistSquared)
			recursiveLookup(p, proc, maxDistSquared, nodeNum+1);
	}
}

} // namespace::kdtree

__END_YAFRAY

#endif // Y_PKDTREE_H
