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

#ifndef LIBYAFARAY_ACCELERATOR_KDTREE_ORIGINAL_H
#define LIBYAFARAY_ACCELERATOR_KDTREE_ORIGINAL_H

#include "accelerator/accelerator.h"
#include "accelerator/accelerator_kdtree_common.h"
#include "common/memory_arena.h"
#include "geometry/bound.h"
#include "geometry/primitive/primitive.h"
#include <array>

namespace yafaray {

#define PRIMITIVE_CLIPPING 1

// ============================================================
/*! This class holds a complete kd-tree with building and
	traversal funtions
*/
class AcceleratorKdTree final : public Accelerator
{
		using ThisClassType_t = AcceleratorKdTree; using ParentClassType_t = Accelerator;

	public:
		inline static std::string getClassName() { return "AcceleratorKdTree"; }
		static std::pair<std::unique_ptr<Accelerator>, ParamResult> factory(Logger &logger, const RenderControl *render_control, const std::vector<const Primitive *> &primitives, const ParamMap &params);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		AcceleratorKdTree(Logger &logger, ParamResult &param_result, const RenderControl *render_control, const std::vector<const Primitive *> &primitives, const ParamMap &param_map);
		~AcceleratorKdTree() override;

	private:
		[[nodiscard]] Type type() const override { return Type::KdTreeOriginal; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(int, max_depth_, 0, "depth", "");
			PARAM_DECL(int, max_leaf_size_, 1, "max_leaf_size_", "");
			PARAM_DECL(float , cost_ratio_, 0.8f, "cost_ratio", "node traversal cost divided by primitive intersection cost");
			PARAM_DECL(float , empty_bonus_, 0.33f, "empty_bonus", "");
			PARAM_DECL(int, num_threads_, 1, "accelerator_threads", "");
			PARAM_DECL(int, min_indices_to_spawn_threads_, 10000, "accelerator_min_indices_threads", "Only spawn threaded subtree building when the number of indices in the subtree is higher than this value to prevent slowdown due to very small subtree left indices");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

		class Node;
		struct Stack;
		struct SplitCost;
		IntersectData intersect(const Ray &ray, float t_max) const override;
		//	bool IntersectDBG(const ray_t &ray, float dist, triangle_t **tr, float &Z) const;
		IntersectData intersectShadow(const Ray &ray, float t_max) const override;
		IntersectData intersectTransparentShadow(const Ray &ray, int max_depth, float t_max, const Camera *camera) const override;
		//	bool IntersectO(const point3d_t &from, const vector3d_t &ray, float dist, Primitive **tr, float &Z) const;
		Bound<float> getBound() const override { return tree_bound_; }

		int buildTree(uint32_t n_prims, const std::vector<const Primitive *> &original_primitives, const Bound<float> &node_bound, uint32_t *prim_nums, uint32_t *left_prims, uint32_t *right_prims, const std::array<std::unique_ptr<kdtree::BoundEdge[]>, 3> &edges, uint32_t right_mem_size, int depth, int bad_refines);

		static SplitCost pigeonMinCost(Logger &logger, float e_bonus, float cost_ratio, uint32_t num_prim_indices, const Bound<float> *bounds, const Bound<float> &node_bound, const uint32_t *prim_indices);
		static SplitCost minimalCost(Logger &logger, float e_bonus, float cost_ratio, uint32_t num_indices, const Bound<float> &node_bound, const uint32_t *prim_idx, const Bound<float> *all_bounds, const Bound<float> *all_bounds_general, const std::array<std::unique_ptr<kdtree::BoundEdge[]>, 3> &edges_all_axes, kdtree::Stats &kd_stats);

		float cost_ratio_{params_.cost_ratio_}; //!< node traversal cost divided by primitive intersection cost
		float e_bonus_{params_.empty_bonus_}; //!< empty bonus
		uint32_t next_free_node_, allocated_nodes_count_, total_prims_;
		int max_depth_{params_.max_depth_};
		unsigned int max_leaf_size_{static_cast<unsigned int>(params_.max_leaf_size_)};
		Bound<float> tree_bound_; 	//!< overall space the tree encloses
		MemoryArena prims_arena_;
		std::vector<Node> nodes_;
		// those are temporary actually, to keep argument counts bearable
		std::unique_ptr<Bound<float>[]> all_bounds_;
#if PRIMITIVE_CLIPPING > 0
		std::unique_ptr<ClipPlane[]> clip_; // indicate clip plane(s) for current level
		std::vector<PolyDouble> cdata_; // clipping data...
#endif
		struct kdtree::Stats kd_stats_; // some statistics:

		static constexpr inline int prim_clip_thresh_ = 32;
		static constexpr inline int pigeonhole_sort_thresh_ = 128;
		static constexpr inline int kd_max_stack_ = 64;
};

// ============================================================
/*! kd-tree nodes, kept as small as possible
    double precision float and/or 64 bit system: 12bytes
    else 8 bytes */

class AcceleratorKdTree::Node
{
	public:
		void createLeaf(const uint32_t *prim_idx, int np, const std::vector<const Primitive *> &prims, MemoryArena &arena, kdtree::Stats &kd_stats);
		void createInterior(int axis, float d, kdtree::Stats &kd_stats);
		float splitPos() const { return division_; }
		Axis splitAxis() const { return static_cast<Axis>(flags_ & 3); }
		uint32_t nPrimitives() const { return flags_ >> 2; }
		const Primitive *getOnePrimitive() const { return one_primitive_; }
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
	Point3f point_; //!< the point coordinates of entry/exit point
	int prev_stack_id_; //!< the pointer to the previous stack item
};

struct AcceleratorKdTree::SplitCost
{
	Axis axis_ = Axis::None;
	int edge_offset_ = -1;
	float cost_;
	float t_;
	int num_edges_;
};

inline void AcceleratorKdTree::Node::createLeaf(const uint32_t *prim_idx, int np, const std::vector<const Primitive *> &prims, MemoryArena &arena, kdtree::Stats &kd_stats) {
	//primitives_ = nullptr;
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

inline void AcceleratorKdTree::Node::createInterior(int axis, float d, kdtree::Stats &kd_stats)
{
	division_ = d;
	flags_ = (flags_ & ~3) | axis;
	kd_stats.kd_inodes_++;
}

inline IntersectData AcceleratorKdTree::intersect(const Ray &ray, float t_max) const
{
	return kdtree::intersect<Node, Stack, kdtree::IntersectTestType::Nearest>(ray, t_max, nodes_, tree_bound_, 0, nullptr);
}

inline IntersectData AcceleratorKdTree::intersectShadow(const Ray &ray, float t_max) const
{
	return kdtree::intersect<Node, Stack, kdtree::IntersectTestType::Shadow>(ray, t_max, nodes_, tree_bound_, 0, nullptr);
}

inline IntersectData AcceleratorKdTree::intersectTransparentShadow(const Ray &ray, int max_depth, float t_max, const Camera *camera) const
{
	return kdtree::intersect<Node, Stack, kdtree::IntersectTestType::TransparentShadow>(ray, t_max, nodes_, tree_bound_, max_depth, camera);
}

} //namespace yafaray

#endif    //LIBYAFARAY_ACCELERATOR_KDTREE_ORIGINAL_H
