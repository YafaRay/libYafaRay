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

#include "accelerator/accelerator_kdtree_original.h"
#include "common/logger.h"
#include "geometry/clip_plane.h"
#include "param/param.h"
#include "image/image_output.h"
#include <cstring>

namespace yafaray {

AcceleratorKdTree::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(max_depth_);
	PARAM_LOAD(max_leaf_size_);
	PARAM_LOAD(cost_ratio_);
	PARAM_LOAD(empty_bonus_);
}

ParamMap AcceleratorKdTree::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(max_depth_);
	PARAM_SAVE(max_leaf_size_);
	PARAM_SAVE(cost_ratio_);
	PARAM_SAVE(empty_bonus_);
	PARAM_SAVE_END;
}

ParamMap AcceleratorKdTree::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Accelerator>, ParamError> AcceleratorKdTree::factory(Logger &logger, const std::vector<const Primitive *> &primitives, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto accelerator {std::make_unique<ThisClassType_t>(logger, param_error, primitives, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<ThisClassType_t>("", {"type"}));
	return {std::move(accelerator), param_error};
}

AcceleratorKdTree::AcceleratorKdTree(Logger &logger, ParamError &param_error, const std::vector<const Primitive *> &primitives, const ParamMap &param_map) : ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	total_prims_ = static_cast<uint32_t>(primitives.size());
	logger_.logInfo(getClassName(), ": Starting build (", total_prims_, " prims, cr:", cost_ratio_, " eb:", e_bonus_, ")");
	clock_t c_start, c_end;
	c_start = clock();
	next_free_node_ = 0;
	allocated_nodes_count_ = 256;
	nodes_.resize(allocated_nodes_count_);
	if(max_depth_ <= 0 && total_prims_ > 0) max_depth_ = static_cast<int>(7.0f + 1.66f * math::log(static_cast<float>(total_prims_)));
	const double log_leaves = 1.442695 * math::log(static_cast<double >(total_prims_)); // = base2 log
	if(max_leaf_size_ <= 0)
	{
		int mls = static_cast<int>(log_leaves - 16.0);
		if(mls <= 0) mls = 1;
		max_leaf_size_ = static_cast<unsigned int>(mls);
	}
	else max_leaf_size_ = static_cast<unsigned int>(max_leaf_size_);
	if(max_depth_ > kd_max_stack_) max_depth_ = kd_max_stack_; //to prevent our stack to overflow
	//experiment: add penalty to cost ratio to reduce memory usage on huge scenes
	if(log_leaves > 16.0) cost_ratio_ += 0.25f * (log_leaves - 16.0);
	all_bounds_ = std::unique_ptr<Bound<float>[]>(new Bound<float>[total_prims_ + prim_clip_thresh_ + 1]);
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Getting primitive bounds...");
	for(uint32_t i = 0; i < total_prims_; i++)
	{
		all_bounds_[i] = primitives[i]->getBound();
		/* calc tree bound. Remember to upgrade bound_t class... */
		if(i > 0) tree_bound_ = Bound<float>{tree_bound_, all_bounds_[i]};
		else tree_bound_ = all_bounds_[i];
	}
	if(total_prims_ == 0) tree_bound_ = Bound<float>{{{0.f, 0.f, 0.f}}, {{0.f, 0.f, 0.f}}};
	//slightly(!) increase tree bound to prevent errors with prims
	//lying in a bound plane (still slight bug with trees where one dim. is 0)
	for(const auto axis : axis::spatial)
	{
		const double offset = (tree_bound_.g_[axis] - tree_bound_.a_[axis]) * 0.001;
		tree_bound_.a_[axis] -= static_cast<float>(offset), tree_bound_.g_[axis] += static_cast<float>(offset);
	}
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Done.");
	// get working memory for tree construction
	std::array<std::unique_ptr<kdtree::BoundEdge[]>, 3> edges;
	const uint32_t r_mem_size = 3 * total_prims_;
	auto left_prims = std::unique_ptr<uint32_t[]>(new uint32_t[std::max(static_cast<uint32_t>(2 * prim_clip_thresh_), total_prims_)]);
	auto right_prims = std::unique_ptr<uint32_t[]>(new uint32_t[r_mem_size]); //just a rough guess, allocating worst case is insane!
	for(int i = 0; i < 3; ++i) edges[i] = std::unique_ptr<kdtree::BoundEdge[]>(new kdtree::BoundEdge[514]);
#if PRIMITIVE_CLIPPING > 0
	clip_ = std::unique_ptr<ClipPlane[]>(new ClipPlane[max_depth_ + 2]);
	cdata_.resize((max_depth_ + 2) * prim_clip_thresh_);
#endif
	// prepare data
	for(uint32_t i = 0; i < total_prims_; i++) left_prims[i] = i;
#if PRIMITIVE_CLIPPING > 0
	for(int i = 0; i < max_depth_ + 2; i++) clip_[i].pos_ = ClipPlane::Pos::None;
#endif
	/* build tree */
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Starting recursive build...");
	buildTree(total_prims_, primitives, tree_bound_, left_prims.get(),
			  left_prims.get(), right_prims.get(), edges, // <= working memory
	          r_mem_size, 0, 0);

	//print some stats:
	c_end = clock() - c_start;
	if(logger_.isVerbose())
	{
		logger_.logVerbose(getClassName(), ": Stats (", static_cast<float>(c_end) / static_cast<float>(CLOCKS_PER_SEC), "s)");
		logger_.logVerbose(getClassName(), ": used/allocated nodes: ", next_free_node_, "/", allocated_nodes_count_
				 , " (", 100.f * static_cast<float>(next_free_node_) / allocated_nodes_count_, "%)");
		logger_.logVerbose(getClassName(), ": Primitives in tree: ", total_prims_);
		logger_.logVerbose(getClassName(), ": Interior nodes: ", kd_stats_.kd_inodes_, " / ", "leaf nodes: ", kd_stats_.kd_leaves_
				 , " (empty: ", kd_stats_.empty_kd_leaves_, " = ", 100.f * static_cast<float>(kd_stats_.empty_kd_leaves_) / kd_stats_.kd_leaves_, "%)");
		logger_.logVerbose(getClassName(), ": Leaf prims: ", kd_stats_.kd_prims_, " (", static_cast<float>(kd_stats_.kd_prims_) / total_prims_, " x prims in tree, leaf size: ", max_leaf_size_, ")");
		logger_.logVerbose(getClassName(), ": => ", static_cast<float>(kd_stats_.kd_prims_) / (kd_stats_.kd_leaves_ - kd_stats_.empty_kd_leaves_), " prims per non-empty leaf");
		logger_.logVerbose(getClassName(), ": Leaves due to depth limit/bad splits: ", kd_stats_.depth_limit_reached_, "/", kd_stats_.num_bad_splits_);
		logger_.logVerbose(getClassName(), ": clipped primitives: ", kd_stats_.clip_, " (", kd_stats_.bad_clip_, " bad clips, ", kd_stats_.null_clip_, " null clips)");
	}
}

AcceleratorKdTree::~AcceleratorKdTree()
{
	if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": Done");
}

// ============================================================
/*!
	Faster cost function: Find the optimal split with SAH
	and binning => O(n)
*/

AcceleratorKdTree::SplitCost AcceleratorKdTree::pigeonMinCost(Logger &logger, float e_bonus, float cost_ratio, uint32_t num_prim_indices, const Bound<float> *bounds, const Bound<float> &node_bound, const uint32_t *prim_indices)
{
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

		static constexpr std::array<std::array<Axis, 3>, 3> axis_lut {{{Axis::X, Axis::Y, Axis::Z}, {Axis::Y, Axis::Z, Axis::X}, {Axis::Z, Axis::X, Axis::Y}}};
		const float cap_area = node_bound_axes[ axis_lut[1][axis::getId(axis)] ] * node_bound_axes[ axis_lut[2][axis::getId(axis)] ];
		const float cap_perim = node_bound_axes[ axis_lut[1][axis::getId(axis)] ] + node_bound_axes[ axis_lut[2][axis::getId(axis)] ];

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

AcceleratorKdTree::SplitCost AcceleratorKdTree::minimalCost(Logger &logger, float e_bonus, float cost_ratio, uint32_t num_indices, const Bound<float> &node_bound, const uint32_t *prim_idx, const Bound<float> *all_bounds, const Bound<float> *all_bounds_general, const std::array<std::unique_ptr<kdtree::BoundEdge[]>, 3> &edges_all_axes, kdtree::Stats &kd_stats)
{
	const std::array<float, 3> node_bound_axes {{node_bound.length(Axis::X), node_bound.length(Axis::Y), node_bound.length(Axis::Z)}};
	const std::array<float, 3> inv_node_bound_axes { 1.f / node_bound_axes[0], 1.f / node_bound_axes[1], 1.f / node_bound_axes[2] };
	SplitCost split;
	split.cost_ = std::numeric_limits<float>::max();
	const float inv_total_sa = 1.f / (node_bound_axes[0] * node_bound_axes[1] + node_bound_axes[0] * node_bound_axes[2] + node_bound_axes[1] * node_bound_axes[2]);
	for(const auto axis : axis::spatial)
	{
		const unsigned char axis_id = axis::getId(axis);
		// << get edges for axis >>
		int num_edges = 0;
		//test!
		if(all_bounds != all_bounds_general) for(unsigned int i = 0; i < num_indices; i++)
		{
			const Bound<float> &bbox = all_bounds[i];
			if(bbox.a_[axis] == bbox.g_[axis])
			{
				edges_all_axes[axis_id][num_edges] = kdtree::BoundEdge(bbox.a_[axis], i /* pn */, kdtree::BoundEdge::EndBound::Both);
				++num_edges;
			}
			else
			{
				edges_all_axes[axis_id][num_edges] = kdtree::BoundEdge(bbox.a_[axis], i /* pn */, kdtree::BoundEdge::EndBound::Left);
				edges_all_axes[axis_id][num_edges + 1] = kdtree::BoundEdge(bbox.g_[axis], i /* pn */, kdtree::BoundEdge::EndBound::Right);
				num_edges += 2;
			}
		}
		else for(unsigned int i = 0; i < num_indices; i++)
		{
			const int pn = prim_idx[i];
			const Bound<float> &bbox = all_bounds[pn];
			if(bbox.a_[axis] == bbox.g_[axis])
			{
				edges_all_axes[axis_id][num_edges] = kdtree::BoundEdge(bbox.a_[axis], pn, kdtree::BoundEdge::EndBound::Both);
				++num_edges;
			}
			else
			{
				edges_all_axes[axis_id][num_edges] = kdtree::BoundEdge(bbox.a_[axis], pn, kdtree::BoundEdge::EndBound::Left);
				edges_all_axes[axis_id][num_edges + 1] = kdtree::BoundEdge(bbox.g_[axis], pn, kdtree::BoundEdge::EndBound::Right);
				num_edges += 2;
			}
		}
		std::sort(&edges_all_axes[axis_id][0], &edges_all_axes[axis_id][num_edges]);
		// Compute cost of all splits for _axis_ to find best
		const std::array<std::array<int, 3>, 3> axis_lut {{ {0, 1, 2}, {1, 2, 0}, {2, 0, 1} }};
		const float cap_area = node_bound_axes[ axis_lut[1][axis_id] ] * node_bound_axes[ axis_lut[2][axis_id] ];
		const float cap_perim = node_bound_axes[ axis_lut[1][axis_id] ] + node_bound_axes[ axis_lut[2][axis_id] ];
		uint32_t num_left = 0;
		uint32_t num_right = num_indices;
		//todo: early-out criteria: if l1 > l2*nPrims (l2 > l1*nPrims) => minimum is lowest (highest) edge!
		if(num_indices > 5)
		{
			float edget = edges_all_axes[axis_id][0].pos_;
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
					split.num_edges_ = num_edges;
					++kd_stats.early_out_;
				}
				continue;
			}
			edget = edges_all_axes[axis_id][num_edges - 1].pos_;
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
					split.edge_offset_ = num_edges - 1;
					split.num_edges_ = num_edges;
					++kd_stats.early_out_;
				}
				continue;
			}
		}

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
				if(num_right == 0) eb = (0.1f + l_above * inv_node_bound_axes[axis_id]) * e_bonus * raw_costs;
				else if(num_left == 0) eb = (0.1f + l_below * inv_node_bound_axes[axis_id]) * e_bonus * raw_costs;
				else eb = 0.0f;

				const float cost = cost_ratio + inv_total_sa * (raw_costs - eb);
				// Update best split if this is lowest cost so far
				if(cost < split.cost_)
				{
					split.cost_ = cost;
					split.axis_ = axis;
					split.edge_offset_ = edge_id;
					split.num_edges_ = num_edges;
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
	return split;
}

// ============================================================
/*!
	recursively build the Kd-tree
	returns:	0 when leaf was created
				1 when either current or at least 1 subsequent split reduced cost
				2 when neither current nor subsequent split reduced cost
*/

int AcceleratorKdTree::buildTree(uint32_t n_prims, const std::vector<const Primitive *> &original_primitives, const Bound<float> &node_bound, uint32_t *prim_nums, uint32_t *left_prims, uint32_t *right_prims, const std::array<std::unique_ptr<kdtree::BoundEdge[]>, 3> &edges, uint32_t right_mem_size, int depth, int bad_refines)
{
	if(next_free_node_ == allocated_nodes_count_)
	{
		int new_count = 2 * allocated_nodes_count_;
		new_count = (new_count > 0x100000) ? allocated_nodes_count_ + 0x80000 : new_count;
//		auto n = std::unique_ptr<Node[]>(new Node[new_count]);
//		memcpy(n.get(), nodes_.get(), allocated_nodes_count_ * sizeof(Node));
//		nodes_ = std::move(n);
		allocated_nodes_count_ = new_count;
		nodes_.resize(allocated_nodes_count_);
	}

#if PRIMITIVE_CLIPPING > 0
	if(n_prims <= prim_clip_thresh_)
	{
		std::array<int, prim_clip_thresh_> o_prims;
		int n_overl = 0;
		std::array<double, 3> b_half_size;
		std::array<Vec3d, 2> b_ext;
		for(const auto axis : axis::spatial)
		{
			const auto axis_id = axis::getId(axis);
			b_half_size[axis_id] = (static_cast<double>(node_bound.g_[axis]) - static_cast<double>(node_bound.a_[axis]));
			double temp  = (static_cast<double>(tree_bound_.g_[axis]) - static_cast<double>(tree_bound_.a_[axis]));
			b_ext[0][axis] = node_bound.a_[axis] - 0.021 * b_half_size[axis_id] - 0.00001 * temp;
			b_ext[1][axis] = node_bound.g_[axis] + 0.021 * b_half_size[axis_id] + 0.00001 * temp;
		}
		for(unsigned int i = 0; i < n_prims; ++i)
		{
			const Primitive *ct = original_primitives[ prim_nums[i] ];
			uint32_t old_idx = 0;
			if(clip_[depth].pos_ != ClipPlane::Pos::None) old_idx = prim_nums[i + n_prims];
			if(ct->clippingSupport())
			{
				//if(ct->clipToBound(b_ext, clip_[depth], all_bounds_[total_prims_ + n_overl], cdata_[prim_clip_thresh_ * depth + old_idx], cdata_[prim_clip_thresh_ * (depth + 1) + n_overl], nullptr))
				const PolyDouble::ClipResultWithBound clip_result = ct->clipToBound(logger_, b_ext, clip_[depth], cdata_[prim_clip_thresh_ * depth + old_idx]);
				//if(logger_.isDebug()) Y_DEBUG PRPREC(12) << " depth=" << depth << " i=" << i << " poly_id=" << old_idx << " i_poly=" << cdata_[prim_clip_thresh_ * depth + old_idx].print() << " result=" << clip_result.clip_result_code_ << " o_poly=" << clip_result.poly_.print(");

				if(clip_result.clip_result_code_ == PolyDouble::ClipResultWithBound::Code::Correct)
				{
					++kd_stats_.clip_;
					cdata_[prim_clip_thresh_ * (depth + 1) + n_overl] = clip_result.poly_;
					all_bounds_[total_prims_ + n_overl] = *clip_result.box_;
					o_prims[n_overl] = prim_nums[i]; n_overl++;
				}
				else ++kd_stats_.null_clip_;
			}
			else
			{
				// no clipping supported by prim, copy old bound:
				all_bounds_[total_prims_ + n_overl] = all_bounds_[ prim_nums[i] ]; //really?? FIXME: what does this comment mean?
				o_prims[n_overl] = prim_nums[i]; n_overl++;
			}
		}
		//copy back
		memcpy(prim_nums, o_prims.data(), n_overl * sizeof(uint32_t));
		n_prims = n_overl;
	}
#endif
	//	<< check if leaf criteria met >>
	if(n_prims <= max_leaf_size_ || depth >= max_depth_)
	{
		nodes_[next_free_node_].createLeaf(prim_nums, n_prims, original_primitives, prims_arena_, kd_stats_);
		next_free_node_++;
		if(depth >= max_depth_) kd_stats_.depth_limit_reached_++;
		return 0;
	}

	//<< calculate cost for all axes and chose minimum >>
	const float base_bonus = e_bonus_;
	e_bonus_ *= 1.1 - static_cast<float>(depth) / static_cast<float>(max_depth_);
	SplitCost split;
	if(n_prims > pigeonhole_sort_thresh_) split = pigeonMinCost(logger_, e_bonus_, cost_ratio_, n_prims, all_bounds_.get(), node_bound, prim_nums);
#if PRIMITIVE_CLIPPING > 0
	else if(n_prims <= prim_clip_thresh_) split = minimalCost(logger_, e_bonus_, cost_ratio_, n_prims, node_bound, prim_nums, all_bounds_.get() + total_prims_, all_bounds_.get(), edges, kd_stats_);
#endif
	else split = minimalCost(logger_, e_bonus_, cost_ratio_, n_prims, node_bound, prim_nums, all_bounds_.get(), all_bounds_.get(), edges, kd_stats_);
	e_bonus_ = base_bonus; //restore eBonus
	//<< if (minimum > leafcost) increase bad refines >>
	if(split.cost_ > n_prims) ++bad_refines;
	if((split.cost_ > 1.6f * n_prims && n_prims < 16) ||
	   split.axis_ == Axis::None || bad_refines == 2)
	{
		nodes_[next_free_node_].createLeaf(prim_nums, n_prims, original_primitives, prims_arena_, kd_stats_);
		next_free_node_++;
		if(bad_refines == 2) ++kd_stats_.num_bad_splits_;
		return 0;
	}

	//todo: check working memory for child recursive calls
	uint32_t remaining_mem, *n_right_prims;
	uint32_t *old_right_prims = right_prims;
	std::unique_ptr<uint32_t[]> more_prims;
	if(n_prims > right_mem_size || 2 * prim_clip_thresh_ > right_mem_size) // *possibly* not enough, get some more
	{
		remaining_mem = n_prims * 3;
		more_prims = std::unique_ptr<uint32_t[]>(new uint32_t[remaining_mem]);
		n_right_prims = more_prims.get();
	}
	else
	{
		n_right_prims = old_right_prims;
		remaining_mem = right_mem_size;
	}

	// Classify primitives with respect to split
	float split_pos;
	int n_0 = 0, n_1 = 0;
	if(n_prims > pigeonhole_sort_thresh_) // we did pigeonhole
	{
		for(unsigned int i = 0; i < n_prims; i++)
		{
			const int pn = prim_nums[i];
			if(all_bounds_[ pn ].a_[split.axis_] >= split.t_) n_right_prims[n_1++] = pn;
			else
			{
				left_prims[n_0++] = pn;
				if(all_bounds_[ pn ].g_[split.axis_] > split.t_) n_right_prims[n_1++] = pn;
			}
		}
		split_pos = split.t_;
	}
#if PRIMITIVE_CLIPPING > 0
	else if(n_prims <= prim_clip_thresh_)
	{
		std::array<int, prim_clip_thresh_> cindizes;
		std::array<uint32_t, prim_clip_thresh_> old_prims;
		const auto split_axis_id = axis::getId(split.axis_);
		memcpy(old_prims.data(), prim_nums, n_prims * sizeof(int));
		for(int i = 0; i < split.edge_offset_; ++i)
		{
			if(edges[split_axis_id][i].end_ != kdtree::BoundEdge::EndBound::Right)
			{
				cindizes[n_0] = edges[split_axis_id][i].index_;
				left_prims[n_0] = old_prims[cindizes[n_0]];
				++n_0;
			}
		}
		for(int i = 0; i < n_0; ++i) { left_prims[n_0 + i] = cindizes[i]; /* std::cout << cindizes[i] << " "; */ }
		if(edges[split_axis_id][split.edge_offset_].end_ == kdtree::BoundEdge::EndBound::Both)
		{
			cindizes[n_1] = edges[split_axis_id][split.edge_offset_].index_;
			n_right_prims[n_1] = old_prims[cindizes[n_1]];
			++n_1;
		}

		for(int i = split.edge_offset_ + 1; i < split.num_edges_; ++i)
		{
			if(edges[split_axis_id][i].end_ != kdtree::BoundEdge::EndBound::Left)
			{
				cindizes[n_1] = edges[split_axis_id][i].index_;
				n_right_prims[n_1] = old_prims[cindizes[n_1]];
				++n_1;
			}
		}
		for(int i = 0; i < n_1; ++i) { n_right_prims[n_1 + i] = cindizes[i]; /* std::cout << cindizes[i] << " "; */ }
		split_pos = edges[split_axis_id][split.edge_offset_].pos_;
	}
#endif //PRIMITIVE_CLIPPING > 0
	else //we did "normal" cost function
	{
		const auto split_axis_id = axis::getId(split.axis_);
		for(int i = 0; i < split.edge_offset_; ++i)
			if(edges[split_axis_id][i].end_ != kdtree::BoundEdge::EndBound::Right)
				left_prims[n_0++] = edges[split_axis_id][i].index_;
		if(edges[split_axis_id][split.edge_offset_].end_ == kdtree::BoundEdge::EndBound::Both)
			n_right_prims[n_1++] = edges[split_axis_id][split.edge_offset_].index_;
		for(int i = split.edge_offset_ + 1; i < split.num_edges_; ++i)
			if(edges[split_axis_id][i].end_ != kdtree::BoundEdge::EndBound::Left)
				n_right_prims[n_1++] = edges[split_axis_id][i].index_;
		split_pos = edges[split_axis_id][split.edge_offset_].pos_;
	}
	//advance right prims pointer
	remaining_mem -= n_1;

	uint32_t cur_node = next_free_node_;
	nodes_[cur_node].createInterior(axis::getId(split.axis_), split_pos, kd_stats_);
	++next_free_node_;
	Bound bound_l = node_bound, bound_r = node_bound;
	bound_l.setAxisMax(split.axis_, split_pos);
	bound_r.setAxisMin(split.axis_, split_pos);

#if PRIMITIVE_CLIPPING > 0
	if(n_prims <= prim_clip_thresh_)
	{
		remaining_mem -= n_1;
		//<< recurse below child >>
		clip_[depth + 1].axis_ = split.axis_;
		clip_[depth + 1].pos_ = ClipPlane::Pos::Upper;
		buildTree(n_0, original_primitives, bound_l, left_prims, left_prims, n_right_prims + 2 * n_1, edges, remaining_mem, depth + 1, bad_refines);
		clip_[depth + 1].pos_ = ClipPlane::Pos::Lower;
		//<< recurse above child >>
		nodes_[cur_node].setRightChild(next_free_node_);
		buildTree(n_1, original_primitives, bound_r, n_right_prims, left_prims, n_right_prims + 2 * n_1, edges, remaining_mem, depth + 1, bad_refines);
		clip_[depth + 1].pos_ = ClipPlane::Pos::None;
	}
	else
#endif
	{
		//<< recurse below child >>
		buildTree(n_0, original_primitives, bound_l, left_prims, left_prims, n_right_prims + n_1, edges, remaining_mem, depth + 1, bad_refines);
		//<< recurse above child >>
		nodes_[cur_node].setRightChild(next_free_node_);
		buildTree(n_1, original_primitives, bound_r, n_right_prims, left_prims, n_right_prims + n_1, edges, remaining_mem, depth + 1, bad_refines);
	}
	return 1;
}

} //namespace yafaray