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

#include "shader/shader_node_value.h"
#include "camera/camera.h"
#include "param/param.h"
#include "geometry/surface.h"
#include "math/interpolation.h"

namespace yafaray {

/* ==========================================
/  The most simple node you could imagine...
/  ========================================== */

ValueNode::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(color_);
	PARAM_LOAD(value_);
	PARAM_LOAD(alpha_);
}

ParamMap ValueNode::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(color_);
	PARAM_SAVE(value_);
	PARAM_SAVE(alpha_);
	PARAM_SAVE_END;
}

ParamMap ValueNode::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ShaderNode::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<ShaderNode *, ParamError> ValueNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new ValueNode(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<ValueNode>(name, {"type"}));
	return {result, param_error};
}

ValueNode::ValueNode(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		ShaderNode{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void ValueNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	node_tree_data[getId()] = { color_, params_.value_ };
}

} //namespace yafaray
