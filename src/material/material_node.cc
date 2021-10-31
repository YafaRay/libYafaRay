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

std::vector<const ShaderNode *> NodeMaterial::recursiveSolver(const ShaderNode *node)
{
	if(node->getId() != 0) return {};
	node->setId(1);
	std::vector<const ShaderNode *> sorted_nodes;
	const std::vector<const ShaderNode *> dependencies = node->getDependencies();
	for(const auto &dependency_node : dependencies)
	{
		const std::vector<const ShaderNode *> dependencies_sorted = recursiveSolver(dependency_node);
		sorted_nodes.insert(sorted_nodes.end(), dependencies_sorted.begin(), dependencies_sorted.end());
	}
	sorted_nodes.push_back(node);
	return sorted_nodes;
}

std::set<const ShaderNode *> NodeMaterial::recursiveFinder(const ShaderNode *node)
{
	std::set<const ShaderNode *> tree;
	const std::vector<const ShaderNode *> dependency_nodes = node->getDependencies();
	for(const auto &dependency_node : dependency_nodes)
	{
		tree.insert(dependency_node);
		const std::set<const ShaderNode *> dependency_tree = recursiveFinder(dependency_node);
		tree.insert(dependency_tree.begin(), dependency_tree.end());
	}
	tree.insert(node);
	return tree;
}

void NodeMaterial::evalNodes(const SurfacePoint &sp, const std::vector<const ShaderNode *> &nodes, NodeStack *stack, const Camera *camera) const
{
	for(const auto &node : nodes) node->eval(stack, sp, camera);
}

std::vector<const ShaderNode *> NodeMaterial::solveNodesOrder(const std::vector<const ShaderNode *> &roots, const std::map<std::string, std::unique_ptr<ShaderNode>> &shaders_table, Logger &logger)
{
	for(const auto &shader : shaders_table) shader.second->setId(0); //set all IDs = 0 to indicate "not tested yet"
	std::vector<const ShaderNode *> color_nodes_sorted;
	for(const auto &root : roots)
	{
		const std::vector<const ShaderNode *> root_nodes = recursiveSolver(root);
		color_nodes_sorted.insert(color_nodes_sorted.end(), root_nodes.begin(), root_nodes.end());
	}
	if(shaders_table.size() != color_nodes_sorted.size())
	{
		logger.logWarning("NodeMaterial: Unreachable nodes!");
	}
	//give the nodes an index to be used as the "stack"-index.
	//using the order of evaluation can't hurt, can it?
	for(unsigned int i = 0; i < color_nodes_sorted.size(); ++i) color_nodes_sorted[i]->setId(i);
	return color_nodes_sorted;
}

/*! get a list of all nodes that are in the tree given by root
	prerequisite: nodes have been successfully loaded and stored into allSorted
	since "solveNodesOrder" sorts allNodes, calling getNodeList afterwards gives
	a list in evaluation order. multiple calls are merged in "nodes" */

std::vector<const ShaderNode *> NodeMaterial::getNodeList(const ShaderNode *root, const std::vector<const ShaderNode *> &nodes_sorted)
{
	std::set<const ShaderNode *> in_tree = recursiveFinder(root);
	std::vector<const ShaderNode *> nodes;
	for(const auto &node : nodes_sorted)
	{
		if(in_tree.find(node) != in_tree.end()) nodes.push_back(node);
	}
	return nodes;
}

void NodeMaterial::evalBump(NodeStack *stack, SurfacePoint &sp, const ShaderNode *bump_shader_node, const Camera *camera) const
{
	for(const auto &node : bump_nodes_) node->evalDerivative(stack, sp, camera);
	const DuDv du_dv = bump_shader_node->getDuDv(stack);
	applyBump(sp, du_dv);
}

std::map<std::string, std::unique_ptr<ShaderNode>> NodeMaterial::loadNodes(const std::list<ParamMap> &params_list, const Scene &scene, Logger &logger)
{
	std::map<std::string, std::unique_ptr<ShaderNode>> shaders_table;
	bool error = false;

	for(const auto &param_map : params_list)
	{
		std::string element;
		if(param_map.getParam("element", element))
		{
			if(element != "shader_node") continue;
		}
		else logger.logWarning("NodeMaterial: No element type given; assuming shader node");

		std::string name;
		if(!param_map.getParam("name", name))
		{
			logger.logError("NodeMaterial: Name of shader node not specified!");
			error = true;
			break;
		}

		if(shaders_table.find(name) != shaders_table.end())
		{
			logger.logError("NodeMaterial: Multiple nodes with identically names!");
			error = true;
			break;
		}

		std::string type;
		if(!param_map.getParam("type", type))
		{
			logger.logError("NodeMaterial: Type of shader node not specified!");
			error = true;
			break;
		}

		std::unique_ptr<ShaderNode> shader = ShaderNode::factory(logger, param_map, scene);
		if(shader)
		{
			shaders_table[name] = std::move(shader);
			if(logger.isVerbose()) logger.logVerbose("NodeMaterial: Added ShaderNode '", name, "'! (", (void *)shaders_table[name].get(), ")");
		}
		else
		{
			logger.logError("NodeMaterial: No shader node could be constructed.'", type, "'!");
			error = true;
			break;
		}
	}

	if(!error) //configure node inputs
	{
		NodeFinder finder(shaders_table);
		for(const auto &param_map : params_list)
		{
			std::string name;
			param_map.getParam("name", name);
			if(!shaders_table[name]->configInputs(logger, param_map, finder))
			{
				logger.logError("NodeMaterial: Shader node configuration failed! (name='", name, "')");
				error = true; break;
			}
		}
	}
	if(error) shaders_table.clear();
	return shaders_table;
}

void NodeMaterial::parseNodes(const ParamMap &params, std::vector<const ShaderNode *> &root_nodes_list, std::map<std::string, const ShaderNode *> &root_nodes_map, const std::map<std::string, std::unique_ptr<ShaderNode>> &shaders_table, Logger &logger)
{
	for(auto &node : root_nodes_map)
	{
		std::string name;
		if(params.getParam(node.first, name))
		{
			const auto node_found = shaders_table.find(name);
			if(node_found != shaders_table.end())
			{
				node.second = node_found->second.get();
				root_nodes_list.push_back(node.second);
			}
			else logger.logWarning("Shader node ", node.first, " '", name, "' does not exist!");
		}
	}
}

END_YAFARAY
