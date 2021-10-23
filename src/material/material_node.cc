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

#include "material/material_node.h"
#include "common/logger.h"
#include "common/param.h"
#include "shader/shader_node.h"

BEGIN_YAFARAY

void NodeMaterial::recursiveSolver(ShaderNode *node, std::vector<ShaderNode *> &sorted)
{
	if(node->getId() != 0) return;
	node->setId(1);
	std::vector<const ShaderNode *> dependency_nodes;
	if(node->getDependencies(dependency_nodes))
	{
		for(const auto &dependency_node : dependency_nodes)
			// FIXME someone tell me a smarter way than casting away a const...
			if(dependency_node->getId() == 0) recursiveSolver((ShaderNode *) dependency_node, sorted);
	}
	sorted.push_back(node);
}

void NodeMaterial::recursiveFinder(const ShaderNode *node, std::set<const ShaderNode *> &tree)
{
	std::vector<const ShaderNode *> dependency_nodes;
	if(node->getDependencies(dependency_nodes))
	{
		for(const auto &dependency_node : dependency_nodes)
		{
			tree.insert(dependency_node);
			recursiveFinder(dependency_node, tree);
		}
	}
	tree.insert(node);
}

void NodeMaterial::evalNodes(const RenderData &render_data, const SurfacePoint &sp, const std::vector<ShaderNode *> &nodes, NodeStack &stack) const {
	for(const auto &node : nodes) node->eval(stack, render_data, sp);
}

void NodeMaterial::solveNodesOrder(const std::vector<ShaderNode *> &roots)
{
	//set all IDs = 0 to indicate "not tested yet"
	for(unsigned int i = 0; i < color_nodes_.size(); ++i) color_nodes_[i]->setId(0);
	for(unsigned int i = 0; i < roots.size(); ++i) recursiveSolver(roots[i], color_nodes_sorted_);
	if(color_nodes_.size() != color_nodes_sorted_.size()) logger_.logWarning("NodeMaterial: Unreachable nodes!");
	//give the nodes an index to be used as the "stack"-index.
	//using the order of evaluation can't hurt, can it?
	for(unsigned int i = 0; i < color_nodes_sorted_.size(); ++i)
	{
		ShaderNode *n = color_nodes_sorted_[i];
		n->setId(i);
	}
	req_node_mem_ = color_nodes_sorted_.size() * sizeof(NodeResult);
}

/*! get a list of all nodes that are in the tree given by root
	prerequisite: nodes have been successfully loaded and stored into allSorted
	since "solveNodesOrder" sorts allNodes, calling getNodeList afterwards gives
	a list in evaluation order. multiple calls are merged in "nodes" */

void NodeMaterial::getNodeList(const ShaderNode *root, std::vector<ShaderNode *> &nodes)
{
	std::set<const ShaderNode *> in_tree;
	for(const auto &node : nodes) in_tree.insert(node);
	recursiveFinder(root, in_tree);
	nodes.clear();
	for(const auto &node : color_nodes_sorted_) if(in_tree.find(node) != in_tree.end()) nodes.push_back(node);
}

void NodeMaterial::evalBump(NodeStack &stack, const RenderData &render_data, SurfacePoint &sp, const ShaderNode *bump_shader_node) const
{
	for(const auto &node : bump_nodes_) node->evalDerivative(stack, render_data, sp);
	float du, dv;
	bump_shader_node->getDerivative(stack, du, dv);
	applyBump(sp, du, dv);
}

bool NodeMaterial::loadNodes(const std::list<ParamMap> &params_list, const Scene &scene)
{
	bool error = false;

	for(const auto &param_map : params_list)
	{
		std::string element;
		if(param_map.getParam("element", element))
		{
			if(element != "shader_node") continue;
		}
		else logger_.logWarning("NodeMaterial: No element type given; assuming shader node");

		std::string name;
		if(!param_map.getParam("name", name))
		{
			logger_.logError("NodeMaterial: Name of shader node not specified!");
			error = true;
			break;
		}

		if(shaders_table_.find(name) != shaders_table_.end())
		{
			logger_.logError("NodeMaterial: Multiple nodes with identically names!");
			error = true;
			break;
		}

		std::string type;
		if(!param_map.getParam("type", type))
		{
			logger_.logError("NodeMaterial: Type of shader node not specified!");
			error = true;
			break;
		}

		std::unique_ptr<ShaderNode> shader = ShaderNode::factory(logger_, param_map, scene);
		if(shader)
		{
			shaders_table_[name] = std::move(shader);
			color_nodes_.push_back(shaders_table_[name].get());
			if(logger_.isVerbose()) logger_.logVerbose("NodeMaterial: Added ShaderNode '", name, "'! (", (void *)shaders_table_[name].get(), ")");
		}
		else
		{
			logger_.logError("NodeMaterial: No shader node could be constructed.'", type, "'!");
			color_nodes_.clear(); //Empty the nodes table, to prevent further crashes later in rendering, when any of the nodes cannot be created
			error = true;
			break;
		}
	}

	if(!error) //configure node inputs
	{
		NodeFinder finder(shaders_table_);
		int n = 0;
		for(const auto &param_map : params_list)
		{
			if(!color_nodes_[n]->configInputs(logger_, param_map, finder))
			{
				logger_.logError("NodeMaterial: Shader node configuration failed! (n=", n, ")");
				error = true; break;
			}
			++n;
		}
	}
	if(error) shaders_table_.clear();
	return !error;
}

void NodeMaterial::parseNodes(const ParamMap &params, std::vector<ShaderNode *> &roots, std::map<std::string, ShaderNode *> &node_list)
{
	for(auto &current_node : node_list)
	{
		std::string name;
		if(params.getParam(current_node.first, name))
		{
			const auto node_found = shaders_table_.find(name);
			if(node_found != shaders_table_.end())
			{
				current_node.second = node_found->second.get();
				roots.push_back(current_node.second);
			}
			else logger_.logWarning("Shader node ", current_node.first, " '", name, "' does not exist!");
		}
	}
}

END_YAFARAY
