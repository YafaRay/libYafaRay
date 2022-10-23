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

ValueNode::Params::Params(ParamResult &param_result, const ParamMap &param_map)
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
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<ShaderNode>, ParamResult> ValueNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto shader_node {std::make_unique<ThisClassType_t>(logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(shader_node), param_result};
}

ValueNode::ValueNode(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void ValueNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	node_tree_data[getId()] = { color_, params_.value_ };
}

} //namespace yafaray
