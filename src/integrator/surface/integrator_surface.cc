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

#include "integrator/surface/integrator_surface.h"
#include "integrator/surface/integrator_bidirectional.h"
#include "integrator/surface/integrator_direct_light.h"
#include "integrator/surface/integrator_path_tracer.h"
#include "integrator/surface/integrator_photon_mapping.h"
#include "integrator/surface/integrator_sppm.h"
#include "integrator/surface/integrator_debug.h"
#include "param/param.h"
#include "render/imagesplitter.h"
#include "scene/scene.h"
#include "render/imagefilm.h"
#include "light/light.h"
#include "common/string.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> SurfaceIntegrator::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(light_names_);
	PARAM_META(time_forced_);
	PARAM_META(time_forced_value_);
	return param_meta_map;
}

SurfaceIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(light_names_);
	PARAM_LOAD(time_forced_);
	PARAM_LOAD(time_forced_value_);
}

ParamMap SurfaceIntegrator::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(light_names_);
	PARAM_SAVE(time_forced_);
	PARAM_SAVE(time_forced_value_);
	return param_map;
}

std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> SurfaceIntegrator::factory(Logger &logger, RenderControl &render_control, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Bidirectional:
		{
			logger.logWarning("The Bidirectional integrator is UNSTABLE at the moment and needs to be improved. It might give unexpected and perhaps even incorrect render results. Use at your own risk.");
			return BidirectionalIntegrator::factory(logger, render_control, name, param_map);
		}
		case Type::Debug: return DebugIntegrator::factory(logger, render_control, name, param_map);
		case Type::DirectLight: return DirectLightIntegrator::factory(logger, render_control, name, param_map);
		case Type::Path: return PathIntegrator::factory(logger, render_control, name, param_map);
		case Type::Photon: return PhotonIntegrator::factory(logger, render_control, name, param_map);
		case Type::Sppm: return SppmIntegrator::factory(logger, render_control, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

bool SurfaceIntegrator::preprocess(FastRandom &fast_random, ImageFilm *image_film, const Scene &scene, const Renderer &renderer)
{
	accelerator_ = scene.getAccelerator();
	if(!accelerator_) return false;
	shadow_bias_ = renderer.getShadowBias();
	ray_min_dist_ = renderer.getRayMinDist();
	num_threads_ = renderer.getNumThreads();
	num_threads_photons_ = renderer.getNumThreadsPhotons();
	shadow_bias_auto_ = renderer.isShadowBiasAuto();
	ray_min_dist_auto_ = renderer.isRayMinDistAuto();
	layers_ = image_film->getLayers();
	image_film_ = image_film;
	camera_ = image_film->getCamera();
	background_ = scene.getBackground();
	aa_noise_params_ = image_film->getAaParameters();
	edge_toon_params_ = image_film->getEdgeToonParams();
	mask_params_ = image_film->getMaskParams();
	timer_ = image_film->getTimer();
	vol_integrator_ = renderer.getVolIntegrator();
	scene_bound_ = scene.getSceneBound();
	lights_map_filtered_ = getFilteredLights(scene, params_.light_names_);
	lights_visible_ = getLightsVisible();
	return static_cast<bool>(layers_) && static_cast<bool>(camera_) && static_cast<bool>(timer_);
}

std::map<std::string, const Light *> SurfaceIntegrator::getFilteredLights(const Scene &scene, const std::string &light_filter_string) const
{
	std::map<std::string, const Light *> filtered_lights;
	const std::vector<std::string> selected_lights_names = string::tokenize(light_filter_string, ";");

	if(selected_lights_names.empty())
	{
		const auto &lights{scene.getLights()};
		for(const auto &[light, light_name, light_enabled] : lights)
		{
			if(light && light_enabled) filtered_lights[light_name] = light.get();
		}
	}
	else
	{
		for(const auto &light_name : selected_lights_names)
		{
			const auto &[light, light_id, light_result]{scene.getLight(light_name)};
			if(light) filtered_lights[light_name] = light;
			else
			{
				logger_.logWarning(getClassName() ,"'", getName(), "' init: could not find light '", light_name, "', skipping...");
				continue;
			}
		}
	}
	if(filtered_lights.empty())
	{
		logger_.logWarning(getClassName() ,"'", getName(), "': Lights not found in the scene.");
	}
	return filtered_lights;
}

std::vector<const Light *> SurfaceIntegrator::getLightsVisible() const
{
	std::vector<const Light *> result;
	for(const auto &[light_name, light] : lights_map_filtered_)
	{
		if(light->lightEnabled() && !light->photonOnly()) result.emplace_back(light);
	}
	return result;
}

std::vector<const Light *> SurfaceIntegrator::getLightsEmittingCausticPhotons() const
{
	std::vector<const Light *> result;
	for(const auto &[light_name, light] : lights_map_filtered_)
	{
		if(light->lightEnabled() && light->shootsCausticP()) result.emplace_back(light);
	}
	return result;
}

std::vector<const Light *> SurfaceIntegrator::getLightsEmittingDiffusePhotons() const
{
	std::vector<const Light *> result;
	for(const auto &[light_name, light] : lights_map_filtered_)
	{
		if(light->lightEnabled() && light->shootsDiffuseP()) result.emplace_back(light);
	}
	return result;
}

} //namespace yafaray
