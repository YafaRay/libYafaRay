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

#ifndef YAFARAY_ACCELERATOR_KDTREE_MULTI_THREAD_H
#define YAFARAY_ACCELERATOR_KDTREE_MULTI_THREAD_H

#include "accelerator/accelerator.h"
#include "accelerator/accelerator_kdtree_common.h"
#include "geometry/bound.h"
#include "geometry/primitive/primitive.h"
#include <array>
#include <atomic>

BEGIN_YAFARAY

#define POLY_CLIPPING_MULTITHREAD 1

struct IntersectData;

// ============================================================
/*! This class holds a complete kd-tree with building and
	traversal funtions
*/
class AcceleratorKdTreeMultiThread final : public Accelerator
{
	public:
		static const Accelerator * factory(Logger &logger, const std::vector<const Primitive *> &primitives, const ParamMap &params);

	private:
		struct Parameters;
		struct Result;
		struct SplitCost;
		class Node;
		struct Stack;
		AcceleratorKdTreeMultiThread(Logger &logger, const std::vector<const Primitive *> &primitives, const Parameters &parameters);
		~AcceleratorKdTreeMultiThread() override;
		AcceleratorIntersectData intersect(const Ray &ray, float t_max) const override;
		AcceleratorIntersectData intersectS(const Ray &ray, float t_max, float shadow_bias) const override;
		AcceleratorTsIntersectData intersectTs(const Ray &ray, int max_depth, float t_max, float shadow_bias, const Camera *camera) const override;
		Bound getBound() const override { return tree_bound_; }

		AcceleratorKdTreeMultiThread::Result buildTree(const std::vector<const Primitive *> &primitives, const Bound &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound> &bounds, const Parameters &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, std::atomic<int> &num_current_threads) const;
		void buildTreeWorker(const std::vector<const Primitive *> &primitives, const Bound &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound> &bounds, const Parameters &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, Result &result, std::atomic<int> &num_current_threads) const;
		static SplitCost pigeonMinCost(Logger &logger, float e_bonus, float cost_ratio, const std::vector<Bound> &bounds, const Bound &node_bound, const std::vector<uint32_t> &prim_indices);
		static SplitCost minimalCost(Logger &logger, float e_bonus, float cost_ratio, const Bound &node_bound, const std::vector<uint32_t> &indices, const std::vector<Bound> &bounds);
		static AcceleratorIntersectData intersect(const Ray &ray, float t_max, const std::vector<Node> &nodes, const Bound &tree_bound);
		static AcceleratorIntersectData intersectS(const Ray &ray, float t_max, float shadow_bias, const std::vector<Node> &nodes, const Bound &tree_bound);
		static AcceleratorTsIntersectData intersectTs(const Ray &ray, int max_depth, float t_max, float, const std::vector<Node> &nodes, const Bound &tree_bound, const Camera *camera);

		alignas(8) std::vector<Node> nodes_;
		Bound tree_bound_; 	//!< overall space the tree encloses
		std::atomic<int> num_current_threads_ { 0 };
		static constexpr int kd_max_stack_ = 64;
};

struct AcceleratorKdTreeMultiThread::Parameters
{
	alignas(8) int max_depth_ = 0;
	int max_leaf_size_ = 1;
	float cost_ratio_ = 0.8f; //!< node traversal cost divided by primitive intersection cost
	float empty_bonus_ = 0.33f;
	int num_threads_ = 1;
	int min_indices_to_spawn_threads_ = 10000; //Only spawn threaded subtree building when the number of indices in the subtree is higher than this value to prevent slowdown due to very small subtree left indices
};

// ============================================================
/*! kd-tree nodes, kept as small as possible
    double precision float and/or 64 bit system: 12bytes
    else 8 bytes */

class AcceleratorKdTreeMultiThread::Node
{
	public:
		Stats createLeaf(const std::vector<uint32_t> &prim_indices, const std::vector<const Primitive *> &primitives);
		Stats createInterior(Axis axis, float d);
		float splitPos() const { return division_; }
		int splitAxis() const { return flags_ & 3; }
		int nPrimitives() const { return primitives_.size(); /*flags_ >> 2; */ }
		bool isLeaf() const { return (flags_ & 3) == 3; }
		uint32_t getRightChild() const { return (flags_ >> 2); }
		void setRightChild(uint32_t i) { flags_ = (flags_ & 3) | (i << 2); }
		alignas(8) std::vector<const Primitive *> primitives_; //!< leaf: list of primitives
		uint32_t flags_; //!< 2bits: isLeaf, axis; 30bits: nprims (leaf) or index of right child
		float division_; //!< interior: division plane position
};

/*! Stack elements for the custom stack of the recursive traversal */
struct AcceleratorKdTreeMultiThread::Stack
{
	alignas(8) const Node *node_; //!< pointer to far child
	float t_; //!< the entry/exit signed distance
	Point3 point_; //!< the point coordinates of entry/exit point
	int prev_stack_id_; //!< the pointer to the previous stack item
};

struct AcceleratorKdTreeMultiThread::SplitCost
{
	alignas(8) int axis_ = Axis::None;
	int edge_offset_ = -1;
	float cost_;
	float t_;
	std::vector<BoundEdge> edges_;
	int stats_early_out_ = 0;
};

struct AcceleratorKdTreeMultiThread::Result
{
	alignas(8) std::vector<Node> nodes_;
	Stats stats_;
};


inline Stats AcceleratorKdTreeMultiThread::Node::createLeaf(const std::vector<uint32_t> &prim_indices, const std::vector<const Primitive *> &primitives)
{
	const uint32_t num_prim_indices = prim_indices.size();
	Stats kd_stats;
	primitives_.clear();
	//flags_ = num_prim_indices << 2;
	flags_ |= 3;
	if(num_prim_indices >= 1)
	{
		primitives_.reserve(num_prim_indices);
		for(const auto &prim_id : prim_indices) primitives_.emplace_back(primitives[prim_id]);
		kd_stats.kd_prims_ += num_prim_indices; //stat
	}
	else kd_stats.empty_kd_leaves_++; //stat
	kd_stats.kd_leaves_++; //stat
	return kd_stats;
}

inline Stats AcceleratorKdTreeMultiThread::Node::createInterior(Axis axis, float d)
{
	Stats kd_stats;
	division_ = d;
	flags_ = (flags_ & ~3) | axis.get();
	kd_stats.kd_inodes_++;
	return kd_stats;
}

inline AcceleratorIntersectData AcceleratorKdTreeMultiThread::intersect(const Ray &ray, float t_max) const
{
	return intersect(ray, t_max, nodes_, tree_bound_);
}

inline AcceleratorIntersectData AcceleratorKdTreeMultiThread::intersectS(const Ray &ray, float t_max, float shadow_bias) const
{
	return intersectS(ray, t_max, shadow_bias, nodes_, tree_bound_);
}

inline AcceleratorTsIntersectData AcceleratorKdTreeMultiThread::intersectTs(const Ray &ray, int max_depth, float t_max, float shadow_bias, const Camera *camera) const
{
	return intersectTs(ray, max_depth, t_max, shadow_bias, nodes_, tree_bound_, camera);
}


END_YAFARAY
#endif    //YAFARAY_ACCELERATOR_KDTREE_MULTI_THREAD_H
