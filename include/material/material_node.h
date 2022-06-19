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

#ifndef YAFARAY_MATERIAL_NODE_H
#define YAFARAY_MATERIAL_NODE_H

#include "material/material.h"
#include <map>
#include <vector>

namespace yafaray {

class Scene;
class ShaderNode;
class NodeTreeData;

class NodeMaterial: public Material
{
	public:
		explicit NodeMaterial(Logger &logger) : Material(logger) { }

	protected:
		/** parse node shaders to fill nodeList */
		static void parseNodes(const ParamMap &params, std::vector<const ShaderNode *> &root_nodes_list, std::map<std::string, const ShaderNode *> &root_nodes_map, const std::map<std::string, std::unique_ptr<ShaderNode>> &shaders_table, Logger &logger);
		/* put nodes in evaluation order in "allSorted" given all root nodes;
		   sets reqNodeMem to the amount of memory the node node_tree_data requires for evaluation of all nodes */
		void evalBump(NodeTreeData &node_tree_data, SurfacePoint &sp, const ShaderNode *bump_shader_node, const Camera *camera) const;
		size_t sizeNodesBytes() const { return (color_nodes_.size() + bump_nodes_.size()) * sizeof(NodeResult); }
		static void evalNodes(const SurfacePoint &sp, const std::vector<const ShaderNode *> &nodes, NodeTreeData &node_tree_data, const Camera *camera);
		static std::vector<const ShaderNode *> recursiveSolver(const ShaderNode *node);
		static std::set<const ShaderNode *> recursiveFinder(const ShaderNode *node);
		static std::vector<const ShaderNode *> solveNodesOrder(const std::vector<const ShaderNode *> &roots, const std::map<std::string, std::unique_ptr<ShaderNode>> &shaders_table, Logger &logger);
		static std::vector<const ShaderNode *> getNodeList(const ShaderNode *root, const std::vector<const ShaderNode *> &nodes_sorted);
		/*! load nodes from parameter map list */
		static std::map<std::string, std::unique_ptr<ShaderNode>> loadNodes(const std::list<ParamMap> &params_list, const Scene &scene, Logger &logger);

		std::map<std::string, std::unique_ptr<ShaderNode>> nodes_map_;
		std::vector<const ShaderNode *> color_nodes_, bump_nodes_;
};

} //namespace yafaray

#endif // YAFARAY_MATERIAL_NODE_H
