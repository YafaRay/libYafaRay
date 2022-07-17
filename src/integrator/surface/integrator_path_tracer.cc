/****************************************************************************
 *      pathtracer.cc: A rather simple MC path integrator
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

#include "integrator/surface/integrator_path_tracer.h"
#include "geometry/surface.h"
#include "common/param.h"
#include "color/color_layers.h"
#include "sampler/sample.h"
#include "background/background.h"
#include "volume/volume.h"
#include "sampler/halton.h"
#include "render/render_data.h"
#include "accelerator/accelerator.h"
#include "render/imagesplitter.h"
#include "math/interpolation.h"

namespace yafaray {

class Pdf1D;

PathIntegrator::PathIntegrator(RenderControl &render_control, Logger &logger, bool transparent_shadows, int shadow_depth) : MonteCarloIntegrator(render_control, logger)
{
	transparent_shadows_ = transparent_shadows;
	s_depth_ = shadow_depth;
	caustic_type_ = CausticType::Path;
	r_depth_ = 6;
	max_bounces_ = 5;
	russian_roulette_min_bounces_ = 0;
	n_paths_ = 64;
	inv_n_paths_ = 1.f / 64.f;
	no_recursive_ = false;
}

bool PathIntegrator::preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene)
{
	bool success = SurfaceIntegrator::preprocess(fast_random, image_film, render_view, scene);
	std::stringstream set;

	timer_->addEvent("prepass");
	timer_->start("prepass");

	lights_ = render_view->getLightsVisible();

	set << "Path Tracing  ";

	if(transparent_shadows_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}
	set << "RayDepth=" << r_depth_ << " npaths=" << n_paths_ << " bounces=" << max_bounces_ << " min_bounces=" << russian_roulette_min_bounces_ << " ";

	trace_caustics_ = false;

	if(caustic_type_ == CausticType::Photon || caustic_type_ == CausticType::Both)
	{
		success = success && createCausticMap(fast_random);
	}

	if(caustic_type_ == CausticType::Path)
	{
		set << "\nCaustics: Path" << " ";
	}
	else if(caustic_type_ == CausticType::Photon)
	{
		set << "\nCaustics: Photons=" << n_caus_photons_ << " search=" << n_caus_search_ << " radius=" << caus_radius_ << " depth=" << caus_depth_ << "  ";
	}
	else if(caustic_type_ == CausticType::Both)
	{
		set << "\nCaustics: Path + Photons=" << n_caus_photons_ << " search=" << n_caus_search_ << " radius=" << caus_radius_ << " depth=" << caus_depth_ << "  ";
	}

	if(caustic_type_ == CausticType::Both || caustic_type_ == CausticType::Path) trace_caustics_ = true;

	if(caustic_type_ == CausticType::Both || caustic_type_ == CausticType::Photon)
	{
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

std::pair<Rgb, float> PathIntegrator::integrate(Ray &ray, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest) const
{
	static int calls = 0;
	++calls;
	Rgb col {0.f};
	float alpha = 1.f;
	float w = 0.f;
	const auto [sp, tmax] = accelerator_->intersect(ray, camera_);
	ray.tmax_ = tmax;
	if(sp)
	{
		const BsdfFlags mat_bsdfs = sp->mat_data_->bsdf_flags_;
		const Vec3f wo{-ray.dir_};
		additional_depth = std::max(additional_depth, sp->getMaterial()->getAdditionalDepth());

		// contribution of light emitting surfaces
		if(flags::have(mat_bsdfs, BsdfFlags::Emit))
		{
			const Rgb col_emit = sp->emit(wo);
			col += col_emit;
			if(color_layers && flags::have(color_layers->getFlags(), LayerDef::Flags::BasicLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_emit;
			}
		}

		if(flags::have(mat_bsdfs, BsdfFlags::Diffuse))
		{
			col += estimateAllDirectLight(random_generator, color_layers, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
			if(caustic_type_ == CausticType::Photon || caustic_type_ == CausticType::Both)
			{
				col += causticPhotons(color_layers, ray, *sp, wo, aa_noise_params_.clamp_indirect_, caustic_map_.get(), caus_radius_, n_caus_search_);
			}
		}
		// path tracing:
		// the first path segment is "unrolled" from the loop because for the spot the camera hit
		// we do things slightly differently (e.g. may not sample specular, need not to init BSDF anymore,
		// have more efficient ways to compute samples...)

		BsdfFlags path_flags = no_recursive_ ? BsdfFlags::All : (BsdfFlags::Diffuse);

		if(flags::have(mat_bsdfs, path_flags))
		{
			Rgb path_col(0.0);
			path_flags |= (BsdfFlags::Diffuse | BsdfFlags::Reflect | BsdfFlags::Transmit);
			int n_samples = std::max(1, n_paths_ / ray_division.division_);
			for(int i = 0; i < n_samples; ++i)
			{
				unsigned int offs = n_paths_ * pixel_sampling_data.sample_ + pixel_sampling_data.offset_ + i; // some redunancy here...
				Rgb throughput(1.0);
				Rgb lcol, scol;
				auto hit = std::make_unique<const SurfacePoint>(*sp);
				Vec3f pwo{wo};
				Ray p_ray;

				const float wavelength_dispersive = chromatic_enabled ? sample::riS(offs) : 0.f;
				//this mat already is initialized, just sample (diffuse...non-specular?)
				float s_1 = sample::riVdC(offs);
				float s_2 = Halton::lowDiscrepancySampling(fast_random, 2, offs);
				if(ray_division.division_ > 1)
				{
					s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
					s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
				}
				// do proper sampling now...
				Sample s(s_1, s_2, path_flags);
				scol = sp->sample(pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength_dispersive, camera_);
				scol *= w;
				throughput = scol;
				p_ray.tmin_ = ray_min_dist_;
				p_ray.tmax_ = -1.f;
				p_ray.from_ = sp->p_;
				std::tie(hit, p_ray.tmax_) = accelerator_->intersect(p_ray, camera_);
				if(!hit) continue; //hit background
				if(s.sampled_flags_ != BsdfFlags::None) pwo = -p_ray.dir_; //Fix for white dots in path tracing with shiny diffuse with transparent PNG texture and transparent shadows, especially in Win32, (precision?). Sometimes the first sampling does not take place and pRay.dir is not initialized, so before this change when that happened pwo = -pRay.dir was getting a random_generator non-initialized value! This fix makes that, if the first sample fails for some reason, pwo is not modified and the rest of the sampling continues with the same pwo value. FIXME: Question: if the first sample fails, should we continue as now or should we exit the loop with the "continue" command?
				lcol = estimateOneDirectLight(random_generator, correlative_sample_number, thread_id, chromatic_enabled, wavelength_dispersive, *hit, pwo, offs, ray_division, pixel_sampling_data);
				const BsdfFlags mat_bsd_fs = hit->mat_data_->bsdf_flags_;
				if(flags::have(mat_bsd_fs, BsdfFlags::Emit))
				{
					const Rgb col_emit = hit->emit(pwo);
					lcol += col_emit;
					if(color_layers && flags::have(color_layers->getFlags(), LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_emit;
					}
				}

				path_col += lcol * throughput;

				bool caustic = false;

				for(int depth = 1; depth < max_bounces_; ++depth)
				{
					int d_4 = 4 * depth;
					s.s_1_ = Halton::lowDiscrepancySampling(fast_random, d_4 + 3, offs); //ourRandom();//
					s.s_2_ = Halton::lowDiscrepancySampling(fast_random, d_4 + 4, offs); //ourRandom();//

					if(ray_division.division_ > 1)
					{
						s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
						s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
					}

					s.flags_ = BsdfFlags::All;

					scol = hit->sample(pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength_dispersive, camera_);
					scol *= w;
					if(scol.isBlack()) break;
					throughput *= scol;
					caustic = trace_caustics_ && flags::have(s.sampled_flags_, BsdfFlags::Specular | BsdfFlags::Glossy | BsdfFlags::Filter);
					p_ray.tmin_ = ray_min_dist_;
					p_ray.tmax_ = -1.f;
					p_ray.from_ = hit->p_;
					auto [intersect_sp, intersect_tmax] = accelerator_->intersect(p_ray, camera_);
					if(!intersect_sp) break; //hit background
					std::swap(hit, intersect_sp);
					pwo = -p_ray.dir_;

					if(flags::have(mat_bsd_fs, BsdfFlags::Diffuse)) lcol = estimateOneDirectLight(random_generator, correlative_sample_number, thread_id, chromatic_enabled, wavelength_dispersive, *hit, pwo, offs, ray_division, pixel_sampling_data);
					else lcol = Rgb(0.f);

					if(flags::have(mat_bsd_fs, BsdfFlags::Volumetric))
					{
						if(const VolumeHandler *vol = hit->getMaterial()->getVolumeHandler(hit->n_ * pwo < 0))
						{
							throughput *= vol->transmittance(p_ray);
						}
					}
					// Russian roulette for terminating paths with low probability
					if(depth > russian_roulette_min_bounces_)
					{
						const float random_value = random_generator();
						const float probability = throughput.maximum();
						if(probability <= 0.f || probability < random_value) break;
						throughput *= 1.f / probability;
					}

					if(flags::have(mat_bsd_fs, BsdfFlags::Emit) && caustic)
					{
						const Rgb col_tmp = hit->emit(pwo);
						lcol += col_tmp;
						if(color_layers && flags::have(color_layers->getFlags(), LayerDef::Flags::BasicLayers))
						{
							if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_tmp;
						}
					}
					path_col += lcol * throughput;
				}
			}
			col += path_col / n_samples;
		}
		const auto [raytrace_col, raytrace_alpha]{recursiveRaytrace(fast_random, random_generator, correlative_sample_number, color_layers, thread_id, ray_level + 1, chromatic_enabled, wavelength, ray, mat_bsdfs, *sp, wo, additional_depth, ray_division, pixel_sampling_data)};
		col += raytrace_col;
		alpha = raytrace_alpha;
		if(color_layers)
		{
			generateCommonLayers(color_layers, *sp, mask_params_, object_index_highest, material_index_highest);
			generateOcclusionLayers(color_layers, *accelerator_, chromatic_enabled, wavelength, ray_division, camera_, pixel_sampling_data, *sp, wo, ao_samples_, shadow_bias_auto_, shadow_bias_, ao_dist_, ao_col_, s_depth_);
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugObjectTime))
			{
				const float col_combined_gray = col.col2Bri();
				if(sp->hasMotionBlur()) *color_layer += {col_combined_gray * ray.time_, 0.f, col_combined_gray * (1.f - ray.time_), 1.f};
				else *color_layer += {0.f, col_combined_gray, 0.f, 1.f};
			}
		}
	}
	else //nothing hit, return background
	{
		std::tie(col, alpha) = background(ray, color_layers, transp_background_, transp_refracted_background_, background_, ray_level);
	}

	if(vol_integrator_)
	{
		applyVolumetricEffects(col, alpha, color_layers, ray, random_generator, vol_integrator_, transp_background_);
	}
	return {std::move(col), alpha};
}

SurfaceIntegrator * PathIntegrator::factory(Logger &logger, RenderControl &render_control, const ParamMap &params, const Scene &scene)
{
	bool transparent_shadows = false, no_rec = false;
	int shadow_depth = 5;
	int path_samples = 32;
	int bounces = 3;
	int russian_roulette_min_bounces = 0;
	int raydepth = 5;
	std::string c_method;
	bool do_ao = false;
	int ao_samples = 32;
	double ao_dist = 1.0;
	Rgb ao_col(1.f);
	bool bg_transp = false;
	bool bg_transp_refract = false;
	bool time_forced = false;
	float time_forced_value = 0.f;
	std::string photon_maps_processing_str = "generate";

	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transparent_shadows);
	params.getParam("shadowDepth", shadow_depth);
	params.getParam("path_samples", path_samples);
	params.getParam("bounces", bounces);
	params.getParam("russian_roulette_min_bounces", russian_roulette_min_bounces);
	params.getParam("no_recursive", no_rec);
	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);
	params.getParam("do_AO", do_ao);
	params.getParam("AO_samples", ao_samples);
	params.getParam("AO_distance", ao_dist);
	params.getParam("AO_color", ao_col);
	params.getParam("photon_maps_processing", photon_maps_processing_str);
	params.getParam("time_forced", time_forced);
	params.getParam("time_forced_value", time_forced_value);

	auto inte = new PathIntegrator(render_control, logger, transparent_shadows, shadow_depth);
	if(params.getParam("caustic_type", c_method))
	{
		bool use_photons = false;
		if(c_method == "photon") { inte->caustic_type_ = CausticType::Photon; use_photons = true; }
		else if(c_method == "both") { inte->caustic_type_ = CausticType::Both; use_photons = true; }
		else if(c_method == "none") inte->caustic_type_ = CausticType::None;
		if(use_photons)
		{
			double c_rad = 0.25;
			int c_depth = 10, search = 100, photons = 500000;
			params.getParam("photons", photons);
			params.getParam("caustic_mix", search);
			params.getParam("caustic_depth", c_depth);
			params.getParam("caustic_radius", c_rad);
			inte->n_caus_photons_ = photons;
			inte->n_caus_search_ = search;
			inte->caus_depth_ = c_depth;
			inte->caus_radius_ = c_rad;
		}
	}
	inte->r_depth_ = raydepth;
	inte->n_paths_ = path_samples;
	inte->inv_n_paths_ = 1.f / (float)path_samples;
	inte->max_bounces_ = bounces;
	inte->russian_roulette_min_bounces_ = russian_roulette_min_bounces;
	inte->no_recursive_ = no_rec;
	// Background settings
	inte->transp_background_ = bg_transp;
	inte->transp_refracted_background_ = bg_transp_refract;
	// AO settings
	inte->use_ambient_occlusion_ = do_ao;
	inte->ao_samples_ = ao_samples;
	inte->ao_dist_ = ao_dist;
	inte->ao_col_ = ao_col;
	inte->time_forced_ = time_forced;
	inte->time_forced_value_ = time_forced_value;

	if(photon_maps_processing_str == "generate-save") inte->photon_map_processing_ = PhotonsGenerateAndSave;
	else if(photon_maps_processing_str == "load") inte->photon_map_processing_ = PhotonsLoad;
	else if(photon_maps_processing_str == "reuse-previous") inte->photon_map_processing_ = PhotonsReuse;
	else inte->photon_map_processing_ = PhotonsGenerateOnly;

	return inte;
}

} //namespace yafaray
