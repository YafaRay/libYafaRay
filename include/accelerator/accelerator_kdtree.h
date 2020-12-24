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

#ifndef YAFARAY_ACCELERATOR_KDTREE_H
#define YAFARAY_ACCELERATOR_KDTREE_H

#include "accelerator/accelerator.h"
#include "common/memory_arena.h"
#include "geometry/bound.h"
#include "geometry/primitive.h"
#include "scene/yafaray/object_yafaray.h"
#include <array>

BEGIN_YAFARAY

#define PRIMITIVE_CLIPPING 1

class RenderData;
struct IntersectData;

// ============================================================
/*! This class holds a complete kd-tree with building and
	traversal funtions
*/
class AcceleratorKdTree final : public Accelerator
{
	public:
		static Accelerator *factory(const std::vector<const Primitive *> &primitives, ParamMap &params);

	private:
		struct Stats;
		class Node;
		struct Stack;
		class BoundEdge;
		struct SplitCost;
		class TreeBin;
		AcceleratorKdTree(const std::vector<const Primitive *> &primitives, int depth = -1, int leaf_size = 2,
						  float cost_ratio = 0.35, float empty_bonus = 0.33);
		virtual ~AcceleratorKdTree() override;
		virtual AcceleratorIntersectData intersect(const Ray &ray, float t_max) const override;
		//	bool IntersectDBG(const ray_t &ray, float dist, triangle_t **tr, float &Z) const;
		virtual AcceleratorIntersectData intersectS(const Ray &ray, float t_max, float shadow_bias) const override;
		virtual AcceleratorTsIntersectData intersectTs(RenderData &render_data, const Ray &ray, int max_depth, float t_max, float shadow_bias) const override;
		//	bool IntersectO(const point3d_t &from, const vector3d_t &ray, float dist, Primitive **tr, float &Z) const;
		virtual Bound getBound() const override { return tree_bound_; }

		int buildTree(uint32_t n_prims, const std::vector<const Primitive *> &original_primitives, const Bound &node_bound, uint32_t *prim_nums, uint32_t *left_prims, uint32_t *right_prims, const std::array<BoundEdge *, 3> &edges, uint32_t right_mem_size, int depth, int bad_refines);

		static SplitCost pigeonMinCost(float e_bonus, float cost_ratio, uint32_t n_prims, const Bound *all_bounds, const Bound &node_bound, const uint32_t *prim_idx);
		static SplitCost minimalCost(float e_bonus, float cost_ratio, uint32_t n_prims, const Bound &node_bound, const uint32_t *prim_idx, const Bound *all_bounds, const Bound *all_bounds_general, const std::array<BoundEdge *, 3> &edges, Stats &kd_stats);

		static AcceleratorIntersectData intersect(const Ray &ray, float t_max, const Node *nodes, const Bound &tree_bound);
		static AcceleratorIntersectData intersectS(const Ray &ray, float t_max, float shadow_bias, const Node *nodes, const Bound &tree_bound);
		static AcceleratorTsIntersectData intersectTs(RenderData &render_data, const Ray &ray, int max_depth, float t_max, float shadow_bias, const Node *nodes, const Bound &tree_bound);

		float cost_ratio_; 	//!< node traversal cost divided by primitive intersection cost
		float e_bonus_; 	//!< empty bonus
		uint32_t next_free_node_, allocated_nodes_count_, total_prims_;
		int max_depth_;
		unsigned int max_leaf_size_;
		Bound tree_bound_; 	//!< overall space the tree encloses
		MemoryArena prims_arena_;
		Node *nodes_;
		// those are temporary actually, to keep argument counts bearable
		Bound *all_bounds_;
#if PRIMITIVE_CLIPPING > 0
		ClipPlane *clip_; // indicate clip plane(s) for current level
		std::vector<PolyDouble> cdata_; // clipping data...
#endif
		// some statistics:
		struct Stats
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
		} kd_stats_;

		static constexpr int prim_clip_thresh_ = 32;
		static constexpr int kd_max_stack_ = 64;
		static constexpr int lower_b_ = 0;
		static constexpr int upper_b_ = 2;
		static constexpr int both_b_ = 1;
};

// ============================================================
/*! kd-tree nodes, kept as small as possible
    double precision float and/or 64 bit system: 12bytes
    else 8 bytes */

class AcceleratorKdTree::Node
{
	public:
		void createLeaf(const uint32_t *prim_idx, int np, const std::vector<const Primitive *> &prims, MemoryArena &arena, Stats &kd_stats);
		void createInterior(int axis, float d, Stats &kd_stats);
		float splitPos() const { return division_; }
		int splitAxis() const { return flags_ & 3; }
		int nPrimitives() const { return flags_ >> 2; }
		bool isLeaf() const { return (flags_ & 3) == 3; }
		uint32_t getRightChild() const { return (flags_ >> 2); }
		void setRightChild(uint32_t i) { flags_ = (flags_ & 3) | (i << 2); }

		union
		{
			float division_; //!< interior: division plane position
			const Primitive **primitives_; //!< leaf: list of primitives
			const Primitive *one_primitive_ = nullptr; //!< leaf: direct inxex of one primitive
		};
		uint32_t flags_; //!< 2bits: isLeaf, axis; 30bits: nprims (leaf) or index of right child
};

/*! Stack elements for the custom stack of the recursive traversal */
struct AcceleratorKdTree::Stack
{
	const Node *node_; //!< pointer to far child
	float t_; //!< the entry/exit signed distance
	Point3 point_; //!< the point coordinates of entry/exit point
	int prev_stack_idx_; //!< the pointer to the previous stack item
};

/*! Serves to store the lower and upper bound edges of the primitives
	for the cost funtion */

class AcceleratorKdTree::BoundEdge
{
	public:
		BoundEdge() = default;
		BoundEdge(float position, int primitive, int bound_end): pos_(position), prim_num_(primitive), end_(bound_end) { }
		bool operator<(const BoundEdge &e) const
		{
			if(pos_ == e.pos_) return end_ > e.end_;
			else return pos_ < e.pos_;
		}
		float pos_;
		int prim_num_;
		int end_;
};

struct AcceleratorKdTree::SplitCost
{
	int best_axis_ = -1;
	int best_offset_ = -1;
	float best_cost_;
	float t_;
	int num_edges_;
};

class AcceleratorKdTree::TreeBin
{
	public:
		bool empty() const { return n_ == 0; };
		void reset() { n_ = 0, c_left_ = 0, c_right_ = 0, c_both_ = 0, c_bleft_ = 0;};
		int n_ = 0;
		int c_left_ = 0, c_right_ = 0;
		int c_bleft_ = 0, c_both_ = 0;
		float t_ = 0.f;
};

inline void AcceleratorKdTree::Node::createLeaf(const uint32_t *prim_idx, int np, const std::vector<const Primitive *> &prims, MemoryArena &arena, AcceleratorKdTree::Stats &kd_stats) {
	primitives_ = nullptr;
	flags_ = np << 2;
	flags_ |= 3;
	if(np > 1)
	{
		primitives_ = static_cast<const Primitive **>(arena.alloc(np * sizeof(const Primitive *)));
		for(int i = 0; i < np; i++) primitives_[i] = static_cast<const Primitive *>(prims[prim_idx[i]]);
		kd_stats.kd_prims_ += np; //stat
	}
	else if(np == 1)
	{
		one_primitive_ = prims[prim_idx[0]];
		kd_stats.kd_prims_++; //stat
	}
	else kd_stats.empty_kd_leaves_++; //stat
	kd_stats.kd_leaves_++; //stat
}

inline void AcceleratorKdTree::Node::createInterior(int axis, float d, AcceleratorKdTree::Stats &kd_stats)
{
	division_ = d;
	flags_ = (flags_ & ~3) | axis;
	kd_stats.kd_inodes_++;
}

END_YAFARAY

#endif    //YAFARAY_ACCELERATOR_KDTREE_H
