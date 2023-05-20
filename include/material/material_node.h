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

#ifndef LIBYAFARAY_MATERIAL_NODE_H
#define LIBYAFARAY_MATERIAL_NODE_H

#include "material/material.h"
#include <map>
#include <vector>
#include <set>

namespace yafaray {

class Scene;
class ShaderNode;
class NodeTreeData;

class NodeMaterial: public Material
{
	public:
		inline static std::string getClassName() { return "NodeMaterial"; }
		explicit NodeMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) : Material{logger, param_result, param_map, materials} { }

	protected:
		[[nodiscard]] std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const override;
		/** parse node shaders to fill nodeList */
		static void parseNodes(Logger &logger, std::vector<const ShaderNode *> &root_nodes_list, std::map<std::string, const ShaderNode *> &root_nodes_map, const std::map<std::string, std::unique_ptr<ShaderNode>> &shaders_table, const ParamMap &param_map);
		/* put nodes in evaluation order in "allSorted" given all root nodes;
		   sets reqNodeMem to the amount of memory the node node_tree_data requires for evaluation of all nodes */
		void evalBump(NodeTreeData &node_tree_data, SurfacePoint &sp, const ShaderNode *bump_shader_node, const Camera *camera) const;
		static void evalNodes(const SurfacePoint &sp, const std::vector<const ShaderNode *> &nodes, NodeTreeData &node_tree_data, const Camera *camera);
		static std::vector<const ShaderNode *> recursiveSolver(const ShaderNode *node);
		static std::set<const ShaderNode *> recursiveFinder(const ShaderNode *node);
		static std::vector<const ShaderNode *> solveNodesOrder(const std::vector<const ShaderNode *> &roots, const std::map<std::string, std::unique_ptr<ShaderNode>> &shaders_table, Logger &logger);
		static std::vector<const ShaderNode *> getNodeList(const ShaderNode *root, const std::vector<const ShaderNode *> &nodes_sorted);
		/*! load nodes from parameter map list */
		static std::map<std::string, std::unique_ptr<ShaderNode>> loadNodes(const std::list<ParamMap> &params_list, const Scene &scene, Logger &logger);
		template <typename ShaderNodeType> static std::array<std::unique_ptr<ParamMeta>, ShaderNodeType::Size> initShaderNames(std::map<std::string, const ParamMeta *> &map);
		template <typename NodeMaterialClass, typename ShaderNodeType> static ParamMap getShaderNodesNames(const std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> &shader_nodes, bool only_non_default);
		template <typename ParamsClass, typename ShaderNodeType> static std::map<std::string, const ParamMeta *> shadersMeta();
		template <typename ParamsClass, typename ShaderNodeType> static ParamResult checkShadersParams(const ParamMap &param_map);

		std::map<std::string, std::unique_ptr<ShaderNode>> nodes_map_;
		std::vector<const ShaderNode *> color_nodes_, bump_nodes_;
};

template <typename ShaderNodeType>
inline std::array<std::unique_ptr<ParamMeta>, ShaderNodeType::Size> NodeMaterial::initShaderNames(std::map<std::string, const ParamMeta *> &map)
{
	std::array<std::unique_ptr<ParamMeta>, ShaderNodeType::Size> result;
	for(size_t index = 0; index < ShaderNodeType::Size; ++index)
	{
		ShaderNodeType shader_node_type{static_cast<unsigned char>(index)};
		const std::string shader_node_type_name{shader_node_type.print()};
		result[index] = std::make_unique<ParamMeta>(shader_node_type_name, shader_node_type.printDescription(), std::string{""}, map);
	}
	return result;
}

template <typename NodeMaterialClass, typename ShaderNodeType>
inline ParamMap NodeMaterial::getShaderNodesNames(const std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> &shader_nodes, bool only_non_default)
{
	ParamMap param_map;
	for(size_t index = 0; index < shader_nodes.size(); ++index)
	{
		const std::string shader_node_type_name{ShaderNodeType{index}.print()};
		if(shader_nodes[index])
		{
			param_map.setParam(shader_node_type_name, shader_nodes[index]->getName());
		}
		else if(!only_non_default)
		{
			param_map.setParam(shader_node_type_name, std::string{});
		}
	}
	return param_map;
}

template <typename ParamsClass, typename ShaderNodeType>
inline std::map<std::string, const ParamMeta *> NodeMaterial::shadersMeta()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	for(size_t index = 0; index < ParamsClass::shader_node_names_meta_.size(); ++index)
	{
		const std::string shader_node_type_name{ShaderNodeType{index}.print()};
		param_meta_map[shader_node_type_name] = &ParamsClass::shader_node_names_meta_[index];
	}
	return param_meta_map;
}

template <typename ParamsClass, typename ShaderNodeType>
inline ParamResult NodeMaterial::checkShadersParams(const ParamMap &param_map)
{
	ParamResult param_result;
	const auto shaders_meta_map{shadersMeta<ParamsClass, ShaderNodeType>()};
	for(const auto &[shader_meta_name, shader_meta] : shaders_meta_map)
	{
		std::string shader_name;
		if(param_map.getParam(shader_meta_name, shader_name) == YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE)
		{
			param_result.flags_ |= ResultFlags{YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE};
			param_result.wrong_type_params_.emplace_back(shader_meta_name);
		};
	}
	return param_result;
}

} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_NODE_H
