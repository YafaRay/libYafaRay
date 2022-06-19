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

#ifndef YAFARAY_ACCELERATOR_KDTREE_COMMON_H
#define YAFARAY_ACCELERATOR_KDTREE_COMMON_H

#include "accelerator.h"
#include "geometry/bound.h"
#include "common/logger.h"
#include <map>

namespace yafaray {

namespace kdtree
{
enum class IntersectTestType : unsigned char { Nearest, Shadow, TransparentShadow };

static constexpr int kd_max_stack_global = 64;

/*! Serves to store the lower and upper bound edges of the primitives
	for the cost funtion */

struct BoundEdge
{
	enum class EndBound : unsigned char { Left, Both, Right };
	BoundEdge() = default;
	BoundEdge(float position, uint32_t index, EndBound bound_end): pos_(position), index_(index), end_(bound_end) { }
	bool operator<(const BoundEdge &e) const
	{
		if(pos_ == e.pos_) return end_ > e.end_;
		else return pos_ < e.pos_;
	}
	alignas(8) float pos_;
	uint32_t index_;
	EndBound end_;
};

struct TreeBin
{
	bool empty() const { return n_ == 0; };
	void reset() { n_ = 0, c_left_ = 0, c_right_ = 0, c_both_ = 0, c_bleft_ = 0;};
	alignas(8) int n_ = 0;
	int c_left_ = 0, c_right_ = 0;
	int c_bleft_ = 0, c_both_ = 0;
	float t_ = 0.f;
};

struct Stats
{
	void outputLog(Logger &logger, uint32_t num_primitives, int max_leaf_size) const;
	Stats operator += (const Stats &kd_stats);
	alignas(8) int kd_inodes_ = 0;
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

inline void Stats::outputLog(Logger &logger, uint32_t num_primitives, int max_leaf_size) const
{
	if(logger.isVerbose())
	{
		logger.logVerbose("Kd-Tree MultiThread: Primitives in tree: ", num_primitives);
		logger.logVerbose("Kd-Tree MultiThread: Interior nodes: ", kd_inodes_, " / ", "leaf nodes: ", kd_leaves_
						  , " (empty: ", empty_kd_leaves_, " = ", 100.f * static_cast<float>(empty_kd_leaves_) / kd_leaves_, "%)");
		logger.logVerbose("Kd-Tree MultiThread: Leaf prims: ", kd_prims_, " (", static_cast<float>(kd_prims_) / num_primitives, " x prims in tree, leaf size: ", max_leaf_size, ")");
		logger.logVerbose("Kd-Tree MultiThread: => ", static_cast<float>(kd_prims_) / (kd_leaves_ - empty_kd_leaves_), " prims per non-empty leaf");
		logger.logVerbose("Kd-Tree MultiThread: Leaves due to depth limit/bad splits: ", depth_limit_reached_, "/", num_bad_splits_);
		logger.logVerbose("Kd-Tree MultiThread: clipped primitives: ", clip_, " (", bad_clip_, " bad clips, ", null_clip_, " null clips)");
	}
}

inline Stats Stats::operator += (const Stats &kd_stats)
{
	kd_inodes_ += kd_stats.kd_inodes_;
	kd_leaves_ += kd_stats.kd_leaves_;
	empty_kd_leaves_ += kd_stats.empty_kd_leaves_;
	kd_prims_ += kd_stats.kd_prims_;
	clip_ += kd_stats.clip_;
	bad_clip_ += kd_stats.bad_clip_;
	null_clip_ += kd_stats.null_clip_;
	early_out_ += kd_stats.early_out_;
	depth_limit_reached_ += kd_stats.depth_limit_reached_;
	num_bad_splits_ += kd_stats.num_bad_splits_;
	return *this;
}

template<typename NodeType, typename NodeStackType, IntersectTestType test_type>
IntersectData intersect(const Ray &ray, float t_max, const std::vector<NodeType> &nodes, const Bound &tree_bound, int transparent_color_max_depth, const Camera *camera)
{
	const Bound::Cross cross{tree_bound.cross(ray, t_max)};
	if(!cross.crossed_)
	{ return {}; }
	const Vec3 inv_dir{math::inverse(ray.dir_.x()), math::inverse(ray.dir_.y()), math::inverse(ray.dir_.z())};
	int depth = 0;
	std::set<const Primitive *> filtered;
	std::array<NodeStackType, kd_max_stack_global> stack;
	const NodeType *far_child;
	const NodeType *curr_node;
	curr_node = nodes.data();

	int entry_id = 0;
	stack[entry_id].t_ = cross.enter_;

	//distinguish between internal and external origin
	if(cross.enter_ >= 0.f) // ray with external origin
		stack[entry_id].point_ = ray.from_ + ray.dir_ * cross.enter_;
	else // ray with internal origin
		stack[entry_id].point_ = ray.from_;

	// setup initial entry and exit point in stack
	int exit_id = 1; // pointer to stack
	stack[exit_id].t_ = cross.leave_;
	stack[exit_id].point_ = ray.from_ + ray.dir_ * cross.leave_;
	stack[exit_id].node_ = nullptr; // "nowhere", termination flag

	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	IntersectData intersect_data;
	intersect_data.t_max_ = t_max;
	const float t_min = (test_type == IntersectTestType::Shadow) ? Accelerator::calculateDynamicRayBias(cross) : std::max(ray.tmin_, Accelerator::calculateDynamicRayBias(cross));

	while(curr_node && stack[entry_id].t_ <= t_max)
	{
		// loop until leaf is found
		while(!curr_node->isLeaf())
		{
			const Axis axis = curr_node->splitAxis();
			const float split_val = curr_node->splitPos();
			if(stack[entry_id].point_[axis] <= split_val)
			{
				if(stack[exit_id].point_[axis] <= split_val)
				{
					++curr_node;
					continue;
				}
					// case N4
				else
				{
					far_child = &nodes[curr_node->getRightChild()];
					++curr_node;
				}
			}
			else
			{
				if(stack[exit_id].point_[axis] > split_val)
				{
					curr_node = &nodes[curr_node->getRightChild()];
					continue;
				}
				else
				{
					far_child = curr_node + 1;
					curr_node = &nodes[curr_node->getRightChild()];
				}
			}
			// traverse both children
			const float t = (split_val - ray.from_[axis]) * inv_dir[axis]; //splitting plane signed distance
			// set up the new exit point
			const int exit_id_prev = exit_id;
			++exit_id;
			// possibly skip current entry point so not to overwrite the data
			if(exit_id == entry_id) ++exit_id;
			// push values onto the stack //todo: lookup table
			const Axis next_axis{axis::getNextSpatial(axis)};
			const Axis prev_axis{axis::getPrevSpatial(axis)};
			stack[exit_id].prev_stack_id_ = exit_id_prev;
			stack[exit_id].t_ = t;
			stack[exit_id].node_ = far_child;
			stack[exit_id].point_[axis] = split_val;
			stack[exit_id].point_[next_axis] = ray.from_[next_axis] + t * ray.dir_[next_axis];
			stack[exit_id].point_[prev_axis] = ray.from_[prev_axis] + t * ray.dir_[prev_axis];
		}
		// Check for intersections inside leaf node
		const uint32_t n_primitives = curr_node->nPrimitives();
		if(n_primitives == 1)
		{
			const Primitive *primitive = curr_node->getOnePrimitive();
			if(test_type == IntersectTestType::Nearest)
			{
				Accelerator::primitiveIntersection(intersect_data, primitive, ray.from_, ray.dir_, t_min, intersect_data.t_max_, ray.time_);
			}
			else if(test_type == IntersectTestType::TransparentShadow)
			{
				if(Accelerator::primitiveIntersectionTransparentShadow(intersect_data, filtered, depth, transparent_color_max_depth, primitive, camera, ray.from_, ray.dir_, t_min, t_max, ray.time_)) return intersect_data;
			}
			else
			{
				if(Accelerator::primitiveIntersectionShadow(intersect_data, primitive, ray.from_, ray.dir_, t_min, t_max, ray.time_)) return intersect_data;
			}
		}
		else
		{
			const auto &prims = curr_node->primitives_;
			for(uint32_t i = 0; i < n_primitives; ++i)
			{
				const Primitive *primitive = prims[i];
				if(test_type == IntersectTestType::Nearest)
				{
					Accelerator::primitiveIntersection(intersect_data, primitive, ray.from_, ray.dir_, t_min, intersect_data.t_max_, ray.time_);
				}
				else if(test_type == IntersectTestType::TransparentShadow)
				{
					if(Accelerator::primitiveIntersectionTransparentShadow(intersect_data, filtered, depth, transparent_color_max_depth, primitive, camera, ray.from_, ray.dir_, t_min, t_max, ray.time_)) return intersect_data;
				}
				else
				{
					if(Accelerator::primitiveIntersectionShadow(intersect_data, primitive, ray.from_, ray.dir_, t_min, t_max, ray.time_)) return intersect_data;
				}
			}
		}
		if(test_type == IntersectTestType::Nearest)
		{
			if(intersect_data.isHit() && intersect_data.t_max_ <= stack[exit_id].t_)
			{
				return intersect_data;
			}
		}
		entry_id = exit_id;
		curr_node = stack[exit_id].node_;
		exit_id = stack[entry_id].prev_stack_id_;
	} // while

	if(test_type == IntersectTestType::Nearest)
	{
		return intersect_data;
	}
	else if(test_type == IntersectTestType::TransparentShadow)
	{
		intersect_data.setNoHit();
		return intersect_data;
	}
	else
	{
		return {};
	}
}
} //namespace kdtree

} //namespace yafaray

#endif //YAFARAY_ACCELERATOR_KDTREE_COMMON_H
