#ifndef __Y_RKDTREE_H
#define __Y_RKDTREE_H

#include <yafray_config.h>

#include <algorithm>

#include <utilities/y_alloc.h>
#include <core_api/bound.h>
#include <core_api/object3d.h>
#include <yafraycore/kdtree.h>
//#include <yafraycore/triangle.h>

__BEGIN_YAFRAY

extern int Kd_inodes, Kd_leaves, _emptyKd_leaves, Kd_prims, _clip, _bad_clip, _null_clip, _early_out;

class renderState_t;

#define PRIM_DAT_SIZE 32

// ============================================================
/*! kd-tree nodes, kept as small as possible
    double precision float and/or 64 bit system: 12bytes
    else 8 bytes */

template<class T> class rkdTreeNode
{
public:
	void createLeaf(u_int32 *primIdx, int np, const T **prims, MemoryArena &arena)
	{
		primitives = 0;
		flags = np << 2;
		flags |= 3;
		if(np>1)
		{
			primitives = (T **)arena.Alloc(np * sizeof(T *));
			for(int i=0;i<np;i++) primitives[i] = (T *)prims[primIdx[i]];
			Kd_prims+=np; //stat
		}
		else if(np==1)
		{
			onePrimitive = (T *)prims[primIdx[0]];
			Kd_prims++; //stat
		}
		else _emptyKd_leaves++; //stat
		Kd_leaves++; //stat
	}
	void createInterior(int axis, PFLOAT d)
	{	division = d; flags = (flags & ~3) | axis; Kd_inodes++; }
	PFLOAT 	SplitPos() const { return division; }
	int 	SplitAxis() const { return flags & 3; }
	int 	nPrimitives() const { return flags >> 2; }
	bool 	IsLeaf() const { return (flags & 3) == 3; }
	u_int32	getRightChild() const { return (flags >> 2); }
	void 	setRightChild(u_int32 i) { flags = (flags&3) | (i << 2); }	
	
	union
	{
		PFLOAT 			division;		//!< interior: division plane position
		T** 	primitives;		//!< leaf: list of primitives
		T*		onePrimitive;	//!< leaf: direct inxex of one primitive
	};
	u_int32	flags;		//!< 2bits: isLeaf, axis; 30bits: nprims (leaf) or index of right child
};

/*! Stack elements for the custom stack of the recursive traversal */
template<class T> struct rKdStack
{
	const rkdTreeNode<T> *node; //!< pointer to far child
	PFLOAT t; 		//!< the entry/exit signed distance
	point3d_t pb; 		//!< the point coordinates of entry/exit point
	int	 prev; 		//!< the pointer to the previous stack item
};

// ============================================================
/*! This class holds a complete kd-tree with building and
	traversal funtions
*/
template<class T> class YAFRAYCORE_EXPORT kdTree_t
{
public:
	kdTree_t(const T **v, int np, int depth=-1, int leafSize=2,
			float cost_ratio=0.35, float emptyBonus=0.33);
	bool Intersect(const ray_t &ray, PFLOAT dist, T **tr, PFLOAT &Z, void *udat) const;
//	bool IntersectDBG(const ray_t &ray, PFLOAT dist, triangle_t **tr, PFLOAT &Z) const;
	bool IntersectS(const ray_t &ray, PFLOAT dist, T **tr) const;
	bool IntersectTS(renderState_t &state, const ray_t &ray, int maxDepth, PFLOAT dist, T **tr, color_t &filt) const;
//	bool IntersectO(const point3d_t &from, const vector3d_t &ray, PFLOAT dist, T **tr, PFLOAT &Z) const;
	bound_t getBound(){ return treeBound; }
	~kdTree_t();
private:
	void pigeonMinCost(u_int32 nPrims, bound_t &nodeBound, u_int32 *primIdx, splitCost_t &split);
	void minimalCost(u_int32 nPrims, bound_t &nodeBound, u_int32 *primIdx,
		const bound_t *allBounds, boundEdge *edges[3], splitCost_t &split);
	int buildTree(u_int32 nPrims, bound_t &nodeBound, u_int32 *primNums,
		u_int32 *leftPrims, u_int32 *rightPrims, boundEdge *edges[3],
		u_int32 rightMemSize, int depth, int badRefines );
	
	float 		costRatio; 	//!< node traversal cost divided by primitive intersection cost
	float 		eBonus; 	//!< empty bonus
	u_int32 	nextFreeNode, allocatedNodesCount, totalPrims;
	int 		maxDepth;
	unsigned int maxLeafSize;
	bound_t 	treeBound; 	//!< overall space the tree encloses
	MemoryArena primsArena;
	rkdTreeNode<T> 	*nodes;
	
	// those are temporary actually, to keep argument counts bearable
	const T **prims;
	bound_t *allBounds;
	int *clip; // indicate clip plane(s) for current level
	char *cdata; // clipping data...
	
	// some statistics:
	int depthLimitReached, NumBadSplits;
};


__END_YAFRAY
#endif	//__Y_RKDTREE_H
