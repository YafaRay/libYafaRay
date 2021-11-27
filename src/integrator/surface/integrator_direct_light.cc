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
#include "common/param.h"
#include "color/color_layers.h"
#include "accelerator/accelerator.h"
#include "render/imagesplitter.h"

BEGIN_YAFARAY

DirectLightIntegrator::DirectLightIntegrator(RenderControl &render_control, Logger &logger, bool transp_shad, int shadow_depth, int ray_depth) : MonteCarloIntegrator(render_control, logger)
{
	caus_radius_ = 0.25;
	caus_depth_ = 10;
	n_caus_photons_ = 100000;
	n_caus_search_ = 100;
	tr_shad_ = transp_shad;
	use_photon_caustics_ = false;
	s_depth_ = shadow_depth;
	r_depth_ = ray_depth;
}

bool DirectLightIntegrator::preprocess(ImageFilm *image_film, const RenderView *render_view, const Scene &scene)
{
	bool success = SurfaceIntegrator::preprocess(image_film, render_view, scene);
	std::stringstream set;

	timer_->addEvent("prepass");
	timer_->start("prepass");

	set << "Direct Light  ";

	if(tr_shad_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}
	set << "RayDepth=" << r_depth_ << "  ";

	if(use_ambient_occlusion_)
	{
		set << "AO samples=" << ao_samples_ << " dist=" << ao_dist_ << "  ";
	}

	lights_ = render_view->getLightsVisible();

	if(use_photon_caustics_)
	{
		success = success && createCausticMap();
		set << "\nCaustic photons=" << n_caus_photons_ << " search=" << n_caus_search_ << " radius=" << caus_radius_ << " depth=" << caus_depth_ << "  ";

		if(photon_map_processing_ == PhotonsLoad)
		{
			set << " (loading photon maps from file)";
		}
		else if(photon_map_processing_ == PhotonsReuse)
		{
			set << " (reusing photon maps from memory)";
		}
		else if(photon_map_processing_ == PhotonsGenerateAndSave) set << " (saving photon maps to file)";
	}

	timer_->stop("prepass");
	logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), timer_->getTime("prepass"), "s", " (", num_threads_photons_, " thread(s))");

	set << "| photon maps: " << std::fixed << std::setprecision(1) << timer_->getTime("prepass") << "s" << " [" << num_threads_photons_ << " thread(s)]";

	render_info_ += set.str();

	if(logger_.isVerbose())
	{
		for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
	}

	return success;
}

std::pair<Rgb, float> DirectLightIntegrator::integrate(Ray &ray, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col {0.f};
	float alpha = 1.f;
	std::unique_ptr<const SurfacePoint> sp;
	std::tie(sp, ray.tmax_) = accelerator_->intersect(ray, camera_);
	if(sp)
	{
		const BsdfFlags &mat_bsdfs = sp->mat_data_->bsdf_flags_;
		const Vec3 wo = -ray.dir_;
		additional_depth = std::max(additional_depth, sp->material_->getAdditionalDepth());
		if(mat_bsdfs.hasAny(BsdfFlags::Emit))
		{
			const Rgb col_emit = sp->emit(wo);
			col += col_emit;
			if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer = col_emit;
			}
		}
		if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
		{
			col += estimateAllDirectLight(random_generator, color_layers, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
			if(use_photon_caustics_)
			{
				col += causticPhotons(color_layers, ray, *sp, wo, aa_noise_params_.clamp_indirect_, caustic_map_.get(), caus_radius_, n_caus_search_);
			}
			if(use_ambient_occlusion_) col += sampleAmbientOcclusion(*accelerator_, chromatic_enabled, wavelength, *sp, wo, ray_division, camera_, pixel_sampling_data, tr_shad_, false, ao_samples_, shadow_bias_auto_, shadow_bias_, ao_dist_, ao_col_, s_depth_);
		}
		const auto recursive_result = recursiveRaytrace(random_generator, color_layers, thread_id, ray_level + 1, chromatic_enabled, wavelength, ray, mat_bsdfs, *sp, wo, additional_depth, ray_division, pixel_sampling_data);
		col += recursive_result.first;
		alpha = recursive_result.second;
		if(color_layers)
		{
			generateCommonLayers(color_layers, *sp, mask_params_);
			generateOcclusionLayers(color_layers, *accelerator_, chromatic_enabled, wavelength, ray_division, camera_, pixel_sampling_data, *sp, wo, ao_samples_, shadow_bias_auto_, shadow_bias_, ao_dist_, ao_col_, s_depth_);
		}
	}
	else // Nothing hit, return background if any
	{
		std::tie(col, alpha) = background(ray, color_layers, transp_background_, transp_refracted_background_, background_, ray_level);
	}
	if(vol_integrator_)
	{
		applyVolumetricEffects(col, alpha, color_layers, ray, random_generator, vol_integrator_, transp_background_);
	}
	return {col, alpha};
}

std::unique_ptr<Integrator> DirectLightIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene, RenderControl &render_control)
{
	bool transp_shad = false;
	bool caustics = false;
	bool do_ao = false;
	int shadow_depth = 5;
	int raydepth = 5, c_depth = 10;
	int search = 100, photons = 500000;
	int ao_samples = 32;
	double c_rad = 0.25;
	double ao_dist = 1.0;
	Rgb ao_col(1.f);
	bool bg_transp = false;
	bool bg_transp_refract = false;
	std::string photon_maps_processing_str = "generate";

	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transp_shad);
	params.getParam("shadowDepth", shadow_depth);
	params.getParam("caustics", caustics);
	params.getParam("photons", photons);
	params.getParam("caustic_mix", search);
	params.getParam("caustic_depth", c_depth);
	params.getParam("caustic_radius", c_rad);
	params.getParam("do_AO", do_ao);
	params.getParam("AO_samples", ao_samples);
	params.getParam("AO_distance", ao_dist);
	params.getParam("AO_color", ao_col);
	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);
	params.getParam("photon_maps_processing", photon_maps_processing_str);

	auto inte = std::unique_ptr<DirectLightIntegrator>(new DirectLightIntegrator(render_control, logger, transp_shad, shadow_depth, raydepth));
	// caustic settings
	inte->use_photon_caustics_ = caustics;
	inte->n_caus_photons_ = photons;
	inte->n_caus_search_ = search;
	inte->caus_depth_ = c_depth;
	inte->caus_radius_ = c_rad;
	// AO settings
	inte->use_ambient_occlusion_ = do_ao;
	inte->ao_samples_ = ao_samples;
	inte->ao_dist_ = ao_dist;
	inte->ao_col_ = ao_col;
	// Background settings
	inte->transp_background_ = bg_transp;
	inte->transp_refracted_background_ = bg_transp_refract;

	if(photon_maps_processing_str == "generate-save") inte->photon_map_processing_ = PhotonsGenerateAndSave;
	else if(photon_maps_processing_str == "load") inte->photon_map_processing_ = PhotonsLoad;
	else if(photon_maps_processing_str == "reuse-previous") inte->photon_map_processing_ = PhotonsReuse;
	else inte->photon_map_processing_ = PhotonsGenerateOnly;

	return inte;
}

END_YAFARAY
