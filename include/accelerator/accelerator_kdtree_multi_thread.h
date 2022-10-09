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

namespace yafaray {

#define POLY_CLIPPING_MULTITHREAD 1

// ============================================================
/*! This class holds a complete kd-tree with building and
	traversal funtions
*/
class AcceleratorKdTreeMultiThread final : public Accelerator
{
	public:
		inline static std::string getClassName() { return "AcceleratorKdTreeMultiThread"; }
		static std::pair<std::unique_ptr<Accelerator>, ParamError> factory(Logger &logger, const std::vector<const Primitive *> &primitives, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		AcceleratorKdTreeMultiThread(Logger &logger, ParamError &param_error, const std::vector<const Primitive *> &primitives, const ParamMap &param_map);
		~AcceleratorKdTreeMultiThread() override;

	private:
		using ThisClassType_t = AcceleratorKdTreeMultiThread;
		using ParentClassType_t = Accelerator;
		[[nodiscard]] Type type() const override { return Type::KdTreeMultiThread; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(int, max_depth_, 0, "depth", "");
			PARAM_DECL(int, max_leaf_size_, 1, "max_leaf_size_", "");
			PARAM_DECL(float , cost_ratio_, 0.8f, "cost_ratio", "node traversal cost divided by primitive intersection cost");
			PARAM_DECL(float , empty_bonus_, 0.33f, "empty_bonus", "");
			PARAM_DECL(int, num_threads_, 1, "accelerator_threads", "");
			PARAM_DECL(int, min_indices_to_spawn_threads_, 10000, "accelerator_min_indices_threads", "Only spawn threaded subtree building when the number of indices in the subtree is higher than this value to prevent slowdown due to very small subtree left indices");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

		struct Result;
		struct SplitCost;
		class Node;
		struct Stack;
		IntersectData intersect(const Ray &ray, float t_max) const override;
		IntersectData intersectShadow(const Ray &ray, float t_max) const override;
		IntersectData intersectTransparentShadow(const Ray &ray, int max_depth, float t_max, const Camera *camera) const override;
		Bound<float> getBound() const override { return tree_bound_; }

		AcceleratorKdTreeMultiThread::Result buildTree(const std::vector<const Primitive *> &primitives, const Bound<float> &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound<float>> &bounds, const Params &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, std::atomic<int> &num_current_threads) const;
		void buildTreeWorker(const std::vector<const Primitive *> &primitives, const Bound<float> &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound<float>> &bounds, const Params &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, Result &result, std::atomic<int> &num_current_threads) const;
		static SplitCost pigeonMinCost(Logger &logger, float e_bonus, float cost_ratio, const std::vector<Bound<float>> &bounds, const Bound<float> &node_bound, const std::vector<uint32_t> &prim_indices);
		static SplitCost minimalCost(Logger &logger, float e_bonus, float cost_ratio, const Bound<float> &node_bound, const std::vector<uint32_t> &indices, const std::vector<Bound<float>> &bounds);

		alignas(8) std::vector<Node> nodes_;
		Bound<float> tree_bound_; 	//!< overall space the tree encloses
		std::atomic<int> num_current_threads_ { 0 };
};

// ============================================================
/*! kd-tree nodes, kept as small as possible
    double precision float and/or 64 bit system: 12bytes
    else 8 bytes */

class AcceleratorKdTreeMultiThread::Node
{
	public:
		kdtree::Stats createLeaf(const std::vector<uint32_t> &prim_indices, const std::vector<const Primitive *> &primitives);
		kdtree::Stats createInterior(Axis axis, float d);
		float splitPos() const { return division_; }
		Axis splitAxis() const { return static_cast<Axis>(flags_ & 3); }
		uint32_t nPrimitives() const { return primitives_.size(); /*flags_ >> 2; */ }
		const Primitive *getOnePrimitive() const { return primitives_[0]; }
		bool isLeaf() const { return (flags_ & 3) == 3; }
		uint32_t getRightChild() const { return (flags_ >> 2); }
		void setRightChild(uint32_t i) { flags_ = (flags_ & 3) | (i << 2); }
		std::vector<const Primitive *> primitives_; //!< leaf: list of primitives
		uint32_t flags_; //!< 2bits: isLeaf, axis; 30bits: nprims (leaf) or index of right child
		float division_; //!< interior: division plane position
};

/*! Stack elements for the custom stack of the recursive traversal */
struct AcceleratorKdTreeMultiThread::Stack
{
	const Node *node_; //!< pointer to far child
	float t_; //!< the entry/exit signed distance
	Point3f point_; //!< the point coordinates of entry/exit point
	int prev_stack_id_; //!< the pointer to the previous stack item
};

struct AcceleratorKdTreeMultiThread::SplitCost
{
	alignas(8) int edge_offset_ = -1;
	float cost_;
	float t_;
	std::vector<kdtree::BoundEdge> edges_;
	int stats_early_out_ = 0;
	Axis axis_ = Axis::None;
};

struct AcceleratorKdTreeMultiThread::Result
{
	alignas(8) std::vector<Node> nodes_;
	kdtree::Stats stats_;
};


inline kdtree::Stats AcceleratorKdTreeMultiThread::Node::createLeaf(const std::vector<uint32_t> &prim_indices, const std::vector<const Primitive *> &primitives)
{
	const uint32_t num_prim_indices = prim_indices.size();
	kdtree::Stats kd_stats;
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

inline kdtree::Stats AcceleratorKdTreeMultiThread::Node::createInterior(Axis axis, float d)
{
	kdtree::Stats kd_stats;
	division_ = d;
	flags_ = (flags_ & ~3) | axis::getId(axis);
	kd_stats.kd_inodes_++;
	return kd_stats;
}

inline IntersectData AcceleratorKdTreeMultiThread::intersect(const Ray &ray, float t_max) const
{
	return kdtree::intersect<Node, Stack, kdtree::IntersectTestType::Nearest>(ray, t_max, nodes_, tree_bound_, 0, nullptr);
}

inline IntersectData AcceleratorKdTreeMultiThread::intersectShadow(const Ray &ray, float t_max) const
{
	return kdtree::intersect<Node, Stack, kdtree::IntersectTestType::Shadow>(ray, t_max, nodes_, tree_bound_, 0, nullptr);
}

inline IntersectData AcceleratorKdTreeMultiThread::intersectTransparentShadow(const Ray &ray, int max_depth, float t_max, const Camera *camera) const
{
	return kdtree::intersect<Node, Stack, kdtree::IntersectTestType::TransparentShadow>(ray, t_max, nodes_, tree_bound_, max_depth, camera);
}


} //namespace yafaray
#endif    //YAFARAY_ACCELERATOR_KDTREE_MULTI_THREAD_H
