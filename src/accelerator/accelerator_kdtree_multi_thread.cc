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
#include "common/logger.h"
#include "param/param.h"
#include "geometry/clip_plane.h"
#include "geometry/primitive/primitive.h"
#include "render/render_control.h"
#include <thread>

namespace yafaray {

std::map<std::string, const ParamMeta *> AcceleratorKdTreeMultiThread::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(max_depth_);
	PARAM_META(max_leaf_size_);
	PARAM_META(cost_ratio_);
	PARAM_META(empty_bonus_);
	PARAM_META(num_threads_);
	PARAM_META(min_indices_to_spawn_threads_);
	return param_meta_map;
}

AcceleratorKdTreeMultiThread::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(max_depth_);
	PARAM_LOAD(max_leaf_size_);
	PARAM_LOAD(cost_ratio_);
	PARAM_LOAD(empty_bonus_);
	PARAM_LOAD(num_threads_);
	PARAM_LOAD(min_indices_to_spawn_threads_);
}

ParamMap AcceleratorKdTreeMultiThread::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(max_depth_);
	PARAM_SAVE(max_leaf_size_);
	PARAM_SAVE(cost_ratio_);
	PARAM_SAVE(empty_bonus_);
	PARAM_SAVE(num_threads_);
	PARAM_SAVE(min_indices_to_spawn_threads_);
	return param_map;
}

std::pair<std::unique_ptr<Accelerator>, ParamResult> AcceleratorKdTreeMultiThread::factory(Logger &logger, const RenderControl *render_control, const std::vector<const Primitive *> &primitives, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto accelerator {std::make_unique<AcceleratorKdTreeMultiThread>(logger, param_result, render_control, primitives, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>("", {"type"}));
	return {std::move(accelerator), param_result};
}

AcceleratorKdTreeMultiThread::AcceleratorKdTreeMultiThread(Logger &logger, ParamResult &param_result, const RenderControl *render_control, const std::vector<const Primitive *> &primitives, const ParamMap &param_map) : ParentClassType_t{logger, param_result, render_control, param_map}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	const auto num_primitives = static_cast<uint32_t>(primitives.size());
	logger_.logInfo(getClassName(), ": Starting build (", num_primitives, " prims, cost_ratio:", params_.cost_ratio_, " empty_bonus:", params_.empty_bonus_, ") [using ", params_.num_threads_, " threads, min indices to spawn threads: ", params_.min_indices_to_spawn_threads_, "]");
	clock_t clock_start = clock();
	Params tree_build_parameters{params_};
	if(tree_build_parameters.max_depth_ <= 0) tree_build_parameters.max_depth_ = static_cast<int>(7.0f + 1.66f * math::log(static_cast<float>(num_primitives)));
	const double log_leaves = 1.442695 * math::log(static_cast<double>(num_primitives)); // = base2 log
	if(tree_build_parameters.max_leaf_size_ <= 0)
	{
		int mls = static_cast<int>(log_leaves - 16.0);
		if(mls <= 0) mls = 1;
		tree_build_parameters.max_leaf_size_ = mls;
	}
	if(tree_build_parameters.max_depth_ > kdtree::kd_max_stack_global) tree_build_parameters.max_depth_ = kdtree::kd_max_stack_global; //to prevent our stack to overflow
	//experiment: add penalty to cost ratio to reduce memory usage on huge scenes
	if(log_leaves > 16.0) tree_build_parameters.cost_ratio_ += 0.25 * (log_leaves - 16.0);
	std::vector<Bound<float>> bounds;
	bounds.reserve(num_primitives);
	if(num_primitives > 0) tree_bound_ = primitives.front()->getBound();
	else tree_bound_ = {{{0.f, 0.f, 0.f}}, {{0.f, 0.f, 0.f}}};
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Getting primitive bounds...");
	for(const auto &primitive : primitives)
	{
		bounds.emplace_back(primitive->getBound());
		/* calc tree bound. Remember to upgrade bound_t class... */
		tree_bound_ = Bound<float>{tree_bound_, bounds.back()};
	}
	//slightly(!) increase tree bound to prevent errors with prims
	//lying in a bound plane (still slight bug with trees where one dim. is 0)
	for(const auto axis : axis::spatial)
	{
		const double offset = (tree_bound_.g_[axis] - tree_bound_.a_[axis]) * 0.001;
		tree_bound_.a_[axis] -= static_cast<float>(offset), tree_bound_.g_[axis] += static_cast<float>(offset);
	}
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Done.");
	std::vector<uint32_t> prim_indices(num_primitives);
	for(uint32_t prim_num = 0; prim_num < num_primitives; prim_num++) prim_indices[prim_num] = prim_num;
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Starting recursive build...");
	Result kd_tree_result = buildTree(primitives, tree_bound_, prim_indices, 0, 0, 0, bounds, tree_build_parameters, ClipPlane(ClipPlane::Pos::None), {}, {}, num_current_threads_);
	nodes_ = std::move(kd_tree_result.nodes_);
	//print some stats:
	const clock_t clock_elapsed = clock() - clock_start;
	if(logger_.isVerbose())
	{
		logger_.logVerbose(getClassName(), ": CPU total clocks (in seconds): ", static_cast<float>(clock_elapsed) / static_cast<float>(CLOCKS_PER_SEC), "s (actual CPU work, including the work done by all threads added together)");
		logger_.logVerbose(getClassName(), ": used/allocated nodes: ", nodes_.size(), "/", nodes_.capacity()
				 , " (", 100.f * static_cast<float>(nodes_.size()) / nodes_.capacity(), "%)");
	}
	kd_tree_result.stats_.outputLog(logger, num_primitives, tree_build_parameters.max_leaf_size_);
}

AcceleratorKdTreeMultiThread::~AcceleratorKdTreeMultiThread()
{
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Done");
}

// ============================================================
/*!
	Faster cost function: Find the optimal split with SAH
	and binning => O(n)
*/

AcceleratorKdTreeMultiThread::SplitCost AcceleratorKdTreeMultiThread::pigeonMinCost(Logger &logger, float e_bonus, float cost_ratio, const std::vector<Bound<float>> &bounds, const Bound<float> &node_bound, const std::vector<uint32_t> &prim_indices)
{
	const auto num_prim_indices = static_cast<uint32_t>(prim_indices.size());
	static constexpr int max_bin = 1024;
	static constexpr int num_bins = max_bin + 1;
	std::array<kdtree::TreeBin, num_bins> bins;
	const Vec3f node_bound_axes {{node_bound.length(Axis::X), node_bound.length(Axis::Y), node_bound.length(Axis::Z)}};
	const Vec3f inv_node_bound_axes {{1.f / node_bound_axes[Axis::X], 1.f / node_bound_axes[Axis::Y], 1.f / node_bound_axes[Axis::Z]}};
	SplitCost split;
	split.cost_ = std::numeric_limits<float>::max();
	const float inv_total_sa = 1.f / (node_bound_axes[Axis::X] * node_bound_axes[Axis::Y] + node_bound_axes[Axis::X] * node_bound_axes[Axis::Z] + node_bound_axes[Axis::Y] * node_bound_axes[Axis::Z]);

	for(const auto axis : axis::spatial)
	{
		const float s = max_bin * inv_node_bound_axes[axis];
		const float min = node_bound.a_[axis];
		// pigeonhole sort:
		for(uint32_t prim_num = 0; prim_num < num_prim_indices; ++prim_num)
		{
			const Bound<float> &bbox = bounds[prim_indices[prim_num]];
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
		const Axis next_axis = axis::getNextSpatial(axis);
		const Axis prev_axis = axis::getPrevSpatial(axis);
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

AcceleratorKdTreeMultiThread::SplitCost AcceleratorKdTreeMultiThread::minimalCost(Logger &logger, float e_bonus, float cost_ratio, const Bound<float> &node_bound, const std::vector<uint32_t> &indices, const std::vector<Bound<float>> &bounds)
{
	const auto num_indices = static_cast<uint32_t>(indices.size());
	const Vec3f node_bound_axes {{node_bound.length(Axis::X), node_bound.length(Axis::Y), node_bound.length(Axis::Z)}};
	const Vec3f inv_node_bound_axes {{1.f / node_bound_axes[Axis::X], 1.f / node_bound_axes[Axis::Y], 1.f / node_bound_axes[Axis::Z]}};
	SplitCost split;
	split.cost_ = std::numeric_limits<float>::max();
	const float inv_total_sa = 1.f / (node_bound_axes[Axis::X] * node_bound_axes[Axis::Y] + node_bound_axes[Axis::X] * node_bound_axes[Axis::Z] + node_bound_axes[Axis::Y] * node_bound_axes[Axis::Z]);
	std::array<std::vector<kdtree::BoundEdge>, 3> edges_all_axes;
	for(auto axis : axis::spatial)
	{
		// << get edges for axis >>
		const Axis next_axis = axis::getNextSpatial(axis);
		const Axis prev_axis = axis::getPrevSpatial(axis);
		const auto axis_id = axis::getId(axis);
		edges_all_axes[axis_id].reserve(num_indices * 2);
		for(const auto &index : indices)
		{
			const Bound<float> &bbox = bounds[index];
			if(bbox.a_[axis] == bbox.g_[axis])
			{
				edges_all_axes[axis_id].emplace_back(kdtree::BoundEdge(bbox.a_[axis], index, kdtree::BoundEdge::EndBound::Both));
			}
			else
			{
				edges_all_axes[axis_id].emplace_back(kdtree::BoundEdge(bbox.a_[axis], index, kdtree::BoundEdge::EndBound::Left));
				edges_all_axes[axis_id].emplace_back(kdtree::BoundEdge(bbox.g_[axis], index, kdtree::BoundEdge::EndBound::Right));
			}
		}
		std::sort(edges_all_axes[axis_id].begin(), edges_all_axes[axis_id].end());
		// Compute cost of all splits for _axis_ to find best
		const float cap_area = node_bound_axes[next_axis] * node_bound_axes[prev_axis];
		const float cap_perim = node_bound_axes[next_axis] + node_bound_axes[prev_axis];
		uint32_t num_left = 0;
		uint32_t num_right = num_indices;
		//todo: early-out criteria: if l1 > l2*nPrims (l2 > l1*nPrims) => minimum is lowest (highest) edge!
		if(num_indices > 5)
		{
			float edget = edges_all_axes[axis_id].front().pos_;
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
			edget = edges_all_axes[axis_id].back().pos_;
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
					split.edge_offset_ = edges_all_axes[axis_id].size() - 1;
					++split.stats_early_out_;
				}
				continue;
			}
		}
		const int num_edges = static_cast<int>(edges_all_axes[axis_id].size());
		for(int edge_id = 0; edge_id < num_edges; ++edge_id)
		{
			if(edges_all_axes[axis_id][edge_id].end_ == kdtree::BoundEdge::EndBound::Right) --num_right;
			const float edget = edges_all_axes[axis_id][edge_id].pos_;
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
			if(edges_all_axes[axis_id][edge_id].end_ != kdtree::BoundEdge::EndBound::Right)
			{
				++num_left;
				if(edges_all_axes[axis_id][edge_id].end_ == kdtree::BoundEdge::EndBound::Both) --num_right;
			}
		}
		if((num_left != num_indices || num_right != 0) && logger.isVerbose()) logger.logVerbose("you screwed your new idea!");
	}
	if(split.axis_ != Axis::None) split.edges_ = std::move(edges_all_axes[axis::getId(split.axis_)]);
	return split;
}

// ============================================================
/*!
	recursively build the Kd-tree
*/
AcceleratorKdTreeMultiThread::Result AcceleratorKdTreeMultiThread::buildTree(const std::vector<const Primitive *> &primitives, const Bound<float> &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound<float>> &bounds, const Params &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, std::atomic<int> &num_current_threads) const
{
	Result kd_tree_result;
	buildTreeWorker(primitives, node_bound, indices, depth, next_node_id, bad_refines, bounds, parameters, clip_plane, polygons, primitive_indices, kd_tree_result, num_current_threads);
	return kd_tree_result;
}

void AcceleratorKdTreeMultiThread::buildTreeWorker(const std::vector<const Primitive *> &primitives, const Bound<float> &node_bound, const std::vector<uint32_t> &indices, int depth, uint32_t next_node_id, int bad_refines, const std::vector<Bound<float>> &bounds, const Params &parameters, const ClipPlane &clip_plane, const std::vector<PolyDouble> &polygons, const std::vector<uint32_t> &primitive_indices, Result &result, std::atomic<int> &num_current_threads) const
{
	if(render_control_ && render_control_->canceled()) return;
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
	std::vector<Bound<float>> poly_bounds;
	const auto num_indices = static_cast<uint32_t>(indices.size());
	const bool do_poly_clipping = (num_indices <= poly_clipping_threshold && num_indices <= primitive_indices.size());
	if(do_poly_clipping)
	{
		poly_indices.reserve(poly_clipping_threshold);
		prim_indices.reserve(poly_clipping_threshold);
		poly_bounds.reserve(poly_clipping_threshold);
		Vec3d b_half_size;
		std::array<Vec3d, 2> b_ext;
		for(const auto axis : axis::spatial)
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
				const PolyDouble::ClipResultWithBound clip_result = primitive->clipToBound(logger_, b_ext, clip_plane, polygons[poly_id]);
				//std::string polygon_str = "(polygons empty!)";
				//if(!polygons.empty()) polygon_str = polygons[poly_id].print();
				//if(logger_.isDebug()) Y_DEBUG PRPREC(12) << " depth=" << depth << " i=" << index_num << " poly_id=" << poly_id << " i_poly=" << polygon_str << " result=" << clip_result.clip_result_code_ << " o_poly=" << clip_result.poly_.print(");

				if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Code::Correct)
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
		new_bounds = std::reference_wrapper<const std::vector<Bound<float>>>(poly_bounds); //Using polygon indices for bounds too, instead of primitive indices now.
	}
#endif

	const auto num_new_indices = static_cast<uint32_t>(new_indices.get().size());
	//	<< check if leaf criteria met >>
	if(num_new_indices <= static_cast<uint32_t>(parameters.max_leaf_size_) || depth >= parameters.max_depth_)
	{
		Node node;
		const kdtree::Stats leaf_stats = node.createLeaf(new_primitive_indices, primitives);
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
		const kdtree::Stats leaf_stats = node.createLeaf(new_primitive_indices, primitives);
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
			if(split.edges_[edge_id].end_ != kdtree::BoundEdge::EndBound::Right)
			{
				left_indices.emplace_back(split.edges_[edge_id].index_);
				left_primitive_indices.emplace_back(new_primitive_indices.get()[left_indices.back()]);
			}
		}
		if(split.edges_[split.edge_offset_].end_ == kdtree::BoundEdge::EndBound::Both)
		{
			right_indices.emplace_back(split.edges_[split.edge_offset_].index_);
			right_primitive_indices.emplace_back(new_primitive_indices.get()[right_indices.back()]);
		}
		const int num_edges = static_cast<int>(split.edges_.size());
		for(int edge_id = split.edge_offset_ + 1; edge_id < num_edges; ++edge_id)
		{
			if(split.edges_[edge_id].end_ != kdtree::BoundEdge::EndBound::Left)
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
			if(split.edges_[edge_id].end_ != kdtree::BoundEdge::EndBound::Right)
			{
				left_indices.emplace_back(split.edges_[edge_id].index_);
			}
		}
		if(split.edges_[split.edge_offset_].end_ == kdtree::BoundEdge::EndBound::Both)
		{
			right_indices.emplace_back(split.edges_[split.edge_offset_].index_);
		}
		const int num_edges = static_cast<int>(split.edges_.size());
		for(int edge_id = split.edge_offset_ + 1; edge_id < num_edges; ++edge_id)
		{
			if(split.edges_[edge_id].end_ != kdtree::BoundEdge::EndBound::Left)
			{
				right_indices.emplace_back(split.edges_[edge_id].index_);
			}
		}
		split_pos = split.edges_[split.edge_offset_].pos_;
		left_primitive_indices = left_indices;
		right_primitive_indices = right_indices;
	}

	Node node;
	const kdtree::Stats interior_stats = node.createInterior(Axis(split.axis_), split_pos);
	result.stats_ += interior_stats;
	result.nodes_.emplace_back(node);
	Bound bound_left = node_bound;
	Bound bound_right = node_bound;
	bound_left.setAxisMax(split.axis_, split_pos);
	bound_right.setAxisMin(split.axis_, split_pos);

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
		const auto next_free_node_original = static_cast<uint32_t>(next_node_id + result.nodes_.size());
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

		const auto num_nodes_left = static_cast<uint32_t>(result_left.nodes_.size());
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

} //namespace yafaray
