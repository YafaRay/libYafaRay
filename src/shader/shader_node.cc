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

ShaderNode::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(name_);
	PARAM_LOAD(element_);
}

ParamMap ShaderNode::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(name_);
	PARAM_SAVE(element_);
	PARAM_SAVE_END;
}

ParamMap ShaderNode::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<std::unique_ptr<ShaderNode>, ParamError> ShaderNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Texture: return TextureMapperNode::factory(logger, scene, name, param_map);
		case Type::Value: return ValueNode::factory(logger, scene, name, param_map);
		case Type::Mix: return MixNode::factory(logger, scene, name, param_map);
		case Type::Layer: return LayerNode::factory(logger, scene, name, param_map);
		default: return {nullptr, ParamError{ResultFlags::ErrorWhileCreating}};
	}
}

ShaderNode::ShaderNode(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

} //namespace yafaray
