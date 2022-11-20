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

RenderView::Params::Params(ParamResult &param_result, const ParamMap &param_map)
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

std::pair<std::unique_ptr<RenderView>, ParamResult> RenderView::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + "::factory 'raw' ParamMap\n" + param_map.logContents());
	auto param_result{Params::meta_.check(param_map, {}, {})};
	auto render_view {std::make_unique<RenderView>(logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<RenderView>(name, {}));
	return {std::move(render_view), param_result};
}

RenderView::RenderView(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

bool RenderView::init(Logger &logger, const Scene &scene)
{
	camera_ = std::get<0>(scene.getCamera(params_.camera_name_));
	if(!camera_)
	{
		logger.logError(getClassName() ,"'", name_, "': Camera '", params_.camera_name_, "' not found in the scene.");
		return false;
	}

	lights_.clear();
	const std::vector<std::string> selected_lights_names = string::tokenize(params_.light_names_, ";");

	const auto &scene_lights{scene.getLights()};
	if(selected_lights_names.empty())
	{
		for(const auto &[light, light_name, light_enabled] : scene_lights)
		{
			if(light && light_enabled) lights_[light_name] = light.get();
		}
	}
	else
	{
		for(const auto &light_name : selected_lights_names)
		{
			const auto &[light, light_id, light_result]{scene.getLight(light_name)};
			if(light) lights_[light_name] = light;
			else
			{
				logger.logWarning(getClassName() ,"'", name_, "' init: could not find light '", light_name, "', skipping...");
				continue;
			}
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

RenderView::Type RenderView::type()
{
	return Type::RenderView;
}

} //namespace yafaray