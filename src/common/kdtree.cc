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

// search for "todo" and "IMPLEMENT" and "<<" or ">>"...

#include "common/kdtree.h"
#include "material/material.h"
#include "common/scene.h"
#include "common/logging.h"
#include "common/triangle.h"
#include <limits>
#include <set>
#include <cstring>
#include <time.h>

BEGIN_YAFARAY

static constexpr int lower_b__ = 0;
static constexpr int upper_b__ = 2;
static constexpr int both_b__ = 1;

#define TRI_CLIP 1 //tempoarily disabled
static constexpr int tri_clip_thresh__ = 32;
static constexpr int clip_data_size__ = 3 * 12 * sizeof(double);
static constexpr int kd_bins__ = 1024;
static constexpr int kd_max_stack__ = 64;

//still in old file...
//int Kd_inodes=0, Kd_leaves=0, _emptyKd_leaves=0, Kd_prims=0, _clip=0, _bad_clip=0, _null_clip=0, _early_out=0;

//bound_t getTriBound(const triangle_t tri);
//int triBoxOverlap(double boxcenter[3],double boxhalfsize[3],double triverts[3][3]);
//int triBoxClip(const double b_min[3], const double b_max[3], const double triverts[3][3], bound_t &box);

template<class T>
KdTree<T>::KdTree(const T **v, int np, int depth, int leaf_size,
				  float cost_ratio, float empty_bonus)
	: cost_ratio_(cost_ratio), e_bonus_(empty_bonus), max_depth_(depth)
{
	Y_INFO << "Kd-Tree: Starting build (" << np << " prims, cr:" << cost_ratio_ << " eb:" << e_bonus_ << ")" << YENDL;
	clock_t c_start, c_end;
	c_start = clock();
	total_prims_ = np;
	next_free_node_ = 0;
	allocated_nodes_count_ = 256;
	nodes_ = (KdTreeNode<T> *) alignedAlloc__(64, 256 * sizeof(KdTreeNode<T>));
	if(max_depth_ <= 0) max_depth_ = int(7.0f + 1.66f * log(float(total_prims_)));
	double log_leaves = 1.442695f * log(double(total_prims_)); // = base2 log
	if(leaf_size <= 0)
	{
		//maxLeafSize = int( 1.442695f * log(float(totalPrims)/62500.0) );
		int mls = int(log_leaves - 16.0);
		if(mls <= 0) mls = 1;
		max_leaf_size_ = (unsigned int) mls;
	}
	else max_leaf_size_ = (unsigned int) leaf_size;
	if(max_depth_ > kd_max_stack__) max_depth_ = kd_max_stack__; //to prevent our stack to overflow
	//experiment: add penalty to cost ratio to reduce memory usage on huge scenes
	if(log_leaves > 16.0) cost_ratio_ += 0.25 * (log_leaves - 16.0);
	all_bounds_ = new Bound[total_prims_ + tri_clip_thresh__ + 1];
	Y_VERBOSE << "Kd-Tree: Getting triangle bounds..." << YENDL;
	for(uint32_t i = 0; i < total_prims_; i++)
	{
		all_bounds_[i] = v[i]->getBound();
		/* calc tree bound. Remember to upgrade bound_t class... */
		if(i) tree_bound_ = Bound(tree_bound_, all_bounds_[i]);
		else tree_bound_ = all_bounds_[i];
	}
	//slightly(!) increase tree bound to prevent errors with prims
	//lying in a bound plane (still slight bug with trees where one dim. is 0)
	for(int i = 0; i < 3; i++)
	{
		double foo = (tree_bound_.g_[i] - tree_bound_.a_[i]) * 0.001;
		tree_bound_.a_[i] -= foo, tree_bound_.g_[i] += foo;
	}
	Y_VERBOSE << "Kd-Tree: Done." << YENDL;
	// get working memory for tree construction
	BoundEdge *edges[3];
	uint32_t r_mem_size = 3 * total_prims_; // (maxDepth+1)*totalPrims;
	uint32_t *left_prims = new uint32_t[std::max((uint32_t)2 * tri_clip_thresh__, total_prims_)];
	uint32_t *right_prims = new uint32_t[r_mem_size]; //just a rough guess, allocating worst case is insane!
	//	uint32_t *primNums = new uint32_t[totalPrims]; //isn't this like...totaly unnecessary? use leftPrims?
	for(int i = 0; i < 3; ++i) edges[i] = new BoundEdge[514/*2*totalPrims*/];
	clip_ = new int[max_depth_ + 2];
	cdata_ = (char *) alignedAlloc__(64, (max_depth_ + 2) * tri_clip_thresh__ * clip_data_size__);

	// prepare data
	for(uint32_t i = 0; i < total_prims_; i++) left_prims[i] = i; //primNums[i] = i;
	for(int i = 0; i < max_depth_ + 2; i++) clip_[i] = -1;

	/* build tree */
	prims_ = v;
	Y_VERBOSE << "Kd-Tree: Starting recursive build..." << YENDL;
	buildTree(total_prims_, tree_bound_, left_prims,
			  left_prims, right_prims, edges, // <= working memory
	          r_mem_size, 0, 0);

	// free working memory
	delete[] left_prims;
	delete[] right_prims;
	delete[] all_bounds_;
	for(int i = 0; i < 3; ++i) delete[] edges[i];
	delete[] clip_;
	alignedFree__(cdata_);
	//print some stats:
	c_end = clock() - c_start;
	Y_VERBOSE << "Kd-Tree: Stats (" << float(c_end) / (float)CLOCKS_PER_SEC << "s)" << YENDL;
	Y_VERBOSE << "Kd-Tree: used/allocated nodes: " << next_free_node_ << "/" << allocated_nodes_count_
			  << " (" << 100.f * float(next_free_node_) / allocated_nodes_count_ << "%)" << YENDL;
	Y_VERBOSE << "Kd-Tree: Primitives in tree: " << total_prims_ << YENDL;
	Y_VERBOSE << "Kd-Tree: Interior nodes: " << kd_stats_.kd_inodes_ << " / " << "leaf nodes: " << kd_stats_.kd_leaves_
			  << " (empty: " << kd_stats_.empty_kd_leaves_ << " = " << 100.f * float(kd_stats_.empty_kd_leaves_) / kd_stats_.kd_leaves_ << "%)" << YENDL;
	Y_VERBOSE << "Kd-Tree: Leaf prims: " << kd_stats_.kd_prims_ << " (" << float(kd_stats_.kd_prims_) / total_prims_ << " x prims in tree, leaf size: " << max_leaf_size_ << ")" << YENDL;
	Y_VERBOSE << "Kd-Tree: => " << float(kd_stats_.kd_prims_) / (kd_stats_.kd_leaves_ - kd_stats_.empty_kd_leaves_) << " prims per non-empty leaf" << YENDL;
	Y_VERBOSE << "Kd-Tree: Leaves due to depth limit/bad splits: " << kd_stats_.depth_limit_reached_ << "/" << kd_stats_.num_bad_splits_ << YENDL;
	Y_VERBOSE << "Kd-Tree: clipped triangles: " << kd_stats_.clip_ << " (" << kd_stats_.bad_clip_ << " bad clips, " << kd_stats_.null_clip_ << " null clips)" << YENDL;
}

template<class T>
KdTree<T>::~KdTree()
{
	Y_INFO << "Kd-Tree: Freeing nodes..." << YENDL;
	alignedFree__(nodes_);
	Y_VERBOSE << "Kd-Tree: Done" << YENDL;
}

// ============================================================
/*!
	Faster cost function: Find the optimal split with SAH
	and binning => O(n)
*/

template<class T>
void KdTree<T>::pigeonMinCost(uint32_t n_prims, Bound &node_bound, uint32_t *prim_idx, SplitCost &split)
{
	TreeBin bin[kd_bins__ + 1 ];
	float d[3];
	d[0] = node_bound.longX();
	d[1] = node_bound.longY();
	d[2] = node_bound.longZ();
	split.old_cost_ = float(n_prims);
	split.best_cost_ = std::numeric_limits<float>::infinity();
	float inv_total_sa = 1.0f / (d[0] * d[1] + d[0] * d[2] + d[1] * d[2]);
	float t_low, t_up;
	int b_left, b_right;

	for(int axis = 0; axis < 3; axis++)
	{
		float s = kd_bins__ / d[axis];
		float min = node_bound.a_[axis];
		// pigeonhole sort:
		for(unsigned int i = 0; i < n_prims; ++i)
		{
			const Bound &bbox = all_bounds_[ prim_idx[i] ];
			t_low = bbox.a_[axis];
			t_up  = bbox.g_[axis];
			b_left = (int)((t_low - min) * s);
			b_right = (int)((t_up - min) * s);
			//			b_left = Y_Round2Int( ((t_low - min)*s) );
			//			b_right = Y_Round2Int( ((t_up - min)*s) );

			if(b_left < 0) b_left = 0;
			else if(b_left > kd_bins__) b_left = kd_bins__;

			if(b_right < 0) b_right = 0;
			else if(b_right > kd_bins__) b_right = kd_bins__;

			if(t_low == t_up)
			{
				if(bin[b_left].empty() || (t_low >= bin[b_left].t_ && !bin[b_left].empty()))
				{
					bin[b_left].t_ = t_low;
					bin[b_left].c_both_++;
				}
				else
				{
					bin[b_left].c_left_++;
					bin[b_left].c_right_++;
				}
				bin[b_left].n_ += 2;
			}
			else
			{
				if(bin[b_left].empty() || (t_low > bin[b_left].t_ && !bin[b_left].empty()))
				{
					bin[b_left].t_ = t_low;
					bin[b_left].c_left_ += bin[b_left].c_both_ + bin[b_left].c_bleft_;
					bin[b_left].c_right_ += bin[b_left].c_both_;
					bin[b_left].c_both_ = bin[b_left].c_bleft_ = 0;
					bin[b_left].c_bleft_++;
				}
				else if(t_low == bin[b_left].t_)
				{
					bin[b_left].c_bleft_++;
				}
				else bin[b_left].c_left_++;
				bin[b_left].n_++;

				bin[b_right].c_right_++;
				if(bin[b_right].empty() || t_up > bin[b_right].t_)
				{
					bin[b_right].t_ = t_up;
					bin[b_right].c_left_ += bin[b_right].c_both_ + bin[b_right].c_bleft_;
					bin[b_right].c_right_ += bin[b_right].c_both_;
					bin[b_right].c_both_ = bin[b_right].c_bleft_ = 0;
				}
				bin[b_right].n_++;
			}

		}

		const int axis_lut[3][3] = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1} };
		float cap_area = d[ axis_lut[1][axis] ] * d[ axis_lut[2][axis] ];
		float cap_perim = d[ axis_lut[1][axis] ] + d[ axis_lut[2][axis] ];

		unsigned int n_below = 0, n_above = n_prims;
		// cumulate prims and evaluate cost
		for(int i = 0; i < kd_bins__ + 1; ++i)
		{
			if(!bin[i].empty())
			{
				n_below += bin[i].c_left_;
				n_above -= bin[i].c_right_;
				// cost:
				float edget = bin[i].t_;
				if(edget > node_bound.a_[axis] && edget < node_bound.g_[axis])
				{
					// Compute cost for split at _i_th edge
					float l_1 = edget - node_bound.a_[axis];
					float l_2 = node_bound.g_[axis] - edget;
					float below_sa = cap_area + l_1 * cap_perim;
					float above_sa = cap_area + l_2 * cap_perim;
					float raw_costs = (below_sa * n_below + above_sa * n_above);
					//float eb = (nAbove == 0 || nBelow == 0) ? eBonus*rawCosts : 0.f;
					float eb;

					if(n_above == 0) eb = (0.1f + l_2 / d[axis]) * e_bonus_ * raw_costs;
					else if(n_below == 0) eb = (0.1f + l_1 / d[axis]) * e_bonus_ * raw_costs;
					else eb = 0.0f;

					float cost = cost_ratio_ + inv_total_sa * (raw_costs - eb);

					// Update best split if this is lowest cost so far
					if(cost < split.best_cost_)
					{
						split.t_ = edget;
						split.best_cost_ = cost;
						split.best_axis_ = axis;
						split.best_offset_ = i; // kinda useless...
						split.n_below_ = n_below;
						split.n_above_ = n_above;
					}
				}
				n_below += bin[i].c_both_ + bin[i].c_bleft_;
				n_above -= bin[i].c_both_;
			}
		} // for all bins
		if(n_below != n_prims || n_above != 0)
		{
			int c_1 = 0, c_2 = 0, c_3 = 0, c_4 = 0, c_5 = 0;
			Y_VERBOSE << "SCREWED!!\n";
			for(int i = 0; i < kd_bins__ + 1; i++) { c_1 += bin[i].n_; std::cout << bin[i].n_ << " ";}
			Y_VERBOSE << "\nn total: " << c_1 << "\n";
			for(int i = 0; i < kd_bins__ + 1; i++) { c_2 += bin[i].c_left_; Y_VERBOSE << bin[i].c_left_ << " ";}
			Y_VERBOSE << "\nc_left total: " << c_2 << "\n";
			for(int i = 0; i < kd_bins__ + 1; i++) { c_3 += bin[i].c_bleft_; Y_VERBOSE << bin[i].c_bleft_ << " ";}
			Y_VERBOSE << "\nc_bleft total: " << c_3 << "\n";
			for(int i = 0; i < kd_bins__ + 1; i++) { c_4 += bin[i].c_both_; Y_VERBOSE << bin[i].c_both_ << " ";}
			Y_VERBOSE << "\nc_both total: " << c_4 << "\n";
			for(int i = 0; i < kd_bins__ + 1; i++) { c_5 += bin[i].c_right_; Y_VERBOSE << bin[i].c_right_ << " ";}
			Y_VERBOSE << "\nc_right total: " << c_5 << "\n";
			Y_VERBOSE << "\nnPrims: " << n_prims << " nBelow: " << n_below << " nAbove: " << n_above << "\n";
			Y_VERBOSE << "total left: " << c_2 + c_3 + c_4 << "\ntotal right: " << c_4 + c_5 << "\n";
			Y_VERBOSE << "n/2: " << c_1 / 2 << "\n";
			throw std::logic_error("cost function mismatch");
		}
		for(int i = 0; i < kd_bins__ + 1; i++) bin[i].reset();
	} // for all axis
}

// ============================================================
/*!
	Cost function: Find the optimal split with SAH
*/

template<class T>
void KdTree<T>::minimalCost(uint32_t n_prims, Bound &node_bound, uint32_t *prim_idx,
							const Bound *all_bounds, BoundEdge *edges[3], SplitCost &split)
{
	float d[3];
	d[0] = node_bound.longX();
	d[1] = node_bound.longY();
	d[2] = node_bound.longZ();
	split.old_cost_ = float(n_prims);
	split.best_cost_ = std::numeric_limits<float>::infinity();
	float inv_total_sa = 1.0f / (d[0] * d[1] + d[0] * d[2] + d[1] * d[2]);
	int n_edge;

	for(int axis = 0; axis < 3; axis++)
	{
		// << get edges for axis >>
		int pn;
		n_edge = 0;
		//test!
		if(all_bounds != all_bounds_) for(unsigned int i = 0; i < n_prims; i++)
			{
				pn = prim_idx[i];
				const Bound &bbox = all_bounds[i];
				if(bbox.a_[axis] == bbox.g_[axis])
				{
					edges[axis][n_edge] = BoundEdge(bbox.a_[axis], i /* pn */, both_b__);
					++n_edge;
				}
				else
				{
					edges[axis][n_edge] = BoundEdge(bbox.a_[axis], i /* pn */, lower_b__);
					edges[axis][n_edge + 1] = BoundEdge(bbox.g_[axis], i /* pn */, upper_b__);
					n_edge += 2;
				}
			}
		else for(unsigned int i = 0; i < n_prims; i++)
			{
				pn = prim_idx[i];
				const Bound &bbox = all_bounds[pn];
				if(bbox.a_[axis] == bbox.g_[axis])
				{
					edges[axis][n_edge] = BoundEdge(bbox.a_[axis], pn, both_b__);
					++n_edge;
				}
				else
				{
					edges[axis][n_edge] = BoundEdge(bbox.a_[axis], pn, lower_b__);
					edges[axis][n_edge + 1] = BoundEdge(bbox.g_[axis], pn, upper_b__);
					n_edge += 2;
				}
			}
		std::sort(&edges[axis][0], &edges[axis][n_edge]);
		// Compute cost of all splits for _axis_ to find best
		const int axis_lut[3][3] = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1} };
		float cap_area = d[ axis_lut[1][axis] ] * d[ axis_lut[2][axis] ];
		float cap_perim = d[ axis_lut[1][axis] ] + d[ axis_lut[2][axis] ];
		unsigned int n_below = 0, n_above = n_prims;
		//todo: early-out criteria: if l1 > l2*nPrims (l2 > l1*nPrims) => minimum is lowest (highest) edge!
		if(n_prims > 5)
		{
			float edget = edges[axis][0].pos_;
			float l_1 = edget - node_bound.a_[axis];
			float l_2 = node_bound.g_[axis] - edget;
			if(l_1 > l_2 * float(n_prims) && l_2 > 0.f)
			{
				float raw_costs = (cap_area + l_2 * cap_perim) * n_prims;
				float cost = cost_ratio_ + inv_total_sa * (raw_costs - e_bonus_); //todo: use proper ebonus...
				//optimal cost is definitely here, and nowhere else!
				if(cost < split.best_cost_)
				{
					split.best_cost_ = cost;
					split.best_axis_ = axis;
					split.best_offset_ = 0;
					split.n_edge_ = n_edge;
					++kd_stats_.early_out_;
				}
				continue;
			}
			edget = edges[axis][n_edge - 1].pos_;
			l_1 = edget - node_bound.a_[axis];
			l_2 = node_bound.g_[axis] - edget;
			if(l_2 > l_1 * float(n_prims) && l_1 > 0.f)
			{
				float raw_costs = (cap_area + l_1 * cap_perim) * n_prims;
				float cost = cost_ratio_ + inv_total_sa * (raw_costs - e_bonus_); //todo: use proper ebonus...
				if(cost < split.best_cost_)
				{
					split.best_cost_ = cost;
					split.best_axis_ = axis;
					split.best_offset_ = n_edge - 1;
					split.n_edge_ = n_edge;
					++kd_stats_.early_out_;
				}
				continue;
			}
		}

		for(int i = 0; i < n_edge; ++i)
		{
			if(edges[axis][i].end_ == upper_b__) --n_above;
			float edget = edges[axis][i].pos_;
			if(edget > node_bound.a_[axis] &&
			   edget < node_bound.g_[axis])
			{
				// Compute cost for split at _i_th edge
				float l_1 = edget - node_bound.a_[axis];
				float l_2 = node_bound.g_[axis] - edget;
				float below_sa = cap_area + (l_1) * cap_perim;
				float above_sa = cap_area + (l_2) * cap_perim;
				float raw_costs = (below_sa * n_below + above_sa * n_above);
				//float eb = (nAbove == 0 || nBelow == 0) ? eBonus*rawCosts : 0.f;
				float eb;

				if(n_above == 0) eb = (0.1f + l_2 / d[axis]) * e_bonus_ * raw_costs;
				else if(n_below == 0) eb = (0.1f + l_1 / d[axis]) * e_bonus_ * raw_costs;
				else eb = 0.0f;

				float cost = cost_ratio_ + inv_total_sa * (raw_costs - eb);
				// Update best split if this is lowest cost so far
				if(cost < split.best_cost_)
				{
					split.best_cost_ = cost;
					split.best_axis_ = axis;
					split.best_offset_ = i;
					split.n_edge_ = n_edge;
					//delete again:
					split.n_below_ = n_below;
					split.n_above_ = n_above;
				}
			}
			if(edges[axis][i].end_ != upper_b__)
			{
				++n_below;
				if(edges[axis][i].end_ == both_b__) --n_above;
			}
		}
		if(n_below != n_prims || n_above != 0) Y_VERBOSE << "you screwed your new idea!\n";
		//		Assert(nBelow == nPrims && nAbove == 0);
	}
}

// ============================================================
/*!
	recursively build the Kd-tree
	returns:	0 when leaf was created
				1 when either current or at least 1 subsequent split reduced cost
				2 when neither current nor subsequent split reduced cost
*/
template<class T>
int KdTree<T>::buildTree(uint32_t n_prims, Bound &node_bound, uint32_t *prim_nums,
						 uint32_t *left_prims, uint32_t *right_prims, BoundEdge *edges[3], //working memory
                           uint32_t right_mem_size, int depth, int bad_refines)  // status
{
	//	std::cout << "tree level: " << depth << std::endl;
	if(next_free_node_ == allocated_nodes_count_)
	{
		int new_count = 2 * allocated_nodes_count_;
		new_count = (new_count > 0x100000) ? allocated_nodes_count_ + 0x80000 : new_count;
		KdTreeNode<T> 	*n = (KdTreeNode<T> *) alignedAlloc__(64, new_count * sizeof(KdTreeNode<T>));
		memcpy(n, nodes_, allocated_nodes_count_ * sizeof(KdTreeNode<T>));
		alignedFree__(nodes_);
		nodes_ = n;
		allocated_nodes_count_ = new_count;
	}

#if TRI_CLIP > 0
	if(n_prims <= tri_clip_thresh__)
	{
		//		exBound_t ebound(nodeBound);
		int o_prims[tri_clip_thresh__], n_overl = 0;
		double b_half_size[3];
		double b_ext[2][3];
		for(int i = 0; i < 3; ++i)
		{
			//			bCenter[i]   = ((double)nodeBound.a[i] + (double)nodeBound.g[i])*0.5;
			b_half_size[i] = ((double)node_bound.g_[i] - (double)node_bound.a_[i]);
			double temp  = ((double)tree_bound_.g_[i] - (double)tree_bound_.a_[i]);
			b_ext[0][i] = node_bound.a_[i] - 0.021 * b_half_size[i] - 0.00001 * temp;
			b_ext[1][i] = node_bound.g_[i] + 0.021 * b_half_size[i] + 0.00001 * temp;
			//			ebound.halfSize[i] *= 1.01;
		}
		char *c_old = cdata_ + (tri_clip_thresh__ * clip_data_size__ * depth);
		char *c_new = cdata_ + (tri_clip_thresh__ * clip_data_size__ * (depth + 1));
		for(unsigned int i = 0; i < n_prims; ++i)
		{
			const T *ct = prims_[ prim_nums[i] ];
			uint32_t old_idx = 0;
			if(clip_[depth] >= 0) old_idx = prim_nums[i + n_prims];
			//			if(old_idx > TRI_CLIP_THRESH){ std::cout << "ouch!\n"; }
			//			std::cout << "parent idx: " << old_idx << std::endl;
			if(ct->clippingSupport())
			{
				if(ct->clipToBound(b_ext, clip_[depth], all_bounds_[total_prims_ + n_overl],
				                   c_old + old_idx * clip_data_size__, c_new + n_overl * clip_data_size__))
				{
					++kd_stats_.clip_;
					o_prims[n_overl] = prim_nums[i]; n_overl++;
				}
				else ++kd_stats_.null_clip_;
			}
			else
			{
				// no clipping supported by prim, copy old bound:
				all_bounds_[total_prims_ + n_overl] = all_bounds_[ prim_nums[i] ]; //really??
				o_prims[n_overl] = prim_nums[i]; n_overl++;
			}
		}
		//copy back
		memcpy(prim_nums, o_prims, n_overl * sizeof(uint32_t));
		n_prims = n_overl;
	}
#endif
	//	<< check if leaf criteria met >>
	if(n_prims <= max_leaf_size_ || depth >= max_depth_)
	{
		//		std::cout << "leaf\n";
		nodes_[next_free_node_].createLeaf(prim_nums, n_prims, prims_, prims_arena_, kd_stats_);
		next_free_node_++;
		if(depth >= max_depth_) kd_stats_.depth_limit_reached_++;   //stat
		return 0;
	}

	//<< calculate cost for all axes and chose minimum >>
	SplitCost split;
	float base_bonus = e_bonus_;
	e_bonus_ *= 1.1 - (float)depth / (float)max_depth_;
	if(n_prims > 128) pigeonMinCost(n_prims, node_bound, prim_nums, split);
#if TRI_CLIP > 0
	else if(n_prims > tri_clip_thresh__) minimalCost(n_prims, node_bound, prim_nums, all_bounds_, edges, split);
	else minimalCost(n_prims, node_bound, prim_nums, all_bounds_ + total_prims_, edges, split);
#else
	else minimalCost(nPrims, nodeBound, primNums, allBounds, edges, split);
#endif
	e_bonus_ = base_bonus; //restore eBonus
	//<< if (minimum > leafcost) increase bad refines >>
	if(split.best_cost_ > split.old_cost_) ++bad_refines;
	if((split.best_cost_ > 1.6f * split.old_cost_ && n_prims < 16) ||
	   split.best_axis_ == -1 || bad_refines == 2)
	{
		nodes_[next_free_node_].createLeaf(prim_nums, n_prims, prims_, prims_arena_, kd_stats_);
		next_free_node_++;
		if(bad_refines == 2) ++kd_stats_.num_bad_splits_;  //stat
		return 0;
	}

	//todo: check working memory for child recursive calls
	uint32_t remaining_mem, *more_prims = nullptr, *n_right_prims;
	uint32_t *old_right_prims = right_prims;
	if(n_prims > right_mem_size || 2 * tri_clip_thresh__ > right_mem_size) // *possibly* not enough, get some more
	{
		//		std::cout << "buildTree: more memory allocated!\n";
		remaining_mem = n_prims * 3;
		more_prims = new uint32_t[remaining_mem];
		n_right_prims = more_prims;
	}
	else
	{
		n_right_prims = old_right_prims;
		remaining_mem = right_mem_size;
	}

	// Classify primitives with respect to split
	float split_pos;
	int n_0 = 0, n_1 = 0;
	if(n_prims > 128) // we did pigeonhole
	{
		int pn;
		for(unsigned int i = 0; i < n_prims; i++)
		{
			pn = prim_nums[i];
			if(all_bounds_[ pn ].a_[split.best_axis_] >= split.t_) n_right_prims[n_1++] = pn;
			else
			{
				left_prims[n_0++] = pn;
				if(all_bounds_[ pn ].g_[split.best_axis_] > split.t_) n_right_prims[n_1++] = pn;
			}
		}
		split_pos = split.t_;
		//if(n_0 != split.n_below_ || n_1 != split.n_above_) Y_WARNING << "Kd-Tree: oops! out of split bounds." << YENDL;
		/*		static int foo=0;
				if(foo<10)
				{
					std::cout << "best axis:"<<split.bestAxis<<", rel. split:" <<(split.t - nodeBound.a[split.bestAxis])/(nodeBound.g[split.bestAxis]-nodeBound.a[split.bestAxis]);
					std::cout << "\nleft Prims:"<<n0<<", right Prims:"<<n1<<", total Prims:"<<nPrims<<" (level:"<<depth<<")\n";
					foo++;
				}*/
	}
	else if(n_prims <= tri_clip_thresh__)
	{
		int cindizes[tri_clip_thresh__];
		uint32_t old_prims[tri_clip_thresh__];
		//		std::cout << "memcpy\n";
		memcpy(old_prims, prim_nums, n_prims * sizeof(int));

		for(int i = 0; i < split.best_offset_; ++i)
		{
			if(edges[split.best_axis_][i].end_ != upper_b__)
			{
				cindizes[n_0] = edges[split.best_axis_][i].prim_num_;
				//				if(cindizes[n0] >nPrims) std::cout << "error!!\n";
				left_prims[n_0] = old_prims[cindizes[n_0]];
				++n_0;
			}
		}
		//		std::cout << "append n0\n";
		for(int i = 0; i < n_0; ++i) { left_prims[n_0 + i] = cindizes[i]; /* std::cout << cindizes[i] << " "; */ }
		if(edges[split.best_axis_][split.best_offset_].end_ == both_b__)
		{
			cindizes[n_1] = edges[split.best_axis_][split.best_offset_].prim_num_;
			//			if(cindizes[n1] >nPrims) std::cout << "error!!\n";
			n_right_prims[n_1] = old_prims[cindizes[n_1]];
			++n_1;
		}

		for(int i = split.best_offset_ + 1; i < split.n_edge_; ++i)
		{
			if(edges[split.best_axis_][i].end_ != lower_b__)
			{
				cindizes[n_1] = edges[split.best_axis_][i].prim_num_;
				//				if(cindizes[n1] >nPrims) std::cout << "error!!\n";
				n_right_prims[n_1] = old_prims[cindizes[n_1]];
				++n_1;
			}
		}
		//		std::cout << "\nappend n1\n";
		for(int i = 0; i < n_1; ++i) { n_right_prims[n_1 + i] = cindizes[i]; /* std::cout << cindizes[i] << " "; */ }
		split_pos = edges[split.best_axis_][split.best_offset_].pos_;
		//		std::cout << "\ndone\n";
	}
	else //we did "normal" cost function
	{
		for(int i = 0; i < split.best_offset_; ++i)
			if(edges[split.best_axis_][i].end_ != upper_b__)
				left_prims[n_0++] = edges[split.best_axis_][i].prim_num_;
		if(edges[split.best_axis_][split.best_offset_].end_ == both_b__)
			n_right_prims[n_1++] = edges[split.best_axis_][split.best_offset_].prim_num_;
		for(int i = split.best_offset_ + 1; i < split.n_edge_; ++i)
			if(edges[split.best_axis_][i].end_ != lower_b__)
				n_right_prims[n_1++] = edges[split.best_axis_][i].prim_num_;
		split_pos = edges[split.best_axis_][split.best_offset_].pos_;
	}
	//	std::cout << "split axis: " << split.bestAxis << ", pos: " << splitPos << "\n";
	//advance right prims pointer
	remaining_mem -= n_1;

	uint32_t cur_node = next_free_node_;
	nodes_[cur_node].createInterior(split.best_axis_, split_pos, kd_stats_);
	++next_free_node_;
	Bound bound_l = node_bound, bound_r = node_bound;
	switch(split.best_axis_)
	{
		case 0: bound_l.setMaxX(split_pos); bound_r.setMinX(split_pos); break;
		case 1: bound_l.setMaxY(split_pos); bound_r.setMinY(split_pos); break;
		case 2: bound_l.setMaxZ(split_pos); bound_r.setMinZ(split_pos); break;
	}

#if TRI_CLIP > 0
	if(n_prims <= tri_clip_thresh__)
	{
		remaining_mem -= n_1;
		//<< recurse below child >>
		clip_[depth + 1] = split.best_axis_;
		buildTree(n_0, bound_l, left_prims, left_prims, n_right_prims + 2 * n_1, edges, remaining_mem, depth + 1, bad_refines);
		clip_[depth + 1] |= 1 << 2;
		//<< recurse above child >>
		nodes_[cur_node].setRightChild(next_free_node_);
		buildTree(n_1, bound_r, n_right_prims, left_prims, n_right_prims + 2 * n_1, edges, remaining_mem, depth + 1, bad_refines);
		clip_[depth + 1] = -1;
	}
	else
	{
#endif
		//<< recurse below child >>
		buildTree(n_0, bound_l, left_prims, left_prims, n_right_prims + n_1, edges, remaining_mem, depth + 1, bad_refines);
		//<< recurse above child >>
		nodes_[cur_node].setRightChild(next_free_node_);
		buildTree(n_1, bound_r, n_right_prims, left_prims, n_right_prims + n_1, edges, remaining_mem, depth + 1, bad_refines);
#if TRI_CLIP > 0
	}
#endif
	// free additional working memory, if present
	if(more_prims) delete[] more_prims;
	return 1;
}



//============================
/*! The standard intersect function,
	returns the closest hit within dist
*/
template<class T>
bool KdTree<T>::intersect(const Ray &ray, float dist, T **tr, float &z, IntersectData &data) const
{
	z = dist;

	float a, b, t; // entry/exit/splitting plane signed distance
	float t_hit;

	if(!tree_bound_.cross(ray, a, b, dist)) { return false; }

	IntersectData current_data, temp_data;
	Vec3 inv_dir(1.0 / ray.dir_.x_, 1.0 / ray.dir_.y_, 1.0 / ray.dir_.z_); //was 1.f!
	//	int rayId = curMailboxId++;
	bool hit = false;

	KdStack<T> stack[kd_max_stack__];
	const KdTreeNode<T> *far_child, *curr_node;
	curr_node = nodes_;

	int en_pt = 0;
	stack[en_pt].t_ = a;

	//distinguish between internal and external origin
	if(a >= 0.0) // ray with external origin
		stack[en_pt].pb_ = ray.from_ + ray.dir_ * a;
	else // ray with internal origin
		stack[en_pt].pb_ = ray.from_;

	// setup initial entry and exit poimt in stack
	int ex_pt = 1; // pointer to stack
	stack[ex_pt].t_ = b;
	stack[ex_pt].pb_ = ray.from_ + ray.dir_ * b;
	stack[ex_pt].node_ = nullptr; // "nowhere", termination flag

	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while(curr_node != nullptr)
	{
		if(dist < stack[en_pt].t_) break;
		// loop until leaf is found
		while(!curr_node->isLeaf())
		{
			int axis = curr_node->splitAxis();
			float split_val = curr_node->splitPos();

			if(stack[en_pt].pb_[axis] <= split_val)
			{
				if(stack[ex_pt].pb_[axis] <= split_val)
				{
					curr_node++;
					continue;
				}
				if(stack[ex_pt].pb_[axis] == split_val)
				{
					curr_node = &nodes_[curr_node->getRightChild()];
					continue;
				}
				// case N4
				far_child = &nodes_[curr_node->getRightChild()];
				curr_node ++;
			}
			else
			{
				if(split_val < stack[ex_pt].pb_[axis])
				{
					curr_node = &nodes_[curr_node->getRightChild()];
					continue;
				}
				far_child = curr_node + 1;
				curr_node = &nodes_[curr_node->getRightChild()];
			}
			// traverse both children

			t = (split_val - ray.from_[axis]) * inv_dir[axis];

			// setup the new exit point
			int tmp = ex_pt;
			ex_pt++;

			// possibly skip current entry point so not to overwrite the data
			if(ex_pt == en_pt) ex_pt++;
			// push values onto the stack //todo: lookup table
			static const int np_axis[2][3] = {{1, 2, 0}, {2, 0, 1} };
			int next_axis = np_axis[0][axis];//(axis+1)%3;
			int prev_axis = np_axis[1][axis];//(axis+2)%3;
			stack[ex_pt].prev_ = tmp;
			stack[ex_pt].t_ = t;
			stack[ex_pt].node_ = far_child;
			stack[ex_pt].pb_[axis] = split_val;
			stack[ex_pt].pb_[next_axis] = ray.from_[next_axis] + t * ray.dir_[next_axis];
			stack[ex_pt].pb_[prev_axis] = ray.from_[prev_axis] + t * ray.dir_[prev_axis];
		}

		// Check for intersections inside leaf node
		uint32_t n_primitives = curr_node->nPrimitives();

		if(n_primitives == 1)
		{
			T *mp = curr_node->one_primitive_;
			if(mp->intersect(ray, &t_hit, temp_data))
			{
				if(t_hit < z && t_hit >= ray.tmin_)
				{
					const Material *mat = mp->getMaterial();

					if(mat->getVisibility() == Material::Visibility::NormalVisible || mat->getVisibility() == Material::Visibility::VisibleNoShadows)
					{
						z = t_hit;
						*tr = mp;
						current_data = temp_data;
						hit = true;
					}
				}
			}
		}
		else
		{
			T **prims = curr_node->primitives_;
			for(uint32_t i = 0; i < n_primitives; ++i)
			{
				T *mp = prims[i];
				if(mp->intersect(ray, &t_hit, temp_data))
				{
					if(t_hit < z && t_hit >= ray.tmin_)
					{
						const Material *mat = mp->getMaterial();

						if(mat->getVisibility() == Material::Visibility::NormalVisible || mat->getVisibility() == Material::Visibility::VisibleNoShadows)
						{
							z = t_hit;
							*tr = mp;
							current_data = temp_data;
							hit = true;
						}
					}
				}
			}
		}

		if(hit && z <= stack[ex_pt].t_)
		{
			data = current_data;
			return true;
		}

		en_pt = ex_pt;
		curr_node = stack[ex_pt].node_;
		ex_pt = stack[en_pt].prev_;

	} // while

	data = current_data;

	return hit;
}

template<class T>
bool KdTree<T>::intersectS(const Ray &ray, float dist, T **tr, float shadow_bias) const
{
	float a, b, t; // entry/exit/splitting plane signed distance
	float t_hit;

	if(!tree_bound_.cross(ray, a, b, dist))
		return false;

	IntersectData bary;
	Vec3 inv_dir(1.f / ray.dir_.x_, 1.f / ray.dir_.y_, 1.f / ray.dir_.z_);

	KdStack<T> stack[kd_max_stack__];
	const KdTreeNode<T> *far_child, *curr_node;
	curr_node = nodes_;

	int en_pt = 0;
	stack[en_pt].t_ = a;

	//distinguish between internal and external origin
	if(a >= 0.0) // ray with external origin
		stack[en_pt].pb_ = ray.from_ + ray.dir_ * a;
	else // ray with internal origin
		stack[en_pt].pb_ = ray.from_;

	// setup initial entry and exit poimt in stack
	int ex_pt = 1; // pointer to stack
	stack[ex_pt].t_ = b;
	stack[ex_pt].pb_ = ray.from_ + ray.dir_ * b;
	stack[ex_pt].node_ = nullptr; // "nowhere", termination flag

	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while(curr_node != nullptr)
	{
		if(dist < stack[en_pt].t_ /*a*/) break;
		// loop until leaf is found
		while(!curr_node->isLeaf())
		{
			int axis = curr_node->splitAxis();
			float split_val = curr_node->splitPos();

			if(stack[en_pt].pb_[axis] <= split_val)
			{
				if(stack[ex_pt].pb_[axis] <= split_val)
				{
					curr_node++;
					continue;
				}
				if(stack[ex_pt].pb_[axis] == split_val)
				{
					curr_node = &nodes_[curr_node->getRightChild()];
					continue;
				}
				// case N4
				far_child = &nodes_[curr_node->getRightChild()];
				curr_node ++;
			}
			else
			{
				if(split_val < stack[ex_pt].pb_[axis])
				{
					curr_node = &nodes_[curr_node->getRightChild()];
					continue;
				}
				far_child = curr_node + 1;
				curr_node = &nodes_[curr_node->getRightChild()];
			}
			// traverse both children

			t = (split_val - ray.from_[axis]) * inv_dir[axis];

			// setup the new exit point
			int tmp = ex_pt;
			ex_pt++;

			// possibly skip current entry point so not to overwrite the data
			if(ex_pt == en_pt) ex_pt++;
			// push values onto the stack //todo: lookup table
			static const int np_axis[2][3] = {{1, 2, 0}, {2, 0, 1} };
			int next_axis = np_axis[0][axis];//(axis+1)%3;
			int prev_axis = np_axis[1][axis];//(axis+2)%3;
			stack[ex_pt].prev_ = tmp;
			stack[ex_pt].t_ = t;
			stack[ex_pt].node_ = far_child;
			stack[ex_pt].pb_[axis] = split_val;
			stack[ex_pt].pb_[next_axis] = ray.from_[next_axis] + t * ray.dir_[next_axis];
			stack[ex_pt].pb_[prev_axis] = ray.from_[prev_axis] + t * ray.dir_[prev_axis];
		}

		// Check for intersections inside leaf node
		uint32_t n_primitives = curr_node->nPrimitives();
		if(n_primitives == 1)
		{
			T *mp = curr_node->one_primitive_;
			if(mp->intersect(ray, &t_hit, bary))
			{
				if(t_hit < dist && t_hit >= 0.f)  // '>=' ?
				{
					const Material *mat = mp->getMaterial();

					if(mat->getVisibility() == Material::Visibility::NormalVisible || mat->getVisibility() == Material::Visibility::InvisibleShadowsOnly) // '>=' ?
					{
						*tr = mp;
						return true;
					}
				}
			}
		}
		else
		{
			T **prims = curr_node->primitives_;
			for(uint32_t i = 0; i < n_primitives; ++i)
			{
				T *mp = prims[i];
				if(mp->intersect(ray, &t_hit, bary))
				{
					if(t_hit < dist && t_hit >= 0.f)
					{
						const Material *mat = mp->getMaterial();

						if(mat->getVisibility() == Material::Visibility::NormalVisible || mat->getVisibility() == Material::Visibility::InvisibleShadowsOnly)
						{
							*tr = mp;
							return true;
						}
					}
				}
			}
		}

		en_pt = ex_pt;
		curr_node = stack[ex_pt].node_;
		ex_pt = stack[en_pt].prev_;

	} // while

	return false;
}

/*=============================================================
	allow for transparent shadows.
=============================================================*/

template<class T>
bool KdTree<T>::intersectTs(RenderState &state, const Ray &ray, int max_depth, float dist, T **tr, Rgb &filt, float shadow_bias) const
{
	float a, b, t; // entry/exit/splitting plane signed distance
	float t_hit;

	if(!tree_bound_.cross(ray, a, b, dist))
		return false;

	IntersectData bary;

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

#if ( HAVE_PTHREAD && defined (__GNUC__) && !defined (__clang__) )
	std::set<const T *, std::less<const T *>, __gnu_cxx::__mt_alloc<const T *>> filtered;
#else
	std::set<const T *> filtered;
#endif
	KdStack<T> stack[kd_max_stack__];
	const KdTreeNode<T> *far_child, *curr_node;
	curr_node = nodes_;

	int en_pt = 0;
	stack[en_pt].t_ = a;

	//distinguish between internal and external origin
	if(a >= 0.0) // ray with external origin
		stack[en_pt].pb_ = ray.from_ + ray.dir_ * a;
	else // ray with internal origin
		stack[en_pt].pb_ = ray.from_;

	// setup initial entry and exit poimt in stack
	int ex_pt = 1; // pointer to stack
	stack[ex_pt].t_ = b;
	stack[ex_pt].pb_ = ray.from_ + ray.dir_ * b;
	stack[ex_pt].node_ = nullptr; // "nowhere", termination flag

	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while(curr_node != nullptr)
	{
		if(dist < stack[en_pt].t_ /*a*/) break;
		// loop until leaf is found
		while(!curr_node->isLeaf())
		{
			int axis = curr_node->splitAxis();
			float split_val = curr_node->splitPos();

			if(stack[en_pt].pb_[axis] <= split_val)
			{
				if(stack[ex_pt].pb_[axis] <= split_val)
				{
					curr_node++;
					continue;
				}
				if(stack[ex_pt].pb_[axis] == split_val)
				{
					curr_node = &nodes_[curr_node->getRightChild()];
					continue;
				}
				// case N4
				far_child = &nodes_[curr_node->getRightChild()];
				curr_node ++;
			}
			else
			{
				if(split_val < stack[ex_pt].pb_[axis])
				{
					curr_node = &nodes_[curr_node->getRightChild()];
					continue;
				}
				far_child = curr_node + 1;
				curr_node = &nodes_[curr_node->getRightChild()];
			}
			// traverse both children

			t = (split_val - ray.from_[axis]) * inv_dir[axis];

			// setup the new exit point
			int tmp = ex_pt;
			ex_pt++;

			// possibly skip current entry point so not to overwrite the data
			if(ex_pt == en_pt) ex_pt++;
			// push values onto the stack //todo: lookup table
			static const int np_axis[2][3] = {{1, 2, 0}, {2, 0, 1} };
			int next_axis = np_axis[0][axis];//(axis+1)%3;
			int prev_axis = np_axis[1][axis];//(axis+2)%3;
			stack[ex_pt].prev_ = tmp;
			stack[ex_pt].t_ = t;
			stack[ex_pt].node_ = far_child;
			stack[ex_pt].pb_[axis] = split_val;
			stack[ex_pt].pb_[next_axis] = ray.from_[next_axis] + t * ray.dir_[next_axis];
			stack[ex_pt].pb_[prev_axis] = ray.from_[prev_axis] + t * ray.dir_[prev_axis];
		}

		// Check for intersections inside leaf node
		uint32_t n_primitives = curr_node->nPrimitives();
		if(n_primitives == 1)
		{
			T *mp = curr_node->one_primitive_;
			if(mp->intersect(ray, &t_hit, bary))
			{
				if(t_hit < dist && t_hit >= ray.tmin_)  // '>=' ?
				{
					const Material *mat = mp->getMaterial();

					if(mat->getVisibility() == Material::Visibility::NormalVisible || mat->getVisibility() == Material::Visibility::InvisibleShadowsOnly) // '>=' ?
					{
						*tr = mp;

						if(!mat->isTransparent()) return true;

						if(filtered.insert(mp).second)
						{
							if(depth >= max_depth) return true;
							Point3 h = ray.from_ + t_hit * ray.dir_;
							SurfacePoint sp;
							mp->getSurface(sp, h, bary);
							filt *= mat->getTransparency(state, sp, ray.dir_);
							++depth;
						}
					}
				}
			}
		}
		else
		{
			T **prims = curr_node->primitives_;
			for(uint32_t i = 0; i < n_primitives; ++i)
			{
				T *mp = prims[i];
				if(mp->intersect(ray, &t_hit, bary))
				{
					if(t_hit < dist && t_hit >= ray.tmin_)
					{
						const Material *mat = mp->getMaterial();

						if(mat->getVisibility() == Material::Visibility::NormalVisible || mat->getVisibility() == Material::Visibility::InvisibleShadowsOnly)
						{
							*tr = mp;

							if(!mat->isTransparent()) return true;

							if(filtered.insert(mp).second)
							{
								if(depth >= max_depth) return true;
								Point3 h = ray.from_ + t_hit * ray.dir_;
								SurfacePoint sp;
								mp->getSurface(sp, h, bary);
								filt *= mat->getTransparency(state, sp, ray.dir_);
								++depth;
							}
						}
					}
				}
			}
		}

		en_pt = ex_pt;
		curr_node = stack[ex_pt].node_;
		ex_pt = stack[en_pt].prev_;

	} // while

	return false;
}

// explicit instantiation of template:
template class KdTree<Triangle>;
template class KdTree<Primitive>;

END_YAFARAY
