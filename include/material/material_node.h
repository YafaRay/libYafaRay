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
#include "common/memory.h"
#include <map>
#include <vector>

BEGIN_YAFARAY

class Scene;
class ShaderNode;
class NodeStack;

class NodeMaterial: public Material
{
	public:
		NodeMaterial() = default;
		virtual ~NodeMaterial() override;

	protected:
		/*! load nodes from parameter map list */
		bool loadNodes(const std::list<ParamMap> &params_list, const Scene &scene);
		/** parse node shaders to fill nodeList */
		void parseNodes(const ParamMap &params, std::vector<ShaderNode *> &roots, std::map<std::string, ShaderNode *> &node_list);
		/* put nodes in evaluation order in "allSorted" given all root nodes;
		   sets reqNodeMem to the amount of memory the node stack requires for evaluation of all nodes */
		void solveNodesOrder(const std::vector<ShaderNode *> &roots);
		void getNodeList(const ShaderNode *root, std::vector<ShaderNode *> &nodes);
		void evalNodes(const RenderData &render_data, const SurfacePoint &sp, const std::vector<ShaderNode *> &nodes, NodeStack &stack) const;
		void evalBump(NodeStack &stack, const RenderData &render_data, SurfacePoint &sp, const ShaderNode *bump_shader_node) const;

		std::vector<ShaderNode *> color_nodes_, color_nodes_sorted_, bump_nodes_;
		std::map<std::string, std::unique_ptr<ShaderNode>> shaders_table_;
		size_t req_node_mem_ = 0;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_NODE_H
