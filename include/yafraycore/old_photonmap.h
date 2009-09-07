#ifndef Y_BOUNDTREE_H
#define Y_BOUNDTREE_H

#include<yafray_config.h>

#include "photon.h"
#include <core_api/vector3d.h>
#include <core_api/matrix4.h>
#include <vector>

__BEGIN_YAFRAY

template<class T>
class gBoundTreeNode_t
{
	//friend class gBoundTree_t;
	public:
		gBoundTreeNode_t(gBoundTreeNode_t<T> *l,gBoundTreeNode_t<T> *r,bound_t &b) 
			{_left=l;_right=r;bound=b;r->_parent=l->_parent=this;_parent=NULL;};
		
		gBoundTreeNode_t(const std::vector<T> & v,const bound_t b):_child(v) 
		{_left=_right=NULL;_parent=NULL;bound=b;};
		
		~gBoundTreeNode_t() {if(_left!=NULL) {delete _left;delete _right;} };
		
		bool isLeaf()const  {return (_left==NULL);};
		gBoundTreeNode_t<T> *right() {return _right;};
		const gBoundTreeNode_t<T> *right()const {return _right;};
		gBoundTreeNode_t<T> *left() {return _left;};
		const gBoundTreeNode_t<T> *left()const {return _left;};
		gBoundTreeNode_t<T> *parent() {return _parent;};
		const gBoundTreeNode_t<T> *parent()const {return _parent;};
		std::vector<T> & child() {return _child;};
		const std::vector<T> & child()const {return _child;};
		bound_t &getBound() {return bound;};
		const bound_t &getBound()const {return bound;};

		typename std::vector<T>::const_iterator begin()const {return _child.begin();};
		typename std::vector<T>::const_iterator end()const {return _child.end();};
	protected:
		gBoundTreeNode_t<T> *_left;
		gBoundTreeNode_t<T> *_right;
		gBoundTreeNode_t<T> *_parent;
		bound_t bound;
		std::vector<T> _child;
};

template<class T>
gBoundTreeNode_t<T> * buildGenericTree(const std::vector<T> &v,
		bound_t (*calc_bound)(const std::vector<T> &v),
		bool (*is_in_bound)(const T &t,bound_t &b),
		point3d_t (*get_pos)(const T &t),
		unsigned int dratio=1,unsigned int depth=1,
		bool skipX=false,bool skipY=false,bool skipZ=false)
{
	typedef typename std::vector<T>::const_iterator vector_const_iterator;
	if((v.size()<=dratio) || (skipX && skipY && skipZ))
		return new gBoundTreeNode_t<T>(v,calc_bound(v));
	PFLOAT lx,ly,lz;
	bool usedX=false,usedY=false,usedZ=false;
	bound_t bound=calc_bound(v);
	lx=bound.longX();
	ly=bound.longY();
	lz=bound.longZ();

	bound_t bl,br;
	if(((lx>=ly) || skipY) && ((lx>=lz) || skipZ) && !skipX)
	{
		PFLOAT media=0;
		for(vector_const_iterator i=v.begin();i!=v.end();++i)
			media+=get_pos(*i).x;
		media/=(PFLOAT)v.size();
		bl=bound;bl.setMaxX(media);
		br=bound;br.setMinX(media);
		usedX=true;
	}
	else if(((ly>=lx) || skipX) && ((ly>=lz) || skipZ) && !skipY)
	{
		PFLOAT media=0;
		for(vector_const_iterator i=v.begin();i!=v.end();++i)
			media+=get_pos(*i).y;
		media/=(PFLOAT)v.size();
		bl=bound;bl.setMaxY(media);
		br=bound;br.setMinY(media);
		usedY=true;
	}
	else
	{
		PFLOAT media=0;
		for(vector_const_iterator i=v.begin();i!=v.end();++i)
			media+=get_pos(*i).z;
		media/=(PFLOAT)v.size();
		bl=bound;bl.setMaxZ(media);
		br=bound;br.setMinZ(media);
		usedZ=true;
	}
	std::vector<T> vl,vr,vm;
	for(vector_const_iterator i=v.begin();i!=v.end();++i)
	{
		if(is_in_bound(*i,bl))
		{
			if(is_in_bound(*i,br)) 
				vm.push_back(*i);
			else
				vl.push_back(*i);
		}
		else
			vr.push_back(*i);
	}

	if( (vl.size()==v.size()) || (vr.size()==v.size()) || (vm.size()==v.size()))
		return buildGenericTree(v,calc_bound,is_in_bound,get_pos,dratio,depth,
						skipX || usedX,skipY || usedY,skipZ || usedZ);
	if(vl.size()==0) return new gBoundTreeNode_t<T>(buildGenericTree(vr,calc_bound,is_in_bound,
				get_pos,dratio,depth+1,skipX,skipY,skipZ),buildGenericTree(vm,calc_bound,
					is_in_bound,get_pos,dratio,depth+1, skipX,skipY,skipZ),bound);
	if(vr.size()==0) return new gBoundTreeNode_t<T>(buildGenericTree(vl,calc_bound,is_in_bound,
				get_pos,dratio,depth+1,skipX,skipY,skipZ),buildGenericTree(vm,calc_bound,
					is_in_bound,get_pos,dratio,depth+1, skipX,skipY,skipZ),bound);
	if(vm.size()==0) return new gBoundTreeNode_t<T>(buildGenericTree(vl,calc_bound,is_in_bound,
				get_pos,dratio,depth+1,skipX,skipY,skipZ),buildGenericTree(vr,calc_bound,
					is_in_bound,get_pos,dratio,depth+1, skipX,skipY,skipZ),bound);
	//else 
	//{
		gBoundTreeNode_t<T> *balanced=new gBoundTreeNode_t<T>( 
			buildGenericTree(vl,calc_bound,is_in_bound,get_pos,dratio,depth+1,
				skipX,skipY,skipZ),
			buildGenericTree(vr,calc_bound,is_in_bound,get_pos,dratio,depth+1,
				skipX,skipY,skipZ),bound);
		if(vm.size()==0) return balanced;
		return new gBoundTreeNode_t<T>(
			balanced,
			buildGenericTree(vm,calc_bound,is_in_bound,get_pos,dratio,depth+1,
				skipX,skipY,skipZ),bound);
	//}
}

template<class T,class D,class CROSS>
class gObjectIterator_t 
{
	public:
		gObjectIterator_t(const gBoundTreeNode_t<T> *r,const D &d);
		void upFirstRight();
		void downLeft();
		void operator ++ ();
		void operator ++ (int) {++(*this);};
		bool operator ! () {return !end;};
		//T & operator * () {return *currT;};
		const T & operator * () {return *currT;};
		const gBoundTreeNode_t<T> *currentNode() {return currN;};
	protected:
		const gBoundTreeNode_t<T> *currN;
		const gBoundTreeNode_t<T> *root;
		const D &dir;
		PFLOAT dist;
		bool end;
		CROSS cross;
		typename std::vector<T>::const_iterator currT;
		typename std::vector<T>::const_iterator currTend;
};

#define DOWN_LEFT(c) c=c->left()
#define DOWN_RIGHT(c) c=c->right()
#define UP(c) c=c->parent()
#define WAS_LEFT(o,c) (c->left()==o)
#define WAS_RIGHT(o,c) (c->right()==o)
#define TOP(c) (c->parent()==NULL)

template<class T,class D,class CROSS>
gObjectIterator_t<T,D,CROSS>::gObjectIterator_t(const gBoundTreeNode_t<T> *r,const D &d):
	dir(d)
{
	root=currN=r;
	if(!cross(dir,currN->getBound()))
	{
		end=true;
		return;
	}
	else end=false;
	downLeft();
	if(!(currN->isLeaf()))
	{
		currT=currTend=currN->end();
		while(currT==currTend)
		{
			bool first=true;
			while(first || !(currN->isLeaf()))
			{
				first=false;
				upFirstRight();
				if(currN==NULL)
				{
					end=true;
					return;
				}
				DOWN_RIGHT(currN);
				downLeft();
			}
			currT=currN->begin();
			currTend=currN->end();
		}
	}
	else
	{
		currT=currN->begin();
		currTend=currN->end();
		//if(currT==currTend) ++(*this);
		while(currT==currTend)
		{
			bool first=true;
			while(first || !(currN->isLeaf()))
			{
				first=false;
				upFirstRight();
				if(currN==NULL)
				{
					end=true;
					return;
				}
				DOWN_RIGHT(currN);
				downLeft();
			}
			currT=currN->begin();
			currTend=currN->end();
		}
	}
}

template<class T,class D,class CROSS>
void gObjectIterator_t<T,D,CROSS>::upFirstRight()
{
	const gBoundTreeNode_t<T> *old=currN;
	if(TOP(currN)) 
	{
		currN=NULL;
		return;
	}
	old=currN;
	UP(currN);
	while(WAS_RIGHT(old,currN) || !cross(dir,currN->right()->getBound()))
	{
		if(TOP(currN))
		{
			currN=NULL;
			return;
		}
		old=currN;
		UP(currN);
	}
}

template<class T,class D,class CROSS>
void gObjectIterator_t<T,D,CROSS>::downLeft()
{
	while(!(currN->isLeaf()))
	{
		while( !(currN->isLeaf()) && cross(dir,currN->left()->getBound()) )
			DOWN_LEFT(currN);
		if(!(currN->isLeaf())) 
		{
			if(cross(dir,currN->right()->getBound()))
				DOWN_RIGHT(currN);
			else return;
		}
	}
}


template<class T,class D,class CROSS>
inline void gObjectIterator_t<T,D,CROSS>::operator ++ ()
{
	currT++;
	while(currT==currTend)
	{
		bool first=true;
		while(first || !(currN->isLeaf()))
		{
			first=false;
			upFirstRight();
			if(currN==NULL)
			{
				end=true;
				return;
			}
			DOWN_RIGHT(currN);
			downLeft();
		}
		currT=currN->begin();
		currTend=currN->end();
	}
	
}


class YAFRAYCORE_EXPORT globalPhotonMap_t
{
	public:

		typedef std::vector<photon_t>::iterator iterator;
		typedef std::vector<photon_t>::const_iterator const_iterator;
		
		globalPhotonMap_t();
		~globalPhotonMap_t();


		void setNumPaths(int n){ paths=n; }
		int nPaths() const{ return paths; }
		int nPhotons() const{ return photons.size(); }
		void pushPhoton(photon_t &p) { photons.push_back(p); }
		void updateTree();
//		void buildTree(std::vector<photon_t> &photons);

		void gather(const point3d_t &P,const vector3d_t &N,
				std::vector<foundPhoton_t> &found,
				unsigned int K,PFLOAT &radius,PFLOAT mincos=0.0)const;

		PFLOAT getMaxRadius()const {return maxradius;};
		void setMaxRadius(PFLOAT r) {maxradius=r;};

//		iterator begin() {return photons.begin();}; 
//		const_iterator begin()const {return photons.begin();}; 
//		iterator end() {return photons.end();}; 
//		const_iterator end()const {return photons.end();}; 

	protected:
		globalPhotonMap_t(const globalPhotonMap_t &s) {}; //forbiden
		PFLOAT maxradius;
		//hash3d_t<storedPhoton_t> hash;
		std::vector<photon_t> photons;
		int paths; //!< amount of photon paths that have been traced for generating the map
		gBoundTreeNode_t<const photon_t *> *tree;
		mutable unsigned int Y_LOOKUPS, Y_PROCS, Y_FIRST;
};


__END_YAFRAY

#endif // Y_BOUNDTREE_H
