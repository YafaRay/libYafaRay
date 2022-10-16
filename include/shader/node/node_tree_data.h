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

#ifndef LIBYAFARAY_NODE_TREE_DATA_H
#define LIBYAFARAY_NODE_TREE_DATA_H

#include "shader/node/node_result.h"
#include <vector>

namespace yafaray {

class NodeTreeData final
{
	public:
		NodeTreeData() = default;
		NodeTreeData(const NodeTreeData &node_tree_data) = default;
		NodeTreeData(NodeTreeData &&node_tree_data) = default;
		NodeTreeData &operator=(const NodeTreeData &node_tree_data) = default;
		NodeTreeData &operator=(NodeTreeData &&node_tree_data) = default;
		explicit NodeTreeData(size_t number_of_nodes) : node_results_(number_of_nodes) { }
		const NodeResult &operator()(unsigned int id) const { return node_results_[id]; }
		NodeResult &operator[](unsigned int id) { return node_results_[id]; }
	private:
		std::vector<NodeResult> node_results_;
};

} //namespace yafaray

#endif //LIBYAFARAY_NODE_TREE_DATA_H
