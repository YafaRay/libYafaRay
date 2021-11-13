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

#include "accelerator/accelerator_kdtree_multi_thread.h"
#include "material/material.h"
#include "common/logger.h"
#include "geometry/surface.h"
#include "geometry/primitive/primitive.h"
#include "geometry/axis.h"
#include "common/param.h"
#include "image/image_output.h"
#include <thread>

BEGIN_YAFARAY

std::unique_ptr<Accelerator> AcceleratorKdTreeMultiThread::factory(Logger &logger, const std::vector<const Primitive *> &primitives, ParamMap &params)
{
	AcceleratorKdTreeMultiThread::Parameters parameters;

	params.getParam("depth", parameters.max_depth_);
	params.getParam("leaf_size", parameters.max_leaf_size_);
	params.getParam("cost_ratio", parameters.cost_ratio_);
	params.getParam("empty_bonus", parameters.empty_bonus_);
	params.getParam("accelerator_threads", parameters.num_threads_);
	params.getParam("accelerator_min_indices_threads", parameters.min_indices_to_spawn_threads_);

	auto accelerator = std::unique_ptr<Accelerator>(new AcceleratorKdTreeMultiThread(logger, primitives, parameters));
	return accelerator;
}

AcceleratorKdTreeMultiThread::AcceleratorKdTreeMultiThread(Logger &logger, const std::vector<const Primitive *> &primitives, const Parameters &parameters) : Accelerator(logger)
{
	Parameters tree_build_parameters = parameters;

	const uint32_t num_primitives = static_cast<uint32_t>(primitives.size());
	logger_.logInfo("Kd-Tree MultiThread: Starting build (", num_primitives, " prims, cost_ratio:", parameters.cost_ratio_, " empty_bonus:", parameters.empty_bonus_, ") [using ", tree_build_parameters.num_threads_, " threads, min indices to spawn threads: ", tree_build_parameters.min_indices_to_spawn_threads_, "]");
	clock_t clock_start = clock();
	if(tree_build_parameters.max_depth_ <= 0) tree_build_parameters.max_depth_ = static_cast<int>(7.0f + 1.66f * log(static_cast<float>(num_primitives)));
	const double log_leaves = 1.442695f * log(static_cast<double >(num_primitives)); // = base2 log
	if(tree_build_parameters.max_leaf_size_ <= 0)
	{
		int mls = static_cast<int>(log_leaves - 16.0);
		if(mls <= 0) mls = 1;
		tree_build_parameters.max_leaf_size_ = mls;
	}
	if(tree_build_parameters.max_depth_ > kd_max_stack_) tree_build_parameters.max_depth_ = kd_max_stack_; //to prevent our stack to overflow
	//experiment: add penalty to cost ratio to reduce memory usage on huge scenes
	if(log_leaves > 16.0) tree_build_parameters.cost_ratio_ += 0.25 * (log_leaves - 16.0);
	std::vector<Bound> bounds;
	bounds.reserve(num_primitives);
	tree_bound_ = primitives.front()->getBound();
	if(logger_.isVerbose()) logger_.logVerbose("Kd-Tree MultiThread: Getting primitive bounds...");
	for(const auto &primitive : primitives)
	{
		bounds.emplace_back(primitive->getBound());
		/* calc tree bound. Remember to upgrade bound_t class... */
		tree_bound_ = Bound(tree_bound_, bounds.back());
	}
	//slightly(!) increase tree bound to prevent errors with prims
	//lying in a bound plane (still slight bug with trees where one dim. is 0)
	for(int axis = 0; axis < 3; ++axis)
	{
		const double foo = (tree_bound_.g_[axis] - tree_bound_.a_[axis]) * 0.001;
		tree_bound_.a_[axis] -= foo, tree_bound_.g_[axis] += foo;
	}
	if(logger_.isVerbose()) logger_.logVerbose("Kd-Tree MultiThread: Done.");
	std::vector<uint32_t> prim_indices(num_primitives);
	for(uint32_t prim_num = 0; prim_num < num_primitives; prim_num++) prim_indices[prim_num] = prim_num;
	if(logger_.isVerbose()) logger_.logVerbose("Kd-Tree MultiThread: Starting recursive build...");
	const Result kd_tree_result = buildTree(primitives, tree_bound_, prim_indices, 0, 0, 0, bounds, tree_build_parameters, ClipPlane::Pos::None, {}, {}, num_current_threads_);
	nodes_ = std::move(kd_tree_result.nodes_);
	//print some stats:
	const clock_t clock_elapsed = clock() - clock_start;
	if(logger_.isVerbose())
	{
		logger_.logVerbose("Kd-Tree MultiThread: CPU total clocks (in seconds): ", static_cast<float>(clock_elapsed) / static_cast<float>(CLOCKS_PER_SEC), "s (actual CPU work, including the work done by all threads added together)");
		logger_.logVerbose("Kd-Tree MultiThread: used/allocated nodes: ", nodes_.size(), "/", nodes_.capacity()
				 , " (", 100.f * static_cast<float>(nodes_.size()) / nodes_.capacity(), "%)");
	}
	kd_tree_result.stats_.outputLog(logger, num_primitives, tree_build_parameters.max_leaf_size_);
}

void AcceleratorKdTreeMultiThread::Stats::outputLog(Logger &logger, uint32_t num_primitives, int max_leaf_size) const
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

AcceleratorKdTreeMultiThread::Stats AcceleratorKdTreeMultiThread::Stats::operator += (const Stats &kd_stats)
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

AcceleratorKdTreeMultiThread::~AcceleratorKdTreeMultiThread()
{
	if(logger_.isVerbose()) logger_.logVerbose("Kd-Tree MultiThread: Done");
}

// ============================================================
/*!
	Faster cost function: Find the optimal split with SAH
	and binning => O(n)
*/

AcceleratorKdTreeMultiThread::SplitCost AcceleratorKdTreeMultiThread::pigeonMinCost(Logger &logger, float e_bonus, float cost_ratio, const std::vector<Bound> &bounds, const Bound &node_bound, const std::vector<uint32_t> &prim_indices)
{
	const uint32_t num_prim_indices = static_cast<uint32_t>(prim_indices.size());
	static constexpr int max_bin = 1024;
	static constexpr int num_bins = max_bin + 1;
	std::array<TreeBin, num_bins> bins;
	const Vec3 node_bound_axes {node_bound.longX(), node_bound.longY(), node_bound.longZ() };
	const Vec3 inv_node_bound_axes { 1.f / node_bound_axes[0], 1.f / node_bound_axes[1], 1.f / node_bound_axes[2] };
	SplitCost split;
	split.cost_ = std::numeric_limits<float>::infinity();
	const float inv_total_sa = 1.f / (node_bound_axes[0] * node_bound_axes[1] + node_bound_axes[0] * node_bound_axes[2] + node_bound_axes[1] * node_bound_axes[2]);

	for(int axis = 0; axis < 3; ++axis)
	{
		const float s = max_bin * inv_node_bound_axes[axis];
		const float min = node_bound.a_[axis];
		// pigeonhole sort:
		for(uint32_t prim_num = 0; prim_num < num_prim_indices; ++prim_num)
		{
			const Bound &bbox = bounds[prim_indices[prim_num]];
			const float t_low = bbox.a_[axis];
			const float t_up  = bbox.g_[axis];
			int b_left = static_cast<int>((t_low - min) * s);
			int b_right = static_cast<int>((t_up - min) * s);
			if(b_left < 0) b_left = 0;
			else if(b_left > max_bin) b_left = max_bin;
			if(b_right < 0) b_right = 0;
			else if(b_right > max_bin) b_right = max_bin;

			if(t_low == t_up)
			{
				if(bins[b_left].empty() || (t_low >= bins[b_left].t_ && !bins[b_left].empty()))
				{
					bins[b_left].t_ = t_low;
					bins[b_left].c_both_++;
				}
				else
				{
					bins[b_left].c_left_++;
					bins[b_left].c_right_++;
				}
				bins[b_left].n_ += 2;
			}
			else
			{
				if(bins[b_left].empty() || (t_low > bins[b_left].t_ && !bins[b_left].empty()))
				{
					bins[b_left].t_ = t_low;
					bins[b_left].c_left_ += bins[b_left].c_both_ + bins[b_left].c_bleft_;
					bins[b_left].c_right_ += bins[b_left].c_both_;
					bins[b_left].c_both_ = bins[b_left].c_bleft_ = 0;
					bins[b_left].c_bleft_++;
				}
				else if(t_low == bins[b_left].t_)
				{
					bins[b_left].c_bleft_++;
				}
				else bins[b_left].c_left_++;
				bins[b_left].n_++;

				bins[b_right].c_right_++;
				if(bins[b_right].empty() || t_up > bins[b_right].t_)
				{
					bins[b_right].t_ = t_up;
					bins[b_right].c_left_ += bins[b_right].c_both_ + bins[b_right].c_bleft_;
					bins[b_right].c_right_ += bins[b_right].c_both_;
					bins[b_right].c_both_ = bins[b_right].c_bleft_ = 0;
				}
				bins[b_right].n_++;
			}
		}
		const int next_axis = Axis::next(axis);
		const int prev_axis = Axis::prev(axis);
		const float cap_area = node_bound_axes[next_axis] * node_bound_axes[prev_axis];
		const float cap_perim = node_bound_axes[next_axis] + node_bound_axes[prev_axis];

		uint32_t num_left = 0;
		uint32_t num_right = num_prim_indices;
		// cumulate prims and evaluate cost
		for(const auto &bin : bins)
		{
			if(!bin.empty())
			{
				num_left += bin.c_left_;
				num_right -= bin.c_right_;
				// cost:
				const float edget = bin.t_;
				if(edget > node_bound.a_[axis] && edget < node_bound.g_[axis])
				{
					// Compute cost for split at _i_th edge
					const float l_below = edget - node_bound.a_[axis];
					const float l_above = node_bound.g_[axis] - edget;
					const float below_sa = cap_area + l_below * cap_perim;
					const float above_sa = cap_area + l_above * cap_perim;
					const float raw_costs = (below_sa * num_left + above_sa * num_right);
					float eb;
					if(num_right == 0) eb = (0.1f + l_above * inv_node_bound_axes[axis]) * e_bonus * raw_costs;
					else if(num_left == 0) eb = (0.1f + l_below * inv_node_bound_axes[axis]) * e_bonus * raw_costs;
					else eb = 0.f;

					const float cost = cost_ratio + inv_total_sa * (raw_costs - eb);

					// Update best split if this is lowest cost so far
					if(cost < split.cost_)
					{
						split.t_ = edget;
						split.cost_ = cost;
						split.axis_ = axis;
					}
				}
				num_left += bin.c_both_ + bin.c_bleft_;
				num_right -= bin.c_both_;
			}
		} // for all bins
		if(num_left != num_prim_indices || num_right != 0)
		{
			if(logger.isVerbose())
			{
				int c_1 = 0, c_2 = 0, c_3 = 0, c_4 = 0, c_5 = 0;
				logger.logVerbose("SCREWED!!");
				for(const auto &bin : bins) { c_1 += bin.n_; logger.logVerbose(bin.n_, " "); }
				logger.logVerbose("\nn total: ", c_1);
				for(const auto &bin : bins) { c_2 += bin.c_left_; logger.logVerbose(bin.c_left_, " "); }
				logger.logVerbose("\nc_left total: ", c_2);
				for(const auto &bin : bins) { c_3 += bin.c_bleft_; logger.logVerbose(bin.c_bleft_, " "); }
				logger.logVerbose("\nc_bleft total: ", c_3);
				for(const auto &bin : bins) { c_4 += bin.c_both_; logger.logVerbose(bin.c_both_, " "); }
				logger.logVerbose("\nc_both total: ", c_4);
				for(const auto &bin : bins) { c_5 += bin.c_right_; logger.logVerbose(bin.c_right_, " "); }
				logger.logVerbose("\nc_right total: ", c_5);
				logger.logVerbose("\nnPrims: ", num_prim_indices, " num_left: ", num_left, " num_right: ", num_right);
				logger.logVerbose("total left: ", c_2 + c_3 + c_4, "\ntotal right: ", c_4 + c_5);
				logger.logVerbose("n/2: ", c_1 / 2);
			}
			throw std::logic_error("cost function mismatch");
		}
		for(auto &bin : bins) bin.reset();
	} // for all axis
	return split;
}

// ============================================================
/*!
	Cost function: Find the optimal split with SAH
*/

AcceleratorKdTreeMultiThread::SplitCost AcceleratorKdTreeMultiThread::minimalCost(Logger &logger, float e_bonus, float cost_ratio, const Bound &node_bound, const std::vector<uint32_t> &indices, const std::vector<Bound> &bounds)
{
	const uint32_t num_indices = static_cast<uint32_t>(indices.size());
	const Vec3 node_bound_axes {node_bound.longX(), node_bound.longY(), node_bound.longZ() };
	const Vec3 inv_node_bound_axes { 1.f / node_bound_axes[0], 1.f / node_bound_axes[1], 1.f / node_bound_axes[2] };
	SplitCost split;
	split.cost_ = std::numeric_limits<float>::infinity();
	const float inv_total_sa = 1.f / (node_bound_axes[0] * node_bound_axes[1] + node_bound_axes[0] * node_bound_axes[2] + node_bound_axes[1] * node_bound_axes[2]);
	std::array<std::vector<BoundEdge>, 3> edges_all_axes;
	for(int axis = 0; axis < 3; ++axis)
	{
		// << get edges for axis >>
		const int next_axis = Axis::next(axis);
		const int prev_axis = Axis::prev(axis);
		edges_all_axes[axis].reserve(num_indices * 2);
		for(const auto &index : indices)
		{
			const Bound &bbox = bounds[index];
			if(bbox.a_[axis] == bbox.g_[axis])
			{
				edges_all_axes[axis].emplace_back(BoundEdge(bbox.a_[axis], index, BoundEdge::EndBound::Both));
			}
			else
			{
				edges_all_axes[axis].emplace_back(BoundEdge(bbox.a_[axis], index, BoundEdge::EndBound::Left));
				edges_all_axes[axis].emplace_back(BoundEdge(bbox.g_[axis], index, BoundEdge::EndBound::Right));
			}
		}
		std::sort(edges_all_axes[axis].begin(), edges_all_axes[axis].end());
		// Compute cost of all splits for _axis_ to find best
		const float cap_area = node_bound_axes[next_axis] * node_bound_axes[prev_axis];
		const float cap_perim = node_bound_axes[next_axis] + node_bound_axes[prev_axis];
		uint32_t num_left = 0;
		uint32_t num_right = num_indices;
		//todo: early-out criteria: if l1 > l2*nPrims (l2 > l1*nPrims) => minimum is lowest (highest) edge!
		if(num_indices > 5)
		{
			float edget = edges_all_axes[axis].front().pos_;
			float l_below = edget - node_bound.a_[axis];
			float l_above = node_bound.g_[axis] - edget;
			if(l_below > l_above * static_cast<float>(num_indices) && l_above > 0.f)
			{
				const float raw_costs = (cap_area + l_above * cap_perim) * num_indices;
				const float cost = cost_ratio + inv_total_sa * (raw_costs - e_bonus); //todo: use proper ebonus...
				//optimal cost is definitely here, and nowhere else!
				if(cost < split.cost_)
				{
					split.cost_ = cost;
					split.axis_ = axis;
					split.edge_offset_ = 0;
					++split.stats_early_out_;
				}
				continue;
			}
			edget = edges_all_axes[axis].back().pos_;
			l_below = edget - node_bound.a_[axis];
			l_above = node_bound.g_[axis] - edget;
			if(l_above > l_below * static_cast<float>(num_indices) && l_below > 0.f)
			{
				const float raw_costs = (cap_area + l_below * cap_perim) * num_indices;
				const float cost = cost_ratio + inv_total_sa * (raw_costs - e_bonus); //todo: use proper ebonus...
				if(cost < split.cost_)
				{
					split.cost_ = cost;
					split.axis_ = axis;
					split.edge_offset_ = edges_all_axes[axis].size() - 1;
					++split.stats_early_out_;
				}
				continue;
			}
		}
		const int num_edges = static_cast<int>(edges_all_axes[axis].size());
		for(int edge_id = 0; edge_id < num_edges; ++edge_id)
		{
			if(edges_all_axes[axis][edge_id].end_ == BoundEdge::EndBound::Right) --num_right;
			const float edget = edges_all_axes[axis][edge_id].pos_;
			if(edget > node_bound.a_[axis] &&
			   edget < node_bound.g_[axis])
			{
				// Compute cost for split at _i_th edge
				const float l_below = edget - node_bound.a_[axis];
				const float l_above = node_bound.g_[axis] - edget;
				const float below_sa = cap_area + (l_below) * cap_perim;
				const float above_sa = cap_area + (l_above) * cap_perim;
				const float raw_costs = (below_sa * num_left + above_sa * num_right);
				float eb;
				if(num_right == 0) eb = (0.1f + l_above * inv_node_bound_axes[axis]) * e_bonus * raw_costs;
				else if(num_left == 0) eb = (0.1f + l_below * inv_node_bound_axes[axis]) * e_bonus * raw_costs;
				else eb = 0.0f;

				const float cost = cost_ratio + inv_total_sa * (raw_costs - eb);
				// Update best split if this is lowest cost so far
				if(cost < split.cost_)
				{
					split.cost_ = cost;
					split.axis_ = axis;
					split.edge_offset_ = edge_id;
				}
			}
			if(edges_all_axes[axis][edge_id].end_ != BoundEdge::EndBound::Right)
			{
				++num_left;
				if(edges_all_axes[axis][edge_id].end_ == BoundEdge::EndBound::Both) --num_right;
			}
		}
		if((num_left != num_indices || num_right != 0) && logger.isVerbose()) logger.logVerbose("you screwed your new idea!");
	}
	if(split.axis_ != Axis::None) split.edges_ = std::move(edges_all_axes[split.axis_]);
	return split;
}

// ============================================================
/*!
	recursively build the Kd-tree
*/
AcceleratorKdTreeMultiThread::Result AcceleratorKdTreeMultiThread::buildTree(const std::vector<const Primitive *> &primitives, const Bound &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound> &bounds, const Parameters &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, std::atomic<int> &num_current_threads) const
{
	Result kd_tree_result;
	buildTreeWorker(primitives, node_bound, indices, depth, next_node_id, bad_refines, bounds, parameters, clip_plane, polygons, primitive_indices, kd_tree_result, num_current_threads);
	return kd_tree_result;
}

void AcceleratorKdTreeMultiThread::buildTreeWorker(const std::vector<const Primitive *> &primitives, const Bound &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound> &bounds, const Parameters &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, Result &result, std::atomic<int> &num_current_threads) const
{
	//Note: "indices" are:
	// * primitive indices when not clipping primitives as polygons. In that case all primitive bounds are present using the same indexing as the complete primitive list
	// * polygon indices when clipping. In that case only the polygon bounds are present using the same indexing as the polygons list. In this case
	//   we need to keep track separately of the primitive indices corresponding to each polygon+bound using a separate prim_indices list.
	static constexpr int pigeon_sort_threshold = 128;
	auto new_indices = std::ref(indices);
	auto new_primitive_indices = std::ref(primitive_indices);
	auto new_bounds = std::ref(bounds);
	std::vector<PolyDouble> new_polygons;
	result = {};

#if POLY_CLIPPING_MULTITHREAD > 0
	static constexpr int poly_clipping_threshold = 32;
	std::vector<uint32_t> poly_indices;
	std::vector<uint32_t> prim_indices;
	std::vector<Bound> poly_bounds;
	const uint32_t num_indices = static_cast<uint32_t>(indices.size());
	const bool do_poly_clipping = (num_indices <= poly_clipping_threshold && num_indices <= primitive_indices.size());
	if(do_poly_clipping)
	{
		poly_indices.reserve(poly_clipping_threshold);
		prim_indices.reserve(poly_clipping_threshold);
		poly_bounds.reserve(poly_clipping_threshold);
		Vec3Double b_half_size;
		std::array<Vec3Double, 2> b_ext;
		for(int axis = 0; axis < 3; ++axis)
		{
			b_half_size[axis] = (static_cast<double>(node_bound.g_[axis]) - static_cast<double>(node_bound.a_[axis]));
			const double additional_offset_factor = (static_cast<double>(tree_bound_.g_[axis]) - static_cast<double>(tree_bound_.a_[axis]));
			b_ext[0][axis] = node_bound.a_[axis] - 0.021 * b_half_size[axis] - 0.00001 * additional_offset_factor;
			b_ext[1][axis] = node_bound.g_[axis] + 0.021 * b_half_size[axis] + 0.00001 * additional_offset_factor;
		}
		new_polygons.reserve(poly_clipping_threshold);
		for(uint32_t index_num = 0; index_num < num_indices; ++index_num)
		{
			const uint32_t prim_id = primitive_indices[index_num];
			const Primitive *primitive = primitives[prim_id];
			if(primitive->clippingSupport())
			{
				const uint32_t poly_id = (clip_plane.pos_ != ClipPlane::Pos::None) ? indices[index_num] : 0; //If the clipping plane is "None" the initial polygon will be ignored anyway, so we can use poly_id=0 to avoid going outside the list of polygons during first clip, when the indices list is a list of primitives indices and not a list of polygons indices
				const PolyDouble::ClipResultWithBound clip_result = primitive->clipToBound(logger_, b_ext, clip_plane, polygons[poly_id], nullptr);
				//std::string polygon_str = "(polygons empty!)";
				//if(!polygons.empty()) polygon_str = polygons[poly_id].print();
				//if(logger_.isDebug()) Y_DEBUG PRPREC(12) << " depth=" << depth << " i=" << index_num << " poly_id=" << poly_id << " i_poly=" << polygon_str << " result=" << clip_result.clip_result_code_ << " o_poly=" << clip_result.poly_.print(");

				if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Correct)
				{
					++result.stats_.clip_;
					new_polygons.emplace_back(clip_result.poly_);
					poly_bounds.emplace_back(*clip_result.box_);
					poly_indices.emplace_back(static_cast<uint32_t>(poly_indices.size()));
					prim_indices.emplace_back(prim_id);
				}
				else ++result.stats_.null_clip_;
			}
			else
			{
				// no clipping supported by prim, copy old bound:
				poly_bounds.emplace_back(bounds[prim_id]);
				prim_indices.emplace_back(prim_id);
			}
		}
		new_indices = std::reference_wrapper<const std::vector<uint32_t>>(poly_indices); //Using polygon indices instead of primitive indices now!
		new_primitive_indices = std::reference_wrapper<const std::vector<uint32_t>>(prim_indices); //Only forwarding the primitive indices corresponding with currently existing polygons
		new_bounds = std::reference_wrapper<const std::vector<Bound>>(poly_bounds); //Using polygon indices for bounds too, instead of primitive indices now.
	}
#endif

	const uint32_t num_new_indices = static_cast<uint32_t>(new_indices.get().size());
	//	<< check if leaf criteria met >>
	if(num_new_indices <= static_cast<uint32_t>(parameters.max_leaf_size_) || depth >= parameters.max_depth_)
	{
		Node node;
		const Stats leaf_stats = node.createLeaf(new_primitive_indices, primitives);
		result.stats_ += leaf_stats;
		result.nodes_.emplace_back(node);
		if(depth >= parameters.max_depth_) result.stats_.depth_limit_reached_++;
		return;
	}

	//<< calculate cost for all axes and chose minimum >>
	const float modified_empty_bonus = parameters.empty_bonus_ * (1.1 - static_cast<float>(depth) / static_cast<float>(parameters.max_depth_));
	SplitCost split;
	if(num_new_indices > pigeon_sort_threshold) split = pigeonMinCost(logger_, modified_empty_bonus, parameters.cost_ratio_, new_bounds, node_bound, new_indices);
	else split = minimalCost(logger_, modified_empty_bonus, parameters.cost_ratio_, node_bound, new_indices, new_bounds);
	result.stats_.early_out_ += split.stats_early_out_;
	//<< if (minimum > leafcost) increase bad refines >>
	if(split.cost_ > static_cast<float>(num_new_indices)) ++bad_refines;
	if((split.cost_ > 1.6f * static_cast<float>(num_new_indices) && num_new_indices < 16) ||
	   split.axis_ == Axis::None || bad_refines == 2)
	{
		Node node;
		const Stats leaf_stats = node.createLeaf(new_primitive_indices, primitives);
		result.stats_ += leaf_stats;
		result.nodes_.emplace_back(node);
		if(bad_refines == 2) ++result.stats_.num_bad_splits_;
		return;
	}

	// Classify primitives with respect to split
	float split_pos;
	std::vector<uint32_t> left_indices;
	std::vector<uint32_t> right_indices;
	std::vector<uint32_t> left_primitive_indices;
	std::vector<uint32_t> right_primitive_indices;
	if(num_new_indices > pigeon_sort_threshold) // we did pigeonhole
	{
		for(uint32_t prim_num = 0; prim_num < num_new_indices; prim_num++)
		{
			const int prim_id = new_indices.get()[prim_num];
			if(new_bounds.get()[prim_id].a_[split.axis_] >= split.t_)
			{
				right_indices.emplace_back(prim_id);
			}
			else
			{
				left_indices.emplace_back(prim_id);
				if(new_bounds.get()[prim_id].g_[split.axis_] > split.t_)
				{
					right_indices.emplace_back(prim_id);
				}
			}
		}
		split_pos = split.t_;
		left_primitive_indices = left_indices;
		right_primitive_indices = right_indices;
	}
#if POLY_CLIPPING_MULTITHREAD > 0
	else if(do_poly_clipping)
	{
		for(int edge_id = 0; edge_id < split.edge_offset_; ++edge_id)
		{
			if(split.edges_[edge_id].end_ != BoundEdge::EndBound::Right)
			{
				left_indices.emplace_back(split.edges_[edge_id].index_);
				left_primitive_indices.emplace_back(new_primitive_indices.get()[left_indices.back()]);
			}
		}
		if(split.edges_[split.edge_offset_].end_ == BoundEdge::EndBound::Both)
		{
			right_indices.emplace_back(split.edges_[split.edge_offset_].index_);
			right_primitive_indices.emplace_back(new_primitive_indices.get()[right_indices.back()]);
		}
		const int num_edges = static_cast<int>(split.edges_.size());
		for(int edge_id = split.edge_offset_ + 1; edge_id < num_edges; ++edge_id)
		{
			if(split.edges_[edge_id].end_ != BoundEdge::EndBound::Left)
			{
				right_indices.emplace_back(split.edges_[edge_id].index_);
				right_primitive_indices.emplace_back(new_primitive_indices.get()[right_indices.back()]);
			}
		}
		split_pos = split.edges_[split.edge_offset_].pos_;
	}
#endif //POLY_CLIPPING_MULTITHREAD > 0
	else //we did "normal" cost function
	{
		for(int edge_id = 0; edge_id < split.edge_offset_; ++edge_id)
		{
			if(split.edges_[edge_id].end_ != BoundEdge::EndBound::Right)
			{
				left_indices.emplace_back(split.edges_[edge_id].index_);
			}
		}
		if(split.edges_[split.edge_offset_].end_ == BoundEdge::EndBound::Both)
		{
			right_indices.emplace_back(split.edges_[split.edge_offset_].index_);
		}
		const int num_edges = static_cast<int>(split.edges_.size());
		for(int edge_id = split.edge_offset_ + 1; edge_id < num_edges; ++edge_id)
		{
			if(split.edges_[edge_id].end_ != BoundEdge::EndBound::Left)
			{
				right_indices.emplace_back(split.edges_[edge_id].index_);
			}
		}
		split_pos = split.edges_[split.edge_offset_].pos_;
		left_primitive_indices = left_indices;
		right_primitive_indices = right_indices;
	}

	Node node;
	const Stats interior_stats = node.createInterior(split.axis_, split_pos);
	result.stats_ += interior_stats;
	result.nodes_.emplace_back(node);
	Bound bound_left = node_bound;
	Bound bound_right = node_bound;
	switch(split.axis_)
	{
		case Axis::X: bound_left.setMaxX(split_pos); bound_right.setMinX(split_pos); break;
		case Axis::Y: bound_left.setMaxY(split_pos); bound_right.setMinY(split_pos); break;
		case Axis::Z: bound_left.setMaxZ(split_pos); bound_right.setMinZ(split_pos); break;
	}

	ClipPlane left_clip_plane;
	ClipPlane right_clip_plane;
#if POLY_CLIPPING_MULTITHREAD > 0
	if(do_poly_clipping)
	{
		left_clip_plane = {split.axis_, ClipPlane::Pos::Upper};
		right_clip_plane = {split.axis_, ClipPlane::Pos::Lower};
	}
#endif //POLY_CLIPPING_MULTITHREAD > 0

	if(num_current_threads_ < parameters.num_threads_ && (left_primitive_indices.size() >= (right_primitive_indices.size() / 10)) && (right_primitive_indices.size() >= (left_primitive_indices.size() / 10)) && (left_primitive_indices.size() >= static_cast<size_t>(parameters.min_indices_to_spawn_threads_)))
	{
		const uint32_t next_free_node_original = static_cast<uint32_t>(next_node_id + result.nodes_.size());
		Result result_left;
		auto left_worker = std::thread(&AcceleratorKdTreeMultiThread::buildTreeWorker, this, primitives, bound_left, left_indices, depth + 1, next_free_node_original, bad_refines, new_bounds, parameters, left_clip_plane, new_polygons, left_primitive_indices, std::ref(result_left), std::ref(num_current_threads));
		num_current_threads++;
		Result result_right;
		auto right_worker = std::thread(&AcceleratorKdTreeMultiThread::buildTreeWorker, this, primitives, bound_right, right_indices, depth + 1, 0, bad_refines, new_bounds, parameters, right_clip_plane, new_polygons, right_primitive_indices, std::ref(result_right), std::ref(num_current_threads)); //We don't need to specify next_free_node (set to 0) because all internor node right childs will be modified later adding the left nodes list size once it's known
		num_current_threads++;
		left_worker.join();
		num_current_threads--;
		right_worker.join();
		num_current_threads--;

		result.stats_ += result_left.stats_;
		result.stats_ += result_right.stats_;

		const uint32_t num_nodes_left = static_cast<uint32_t>(result_left.nodes_.size());
		const uint32_t next_free_node_left = next_free_node_original + num_nodes_left;
		result.nodes_.back().setRightChild(next_free_node_left);
		//Correct the "right child" in the right nodes adding the original nodes + left nodes list sizes as offset to each
		for(auto &right_node : result_right.nodes_)
		{
			if(!right_node.isLeaf()) right_node.setRightChild(right_node.getRightChild() + next_free_node_left);
		}
		result.nodes_.reserve(next_free_node_original + num_nodes_left + result_right.nodes_.size());
		result.nodes_.insert(result.nodes_.end(), result_left.nodes_.begin(), result_left.nodes_.end());
		result.nodes_.insert(result.nodes_.end(), result_right.nodes_.begin(), result_right.nodes_.end());
	}
	else
	{
		//<< recurse left child >>
		const Result result_left = buildTree(primitives, bound_left, left_indices, depth + 1, next_node_id + result.nodes_.size(), bad_refines, new_bounds, parameters, left_clip_plane, new_polygons, left_primitive_indices, num_current_threads);
		result.stats_ += result_left.stats_;
		result.nodes_.back().setRightChild(next_node_id + result.nodes_.size() + result_left.nodes_.size());
		result.nodes_.insert(result.nodes_.end(), result_left.nodes_.begin(), result_left.nodes_.end());

		//<< recurse right child >>
		const Result result_right = buildTree(primitives, bound_right, right_indices, depth + 1, next_node_id + result.nodes_.size(), bad_refines, new_bounds, parameters, right_clip_plane, new_polygons, right_primitive_indices, num_current_threads);
		result.stats_ += result_right.stats_;
		result.nodes_.insert(result.nodes_.end(), result_right.nodes_.begin(), result_right.nodes_.end());
	}
}

//============================
/*! The standard sphereIntersect function,
	returns the closest hit within dist
*/
AcceleratorIntersectData AcceleratorKdTreeMultiThread::intersect(const Ray &ray, float t_max) const
{
	return intersect(ray, t_max, nodes_, tree_bound_);
}

AcceleratorIntersectData AcceleratorKdTreeMultiThread::intersect(const Ray &ray, float t_max, const std::vector<Node> &nodes, const Bound &tree_bound)
{
	AcceleratorIntersectData accelerator_intersect_data;
	accelerator_intersect_data.t_max_ = t_max;
	const Bound::Cross cross = tree_bound.cross(ray, t_max);
	if(!cross.crossed_) { return {}; }

	const Vec3 inv_dir(1.f / ray.dir_.x_, 1.f / ray.dir_.y_, 1.f / ray.dir_.z_);

	std::array<Stack, kd_max_stack_> stack;
	const Node *far_child;
	const Node *curr_node;
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
	while(curr_node)
	{
		if(t_max < stack[entry_id].t_) break;
		// loop until leaf is found
		while(!curr_node->isLeaf())
		{
			const int axis = curr_node->splitAxis();
			const float split_val = curr_node->splitPos();

			if(stack[entry_id].point_[axis] <= split_val)
			{
				if(stack[exit_id].point_[axis] <= split_val)
				{
					curr_node++;
					continue;
				}
				if(stack[exit_id].point_[axis] == split_val)
				{
					curr_node = &nodes[curr_node->getRightChild()];
					continue;
				}
				// case N4
				far_child = &nodes[curr_node->getRightChild()];
				curr_node ++;
			}
			else
			{
				if(split_val < stack[exit_id].point_[axis])
				{
					curr_node = &nodes[curr_node->getRightChild()];
					continue;
				}
				far_child = curr_node + 1;
				curr_node = &nodes[curr_node->getRightChild()];
			}
			// traverse both children

			const float t = (split_val - ray.from_[axis]) * inv_dir[axis]; //splitting plane signed distance

			// set up the new exit point
			const int tmp_id = exit_id;
			exit_id++;

			// possibly skip current entry point so not to overwrite the data
			if(exit_id == entry_id) exit_id++;
			// push values onto the stack //todo: lookup table
			static constexpr std::array<std::array<int, 3>, 2> np_axis {{{1, 2, 0}, {2, 0, 1} }};
			const int next_axis = np_axis[0][axis];
			const int prev_axis = np_axis[1][axis];
			stack[exit_id].prev_stack_id_ = tmp_id;
			stack[exit_id].t_ = t;
			stack[exit_id].node_ = far_child;
			stack[exit_id].point_[axis] = split_val;
			stack[exit_id].point_[next_axis] = ray.from_[next_axis] + t * ray.dir_[next_axis];
			stack[exit_id].point_[prev_axis] = ray.from_[prev_axis] + t * ray.dir_[prev_axis];
		}

		// Check for intersections inside leaf node
		const auto &primitive_intersection = [](AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray) -> void
		{
			const IntersectData intersect_data = primitive->intersect(ray);
			if(intersect_data.hit_)
			{
				if(intersect_data.t_hit_ < accelerator_intersect_data.t_max_ && intersect_data.t_hit_ >= ray.tmin_)
				{
					const Visibility prim_visibility = primitive->getVisibility();
					if(prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::VisibleNoShadows)
					{
						const Visibility mat_visibility = primitive->getMaterial()->getVisibility();
						if(mat_visibility == Visibility::NormalVisible || mat_visibility == Visibility::VisibleNoShadows)
						{
							accelerator_intersect_data.setIntersectData(intersect_data);
							accelerator_intersect_data.t_max_ = intersect_data.t_hit_;
							accelerator_intersect_data.hit_primitive_ = primitive;
						}
					}
				}
			}
		};
		for(const auto &prim : curr_node->primitives_)
		{
			primitive_intersection(accelerator_intersect_data, prim, ray);
		}
		if(accelerator_intersect_data.hit_ && accelerator_intersect_data.t_max_ <= stack[exit_id].t_)
		{
			return accelerator_intersect_data;
		}

		entry_id = exit_id;
		curr_node = stack[exit_id].node_;
		exit_id = stack[entry_id].prev_stack_id_;
	} // while
	return accelerator_intersect_data;
}

AcceleratorIntersectData AcceleratorKdTreeMultiThread::intersectS(const Ray &ray, float t_max, float shadow_bias) const
{
	return intersectS(ray, t_max, shadow_bias, nodes_, tree_bound_);
}

AcceleratorIntersectData AcceleratorKdTreeMultiThread::intersectS(const Ray &ray, float t_max, float, const std::vector<Node> &nodes, const Bound &tree_bound)
{
	AcceleratorIntersectData accelerator_intersect_data;
	const Bound::Cross cross = tree_bound.cross(ray, t_max);
	if(!cross.crossed_) { return {}; }
	const Vec3 inv_dir(1.f / ray.dir_.x_, 1.f / ray.dir_.y_, 1.f / ray.dir_.z_);
	std::array<Stack, kd_max_stack_> stack;
	const Node *far_child, *curr_node;
	curr_node = nodes.data();
	int entry_id = 0;
	stack[entry_id].t_ = cross.enter_;

	//distinguish between internal and external origin
	if(cross.enter_ >= 0.f) // ray with external origin
		stack[entry_id].point_ = ray.from_ + ray.dir_ * cross.enter_;
	else // ray with internal origin
		stack[entry_id].point_ = ray.from_;

	// setup initial entry and exit poimt in stack
	int exit_id = 1; // pointer to stack
	stack[exit_id].t_ = cross.leave_;
	stack[exit_id].point_ = ray.from_ + ray.dir_ * cross.leave_;
	stack[exit_id].node_ = nullptr; // "nowhere", termination flag

	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while(curr_node)
	{
		if(t_max < stack[entry_id].t_ /*a*/) break;
		// loop until leaf is found
		while(!curr_node->isLeaf())
		{
			const int axis = curr_node->splitAxis();
			const float split_val = curr_node->splitPos();
			if(stack[entry_id].point_[axis] <= split_val)
			{
				if(stack[exit_id].point_[axis] <= split_val)
				{
					curr_node++;
					continue;
				}
				if(stack[exit_id].point_[axis] == split_val)
				{
					curr_node = &nodes[curr_node->getRightChild()];
					continue;
				}
				// case N4
				far_child = &nodes[curr_node->getRightChild()];
				curr_node ++;
			}
			else
			{
				if(split_val < stack[exit_id].point_[axis])
				{
					curr_node = &nodes[curr_node->getRightChild()];
					continue;
				}
				far_child = curr_node + 1;
				curr_node = &nodes[curr_node->getRightChild()];
			}
			// traverse both children

			const float t = (split_val - ray.from_[axis]) * inv_dir[axis]; //splitting plane signed distance

			// set up the new exit point
			const int tmp_id = exit_id;
			exit_id++;

			// possibly skip current entry point so not to overwrite the data
			if(exit_id == entry_id) exit_id++;
			// push values onto the stack //todo: lookup table
			static constexpr std::array<std::array<int, 3>, 2> np_axis {{{1, 2, 0}, {2, 0, 1} }};
			const int next_axis = np_axis[0][axis];//(axis+1)%3;
			const int prev_axis = np_axis[1][axis];//(axis+2)%3;
			stack[exit_id].prev_stack_id_ = tmp_id;
			stack[exit_id].t_ = t;
			stack[exit_id].node_ = far_child;
			stack[exit_id].point_[axis] = split_val;
			stack[exit_id].point_[next_axis] = ray.from_[next_axis] + t * ray.dir_[next_axis];
			stack[exit_id].point_[prev_axis] = ray.from_[prev_axis] + t * ray.dir_[prev_axis];
		}

		// Check for intersections inside leaf node
		const auto &primitive_intersection = [](AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray, float t_max) -> bool
		{
			const IntersectData intersect_data = primitive->intersect(ray);
			if(intersect_data.hit_)
			{
				if(intersect_data.t_hit_ < t_max && intersect_data.t_hit_ >= 0.f)  // '>=' ?
				{
					const Visibility prim_visibility = primitive->getVisibility();
					if(prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::InvisibleShadowsOnly)
					{
						const Visibility mat_visibility = primitive->getMaterial()->getVisibility();
						if(mat_visibility == Visibility::NormalVisible || mat_visibility == Visibility::InvisibleShadowsOnly)
						{
							accelerator_intersect_data.setIntersectData(intersect_data);
							accelerator_intersect_data.hit_primitive_ = primitive;
							return true;
						}
					}
				}
			}
			return false;
		};
		for(const auto &prim : curr_node->primitives_)
			{
				if(primitive_intersection(accelerator_intersect_data, prim, ray, t_max)) return accelerator_intersect_data;
			}
		entry_id = exit_id;
		curr_node = stack[exit_id].node_;
		exit_id = stack[entry_id].prev_stack_id_;
	} // while
	return {};
}

/*=============================================================
	allow for transparent shadows.
=============================================================*/


AcceleratorTsIntersectData AcceleratorKdTreeMultiThread::intersectTs(const Ray &ray, int max_depth, float t_max, float shadow_bias, const Camera *camera) const
{
	return intersectTs(ray, max_depth, t_max, shadow_bias, nodes_, tree_bound_, camera);
}

AcceleratorTsIntersectData AcceleratorKdTreeMultiThread::intersectTs(const Ray &ray, int max_depth, float t_max, float, const std::vector<Node> &nodes, const Bound &tree_bound, const Camera *camera)
{
	AcceleratorTsIntersectData accelerator_intersect_data;
	const Bound::Cross cross = tree_bound.cross(ray, t_max);
	if(!cross.crossed_) { return {}; }

	//To avoid division by zero
	float inv_dir_x, inv_dir_y, inv_dir_z;

	if(ray.dir_.x_ == 0.f) inv_dir_x = std::numeric_limits<float>::max();
	else inv_dir_x = 1.f / ray.dir_.x_;

	if(ray.dir_.y_ == 0.f) inv_dir_y = std::numeric_limits<float>::max();
	else inv_dir_y = 1.f / ray.dir_.y_;

	if(ray.dir_.z_ == 0.f) inv_dir_z = std::numeric_limits<float>::max();
	else inv_dir_z = 1.f / ray.dir_.z_;

	Vec3 inv_dir(inv_dir_x, inv_dir_y, inv_dir_z);
	int depth = 0;

	std::set<const Primitive *> filtered;
	std::array<Stack, kd_max_stack_> stack;
	const Node *far_child, *curr_node;
	curr_node = nodes.data();

	int entry_id = 0;
	stack[entry_id].t_ = cross.enter_;

	//distinguish between internal and external origin
	if(cross.enter_ >= 0.f) // ray with external origin
		stack[entry_id].point_ = ray.from_ + ray.dir_ * cross.enter_;
	else // ray with internal origin
		stack[entry_id].point_ = ray.from_;

	// setup initial entry and exit poimt in stack
	int exit_id = 1; // pointer to stack
	stack[exit_id].t_ = cross.leave_;
	stack[exit_id].point_ = ray.from_ + ray.dir_ * cross.leave_;
	stack[exit_id].node_ = nullptr; // "nowhere", termination flag

	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while(curr_node)
	{
		if(t_max < stack[entry_id].t_ /*a*/) break;
		// loop until leaf is found
		while(!curr_node->isLeaf())
		{
			const int axis = curr_node->splitAxis();
			const float split_val = curr_node->splitPos();
			if(stack[entry_id].point_[axis] <= split_val)
			{
				if(stack[exit_id].point_[axis] <= split_val)
				{
					curr_node++;
					continue;
				}
				if(stack[exit_id].point_[axis] == split_val)
				{
					curr_node = &nodes[curr_node->getRightChild()];
					continue;
				}
				// case N4
				far_child = &nodes[curr_node->getRightChild()];
				curr_node ++;
			}
			else
			{
				if(split_val < stack[exit_id].point_[axis])
				{
					curr_node = &nodes[curr_node->getRightChild()];
					continue;
				}
				far_child = curr_node + 1;
				curr_node = &nodes[curr_node->getRightChild()];
			}
			// traverse both children
			const float t = (split_val - ray.from_[axis]) * inv_dir[axis]; //splitting plane signed distance

			// set up the new exit point
			const int tmp_id = exit_id;
			exit_id++;

			// possibly skip current entry point so not to overwrite the data
			if(exit_id == entry_id) exit_id++;
			// push values onto the stack //todo: lookup table
			static constexpr std::array<std::array<int, 3>, 2> np_axis {{{1, 2, 0}, {2, 0, 1}}};
			const int next_axis = np_axis[0][axis];//(axis+1)%3;
			const int prev_axis = np_axis[1][axis];//(axis+2)%3;
			stack[exit_id].prev_stack_id_ = tmp_id;
			stack[exit_id].t_ = t;
			stack[exit_id].node_ = far_child;
			stack[exit_id].point_[axis] = split_val;
			stack[exit_id].point_[next_axis] = ray.from_[next_axis] + t * ray.dir_[next_axis];
			stack[exit_id].point_[prev_axis] = ray.from_[prev_axis] + t * ray.dir_[prev_axis];
		}

		// Check for intersections inside leaf node
		const auto &primitive_intersection = [](AcceleratorTsIntersectData &accelerator_intersect_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Ray &ray, float t_max, const Camera *camera) -> bool
		{
			const IntersectData intersect_data = primitive->intersect(ray);
			if(intersect_data.hit_)
			{
				if(intersect_data.t_hit_ < t_max && intersect_data.t_hit_ >= ray.tmin_)  // '>=' ?
				{
					const Material *mat = primitive->getMaterial();
					if(mat->getVisibility() == Visibility::NormalVisible || mat->getVisibility() == Visibility::InvisibleShadowsOnly)
					{
						accelerator_intersect_data.setIntersectData(intersect_data);
						accelerator_intersect_data.hit_primitive_ = primitive;
						if(!mat->isTransparent()) return true;
						if(filtered.insert(primitive).second)
						{
							if(depth >= max_depth) return true;
							const Point3 hit_point = ray.from_ + accelerator_intersect_data.t_hit_ * ray.dir_;
							const SurfacePoint sp = primitive->getSurface(ray.differentials_.get(), hit_point, accelerator_intersect_data, nullptr, camera);
							accelerator_intersect_data.transparent_color_ *= mat->getTransparency(sp.mat_data_.get(), sp, ray.dir_, camera);
							++depth;
						}
					}
				}
			}
			return false;
		};


		for(const auto &prim : curr_node->primitives_)
		{
				if(primitive_intersection(accelerator_intersect_data, filtered, depth, max_depth, prim, ray, t_max, camera)) return accelerator_intersect_data;
		}
		entry_id = exit_id;
		curr_node = stack[exit_id].node_;
		exit_id = stack[entry_id].prev_stack_id_;
	} // while
	accelerator_intersect_data.hit_ = false;
	return accelerator_intersect_data;
}

END_YAFARAY
