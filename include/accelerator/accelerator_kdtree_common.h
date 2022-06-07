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

BEGIN_YAFARAY

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

END_YAFARAY

#endif //YAFARAY_ACCELERATOR_KDTREE_COMMON_H
