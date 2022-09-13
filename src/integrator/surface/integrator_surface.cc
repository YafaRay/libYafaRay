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

namespace yafaray {

SurfaceIntegrator::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(time_forced_);
	PARAM_LOAD(time_forced_value_);
}

ParamMap SurfaceIntegrator::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(time_forced_);
	PARAM_SAVE(time_forced_value_);
	PARAM_SAVE_END;
}

ParamMap SurfaceIntegrator::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<SurfaceIntegrator *, ParamError> SurfaceIntegrator::factory(Logger &logger, RenderControl &render_control, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Bidirectional:
		{
			logger.logWarning("The Bidirectional integrator is UNSTABLE at the moment and needs to be improved. It might give unexpected and perhaps even incorrect render results. Use at your own risk.");
			return BidirectionalIntegrator::factory(logger, render_control, param_map, scene);
		}
		case Type::Debug: return DebugIntegrator::factory(logger, render_control, param_map, scene);
		case Type::DirectLight: return DirectLightIntegrator::factory(logger, render_control, param_map, scene);
		case Type::Path: return PathIntegrator::factory(logger, render_control, param_map, scene);
		case Type::Photon: return PhotonIntegrator::factory(logger, render_control, param_map, scene);
		case Type::Sppm: return SppmIntegrator::factory(logger, render_control, param_map, scene);
		default: return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	}
}

bool SurfaceIntegrator::preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene)
{
	accelerator_ = scene.getAccelerator();
	if(!accelerator_) return false;
	shadow_bias_ = scene.getShadowBias();
	ray_min_dist_ = scene.getRayMinDist();
	num_threads_ = scene.getNumThreads();
	num_threads_photons_ = scene.getNumThreadsPhotons();
	shadow_bias_auto_ = scene.isShadowBiasAuto();
	ray_min_dist_auto_ = scene.isRayMinDistAuto();
	layers_ = scene.getLayers();
	image_film_ = image_film;
	render_view_ = render_view;
	camera_ = render_view->getCamera();
	background_ = scene.getBackground();
	aa_noise_params_ = scene.getAaParameters();
	edge_toon_params_ = scene.getEdgeToonParams();
	mask_params_ = scene.getMaskParams();
	timer_ = image_film->getTimer();
	vol_integrator_ = scene.getVolIntegrator();
	scene_bound_ = scene.getSceneBound();
	return static_cast<bool>(layers_) && static_cast<bool>(image_film) && static_cast<bool>(render_view_) && static_cast<bool>(camera_) && static_cast<bool>(timer_);
}

} //namespace yafaray
