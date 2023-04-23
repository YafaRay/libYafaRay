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

#ifndef LIBYAFARAY_PKDTREE_H
#define LIBYAFARAY_PKDTREE_H

#include "common/logger.h"
#include "geometry/bound.h"
#include "render/render_control.h"
#include <vector>
#include <cstdlib>
#include <thread>

namespace yafaray {

template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;

namespace kdtree
{

#define NON_REC_LOOKUP 1

template <typename T>
struct KdNode
{
	void createLeaf(const T *d)
	{
		flags_ = 3;
		data_ = d;
	}
	void createInterior(Axis axis, float d)
	{
		division_ = d;
		flags_ = (flags_ & ~3) | axis::getId(axis);
	}
	float splitPos() const { return division_; }
	Axis splitAxis() const { return static_cast<Axis>(flags_ & 3); }
	uint32_t nPrimitives() const { return flags_ >> 2; }
	bool isLeaf() const { return (flags_ & 3) == 3; }
	uint32_t getRightChild() const { return (flags_ >> 2); }
	void setRightChild(uint32_t i) { flags_ = (flags_ & 3) | (i << 2); }
	union
	{
		float division_;
		const T *data_;
	};
	uint32_t flags_;
};

template<typename NodeData> struct CompareNode
{
	explicit CompareNode(Axis axis) { axis_ = axis; }
	Axis axis_;
	bool operator()(const NodeData *d_1, const NodeData *d_2) const
	{
		return d_1->pos_[axis_] == d_2->pos_[axis_] ? (d_1 < d_2) : d_1->pos_[axis_] < d_2->pos_[axis_];
	}
};

template <typename T>
class PointKdTree
{
	public:
		PointKdTree() = default;
		PointKdTree(Logger &logger, const RenderMonitor &render_monitor, const RenderControl &render_control, const std::vector<T> &dat, const std::string &map_name, int num_threads = 1);
		~PointKdTree() { if(nodes_) free(nodes_); }
		template<typename LookupProc> void lookup(const Point3f &p, LookupProc &proc, float &max_dist_squared) const;
	protected:
		template<typename LookupProc> void recursiveLookup(const Point3f &p, const LookupProc &proc, float &max_dist_squared, int node_num) const;
		struct KdStack
		{
			const KdNode<T> *node_; //!< pointer to far child
			float s_; //!< the split val of parent node
			Axis axis_; //!< the split axis of parent node
		};
		void buildTree(uint32_t start, uint32_t end, Bound<float> &node_bound, const T **prims);
		void buildTreeWorker(uint32_t start, uint32_t end, Bound<float> &node_bound, const T **prims, int level, uint32_t &local_next_free_node, KdNode<T> *local_nodes);
		const RenderControl &render_control_;
		KdNode<T> *nodes_;
		uint32_t n_elements_, next_free_node_;
		Bound<float> tree_bound_;
		int max_level_threads_ = 0;  //max level where we will launch threads. We will try to launch at least as many threads as scene threads parameter
		static constexpr inline unsigned int kd_max_stack_ = 64;
		std::mutex mutx_;
};

template<typename T>
PointKdTree<T>::PointKdTree(Logger &logger, const RenderMonitor &render_monitor, const RenderControl &render_control, const std::vector<T> &dat, const std::string &map_name, int num_threads) : render_control_{render_control}
{
	next_free_node_ = 0;
	n_elements_ = dat.size();

	if(n_elements_ == 0)
	{
		logger.logError("pointKdTree: ", map_name, " empty vector!");
		return;
	}

	nodes_ = (KdNode<T> *) malloc(4 * n_elements_ * sizeof(KdNode<T>)); //actually we could allocate one less...2n-1

	auto elements = std::unique_ptr<const T*[]>(new const T*[n_elements_]);

	for(uint32_t i = 0; i < n_elements_; ++i) elements[i] = &dat[i];

	tree_bound_ = {dat[0].pos_, dat[0].pos_};

	for(uint32_t i = 1; i < n_elements_; ++i) tree_bound_.include(dat[i].pos_);

	max_level_threads_ = (int) std::ceil(math::log2((float) num_threads)); //in how many pkdtree levels we will spawn threads, so we create at least as many threads as scene threads parameter (or more)
	int real_threads = static_cast<int>(math::pow(2.f, max_level_threads_)); //real amount of threads we will create during pkdtree creation depending on the maximum level where we will generate threads

	logger.logInfo("pointKdTree: Starting ", map_name, " recusive tree build for ", n_elements_, " elements [using ", real_threads, " threads]");

	buildTree(0, n_elements_, tree_bound_, elements.get());

	if(logger.isVerbose()) logger.logVerbose("pointKdTree: ", map_name, " tree built.");
}

template<typename T>
void PointKdTree<T>::buildTree(uint32_t start, uint32_t end, Bound<float> &node_bound, const T **prims)
{
	buildTreeWorker(start, end, node_bound, prims, 0, next_free_node_, nodes_);
}

template<typename T>
void PointKdTree<T>::buildTreeWorker(uint32_t start, uint32_t end, Bound<float> &node_bound, const T **prims, int level, uint32_t &local_next_free_node, KdNode<T> *local_nodes)
{
	if(render_control_.canceled()) return;
	++level;
	if(end - start == 1)
	{
		local_nodes[local_next_free_node].createLeaf(prims[start]);
		local_next_free_node++;
		--level;
		return;
	}
	Axis split_axis = node_bound.largestAxis();
	int split_el = (start + end) / 2;
	std::nth_element(&prims[start], &prims[split_el],
	                 &prims[end], CompareNode<T>(split_axis));
	uint32_t cur_node = local_next_free_node;
	float split_pos = prims[split_el]->pos_[split_axis];
	local_nodes[cur_node].createInterior(split_axis, split_pos);
	++local_next_free_node;
	Bound bound_l = node_bound, bound_r = node_bound;
	bound_l.setAxisMax(split_axis, split_pos);
	bound_r.setAxisMin(split_axis, split_pos);

	if(level <= max_level_threads_)   //launch threads for the first "x" levels to try to match (at least) the scene threads parameter
	{
		//<< recurse below child >>
		uint32_t next_free_node_1 = 0;
		auto *nodes_1 = (KdNode<T> *) malloc(4 * (split_el - start) * sizeof(KdNode<T>));
		auto below_worker = std::thread( &PointKdTree<T>::buildTreeWorker, this, start, split_el, std::ref(bound_l), prims, level, std::ref(next_free_node_1), nodes_1 );

		//<< recurse above child >>
		uint32_t next_free_node_2 = 0;
		auto *nodes_2 = (KdNode<T> *) malloc(4 * (end - split_el) * sizeof(KdNode<T>));
		auto above_worker = std::thread( &PointKdTree<T>::buildTreeWorker, this, split_el, end, std::ref(bound_r), prims, level, std::ref(next_free_node_2), nodes_2 );

		below_worker.join();
		above_worker.join();

		if(nodes_1)
		{
			for(uint32_t i = 0; i < next_free_node_1; ++i)
			{
				local_nodes[i + local_next_free_node] = nodes_1[i];
				if(!local_nodes[i + local_next_free_node].isLeaf())
				{
					uint32_t right_child = local_nodes[i + local_next_free_node].getRightChild();
					local_nodes[i + local_next_free_node].setRightChild(right_child + local_next_free_node);
				}
			}
			free(nodes_1);
		}

		if(nodes_2)
		{
			for(uint32_t i = 0; i < next_free_node_2; ++i)
			{
				local_nodes[i + local_next_free_node + next_free_node_1] = nodes_2[i];
				if(!local_nodes[i + local_next_free_node + next_free_node_1].isLeaf())
				{
					uint32_t right_child = local_nodes[i + local_next_free_node + next_free_node_1].getRightChild();
					local_nodes[i + local_next_free_node + next_free_node_1].setRightChild(right_child + local_next_free_node + next_free_node_1);
				}
			}
			free(nodes_2);
		}

		local_nodes[cur_node].setRightChild(local_next_free_node + next_free_node_1);
		local_next_free_node = local_next_free_node + next_free_node_1 + next_free_node_2;
	}
	else  //for the rest of the levels in the tree, don't launch more threads, do normal "sequential" operation
	{
		//<< recurse below child >>
		buildTreeWorker(start, split_el, bound_l, prims, level, local_next_free_node, local_nodes);
		//<< recurse above child >>
		local_nodes[cur_node].setRightChild(local_next_free_node);
		buildTreeWorker(split_el, end, bound_r, prims, level, local_next_free_node, local_nodes);
	}
	--level;
}


template<typename T> template<typename LookupProc>
void PointKdTree<T>::lookup(const Point3f &p, LookupProc &proc, float &max_dist_squared) const
{
#if NON_REC_LOOKUP > 0
	KdStack stack[kd_max_stack_];
	const KdNode<T> *far_child, *curr_node = nodes_;

	int stack_ptr = 1;
	stack[stack_ptr].node_ = nullptr; // "nowhere", termination flag

	while(true)
	{
		while(!curr_node->isLeaf())
		{
			Axis axis = curr_node->splitAxis();
			float split_val = curr_node->splitPos();

			if(p[axis] <= split_val)   //need traverse left first
			{
				far_child = &nodes_[curr_node->getRightChild()];
				curr_node++;
			}
			else //need traverse right child first
			{
				far_child = curr_node + 1;
				curr_node = &nodes_[curr_node->getRightChild()];
			}
			++stack_ptr;
			stack[stack_ptr].node_ = far_child;
			stack[stack_ptr].axis_ = axis;
			stack[stack_ptr].s_ = split_val;
		}

		// Hand leaf-data kd-tree to processing function
		const Vec3f v{curr_node->data_->pos_ - p};
		float dist_2 = v.lengthSquared();

		if(dist_2 < max_dist_squared)
		{
			proc(curr_node->data_, dist_2, max_dist_squared);
		}

		if(!stack[stack_ptr].node_) return; // stack empty, done.
		//radius probably lowered so we may pop additional elements:
		Axis axis = stack[stack_ptr].axis_;
		dist_2 = p[axis] - stack[stack_ptr].s_;
		dist_2 *= dist_2;

		while(dist_2 > max_dist_squared)
		{
			--stack_ptr;
			if(!stack[stack_ptr].node_) return;// stack empty, done.
			axis = stack[stack_ptr].axis_;
			dist_2 = p[axis] - stack[stack_ptr].s_;
			dist_2 *= dist_2;
		}
		curr_node = stack[stack_ptr].node_;
		--stack_ptr;
	}
#else
	recursiveLookup(p, proc, maxDistSquared, 0);
	++Y_LOOKUPS;
	if(Y_LOOKUPS == 159999)
	{
		if(logger_.isVerbose()) logger_.logVerbose("pointKd-Tree:average photons tested per lookup:", double(Y_PROCS) / double(Y_LOOKUPS));
	}
#endif
}

template<typename T> template<typename LookupProc>
void PointKdTree<T>::recursiveLookup(const Point3f &p, const LookupProc &proc, float &max_dist_squared, int node_num) const
{
	const KdNode<T> *curr_node = &nodes_[node_num];
	if(curr_node->isLeaf())
	{
		const Vec3f v{curr_node->data_->pos_ - p};
		float dist_2 = v.lengthSquared();
		if(dist_2 < max_dist_squared)
		{
			proc(curr_node->data_, dist_2, max_dist_squared);
		}
		return;
	}
	Axis axis = curr_node->splitAxis();
	float dist_2 = p[axis] - curr_node->splitPos();
	dist_2 *= dist_2;
	if(p[axis] <= curr_node->splitPos())
	{
		recursiveLookup(p, proc, max_dist_squared, node_num + 1);
		if(dist_2 < max_dist_squared)
			recursiveLookup(p, proc, max_dist_squared, curr_node->getRightChild());
	}
	else
	{
		recursiveLookup(p, proc, max_dist_squared, curr_node->getRightChild());
		if(dist_2 < max_dist_squared)
			recursiveLookup(p, proc, max_dist_squared, node_num + 1);
	}
}

} // namespace::kdtree

} //namespace yafaray

#endif // LIBYAFARAY_PKDTREE_H
