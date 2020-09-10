#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef YAFARAY_KDTREE_H
#define YAFARAY_KDTREE_H

#include "constants.h"
#include "utility/util_memory_arena.h"
#include "common/bound.h"
#include "object_geom/object_geom.h"
#include <algorithm>

BEGIN_YAFARAY

struct RenderState;
class IntersectData;

// ============================================================
/*! kd-tree nodes, kept as small as possible
    double precision float and/or 64 bit system: 12bytes
    else 8 bytes */

struct KdStats
{
	int kd_inodes_ = 0;
	int kd_leaves_ = 0;
	int empty_kd_leaves_ = 0;
	int kd_prims_ = 0;
	int clip_ = 0;
	int bad_clip_ = 0;
	int null_clip_ = 0;
	int early_out_ = 0;
	int depth_limit_reached_ = 0;
	int num_bad_splits_ = 0;
};

template<class T> class KdTreeNode
{
	public:
		void createLeaf(uint32_t *prim_idx, int np, const T **prims, MemoryArena &arena, KdStats &kd_stats);
		void createInterior(int axis, float d, KdStats &kd_stats);
		float 	splitPos() const { return division_; }
		int 	splitAxis() const { return flags_ & 3; }
		int 	nPrimitives() const { return flags_ >> 2; }
		bool 	isLeaf() const { return (flags_ & 3) == 3; }
		uint32_t	getRightChild() const { return (flags_ >> 2); }
		void 	setRightChild(uint32_t i) { flags_ = (flags_ & 3) | (i << 2); }

		union
		{
			float 			division_;		//!< interior: division plane position
			T 	**primitives_;		//!< leaf: list of primitives
			T		*one_primitive_;	//!< leaf: direct inxex of one primitive
		};
		uint32_t	flags_;		//!< 2bits: isLeaf, axis; 30bits: nprims (leaf) or index of right child
};

template<class T>
void KdTreeNode<T>::createLeaf(uint32_t *prim_idx, int np, const T **prims, MemoryArena &arena, KdStats &kd_stats) {
	primitives_ = nullptr;
	flags_ = np << 2;
	flags_ |= 3;
	if(np > 1)
	{
		primitives_ = (T **) arena.alloc(np * sizeof(T *));
		for(int i = 0; i < np; i++) primitives_[i] = (T *)prims[prim_idx[i]];
		kd_stats.kd_prims_ += np; //stat
	}
	else if(np == 1)
	{
		one_primitive_ = (T *)prims[prim_idx[0]];
		kd_stats.kd_prims_++; //stat
	}
	else kd_stats.empty_kd_leaves_++; //stat
	kd_stats.kd_leaves_++; //stat
}

template<class T>
void KdTreeNode<T>::createInterior(int axis, float d, KdStats &kd_stats)
{
	division_ = d;
	flags_ = (flags_ & ~3) | axis;
	kd_stats.kd_inodes_++;
}

/*! Stack elements for the custom stack of the recursive traversal */
template<class T> struct KdStack
{
	const KdTreeNode<T> *node_; //!< pointer to far child
	float t_; 		//!< the entry/exit signed distance
	Point3 pb_; 		//!< the point coordinates of entry/exit point
	int	 prev_; 		//!< the pointer to the previous stack item
};

/*! Serves to store the lower and upper bound edges of the primitives
	for the cost funtion */

class BoundEdge
{
	public:
		BoundEdge() = default;
		BoundEdge(float position, int primitive, int bound_end):
				pos_(position), prim_num_(primitive), end_(bound_end) { };
		bool operator<(const BoundEdge &e) const
		{
			if(pos_ == e.pos_)
				return (int)end_ > (int)e.end_;
			else return pos_ < e.pos_;
		}
		float pos_;
		int prim_num_;
		int end_;
};

struct SplitCost
{
	int best_axis_ = -1;
	int best_offset_ = -1;
	float best_cost_;
	float old_cost_;
	float t_;
	int n_below_, n_above_, n_edge_;
};

class TreeBin
{
	public:
		bool empty() { return n_ == 0; };
		void reset() { n_ = 0, c_left_ = 0, c_right_ = 0, c_both_ = 0, c_bleft_ = 0;};
		int 	n_ = 0;
		int 	c_left_ = 0, c_right_ = 0;
		int 	c_bleft_ = 0, c_both_ = 0;
		float 	t_;
};

// ============================================================
/*! This class holds a complete kd-tree with building and
	traversal funtions
*/
template<class T> class KdTree
{
	public:
		KdTree(const T **v, int np, int depth = -1, int leaf_size = 2,
			   float cost_ratio = 0.35, float empty_bonus = 0.33);
		bool intersect(const Ray &ray, float dist, T **tr, float &z, IntersectData &data) const;
		//	bool IntersectDBG(const ray_t &ray, float dist, triangle_t **tr, float &Z) const;
		bool intersectS(const Ray &ray, float dist, T **tr, float shadow_bias) const;
		bool intersectTs(RenderState &state, const Ray &ray, int max_depth, float dist, T **tr, Rgb &filt, float shadow_bias) const;
		//	bool IntersectO(const point3d_t &from, const vector3d_t &ray, float dist, T **tr, float &Z) const;
		Bound getBound() { return tree_bound_; }
		~KdTree();
	private:
		void pigeonMinCost(uint32_t n_prims, Bound &node_bound, uint32_t *prim_idx, SplitCost &split);
		void minimalCost(uint32_t n_prims, Bound &node_bound, uint32_t *prim_idx,
						 const Bound *all_bounds, BoundEdge *edges[3], SplitCost &split);
		int buildTree(uint32_t n_prims, Bound &node_bound, uint32_t *prim_nums,
					  uint32_t *left_prims, uint32_t *right_prims, BoundEdge *edges[3],
					  uint32_t right_mem_size, int depth, int bad_refines);

		float 		cost_ratio_; 	//!< node traversal cost divided by primitive intersection cost
		float 		e_bonus_; 	//!< empty bonus
		uint32_t 	next_free_node_, allocated_nodes_count_, total_prims_;
		int 		max_depth_;
		unsigned int max_leaf_size_;
		Bound 	tree_bound_; 	//!< overall space the tree encloses
		MemoryArena prims_arena_;
		KdTreeNode<T> 	*nodes_;

		// those are temporary actually, to keep argument counts bearable
		const T **prims_;
		Bound *all_bounds_;
		int *clip_; // indicate clip plane(s) for current level
		char *cdata_; // clipping data...

		// some statistics:
		KdStats kd_stats_;
};

END_YAFARAY
#endif    //YAFARAY_KDTREE_H
