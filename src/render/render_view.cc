/****************************************************************************
 *
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
 *
 */

#include "render/render_view.h"
#include "param/param.h"
#include "scene/scene.h"
#include "common/string.h"
#include "common/logger.h"
#include "light/light.h"

namespace yafaray {

RenderView::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(camera_name_);
	PARAM_LOAD(light_names_);
	PARAM_LOAD(wavelength_);
}

ParamMap RenderView::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(camera_name_);
	PARAM_SAVE(light_names_);
	PARAM_SAVE(wavelength_);
	PARAM_SAVE_END;
}

ParamMap RenderView::getAsParamMap(bool only_non_default) const
{
	return params_.getAsParamMap(only_non_default);
}

std::pair<std::unique_ptr<RenderView>, ParamError> RenderView::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + "::factory 'raw' ParamMap\n" + param_map.logContents());
	auto param_error{Params::meta_.check(param_map, {}, {})};
	auto render_view {std::make_unique<RenderView>(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<RenderView>(name, {}));
	return {std::move(render_view), param_error};
}

RenderView::RenderView(Logger &logger, ParamError &param_error, const ParamMap &param_map) : params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

bool RenderView::init(Logger &logger, const Scene &scene)
{
	camera_ = scene.getCamera(params_.camera_name_);
	if(!camera_)
	{
		logger.logError(getClassName() ,"'", name_, "': Camera not found in the scene.");
		return false;
	}

	lights_.clear();
	const std::vector<std::string> selected_lights_names = string::tokenize(params_.light_names_, ";");

	std::map<std::string, const Light *> scene_lights{scene.getLights()};
	if(selected_lights_names.empty())
	{
		lights_ = scene_lights;
	}
	else
	{
		for(const auto &light_name : selected_lights_names)
		{
			const Light *light = findPointerInMap(scene_lights, light_name);
			if(!light)
			{
				logger.logWarning(getClassName() ,"'", name_, "' init: view '", name_, "' could not find light '", light_name, "', skipping...");
				continue;
			}
			lights_[light_name] = light;
		}
	}
	if(lights_.empty())
	{
		logger.logWarning(getClassName() ,"'", name_, "': Lights not found in the scene.");
	}
	return true;
}

std::vector<const Light *> RenderView::getLightsVisible() const
{
	std::vector<const Light *> result;
	for(const auto &[light_name, light] : lights_)
	{
		if(light->lightEnabled() && !light->photonOnly()) result.emplace_back(light);
	}
	return result;
}

std::vector<const Light *> RenderView::getLightsEmittingCausticPhotons() const
{
	std::vector<const Light *> result;
	for(const auto &[light_name, light] : lights_)
	{
		if(light->lightEnabled() && light->shootsCausticP()) result.emplace_back(light);
	}
	return result;
}

std::vector<const Light *> RenderView::getLightsEmittingDiffusePhotons() const
{
	std::vector<const Light *> result;
	for(const auto &[light_name, light] : lights_)
	{
		if(light->lightEnabled() && light->shootsDiffuseP()) result.emplace_back(light);
	}
	return result;
}

} //namespace yafaray