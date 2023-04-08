/****************************************************************************
 *      directlight.cc: an integrator for direct lighting only
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
 *      Copyright (C) 2009  Rodrigo Placencia (DarkTide)
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

#include "integrator/surface/integrator_direct_light.h"
#include "geometry/surface.h"
#include "param/param.h"
#include "color/color_layers.h"
#include "render/imagesplitter.h"
#include "integrator/volume/integrator_volume.h"
#include "common/timer.h"
#include "render/imagefilm.h"

namespace yafaray {

ParamMap DirectLightIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	param_map.append(ParentClassType_t::getAsParamMap(only_non_default));
	return param_map;
}

std::pair<SurfaceIntegrator *, ParamResult> DirectLightIntegrator::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto integrator {new DirectLightIntegrator(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {integrator, param_result};
}

DirectLightIntegrator::DirectLightIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : ParentClassType_t(logger, param_result, name, param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

bool DirectLightIntegrator::preprocess(RenderControl &render_control, const Scene &scene)
{
	bool success = SurfaceIntegrator::preprocess(render_control, scene);
	std::stringstream set;

	render_control.addTimerEvent("prepass");
	render_control.startTimer("prepass");

	set << "Direct Light  ";

	if(MonteCarloIntegrator::params_.transparent_shadows_)
	{
		set << "ShadowDepth=" << MonteCarloIntegrator::params_.shadow_depth_ << "  ";
	}
	set << "RayDepth=" << MonteCarloIntegrator::params_.r_depth_ << "  ";

	if(MonteCarloIntegrator::params_.ao_)
	{
		set << "AO samples=" << MonteCarloIntegrator::params_.ao_samples_ << " dist=" << MonteCarloIntegrator::params_.ao_distance_ << "  ";
	}

	if(CausticPhotonIntegrator::params_.use_photon_caustics_)
	{
		success = success && createCausticMap(render_control);
		set << "\nCaustic photons=" << n_caus_photons_ << " search=" << CausticPhotonIntegrator::params_.n_caus_search_ << " radius=" << CausticPhotonIntegrator::params_.caus_radius_ << " depth=" << CausticPhotonIntegrator::params_.caus_depth_ << "  ";
	}

	render_control.stopTimer("prepass");
	logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), render_control.getTimerTime("prepass"), "s", " (", num_threads_photons_, " thread(s))");

	set << "| photon maps: " << std::fixed << std::setprecision(1) << render_control.getTimerTime("prepass") << "s" << " [" << num_threads_photons_ << " thread(s)]";

	render_control.setRenderInfo(render_control.getRenderInfo() + set.str());

	if(logger_.isVerbose())
	{
		for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
	}

	return success;
}

std::pair<Rgb, float> DirectLightIntegrator::integrate(ImageFilm *image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier)
{
	Rgb col {0.f};
	float alpha = 1.f;
	const auto base_sampling_offset{image_film->getBaseSamplingOffset()};
	const auto camera{image_film->getCamera()};
	const auto [sp, tmax] = accelerator_->intersect(ray, camera);
	ray.tmax_ = tmax;
	if(sp)
	{
		const BsdfFlags mat_bsdfs = sp->mat_data_->bsdf_flags_;
		const Vec3f wo{-ray.dir_};
		additional_depth = std::max(additional_depth, sp->getMaterial()->getAdditionalDepth());
		if(mat_bsdfs.has(BsdfFlags::Emit))
		{
			const Rgb col_emit = sp->emit(wo);
			col += col_emit;
			if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer = Rgba{col_emit};
			}
		}
		if(mat_bsdfs.has(BsdfFlags::Diffuse))
		{
			col += estimateAllDirectLight(random_generator, color_layers, camera, chromatic_enabled, wavelength, aa_light_sample_multiplier, *sp, wo, ray_division, pixel_sampling_data);
			if(CausticPhotonIntegrator::params_.use_photon_caustics_)
			{
				col += causticPhotons(color_layers, ray, *sp, wo, aa_noise_params_.clamp_indirect_, caustic_map_.get(), CausticPhotonIntegrator::params_.caus_radius_, CausticPhotonIntegrator::params_.n_caus_search_);
			}
			if(MonteCarloIntegrator::params_.ao_) col += sampleAmbientOcclusion(*accelerator_, chromatic_enabled, wavelength, *sp, wo, ray_division, camera, pixel_sampling_data, MonteCarloIntegrator::params_.transparent_shadows_, false, MonteCarloIntegrator::params_.ao_samples_, SurfaceIntegrator::params_.shadow_bias_auto_, shadow_bias_, MonteCarloIntegrator::params_.ao_distance_, MonteCarloIntegrator::params_.ao_color_, MonteCarloIntegrator::params_.shadow_depth_);
		}
		const auto [raytrace_col, raytrace_alpha]{recursiveRaytrace(image_film, random_generator, correlative_sample_number, color_layers, thread_id, ray_level + 1, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, mat_bsdfs, *sp, wo, additional_depth, ray_division, pixel_sampling_data)};
		col += raytrace_col;
		alpha = raytrace_alpha;
		if(color_layers)
		{
			generateCommonLayers(color_layers, *sp, mask_params_, object_index_highest, material_index_highest);
			generateOcclusionLayers(color_layers, *accelerator_, chromatic_enabled, wavelength, ray_division, camera, pixel_sampling_data, *sp, wo, MonteCarloIntegrator::params_.ao_samples_, SurfaceIntegrator::params_.shadow_bias_auto_, shadow_bias_, MonteCarloIntegrator::params_.ao_distance_, MonteCarloIntegrator::params_.ao_color_, MonteCarloIntegrator::params_.shadow_depth_);
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugObjectTime))
			{
				const float col_combined_gray = col.col2Bri();
				if(sp->hasMotionBlur()) *color_layer += {col_combined_gray * ray.time_, 0.f, col_combined_gray * (1.f - ray.time_), 1.f};
				else *color_layer += {0.f, col_combined_gray, 0.f, 1.f};
			}
		}
	}
	else // Nothing hit, return background if any
	{
		std::tie(col, alpha) = background(ray, color_layers, MonteCarloIntegrator::params_.transparent_background_, MonteCarloIntegrator::params_.transparent_background_refraction_, background_, ray_level);
	}
	if(vol_integrator_)
	{
		applyVolumetricEffects(col, alpha, color_layers, ray, random_generator, vol_integrator_.get(), MonteCarloIntegrator::params_.transparent_background_);
	}
	return {std::move(col), alpha};
}

} //namespace yafaray
