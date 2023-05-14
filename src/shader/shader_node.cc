/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "shader/shader_node.h"
#include "shader/shader_node_texture.h"
#include "shader/shader_node_layer.h"
#include "shader/shader_node_mix.h"
#include "shader/shader_node_value.h"
#include "param/param.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> ShaderNode::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(name_);
	return param_meta_map;
}

ShaderNode::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(name_);
}

ParamMap ShaderNode::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(name_);
	return param_map;
}

std::pair<std::unique_ptr<ShaderNode>, ParamResult> ShaderNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Texture: return TextureMapperNode::factory(logger, scene, name, param_map);
		case Type::Value: return ValueNode::factory(logger, scene, name, param_map);
		case Type::Mix: return MixNode::factory(logger, scene, name, param_map);
		case Type::Layer: return LayerNode::factory(logger, scene, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

ShaderNode::ShaderNode(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}
std::string ShaderNode::exportToString(yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	ss << "\t\t\t<shader_node>" << std::endl;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << param_map.exportMap(4, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << "\t\t\t</shader_node>" << std::endl;
	return ss.str();
}

} //namespace yafaray
