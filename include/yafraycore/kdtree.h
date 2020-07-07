#ifndef __Y_KDTREE_H
#define __Y_KDTREE_H

#include <yafray_constants.h>
#include "meshtypes.h"
#include "triangle.h"
#include <utilities/y_alloc.h>
#include <core_api/vector3d.h>
#include <core_api/bound.h>
#include <cstdint>

__BEGIN_YAFRAY

extern int Kd_inodes, Kd_leaves, _emptyKd_leaves, Kd_prims;

struct renderState_t;
class intersectData_t;
class triangle_t;
class ray_t;
class color_t;

#define PRIM_DAT_SIZE 32

// ============================================================
/*! kd-tree nodes, kept as small as possible
    double precision float and/or 64 bit system: 12bytes
    else 8 bytes */

class kdTreeNode
{
public:
	void createLeaf(uint32_t *primIdx, int np, const triangle_t **prims, MemoryArena &arena)
	{
		primitives = 0;
		flags = np << 2;
		flags |= 3;
		if(np>1)
		{
			primitives = (triangle_t **)arena.Alloc(np * sizeof(triangle_t *));
			for(int i=0;i<np;i++) primitives[i] = (triangle_t *)prims[primIdx[i]];
			Kd_prims+=np; //stat
		}
		else if(np==1)
		{
			onePrimitive = (triangle_t *)prims[primIdx[0]];
			Kd_prims++; //stat
		}
		else _emptyKd_leaves++; //stat
		Kd_leaves++; //stat
	}
	void createInterior(int axis, float d)
	{	division = d; flags = (flags & ~3) | axis; Kd_inodes++; }
	float 	SplitPos() const { return division; }
	int 	SplitAxis() const { return flags & 3; }
	int 	nPrimitives() const { return flags >> 2; }
	bool 	IsLeaf() const { return (flags & 3) == 3; }
	uint32_t	getRightChild() const { return (flags >> 2); }
	void 	setRightChild(uint32_t i) { flags = (flags&3) | (i << 2); }	
	
	union
	{
		float 			division;		//!< interior: division plane position
		triangle_t** 	primitives;		//!< leaf: list of primitives
		triangle_t*		onePrimitive;	//!< leaf: direct inxex of one primitive
	};
	uint32_t	flags;		//!< 2bits: isLeaf, axis; 30bits: nprims (leaf) or index of right child
};

/*! Serves to store the lower and upper bound edges of the primitives
	for the cost funtion */

class boundEdge {
public:
	boundEdge(){};
	boundEdge(float position, int primitive, int bound_end):
		pos(position), primNum(primitive), end(bound_end) {};
	bool operator<(const boundEdge &e) const {
		if (pos == e.pos)
			return (int)end > (int)e.end;
		else return pos < e.pos;
	}
	float pos;
	int primNum;
	int end;
};

/*! Stack elements for the custom stack of the recursive traversal */
struct KdStack
{
	const kdTreeNode *node; //!< pointer to far child
	float t; 		//!< the entry/exit signed distance
	point3d_t pb; 		//!< the point coordinates of entry/exit point
	int	 prev; 		//!< the pointer to the previous stack item
};

struct KdToDo
{
	const kdTreeNode *node;
	float tmin, tmax;
};

class splitCost_t
{
public:
	splitCost_t(): bestAxis(-1), bestOffset(-1) {};
	int bestAxis;
	int bestOffset;
	float bestCost;
	float oldCost;
	float t;
	int nBelow, nAbove, nEdge;
};

class bin_t
{
	public:
	bin_t(): n(0), c_left(0), c_right(0), c_bleft(0), c_both(0) {};
	bool empty(){ return n==0; };
	void reset(){n=0, c_left=0, c_right=0, c_both=0, c_bleft=0;};
	int 	n;
	int 	c_left, c_right;
	int 	c_bleft, c_both;
	float 	t;
};

// ============================================================
/*! This class holds a complete kd-tree with building and
	traversal funtions
*/
class YAFRAYCORE_EXPORT triKdTree_t
{
public:
	triKdTree_t(const triangle_t **v, int np, int depth=-1, int leafSize=2,
			float cost_ratio=0.35, float emptyBonus=0.33);
	bool Intersect(const ray_t &ray, float dist, triangle_t **tr, float &Z, intersectData_t &data) const;
//	bool IntersectDBG(const ray_t &ray, float dist, triangle_t **tr, float &Z) const;
	bool IntersectS(const ray_t &ray, float dist, triangle_t **tr, float shadow_bias) const;
	bool IntersectTS(renderState_t &state, const ray_t &ray, int maxDepth, float dist, triangle_t **tr, color_t &filt, float shadow_bias) const;
//	bool IntersectO(const point3d_t &from, const vector3d_t &ray, float dist, triangle_t **tr, float &Z) const;
	bound_t getBound(){ return treeBound; }
	~triKdTree_t();
private:
	void pigeonMinCost(uint32_t nPrims, bound_t &nodeBound, uint32_t *primIdx, splitCost_t &split);
	void minimalCost(uint32_t nPrims, bound_t &nodeBound, uint32_t *primIdx,
		const bound_t *allBounds, boundEdge *edges[3], splitCost_t &split);
	int buildTree(uint32_t nPrims, bound_t &nodeBound, uint32_t *primNums,
		uint32_t *leftPrims, uint32_t *rightPrims, boundEdge *edges[3],
		uint32_t rightMemSize, int depth, int badRefines );
	
	float 		costRatio; 	//!< node traversal cost divided by primitive intersection cost
	float 		eBonus; 	//!< empty bonus
	uint32_t 	nextFreeNode, allocatedNodesCount, totalPrims;
	int 		maxDepth;
	unsigned int maxLeafSize;
	bound_t 	treeBound; 	//!< overall space the tree encloses
	MemoryArena primsArena;
	kdTreeNode 	*nodes;
	
	// those are temporary actually, to keep argument counts bearable
	const triangle_t **prims;
	bound_t *allBounds;
	int *clip; // indicate clip plane(s) for current level
	char *cdata; // clipping data...
	
	// some statistics:
	int depthLimitReached, NumBadSplits;
};


__END_YAFRAY
#endif	//__Y_KDTREE_H
