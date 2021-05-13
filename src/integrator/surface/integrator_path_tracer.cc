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
#include "scene/scene.h"
#include "sampler/sample.h"
#include "common/timer.h"
#include "material/material.h"
#include "background/background.h"
#include "volume/volume.h"
#include "sampler/halton.h"
#include "common/logger.h"
#include "render/render_data.h"
#include "render/imagesplitter.h"

BEGIN_YAFARAY

class Pdf1D;

PathIntegrator::PathIntegrator(bool transp_shad, int shadow_depth)
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

bool PathIntegrator::preprocess(const RenderControl &render_control, const RenderView *render_view, ImageFilm *image_film)
{
	image_film_ = image_film;
	std::stringstream set;
	g_timer_global.addEvent("prepass");
	g_timer_global.start("prepass");

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
		success = createCausticMap(render_view, render_control);
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

	g_timer_global.stop("prepass");
	Y_INFO << getName() << ": Photonmap building time: " << std::fixed << std::setprecision(1) << g_timer_global.getTime("prepass") << "s" << " (" << scene_->getNumThreadsPhotons() << " thread(s))" << YENDL;

	set << "| photon maps: " << std::fixed << std::setprecision(1) << g_timer_global.getTime("prepass") << "s" << " [" << scene_->getNumThreadsPhotons() << " thread(s)]";

	render_info_ += set.str();

	if(Y_LOG_HAS_VERBOSE)
	{
		for(std::string line; std::getline(set, line, '\n');) Y_VERBOSE << line << YENDL;
	}

	return success;
}

Rgba PathIntegrator::integrate(RenderData &render_data, const DiffRay &ray, int additional_depth, ColorLayers *color_layers, const RenderView *render_view) const
{
	const bool layers_used = render_data.raylevel_ == 0 && color_layers && color_layers->getFlags() != Layer::Flags::None;

	static int calls = 0;
	++calls;
	Rgb col(0.0);
	float alpha;
	SurfacePoint sp;
	void *o_udat = render_data.arena_;
	float w = 0.f;

	if(transp_background_) alpha = 0.0;
	else alpha = 1.0;

	//shoot ray into scene
	if(scene_->intersect(ray, sp))
	{
		// if camera ray initialize sampling offset:
		if(render_data.raylevel_ == 0)
		{
			render_data.lights_geometry_material_emit_ = true;
			//...
		}
		alignas (16) unsigned char userdata[user_data_size_];
		render_data.arena_ = static_cast<void *>(userdata);
		BsdfFlags bsdfs;

		const Material *material = sp.material_;
		material->initBsdf(render_data, sp, bsdfs);
		Vec3 wo = -ray.dir_;
		const VolumeHandler *vol;
		Rgb vcol(0.f);

		Random &prng = *(render_data.prng_);

		if(additional_depth < material->getAdditionalDepth()) additional_depth = material->getAdditionalDepth();

		// contribution of light emitting surfaces
		if(bsdfs.hasAny(BsdfFlags::Emit))
		{
			const Rgb col_tmp = material->emit(render_data, sp, wo);
			col += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_tmp;
			}
		}

		if(bsdfs.hasAny(BsdfFlags::Diffuse))
		{
			col += estimateAllDirectLight(render_data, sp, wo, color_layers);

			if(caustic_type_ == CausticType::Photon || caustic_type_ == CausticType::Both)
			{
				Rgb col_tmp = estimateCausticPhotons(render_data, sp, wo);
				if(aa_noise_params_.clamp_indirect_ > 0.f) col_tmp.clampProportionalRgb(aa_noise_params_.clamp_indirect_);
				col += col_tmp;
				if(layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::Indirect)) color_layer->color_ = col_tmp;
				}
			}
		}

		// path tracing:
		// the first path segment is "unrolled" from the loop because for the spot the camera hit
		// we do things slightly differently (e.g. may not sample specular, need not to init BSDF anymore,
		// have more efficient ways to compute samples...)

		bool was_chromatic = render_data.chromatic_;
		BsdfFlags path_flags = no_recursive_ ? BsdfFlags::All : (BsdfFlags::Diffuse);

		if(bsdfs.hasAny(path_flags))
		{
			Rgb path_col(0.0);
			path_flags |= (BsdfFlags::Diffuse | BsdfFlags::Reflect | BsdfFlags::Transmit);
			int n_samples = std::max(1, n_paths_ / render_data.ray_division_);
			for(int i = 0; i < n_samples; ++i)
			{
				void *first_udat = render_data.arena_;
				alignas (16) unsigned char n_userdata[user_data_size_];
				void *n_udat = static_cast<void *>(n_userdata);
				unsigned int offs = n_paths_ * render_data.pixel_sample_ + render_data.sampling_offs_ + i; // some redunancy here...
				Rgb throughput(1.0);
				Rgb lcol, scol;
				SurfacePoint sp_1 = sp, sp_2;
				SurfacePoint *hit = &sp_1, *hit_2 = &sp_2;
				Vec3 pwo = wo;
				Ray p_ray;

				render_data.chromatic_ = was_chromatic;
				if(was_chromatic) render_data.wavelength_ = sample::riS(offs);
				//this mat already is initialized, just sample (diffuse...non-specular?)
				float s_1 = sample::riVdC(offs);
				float s_2 = Halton::lowDiscrepancySampling(2, offs);
				if(render_data.ray_division_ > 1)
				{
					s_1 = math::addMod1(s_1, render_data.dc_1_);
					s_2 = math::addMod1(s_2, render_data.dc_2_);
				}
				// do proper sampling now...
				Sample s(s_1, s_2, path_flags);
				scol = material->sample(render_data, sp, pwo, p_ray.dir_, s, w);

				scol *= w;
				throughput = scol;
				render_data.lights_geometry_material_emit_ = false;

				p_ray.tmin_ = scene_->ray_min_dist_;
				p_ray.tmax_ = -1.0;
				p_ray.from_ = sp.p_;

				if(!scene_->intersect(p_ray, *hit)) continue; //hit background

				render_data.arena_ = n_udat;
				const Material *p_mat = hit->material_;
				BsdfFlags mat_bsd_fs;
				p_mat->initBsdf(render_data, *hit, mat_bsd_fs);
				if(s.sampled_flags_ != BsdfFlags::None) pwo = -p_ray.dir_; //Fix for white dots in path tracing with shiny diffuse with transparent PNG texture and transparent shadows, especially in Win32, (precision?). Sometimes the first sampling does not take place and pRay.dir is not initialized, so before this change when that happened pwo = -pRay.dir was getting a random non-initialized value! This fix makes that, if the first sample fails for some reason, pwo is not modified and the rest of the sampling continues with the same pwo value. FIXME: Question: if the first sample fails, should we continue as now or should we exit the loop with the "continue" command?
				lcol = estimateOneDirectLight(render_data, *hit, pwo, offs);
				if(mat_bsd_fs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = p_mat->emit(render_data, *hit, pwo);
					lcol += col_tmp;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_tmp;
					}
				}

				path_col += lcol * throughput;

				bool caustic = false;

				for(int depth = 1; depth < max_bounces_; ++depth)
				{
					int d_4 = 4 * depth;
					s.s_1_ = Halton::lowDiscrepancySampling(d_4 + 3, offs); //ourRandom();//
					s.s_2_ = Halton::lowDiscrepancySampling(d_4 + 4, offs); //ourRandom();//

					if(render_data.ray_division_ > 1)
					{
						s_1 = math::addMod1(s_1, render_data.dc_1_);
						s_2 = math::addMod1(s_2, render_data.dc_2_);
					}

					s.flags_ = BsdfFlags::All;

					scol = p_mat->sample(render_data, *hit, pwo, p_ray.dir_, s, w);
					scol *= w;

					if(scol.isBlack()) break;

					throughput *= scol;
					caustic = trace_caustics_ && s.sampled_flags_.hasAny(BsdfFlags::Specular | BsdfFlags::Glossy | BsdfFlags::Filter);
					render_data.lights_geometry_material_emit_ = caustic;

					p_ray.tmin_ = scene_->ray_min_dist_;
					p_ray.tmax_ = -1.0;
					p_ray.from_ = hit->p_;

					if(!scene_->intersect(p_ray, *hit_2)) //hit background
					{
						const auto &background = scene_->getBackground();
						if((caustic && background && background->hasIbl() && background->shootsCaustic()))
						{
							path_col += throughput * (*background)(p_ray, render_data, true);
						}
						break;
					}

					std::swap(hit, hit_2);
					p_mat = hit->material_;
					p_mat->initBsdf(render_data, *hit, mat_bsd_fs);
					pwo = -p_ray.dir_;

					if(mat_bsd_fs.hasAny(BsdfFlags::Diffuse)) lcol = estimateOneDirectLight(render_data, *hit, pwo, offs);
					else lcol = Rgb(0.f);

					if(mat_bsd_fs.hasAny(BsdfFlags::Volumetric) && (vol = p_mat->getVolumeHandler(hit->n_ * pwo < 0)))
					{
						if(vol->transmittance(render_data, p_ray, vcol)) throughput *= vcol;
					}

					// Russian roulette for terminating paths with low probability
					if(depth > russian_roulette_min_bounces_)
					{
						float random_value = prng();
						float probability = throughput.maximum();
						if(probability <= 0.f || probability < random_value) break;
						throughput *= 1.f / probability;
					}

					if(mat_bsd_fs.hasAny(BsdfFlags::Emit) && caustic)
					{
						const Rgb col_tmp = p_mat->emit(render_data, *hit, pwo);
						lcol += col_tmp;
						if(layers_used)
						{
							if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_tmp;
						}
					}

					path_col += lcol * throughput;
				}
				render_data.arena_ = first_udat;

			}
			col += path_col / n_samples;
		}
		//reset chromatic state:
		render_data.chromatic_ = was_chromatic;

		recursiveRaytrace(render_data, ray, bsdfs, sp, wo, col, alpha, additional_depth, color_layers);

		if(layers_used)
		{
			generateCommonLayers(render_data, sp, ray, color_layers);

			if(ColorLayer *color_layer = color_layers->find(Layer::Ao))
			{
				color_layer->color_ = sampleAmbientOcclusionLayer(render_data, sp, wo);
			}

			if(ColorLayer *color_layer = color_layers->find(Layer::AoClay))
			{
				color_layer->color_ = sampleAmbientOcclusionClayLayer(render_data, sp, wo);
			}
		}

		if(transp_refracted_background_)
		{
			float m_alpha = material->getAlpha(render_data, sp, wo);
			alpha = m_alpha + (1.f - m_alpha) * alpha;
		}
		else alpha = 1.0;
	}
	else //nothing hit, return background
	{
		if(scene_->getBackground() && !transp_refracted_background_)
		{
			const Rgb col_tmp = (*scene_->getBackground())(ray, render_data);
			col += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Env)) color_layer->color_ = col_tmp;
			}
		}
	}

	render_data.arena_ = o_udat;

	const Rgb col_vol_transmittance = scene_->vol_integrator_->transmittance(render_data, ray);
	const Rgb col_vol_integration = scene_->vol_integrator_->integrate(render_data, ray);

	if(transp_background_) alpha = std::max(alpha, 1.f - col_vol_transmittance.r_);

	if(layers_used)
	{
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeTransmittance)) color_layer->color_ = col_vol_transmittance;
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeIntegration)) color_layer->color_ = col_vol_integration;
	}

	col = (col * col_vol_transmittance) + col_vol_integration;

	return Rgba(col, alpha);
}

std::unique_ptr<Integrator> PathIntegrator::factory(ParamMap &params, const Scene &scene)
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

	auto inte = std::unique_ptr<PathIntegrator>(new PathIntegrator(transp_shad, shadow_depth));
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
