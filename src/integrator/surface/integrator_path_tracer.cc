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

BEGIN_YAFARAY

class Pdf1D;

PathIntegrator::PathIntegrator(Logger &logger, bool transp_shad, int shadow_depth) : MonteCarloIntegrator(logger)
{
	tr_shad_ = transp_shad;
	s_depth_ = shadow_depth;
	caustic_type_ = CausticType::Path;
	r_depth_ = 6;
	max_bounces_ = 5;
	russian_roulette_min_bounces_ = 0;
	n_paths_ = 64;
	inv_n_paths_ = 1.f / 64.f;
	no_recursive_ = false;
}

bool PathIntegrator::preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film)
{
	image_film_ = image_film;
	std::stringstream set;

	timer.addEvent("prepass");
	timer.start("prepass");

	lights_ = render_view->getLightsVisible();

	set << "Path Tracing  ";

	if(tr_shad_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}
	set << "RayDepth=" << r_depth_ << " npaths=" << n_paths_ << " bounces=" << max_bounces_ << " min_bounces=" << russian_roulette_min_bounces_ << " ";

	bool success = true;
	trace_caustics_ = false;

	if(caustic_type_ == CausticType::Photon || caustic_type_ == CausticType::Both)
	{
		success = createCausticMap(render_view, render_control, timer);
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

	timer.stop("prepass");
	logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), timer.getTime("prepass"), "s", " (", scene_->getNumThreadsPhotons(), " thread(s))");

	set << "| photon maps: " << std::fixed << std::setprecision(1) << timer.getTime("prepass") << "s" << " [" << scene_->getNumThreadsPhotons() << " thread(s)]";

	render_info_ += set.str();

	if(logger_.isVerbose())
	{
		for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
	}

	return success;
}

std::pair<Rgb, float> PathIntegrator::integrate(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, Ray &ray, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const
{
	static int calls = 0;
	++calls;
	Rgb col {0.f};
	float alpha = 1.f;
	SurfacePoint sp;
	float w = 0.f;

	//shoot ray into scene
	if(accelerator.intersect(ray, sp, camera))
	{
		const Material *material = sp.material_;
		const BsdfFlags &mat_bsdfs = sp.mat_data_->bsdf_flags_;
		const Vec3 wo = -ray.dir_;
		additional_depth = std::max(additional_depth, material->getAdditionalDepth());

		// contribution of light emitting surfaces
		if(mat_bsdfs.hasAny(BsdfFlags::Emit))
		{
			const Rgb col_tmp = material->emit(sp.mat_data_.get(), sp, wo);
			col += col_tmp;
			if(color_layers)
			{
				if(Rgba *color_layer = color_layers->find(Layer::Emit)) *color_layer += col_tmp;
			}
		}

		if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
		{
			col += estimateAllDirectLight(accelerator, chromatic_enabled, wavelength, sp, wo, ray_division, color_layers, camera, random_generator, pixel_sampling_data);
			if(caustic_type_ == CausticType::Photon || caustic_type_ == CausticType::Both)
			{
				col += causticPhotons(ray, color_layers, sp, wo, aa_noise_params_.clamp_indirect_, caustic_map_.get(), caus_radius_, n_caus_search_);
			}
		}
		// path tracing:
		// the first path segment is "unrolled" from the loop because for the spot the camera hit
		// we do things slightly differently (e.g. may not sample specular, need not to init BSDF anymore,
		// have more efficient ways to compute samples...)

		BsdfFlags path_flags = no_recursive_ ? BsdfFlags::All : (BsdfFlags::Diffuse);

		if(mat_bsdfs.hasAny(path_flags))
		{
			Rgb path_col(0.0);
			path_flags |= (BsdfFlags::Diffuse | BsdfFlags::Reflect | BsdfFlags::Transmit);
			int n_samples = std::max(1, n_paths_ / ray_division.division_);
			for(int i = 0; i < n_samples; ++i)
			{
				unsigned int offs = n_paths_ * pixel_sampling_data.sample_ + pixel_sampling_data.offset_ + i; // some redunancy here...
				Rgb throughput(1.0);
				Rgb lcol, scol;
				SurfacePoint sp_1 {sp};
				SurfacePoint sp_2;
				SurfacePoint *hit = &sp_1, *hit_2 = &sp_2;
				Vec3 pwo = wo;
				Ray p_ray;

				const float wavelength_dispersive = chromatic_enabled ? sample::riS(offs) : 0.f;
				//this mat already is initialized, just sample (diffuse...non-specular?)
				float s_1 = sample::riVdC(offs);
				float s_2 = Halton::lowDiscrepancySampling(2, offs);
				if(ray_division.division_ > 1)
				{
					s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
					s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
				}
				// do proper sampling now...
				Sample s(s_1, s_2, path_flags);
				scol = material->sample(sp.mat_data_.get(), sp, pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength_dispersive, camera);
				scol *= w;
				throughput = scol;
				p_ray.tmin_ = scene_->ray_min_dist_;
				p_ray.tmax_ = -1.f;
				p_ray.from_ = sp.p_;
				if(!accelerator.intersect(p_ray, *hit, camera)) continue; //hit background
				const Material *p_mat = hit->material_;
				if(s.sampled_flags_ != BsdfFlags::None) pwo = -p_ray.dir_; //Fix for white dots in path tracing with shiny diffuse with transparent PNG texture and transparent shadows, especially in Win32, (precision?). Sometimes the first sampling does not take place and pRay.dir is not initialized, so before this change when that happened pwo = -pRay.dir was getting a random_generator non-initialized value! This fix makes that, if the first sample fails for some reason, pwo is not modified and the rest of the sampling continues with the same pwo value. FIXME: Question: if the first sample fails, should we continue as now or should we exit the loop with the "continue" command?
				lcol = estimateOneDirectLight(accelerator, thread_id, chromatic_enabled, wavelength_dispersive, *hit, pwo, offs, ray_division, camera, random_generator, pixel_sampling_data);
				const BsdfFlags mat_bsd_fs = hit->mat_data_->bsdf_flags_;
				if(mat_bsd_fs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = p_mat->emit(hit->mat_data_.get(), *hit, pwo);
					lcol += col_tmp;
					if(color_layers)
					{
						if(Rgba *color_layer = color_layers->find(Layer::Emit)) *color_layer += col_tmp;
					}
				}

				path_col += lcol * throughput;

				bool caustic = false;

				for(int depth = 1; depth < max_bounces_; ++depth)
				{
					int d_4 = 4 * depth;
					s.s_1_ = Halton::lowDiscrepancySampling(d_4 + 3, offs); //ourRandom();//
					s.s_2_ = Halton::lowDiscrepancySampling(d_4 + 4, offs); //ourRandom();//

					if(ray_division.division_ > 1)
					{
						s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
						s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
					}

					s.flags_ = BsdfFlags::All;

					scol = p_mat->sample(hit->mat_data_.get(), *hit, pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength_dispersive, camera);
					scol *= w;
					if(scol.isBlack()) break;
					throughput *= scol;
					caustic = trace_caustics_ && s.sampled_flags_.hasAny(BsdfFlags::Specular | BsdfFlags::Glossy | BsdfFlags::Filter);
					p_ray.tmin_ = scene_->ray_min_dist_;
					p_ray.tmax_ = -1.f;
					p_ray.from_ = hit->p_;
					if(!accelerator.intersect(p_ray, *hit_2, camera)) //hit background
					{
						const Background *background = scene_->getBackground();
						if((caustic && background && background->hasIbl() && background->shootsCaustic()))
						{
							path_col += throughput * (*background)(p_ray.dir_, true);
						}
						break;
					}
					std::swap(hit, hit_2);
					p_mat = hit->material_;
					pwo = -p_ray.dir_;

					if(mat_bsd_fs.hasAny(BsdfFlags::Diffuse)) lcol = estimateOneDirectLight(accelerator, thread_id, chromatic_enabled, wavelength_dispersive, *hit, pwo, offs, ray_division, camera, random_generator, pixel_sampling_data);
					else lcol = Rgb(0.f);

					if(mat_bsd_fs.hasAny(BsdfFlags::Volumetric))
					{
						if(const VolumeHandler *vol = p_mat->getVolumeHandler(hit->n_ * pwo < 0))
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

					if(mat_bsd_fs.hasAny(BsdfFlags::Emit) && caustic)
					{
						const Rgb col_tmp = p_mat->emit(hit->mat_data_.get(), *hit, pwo);
						lcol += col_tmp;
						if(color_layers)
						{
							if(Rgba *color_layer = color_layers->find(Layer::Emit)) *color_layer += col_tmp;
						}
					}
					path_col += lcol * throughput;
				}
			}
			col += path_col / n_samples;
		}
		const auto recursive_result = recursiveRaytrace(accelerator, thread_id, ray_level + 1, chromatic_enabled, wavelength, ray, mat_bsdfs, sp, wo, additional_depth, ray_division, color_layers, camera, random_generator, pixel_sampling_data);
		col += recursive_result.first;
		alpha = recursive_result.second;
		if(color_layers)
		{
			generateCommonLayers(sp, scene_->getMaskParams(), color_layers);
			generateOcclusionLayers(accelerator, chromatic_enabled, wavelength, ray_division, color_layers, camera, pixel_sampling_data, sp, wo, ao_samples_, scene_->shadow_bias_auto_, scene_->shadow_bias_, ao_dist_, ao_col_, s_depth_);
		}
	}
	else //nothing hit, return background
	{
		std::tie(col, alpha) = background(ray, color_layers, transp_background_, transp_refracted_background_, scene_->getBackground(), ray_level);
	}

	if(scene_->vol_integrator_)
	{
		std::tie(col, alpha) = volumetricEffects(ray, color_layers, random_generator, std::move(col), std::move(alpha), scene_->vol_integrator_, transp_background_);
	}
	return {col, alpha};
}

std::unique_ptr<Integrator> PathIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	bool transp_shad = false, no_rec = false;
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
	std::string photon_maps_processing_str = "generate";

	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transp_shad);
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

	auto inte = std::unique_ptr<PathIntegrator>(new PathIntegrator(logger, transp_shad, shadow_depth));
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

	if(photon_maps_processing_str == "generate-save") inte->photon_map_processing_ = PhotonsGenerateAndSave;
	else if(photon_maps_processing_str == "load") inte->photon_map_processing_ = PhotonsLoad;
	else if(photon_maps_processing_str == "reuse-previous") inte->photon_map_processing_ = PhotonsReuse;
	else inte->photon_map_processing_ = PhotonsGenerateOnly;

	return inte;
}

END_YAFARAY
