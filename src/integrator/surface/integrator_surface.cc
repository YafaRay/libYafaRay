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
#include "integrator/volume/integrator_volume.h"
#include "param/param.h"
#include "render/imagesplitter.h"
#include "scene/scene.h"
#include "render/imagefilm.h"
#include "light/light.h"
#include "common/string.h"
#include "common/sysinfo.h"
#include "render/progress_bar.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> SurfaceIntegrator::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(light_names_);
	PARAM_META(time_forced_);
	PARAM_META(time_forced_value_);
	PARAM_META(nthreads_);
	PARAM_META(shadow_bias_auto_);
	PARAM_META(shadow_bias_);
	PARAM_META(ray_min_dist_auto_);
	PARAM_META(ray_min_dist_);
	return param_meta_map;
}

SurfaceIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(light_names_);
	PARAM_LOAD(time_forced_);
	PARAM_LOAD(time_forced_value_);
	PARAM_LOAD(nthreads_);
	PARAM_LOAD(shadow_bias_auto_);
	PARAM_LOAD(shadow_bias_);
	PARAM_LOAD(ray_min_dist_auto_);
	PARAM_LOAD(ray_min_dist_);
}

ParamMap SurfaceIntegrator::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(light_names_);
	PARAM_SAVE(time_forced_);
	PARAM_SAVE(time_forced_value_);
	PARAM_SAVE(nthreads_);
	PARAM_SAVE(shadow_bias_auto_);
	PARAM_SAVE(shadow_bias_);
	PARAM_SAVE(ray_min_dist_auto_);
	PARAM_SAVE(ray_min_dist_);
	return param_map;
}

std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> SurfaceIntegrator::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Bidirectional:
		{
			logger.logWarning("The Bidirectional integrator is UNSTABLE at the moment and needs to be improved. It might give unexpected and perhaps even incorrect render results. Use at your own risk.");
			return BidirectionalIntegrator::factory(logger, name, param_map);
		}
		case Type::Debug: return DebugIntegrator::factory(logger, name, param_map);
		case Type::DirectLight: return DirectLightIntegrator::factory(logger, name, param_map);
		case Type::Path: return PathIntegrator::factory(logger, name, param_map);
		case Type::Photon: return PhotonIntegrator::factory(logger, name, param_map);
		case Type::Sppm: return SppmIntegrator::factory(logger, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

SurfaceIntegrator::SurfaceIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : name_{name}, logger_{logger}, params_{param_result, param_map}
{
	//Empty
}

SurfaceIntegrator::~SurfaceIntegrator() = default;

bool SurfaceIntegrator::preprocess(RenderMonitor &render_monitor, const RenderControl &render_control, const Scene &scene)
{
	if(render_control.canceled() || render_control.finished()) return false;
	bool result{true};
	accelerator_ = scene.getAccelerator();
	if(!accelerator_) result = false;
	background_ = scene.getBackground();
	scene_bound_ = scene.getSceneBound();
	object_index_highest_ = scene.getObjectIndexHighest();
	material_index_highest_ = scene.getMaterialIndexHighest();
	lights_map_filtered_ = getFilteredLights(scene, params_.light_names_);
	lights_visible_ = getLightsVisible();
	ray_differentials_enabled_ = scene.mipMapInterpolationRequired();
	if(vol_integrator_) result = result && vol_integrator_->preprocess(scene, *this);
	return result;
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
				logger_.logWarning(getClassName() ," '", getName(), "' init: could not find light '", light_name, "', skipping...");
				continue;
			}
		}
	}
	if(filtered_lights.empty())
	{
		logger_.logWarning(getClassName() ," '", getName(), "': Lights not found in the scene.");
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

ParamResult SurfaceIntegrator::defineVolumeIntegrator(const Scene &scene, const ParamMap &param_map)
{
	if(param_map.empty())
	{
		vol_integrator_ = nullptr;
		logger_.logVerbose(getClassName() + " '", getName(), "': Removed volume integrator, param map was empty.");
		return {YAFARAY_RESULT_WARNING_PARAM_NOT_SET};
	}
	auto [volume_integrator, volume_integrator_result]{VolumeIntegrator::factory(logger_, scene.getVolumeRegions(), param_map)};
	if(logger_.isVerbose() && volume_integrator)
	{
		logger_.logVerbose(getClassName() + " '", getName(), "': Added ", volume_integrator->getClassName(), "' (", volume_integrator->type().print(), ")");
	}
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	vol_integrator_ = std::move(volume_integrator);
	return volume_integrator_result;
}

int SurfaceIntegrator::setNumThreads(int threads)
{
	int result = threads;

	if(threads == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads: Active.");
		result = sysinfo::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported: [", result, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads: Inactive.");
	}

	logger_.logParams("Renderer '", getName(), "' using [", result, "] Threads.");

	std::stringstream set;
	set << "CPU threads=" << result << std::endl;
	//render_control_.setRenderInfo(set.str()); //FIXME Render stats
	return result;
}

bool SurfaceIntegrator::render(RenderControl &render_control, RenderMonitor &render_monitor, ImageFilm *image_film)
{
	if(!image_film || render_control.canceled() || render_control.finished()) return false;
	image_film_ = image_film;
	aa_noise_params_ = image_film_->getAaParameters();
	const bool success = render(render_control, render_monitor);
	if(!success)
	{
		logger_.logError(getClassName(), " '", getName(), "': Rendering process failed, exiting...");
		return false;
	}
	image_film_->flush(render_control, render_monitor, ImageFilm::All);
	render_control.setFinished();
	image_film_ = nullptr;
	return true;
}

std::string SurfaceIntegrator::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<surface_integrator>" << std::endl;
	ss << std::string(indent_level + 1, '\t') << "<parameters name=\"" << getName() << "\">" << std::endl;
	ss << param_map.exportMap(indent_level + 2, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level + 1, '\t') << "</parameters>" << std::endl;
	if(vol_integrator_) ss << vol_integrator_->exportToString(indent_level + 1, container_export_type, only_export_non_default_parameters);
	ss << std::string(indent_level, '\t') << "</surface_integrator>" << std::endl;
	return ss.str();
}

} //namespace yafaray
