/****************************************************************************
 *      mcintegrator.h: A basic abstract integrator for MC sampling
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010  Rodrigo Placencia (DarkTide)
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

#include "integrator/surface/integrator_montecarlo.h"
#include "geometry/surface.h"
#include "common/layers.h"
#include "color/color_layers.h"
#include "common/logger.h"
#include "material/material.h"
#include "scene/scene.h"
#include "volume/volume.h"
#include "common/session.h"
#include "light/light.h"
#include "color/spectrum.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "render/monitor.h"
#include "photon/photon.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "render/render_data.h"

#ifdef __clang__
#define inline  // aka inline removal
#endif

BEGIN_YAFARAY

static constexpr int loffs_delta_global = 4567; //just some number to have different sequences per light...and it's a prime even...

//Constructor and destructor defined here to avoid issues with std::unique_ptr<Pdf1D> being Pdf1D incomplete in the header (forward declaration)
MonteCarloIntegrator::MonteCarloIntegrator() = default;
MonteCarloIntegrator::~MonteCarloIntegrator() = default;

Rgb MonteCarloIntegrator::estimateAllDirectLight(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, ColorLayers *color_layers) const
{
	const bool layers_used = render_data.raylevel_ == 0 && color_layers && color_layers->getFlags() != Layer::Flags::None;

	Rgb col;
	unsigned int loffs = 0;
	for(auto l = lights_.begin(); l != lights_.end(); ++l)
	{
		col += doLightEstimation(render_data, (*l), sp, wo, loffs, color_layers);
		loffs++;
	}

	if(layers_used)
	{
		if(ColorLayer *color_layer = color_layers->find(Layer::Shadow)) color_layer->color_ *= (1.f / (float) loffs);
	}

	return col;
}

Rgb MonteCarloIntegrator::estimateOneDirectLight(RenderData &render_data, const SurfacePoint &sp, Vec3 wo, int n) const
{
	const int light_num = lights_.size();

	if(light_num == 0) return Rgb(0.f); //??? if you get this far the lights must be >= 1 but, what the hell... :)

	Halton hal_2(2,  scene_->getImageFilm()->getBaseSamplingOffset() + correlative_sample_number_[render_data.thread_id_] - 1); //Probably with this change the parameter "n" is no longer necessary, but I will keep it just in case I have to revert back this change!
	const int lnum = std::min(static_cast<int>(hal_2.getNext() * static_cast<float>(light_num)), light_num - 1);

	++correlative_sample_number_[render_data.thread_id_];

	return doLightEstimation(render_data, lights_[lnum], sp, wo, lnum) * light_num;
}

Rgb MonteCarloIntegrator::doLightEstimation(RenderData &render_data, const Light *light, const SurfacePoint &sp, const Vec3 &wo, const unsigned int  &loffs, ColorLayers *color_layers) const
{
	const bool layers_used = render_data.raylevel_ == 0 && color_layers && color_layers->getFlags() != Layer::Flags::None;

	Rgb col(0.f);
	bool shadowed;
	unsigned int l_offs = loffs * loffs_delta_global;
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	Rgb lcol(0.f), scol;
	float light_pdf;
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	bool cast_shadows = light->castShadows() && material->getReceiveShadows();

	// handle lights with delta distribution, e.g. point and directional lights
	if(light->diracLight())
	{
		Rgba col_shadow(0.f), col_shadow_obj_mask(0.f), col_shadow_mat_mask(0.f), col_diff_dir(0.f), col_diff_no_shadow(0.f), col_glossy_dir(0.f);

		if(light->illuminate(sp, lcol, light_ray))
		{
			// ...shadowed...
			if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
			else light_ray.tmin_ = scene_->shadow_bias_;

			if(cast_shadows) shadowed = (tr_shad_) ? scene_->isShadowed(render_data, light_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, light_ray, mask_obj_index, mask_mat_index);
			else shadowed = false;

			const float angle_light_normal = (material->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_));	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal

			if(!shadowed || (layers_used && color_layers->find(Layer::DiffuseNoShadow)))
			{
				if(!shadowed && layers_used && color_layers->find(Layer::Shadow)) col_shadow += Rgb(1.f);

				const Rgb surf_col = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::All);
				const Rgb transmit_col = scene_->vol_integrator_->transmittance(render_data, light_ray);
				const Rgba tmp_col_no_shadow = surf_col * lcol * angle_light_normal * transmit_col;
				if(tr_shad_ && cast_shadows) lcol *= scol;

				if(layers_used)
				{
					if(color_layers->isDefinedAny({Layer::Diffuse, Layer::DiffuseNoShadow}))
					{
						col_diff_no_shadow += tmp_col_no_shadow;
						if(!shadowed) col_diff_dir += material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * lcol * angle_light_normal * transmit_col;
					}

					if(color_layers->find(Layer::Glossy))
					{
						if(!shadowed) col_glossy_dir += material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * lcol * angle_light_normal * transmit_col;
					}
				}

				if(!shadowed) col += surf_col * lcol * angle_light_normal * transmit_col;
			}

			if(shadowed && layers_used)
			{
				const MaskParams &mask_params = color_layers->getMaskParams();
				if(color_layers->find(Layer::MatIndexMaskShadow) && mask_mat_index == mask_params.mat_index_) col_shadow_mat_mask += Rgb(1.f);
				if(color_layers->find(Layer::ObjIndexMaskShadow) && mask_obj_index == mask_params.obj_index_) col_shadow_obj_mask += Rgb(1.f);
			}
		}
		if(layers_used)
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::Shadow)) color_layer->color_ += col_shadow;
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexMaskShadow)) color_layer->color_ += col_shadow_mat_mask;
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexMaskShadow)) color_layer->color_ += col_shadow_obj_mask;
			if(ColorLayer *color_layer = color_layers->find(Layer::Diffuse)) color_layer->color_ += col_diff_dir;
			if(ColorLayer *color_layer = color_layers->find(Layer::DiffuseNoShadow)) color_layer->color_ += col_diff_no_shadow;
			if(ColorLayer *color_layer = color_layers->find(Layer::Glossy)) color_layer->color_ += col_glossy_dir;
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugLightEstimationLightDirac)) color_layer->color_ += col;
		}
	}
	else // area light and suchlike
	{
		int n = (int) ceilf(light->nSamples() * aa_light_sample_multiplier_);
		if(render_data.ray_division_ > 1) n = std::max(1, n / render_data.ray_division_);
		const float inv_ns = 1.f / (float)n;
		const unsigned int offs = n * render_data.pixel_sample_ + render_data.sampling_offs_ + l_offs;
		const bool can_intersect = light->canIntersect();
		Rgb ccol(0.0);
		LSample ls;

		Halton hal_2(2, offs - 1);
		Halton hal_3(3, offs - 1);

		Rgba col_shadow(0.f), col_shadow_obj_mask(0.f), col_shadow_mat_mask(0.f), col_diff_dir(0.f), col_diff_no_shadow(0.f), col_glossy_dir(0.f);

		for(int i = 0; i < n; ++i)
		{
			// ...get sample val...
			ls.s_1_ = hal_2.getNext();
			ls.s_2_ = hal_3.getNext();

			if(light->illumSample(sp, ls, light_ray))
			{
				// ...shadowed...
				if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
				else light_ray.tmin_ = scene_->shadow_bias_;

				if(cast_shadows) shadowed = (tr_shad_) ? scene_->isShadowed(render_data, light_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, light_ray, mask_obj_index, mask_mat_index);
				else shadowed = false;

				if((!shadowed && ls.pdf_ > 1e-6f)  || (layers_used && color_layers->find(Layer::DiffuseNoShadow)))
				{
					const Rgb ls_col_no_shadow = ls.col_;
					if(tr_shad_ && cast_shadows) ls.col_ *= scol;
					const Rgb transmit_col = scene_->vol_integrator_->transmittance(render_data, light_ray);
					ls.col_ *= transmit_col;
					const Rgb surf_col = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::All);

					if(layers_used && (!shadowed && ls.pdf_ > 1e-6f) && color_layers->find(Layer::Shadow)) col_shadow += Rgb(1.f);

					const float angle_light_normal = (material->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_));	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal

					if(can_intersect)
					{
						const float m_pdf = material->pdf(render_data, sp, wo, light_ray.dir_, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
						if(m_pdf > 1e-6f)
						{
							const float l_2 = ls.pdf_ * ls.pdf_;
							const float m_2 = m_pdf * m_pdf;
							const float w = l_2 / (l_2 + m_2);

							if(layers_used)
							{
								if(color_layers->isDefinedAny({Layer::Diffuse, Layer::DiffuseNoShadow}))
								{
									const Rgb tmp_col_no_light_color = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal * w / ls.pdf_;
									col_diff_no_shadow += tmp_col_no_light_color * ls_col_no_shadow;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_diff_dir += tmp_col_no_light_color * ls.col_;
								}
								if(color_layers->find(Layer::Glossy))
								{
									const Rgb tmp_col = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal * w / ls.pdf_;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_glossy_dir += tmp_col;
								}
							}

							if((!shadowed && ls.pdf_ > 1e-6f)) ccol += surf_col * ls.col_ * angle_light_normal * w / ls.pdf_;
						}
						else
						{
							if(layers_used)
							{
								if(color_layers->isDefinedAny({Layer::Diffuse, Layer::DiffuseNoShadow}))
								{
									const Rgb tmp_col_no_light_color = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal / ls.pdf_;
									col_diff_no_shadow += tmp_col_no_light_color * ls_col_no_shadow;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_diff_dir += tmp_col_no_light_color * ls.col_;
								}

								if(color_layers->find(Layer::Glossy))
								{
									const Rgb tmp_col = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal / ls.pdf_;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_glossy_dir += tmp_col;
								}
							}

							if((!shadowed && ls.pdf_ > 1e-6f)) ccol += surf_col * ls.col_ * angle_light_normal / ls.pdf_;
						}
					}
					else
					{
						if(layers_used)
						{
							if(color_layers->isDefinedAny({Layer::Diffuse, Layer::DiffuseNoShadow}))
							{
								const Rgb tmp_col_no_light_color = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal / ls.pdf_;
								col_diff_no_shadow += tmp_col_no_light_color * ls_col_no_shadow;
								if((!shadowed && ls.pdf_ > 1e-6f)) col_diff_dir += tmp_col_no_light_color * ls.col_;
							}

							if(color_layers->find(Layer::Glossy))
							{
								const Rgb tmp_col = material->eval(render_data, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal / ls.pdf_;
								if((!shadowed && ls.pdf_ > 1e-6f)) col_glossy_dir += tmp_col;
							}
						}

						if((!shadowed && ls.pdf_ > 1e-6f)) ccol += surf_col * ls.col_ * angle_light_normal / ls.pdf_;
					}
				}

				if(layers_used && (shadowed || ls.pdf_ <= 1e-6f))
				{
					const MaskParams &mask_params = color_layers->getMaskParams();
					if(color_layers->find(Layer::MatIndexMaskShadow) && mask_mat_index == mask_params.mat_index_) col_shadow_mat_mask += Rgb(1.f);
					if(color_layers->find(Layer::ObjIndexMaskShadow) && mask_obj_index == mask_params.obj_index_) col_shadow_obj_mask += Rgb(1.f);
				}
			}
		}

		col += ccol * inv_ns;

		if(layers_used)
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugLightEstimationLightSampling)) color_layer->color_ += ccol * inv_ns;
			if(ColorLayer *color_layer = color_layers->find(Layer::Shadow)) color_layer->color_ += col_shadow * inv_ns;
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexMaskShadow)) color_layer->color_ += col_shadow_mat_mask * inv_ns;
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexMaskShadow)) color_layer->color_ += col_shadow_obj_mask * inv_ns;
			if(ColorLayer *color_layer = color_layers->find(Layer::Diffuse)) color_layer->color_ += col_diff_dir * inv_ns;
			if(ColorLayer *color_layer = color_layers->find(Layer::DiffuseNoShadow)) color_layer->color_ += col_diff_no_shadow * inv_ns;
			if(ColorLayer *color_layer = color_layers->find(Layer::Glossy)) color_layer->color_ += col_glossy_dir * inv_ns;
		}

		if(can_intersect) // sample from BSDF to complete MIS
		{
			Rgb ccol_2(0.f);

			if(layers_used)
			{
				if(color_layers->isDefinedAny({Layer::Diffuse, Layer::DiffuseNoShadow}))
				{
					col_diff_no_shadow = Rgba(0.f);
					col_diff_dir = Rgba(0.f);
				}
				if(color_layers->find(Layer::Glossy)) col_glossy_dir = Rgba(0.f);
			}

			hal_2.setStart(offs - 1);
			hal_3.setStart(offs - 1);

			for(int i = 0; i < n; ++i)
			{
				Ray b_ray;
				if(scene_->ray_min_dist_auto_) b_ray.tmin_ = scene_->ray_min_dist_ * std::max(1.f, Vec3(sp.p_).length());
				else b_ray.tmin_ = scene_->ray_min_dist_;

				b_ray.from_ = sp.p_;

				const float s_1 = hal_2.getNext();
				const float s_2 = hal_3.getNext();
				float W = 0.f;

				Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
				const Rgb surf_col = material->sample(render_data, sp, wo, b_ray.dir_, s, W);
				if(s.pdf_ > 1e-6f && light->intersect(b_ray, b_ray.tmax_, lcol, light_pdf))
				{
					if(cast_shadows) shadowed = (tr_shad_) ? scene_->isShadowed(render_data, b_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, b_ray, mask_obj_index, mask_mat_index);
					else shadowed = false;

					if((!shadowed && light_pdf > 1e-6f) || (layers_used && color_layers->find(Layer::DiffuseNoShadow)))
					{
						if(tr_shad_ && cast_shadows) lcol *= scol;
						const Rgb transmit_col = scene_->vol_integrator_->transmittance(render_data, light_ray);
						lcol *= transmit_col;
						const float l_pdf = 1.f / light_pdf;
						const float l_2 = l_pdf * l_pdf;
						const float m_2 = s.pdf_ * s.pdf_;
						const float w = m_2 / (l_2 + m_2);

						if(layers_used)
						{
							if(color_layers->isDefinedAny({Layer::Diffuse, Layer::DiffuseNoShadow}))
							{
								const Rgb tmp_col = material->sample(render_data, sp, wo, b_ray.dir_, s, W) * lcol * w * W;
								col_diff_no_shadow += tmp_col;
								if((!shadowed && light_pdf > 1e-6f) && s.sampled_flags_.hasAny(BsdfFlags::Diffuse)) col_diff_dir += tmp_col;
							}

							if(color_layers->find(Layer::Glossy))
							{
								const Rgb tmp_col = material->sample(render_data, sp, wo, b_ray.dir_, s, W) * lcol * w * W;
								if((!shadowed && light_pdf > 1e-6f) && s.sampled_flags_.hasAny(BsdfFlags::Glossy)) col_glossy_dir += tmp_col;
							}
						}

						if((!shadowed && light_pdf > 1e-6f)) ccol_2 += surf_col * lcol * w * W;
					}
				}
			}

			col += ccol_2 * inv_ns;

			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::DebugLightEstimationMatSampling)) color_layer->color_ += ccol_2 * inv_ns;
				if(ColorLayer *color_layer = color_layers->find(Layer::Diffuse)) color_layer->color_ += col_diff_dir * inv_ns;
				if(ColorLayer *color_layer = color_layers->find(Layer::DiffuseNoShadow)) color_layer->color_ += col_diff_no_shadow * inv_ns;
				if(ColorLayer *color_layer = color_layers->find(Layer::Glossy)) color_layer->color_ += col_glossy_dir * inv_ns;
			}
		}
	}

	return col;
}

void MonteCarloIntegrator::causticWorker(PhotonMap *caustic_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, unsigned int n_caus_photons, Pdf1D *light_power_d, int num_lights, const std::vector<const Light *> &caus_lights, int caus_depth, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot)
{
	bool done = false;
	float s_1, s_2, s_3, s_4, s_5, s_6, s_7, s_l;
	const float f_num_lights = (float)num_lights;
	float light_num_pdf, light_pdf;

	unsigned int curr = 0;
	const unsigned int n_caus_photons_thread = 1 + ((n_caus_photons - 1) / scene->getNumThreadsPhotons());

	std::vector<Photon> local_caustic_photons;

	SurfacePoint sp_1, sp_2;
	SurfacePoint *hit = &sp_1, *hit_2 = &sp_2;
	Ray ray;

	RenderData render_data;
	render_data.cam_ = render_view->getCamera();
	alignas (16) unsigned char userdata[user_data_size_];
	render_data.arena_ = static_cast<void *>(userdata);

	local_caustic_photons.clear();
	local_caustic_photons.reserve(n_caus_photons_thread);

	while(!done)
	{
		const unsigned int haltoncurr = curr + n_caus_photons_thread * thread_id;

		render_data.chromatic_ = true;
		render_data.wavelength_ = sample::riS(haltoncurr);

		s_1 = sample::riVdC(haltoncurr);
		s_2 = Halton::lowDiscrepancySampling(2, haltoncurr);
		s_3 = Halton::lowDiscrepancySampling(3, haltoncurr);
		s_4 = Halton::lowDiscrepancySampling(4, haltoncurr);

		s_l = float(haltoncurr) / float(n_caus_photons);

		const int light_num = light_power_d->dSample(s_l, &light_num_pdf);

		if(light_num >= num_lights)
		{
			caustic_map->mutx_.lock();
			Y_ERROR << getName() << ": lightPDF sample error! " << s_l << "/" << light_num << YENDL;
			caustic_map->mutx_.unlock();
			return;
		}

		Rgb pcol = caus_lights[light_num]->emitPhoton(s_1, s_2, s_3, s_4, ray, light_pdf);
		ray.tmin_ = scene->ray_min_dist_;
		ray.tmax_ = -1.0;
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= n_caus_photons_thread);
			continue;
		}
		BsdfFlags bsdfs = BsdfFlags::None;
		int n_bounces = 0;
		bool caustic_photon = false;
		bool direct_photon = true;
		const Material *material = nullptr;
		const VolumeHandler *vol = nullptr;

		while(scene->intersect(ray, *hit_2))
		{
			if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
			{
				caustic_map->mutx_.lock();
				Y_WARNING << getName() << ": NaN (photon color)" << YENDL;
				caustic_map->mutx_.unlock();
				break;
			}
			Rgb transm(1.f), vcol;
			// check for volumetric effects
			if(material)
			{
				if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(hit->ng_ * ray.dir_ < 0)))
				{
					vol->transmittance(render_data, ray, vcol);
					transm = vcol;
				}
			}
			std::swap(hit, hit_2);
			Vec3 wi = -ray.dir_, wo;
			material = hit->material_;
			material->initBsdf(render_data, *hit, bsdfs);
			if(bsdfs.hasAny((BsdfFlags::Diffuse | BsdfFlags::Glossy)))
			{
				//deposit caustic photon on surface
				if(caustic_photon)
				{
					Photon np(wi, hit->p_, pcol);
					local_caustic_photons.push_back(np);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == caus_depth) break;
			// scatter photon
			const int d_5 = 3 * n_bounces + 5;
			//int d6 = d5 + 1;

			s_5 = Halton::lowDiscrepancySampling(d_5, haltoncurr);
			s_6 = Halton::lowDiscrepancySampling(d_5 + 1, haltoncurr);
			s_7 = Halton::lowDiscrepancySampling(d_5 + 2, haltoncurr);

			PSample sample(s_5, s_6, s_7, BsdfFlags::AllSpecular | BsdfFlags::Glossy | BsdfFlags::Filter | BsdfFlags::Dispersive, pcol, transm);
			bool scattered = material->scatterPhoton(render_data, *hit, wi, wo, sample);
			if(!scattered) break; //photon was absorped.
			pcol = sample.color_;
			// hm...dispersive is not really a scattering qualifier like specular/glossy/diffuse or the special case filter...
			caustic_photon = (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
			// light through transparent materials can be calculated by direct lighting, so still consider them direct!
			direct_photon = sample.sampled_flags_.hasAny(BsdfFlags::Filter) && direct_photon;
			// caustic-only calculation can be stopped if:
			if(!(caustic_photon || direct_photon)) break;

			if(render_data.chromatic_ && sample.sampled_flags_.hasAny(BsdfFlags::Dispersive))
			{
				render_data.chromatic_ = false;
				Rgb wl_col;
				wl2Rgb_global(render_data.wavelength_, wl_col);
				pcol *= wl_col;
			}
			ray.from_ = hit->p_;
			ray.dir_ = wo;
			ray.tmin_ = scene->ray_min_dist_;
			ray.tmax_ = -1.0;
			++n_bounces;
		}
		++curr;
		if(curr % pb_step == 0)
		{
			pb->mutx_.lock();
			pb->update();
			pb->mutx_.unlock();
			if(render_control.aborted()) { return; }
		}
		done = (curr >= n_caus_photons_thread);
	}
	caustic_map->mutx_.lock();
	caustic_map->appendVector(local_caustic_photons, curr);
	total_photons_shot += curr;
	caustic_map->mutx_.unlock();
}

bool MonteCarloIntegrator::createCausticMap(const RenderView *render_view, const RenderControl &render_control)
{
	std::shared_ptr<ProgressBar> pb;
	if(intpb_) pb = intpb_;
	else pb = std::make_shared<ConsoleProgressBar>(80);

	if(photon_map_processing_ == PhotonsLoad)
	{
		pb->setTag("Loading caustic photon map from file...");
		const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_caustic.photonmap";
		Y_INFO << getName() << ": Loading caustic photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
		if(session_global.caustic_map_.get()->load(filename))
		{
			if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Caustic map loaded." << YENDL;
			return true;
		}
		else
		{
			photon_map_processing_ = PhotonsGenerateAndSave;
			Y_WARNING << getName() << ": photon map loading failed, changing to Generate and Save mode." << YENDL;
		}
	}

	if(photon_map_processing_ == PhotonsReuse)
	{
		Y_INFO << getName() << ": Reusing caustics photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
		if(session_global.caustic_map_.get()->nPhotons() == 0)
		{
			photon_map_processing_ = PhotonsGenerateOnly;
			Y_WARNING << getName() << ": One of the photon maps in memory was empty, they cannot be reused: changing to Generate mode." << YENDL;
		}
		else return true;
	}

	session_global.caustic_map_.get()->clear();
	session_global.caustic_map_.get()->setNumPaths(0);
	session_global.caustic_map_.get()->reserveMemory(n_caus_photons_);
	session_global.caustic_map_.get()->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

	Ray ray;
	std::vector<const Light *> caus_lights;

	for(unsigned int i = 0; i < lights_.size(); ++i)
	{
		if(lights_[i]->shootsCausticP())
		{
			caus_lights.push_back(lights_[i]);
		}
	}

	const int num_lights = caus_lights.size();

	if(num_lights > 0)
	{
		float light_num_pdf, light_pdf;
		const float f_num_lights = (float)num_lights;
		auto energies = std::unique_ptr<float[]>(new float[num_lights]);
		for(int i = 0; i < num_lights; ++i) energies[i] = caus_lights[i]->totalEnergy().energy();
		auto light_power_d = std::unique_ptr<Pdf1D>(new Pdf1D(energies.get(), num_lights));

		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Light(s) photon color testing for caustics map:" << YENDL;

		for(int i = 0; i < num_lights; ++i)
		{
			Rgb pcol(0.f);
			pcol = caus_lights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			light_num_pdf = light_power_d->func_[i] * light_power_d->inv_integral_;
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Light [" << i + 1 << "] Photon col:" << pcol << " | lnpdf: " << light_num_pdf << YENDL;
		}

		int pb_step;
		Y_INFO << getName() << ": Building caustics photon map..." << YENDL;
		pb->init(128);
		pb_step = std::max(1U, n_caus_photons_ / 128);
		pb->setTag("Building caustics photon map...");

		unsigned int curr = 0;

		const int n_threads = scene_->getNumThreadsPhotons();

		n_caus_photons_ = std::max((unsigned int) n_threads, (n_caus_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		Y_PARAMS << getName() << ": Shooting " << n_caus_photons_ << " photons across " << n_threads << " threads (" << (n_caus_photons_ / n_threads) << " photons/thread)" << YENDL;

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&MonteCarloIntegrator::causticWorker, this, session_global.caustic_map_.get(), i, scene_, render_view, std::ref(render_control), n_caus_photons_, light_power_d.get(), num_lights, caus_lights, caus_depth_, pb.get(), pb_step, std::ref(curr)));
		for(auto &t : threads) t.join();

		pb->done();
		pb->setTag("Caustic photon map built.");
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Done." << YENDL;
		Y_INFO << getName() << ": Shot " << curr << " caustic photons from " << num_lights << " light(s)." << YENDL;
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Stored caustic photons: " << session_global.caustic_map_.get()->nPhotons() << YENDL;

		if(session_global.caustic_map_.get()->nPhotons() > 0)
		{
			pb->setTag("Building caustic photons kd-tree...");
			session_global.caustic_map_.get()->updateTree();
			if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Done." << YENDL;
		}

		if(photon_map_processing_ == PhotonsGenerateAndSave)
		{
			pb->setTag("Saving caustic photon map to file...");
			std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_caustic.photonmap";
			Y_INFO << getName() << ": Saving caustic photon map to: " << filename << YENDL;
			if(session_global.caustic_map_.get()->save(filename) && Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Caustic map saved." << YENDL;
		}
	}
	else if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": No caustic source lights found, skiping caustic map building..." << YENDL;
	return true;
}

Rgb MonteCarloIntegrator::estimateCausticPhotons(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	if(!session_global.caustic_map_.get()->ready()) return Rgb(0.f);

	auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_caus_search_]);//(foundPhoton_t *)alloca(nCausSearch * sizeof(foundPhoton_t));
	int n_gathered = 0;

	float g_radius_square = caus_radius_ * caus_radius_;

	n_gathered = session_global.caustic_map_.get()->gather(sp.p_, gathered.get(), n_caus_search_, g_radius_square);

	g_radius_square = 1.f / g_radius_square;

	Rgb sum(0.f);

	if(n_gathered > 0)
	{
		const Material *material = sp.material_;
		Rgb surf_col(0.f);
		float k = 0.f;
		const Photon *photon;

		for(int i = 0; i < n_gathered; ++i)
		{
			photon = gathered[i].photon_;
			surf_col = material->eval(render_data, sp, wo, photon->direction(), BsdfFlags::All);
			k = sample::kernel(gathered[i].dist_square_, g_radius_square);
			sum += surf_col * k * photon->color();
		}
		sum *= 1.f / (float(session_global.caustic_map_.get()->nPaths()));
	}
	return sum;
}

void MonteCarloIntegrator::recursiveRaytrace(RenderData &render_data, const DiffRay &ray, const BsdfFlags &bsdfs, SurfacePoint &sp, const Vec3 &wo, Rgb &col, float &alpha, int additional_depth, ColorLayers *color_layers) const
{
	const bool layers_used = render_data.raylevel_ == 0 && color_layers && color_layers->getFlags() != Layer::Flags::None;

	const Material *material = sp.material_;
	const SpDifferentials sp_diff(sp, ray);

	render_data.raylevel_++;

	if(render_data.raylevel_ <= (r_depth_ + additional_depth))
	{
		Halton hal_2(2);
		Halton hal_3(3);

		// dispersive effects with recursive raytracing:
		if(bsdfs.hasAny(BsdfFlags::Dispersive) && render_data.chromatic_)
		{
			render_data.lights_geometry_material_emit_ = false; //debatable...
			int dsam = 8;
			const int old_division = render_data.ray_division_;
			const int old_offset = render_data.ray_offset_;
			const float old_dc_1 = render_data.dc_1_, old_dc_2 = render_data.dc_2_;
			if(render_data.ray_division_ > 1) dsam = std::max(1, dsam / old_division);
			render_data.ray_division_ *= dsam;
			int branch = render_data.ray_division_ * old_offset;
			const float d_1 = 1.f / (float)dsam;
			const float ss_1 = sample::riS(render_data.pixel_sample_ + render_data.sampling_offs_);
			Rgb dcol(0.f), vcol(1.f);
			float w = 0.f;

			Rgb dcol_trans_accum;
			for(int ns = 0; ns < dsam; ++ns)
			{
				render_data.wavelength_ = (ns + ss_1) * d_1;
				render_data.dc_1_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 1, branch + render_data.sampling_offs_);
				render_data.dc_2_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 2, branch + render_data.sampling_offs_);
				if(old_division > 1) render_data.wavelength_ = math::addMod1(render_data.wavelength_, old_dc_1);
				render_data.ray_offset_ = branch;
				++branch;
				Sample s(0.5f, 0.5f, BsdfFlags::Reflect | BsdfFlags::Transmit | BsdfFlags::Dispersive);
				Vec3 wi;
				const Rgb mcol = material->sample(render_data, sp, wo, wi, s, w);

				if(s.pdf_ > 1.0e-6f && s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
				{
					render_data.chromatic_ = false;
					Rgb wl_col;
					wl2Rgb_global(render_data.wavelength_, wl_col);
					const DiffRay ref_ray(sp.p_, wi, scene_->ray_min_dist_);
					const Rgb dcol_trans = static_cast<Rgb>(integrate(render_data, ref_ray, additional_depth, nullptr, nullptr)) * mcol * wl_col * w;
					dcol += dcol_trans;
					if(layers_used) dcol_trans_accum += dcol_trans;

					const VolumeHandler *vol;
					if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
					{
						vol->transmittance(render_data, ref_ray, vcol);
						dcol *= vcol;
					}
					render_data.chromatic_ = true;
				}
			}
			col += dcol * d_1;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Trans))
				{
					dcol_trans_accum *= d_1;
					color_layer->color_ += dcol_trans_accum;
				}
			}

			render_data.ray_division_ = old_division;
			render_data.ray_offset_ = old_offset;
			render_data.dc_1_ = old_dc_1;
			render_data.dc_2_ = old_dc_2;
		}

		// glossy reflection with recursive raytracing:
		if(bsdfs.hasAny(BsdfFlags::Glossy) && render_data.raylevel_ < 20)
		{
			render_data.lights_geometry_material_emit_ = true;
			int gsam = 8;
			const int old_division = render_data.ray_division_;
			const int old_offset = render_data.ray_offset_;
			const float old_dc_1 = render_data.dc_1_, old_dc_2 = render_data.dc_2_;
			if(render_data.ray_division_ > 1) gsam = std::max(1, gsam / old_division);
			render_data.ray_division_ *= gsam;
			int branch = render_data.ray_division_ * old_offset;
			unsigned int offs = gsam * render_data.pixel_sample_ + render_data.sampling_offs_;
			const float d_1 = 1.f / (float)gsam;
			Rgb gcol(0.f), vcol(1.f);

			hal_2.setStart(offs);
			hal_3.setStart(offs);

			Rgb gcol_indirect_accum;
			Rgb gcol_reflect_accum;
			Rgb gcol_transmit_accum;

			for(int ns = 0; ns < gsam; ++ns)
			{
				render_data.dc_1_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 1, branch + render_data.sampling_offs_);
				render_data.dc_2_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 2, branch + render_data.sampling_offs_);
				render_data.ray_offset_ = branch;
				++offs;
				++branch;

				const float s_1 = hal_2.getNext();
				const float s_2 = hal_3.getNext();

				if(material->getFlags().hasAny(BsdfFlags::Glossy))
				{
					if(material->getFlags().hasAny(BsdfFlags::Reflect) && !material->getFlags().hasAny(BsdfFlags::Transmit))
					{
						float w = 0.f;
						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Reflect);
						Vec3 wi;
						const Rgb mcol = material->sample(render_data, sp, wo, wi, s, w);
						DiffRay ref_ray(sp.p_, wi, scene_->ray_min_dist_);
						if(diff_rays_enabled_)
						{
							if(s.sampled_flags_.hasAny(BsdfFlags::Reflect)) sp_diff.reflectedRay(ray, ref_ray);
							else if(s.sampled_flags_.hasAny(BsdfFlags::Transmit)) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
						}
						Rgba integ = static_cast<Rgb>(integrate(render_data, ref_ray, additional_depth, nullptr, nullptr));
						const VolumeHandler *vol;
						if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
						{
							if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
						}
						const Rgb g_ind_col = static_cast<Rgb>(integ) * mcol * w;
						gcol += g_ind_col;
						if(layers_used) gcol_indirect_accum += g_ind_col;
					}
					else if(material->getFlags().hasAny(BsdfFlags::Reflect) && material->getFlags().hasAny(BsdfFlags::Transmit))
					{
						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::AllGlossy);
						Rgb mcol[2];
						float w[2];
						Vec3 dir[2];

						mcol[0] = material->sample(render_data, sp, wo, dir, mcol[1], s, w);

						if(s.sampled_flags_.hasAny(BsdfFlags::Reflect) && !s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
						{
							DiffRay ref_ray = DiffRay(sp.p_, dir[0], scene_->ray_min_dist_);
							if(diff_rays_enabled_) sp_diff.reflectedRay(ray, ref_ray);
							Rgba integ = integrate(render_data, ref_ray, additional_depth, nullptr, nullptr);
							const VolumeHandler *vol;
							if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
							{
								if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
							}
							const Rgb col_reflect_factor = mcol[0] * w[0];
							const Rgb g_ind_col = static_cast<Rgb>(integ) * col_reflect_factor;
							gcol += g_ind_col;
							if(layers_used) gcol_reflect_accum += g_ind_col;
						}

						if(s.sampled_flags_.hasAny(BsdfFlags::Transmit))
						{
							DiffRay ref_ray = DiffRay(sp.p_, dir[1], scene_->ray_min_dist_);
							if(diff_rays_enabled_) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
							Rgba integ = integrate(render_data, ref_ray, additional_depth, nullptr, nullptr);
							const VolumeHandler *vol;
							if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
							{
								if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
							}
							const Rgb col_transmit_factor = mcol[1] * w[1];
							const Rgb g_ind_col = static_cast<Rgb>(integ) * col_transmit_factor;
							gcol += g_ind_col;
							if(layers_used) gcol_transmit_accum += g_ind_col;
							alpha = integ.a_;
						}
					}
				}
			}

			col += gcol * d_1;

			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::GlossyIndirect))
				{
					gcol_indirect_accum *= d_1;
					color_layer->color_ += gcol_indirect_accum;
				}
				if(ColorLayer *color_layer = color_layers->find(Layer::Trans))
				{
					gcol_reflect_accum *= d_1;
					color_layer->color_ += gcol_reflect_accum;
				}
				if(ColorLayer *color_layer = color_layers->find(Layer::GlossyIndirect))
				{
					gcol_transmit_accum *= d_1;
					color_layer->color_ += gcol_transmit_accum;
				}
			}

			render_data.ray_division_ = old_division;
			render_data.ray_offset_ = old_offset;
			render_data.dc_1_ = old_dc_1;
			render_data.dc_2_ = old_dc_2;
		}

		//...perfect specular reflection/refraction with recursive raytracing...
		if(bsdfs.hasAny((BsdfFlags::Specular | BsdfFlags::Filter)) && render_data.raylevel_ < 20)
		{
			render_data.lights_geometry_material_emit_ = true;
			const Material::Specular specular = material->getSpecular(render_data, sp, wo);
			if(specular.reflect_.enabled_)
			{
				DiffRay ref_ray(sp.p_, specular.reflect_.dir_, scene_->ray_min_dist_);
				if(diff_rays_enabled_) sp_diff.reflectedRay(ray, ref_ray);
				Rgb integ = integrate(render_data, ref_ray, additional_depth, nullptr, nullptr);
				const VolumeHandler *vol;
				Rgb vcol;
				if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
				{
					if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
				}
				const Rgb col_ind = static_cast<Rgb>(integ) * specular.reflect_.col_;
				col += col_ind;
				if(layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::ReflectPerfect)) color_layer->color_ += col_ind;
				}
			}
			if(specular.refract_.enabled_)
			{
				DiffRay ref_ray;
				float transp_bias_factor = material->getTransparentBiasFactor();
				if(transp_bias_factor > 0.f)
				{
					const bool transpbias_multiply_raydepth = material->getTransparentBiasMultiplyRayDepth();
					if(transpbias_multiply_raydepth) transp_bias_factor *= render_data.raylevel_;
					ref_ray = DiffRay(sp.p_ + specular.refract_.dir_ * transp_bias_factor, specular.refract_.dir_, scene_->ray_min_dist_);
				}
				else ref_ray = DiffRay(sp.p_, specular.refract_.dir_, scene_->ray_min_dist_);

				if(diff_rays_enabled_) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
				Rgba integ = integrate(render_data, ref_ray, additional_depth, nullptr, nullptr);

				const VolumeHandler *vol;
				Rgb vcol;
				if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
				{
					if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
				}
				const Rgb col_ind = static_cast<Rgb>(integ) * specular.refract_.col_;
				col += col_ind;
				if(layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::RefractPerfect)) color_layer->color_ += col_ind;
				}
				alpha = integ.a_;
			}
		}
	}
	--render_data.raylevel_;
}

Rgb MonteCarloIntegrator::sampleAmbientOcclusion(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f);
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	light_ray.dir_ = Vec3(0.f);
	float mask_obj_index = 0.f, mask_mat_index = 0.f;
	int n = ao_samples_;//(int) ceilf(aoSamples*getSampleMultiplier());
	if(render_data.ray_division_ > 1) n = std::max(1, n / render_data.ray_division_);
	const unsigned int offs = n * render_data.pixel_sample_ + render_data.sampling_offs_;
	Halton hal_2(2, offs - 1);
	Halton hal_3(3, offs - 1);
	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();
		if(render_data.ray_division_ > 1)
		{
			s_1 = math::addMod1(s_1, render_data.dc_1_);
			s_2 = math::addMod1(s_2, render_data.dc_2_);
		}
		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
		else light_ray.tmin_ = scene_->shadow_bias_;
		light_ray.tmax_ = ao_dist_;
		float w = 0.f;
		Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Reflect);
		const Rgb surf_col = material->sample(render_data, sp, wo, light_ray.dir_, s, w);
		if(material->getFlags().hasAny(BsdfFlags::Emit))
		{
			col += material->emit(render_data, sp, wo) * s.pdf_;
		}
		Rgb scol;
		const bool shadowed = (tr_shad_) ? scene_->isShadowed(render_data, light_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, light_ray, mask_obj_index, mask_mat_index);
		if(!shadowed)
		{
			const float cos = std::abs(sp.n_ * light_ray.dir_);
			if(tr_shad_) col += ao_col_ * scol * surf_col * cos * w;
			else col += ao_col_ * surf_col * cos * w;
		}
	}
	return col / static_cast<float>(n);
}

Rgb MonteCarloIntegrator::sampleAmbientOcclusionLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f);
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	light_ray.dir_ = Vec3(0.f);
	float mask_obj_index = 0.f, mask_mat_index = 0.f;
	int n = ao_samples_;//(int) ceilf(aoSamples*getSampleMultiplier());
	if(render_data.ray_division_ > 1) n = std::max(1, n / render_data.ray_division_);
	const unsigned int offs = n * render_data.pixel_sample_ + render_data.sampling_offs_;
	Halton hal_2(2, offs - 1);
	Halton hal_3(3, offs - 1);
	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();
		if(render_data.ray_division_ > 1)
		{
			s_1 = math::addMod1(s_1, render_data.dc_1_);
			s_2 = math::addMod1(s_2, render_data.dc_2_);
		}
		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
		else light_ray.tmin_ = scene_->shadow_bias_;
		light_ray.tmax_ = ao_dist_;
		float w = 0.f;
		Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Reflect);
		const Rgb surf_col = material->sample(render_data, sp, wo, light_ray.dir_, s, w);
		if(material->getFlags().hasAny(BsdfFlags::Emit))
		{
			col += material->emit(render_data, sp, wo) * s.pdf_;
		}
		const bool shadowed = scene_->isShadowed(render_data, light_ray, mask_obj_index, mask_mat_index);
		if(!shadowed)
		{
			float cos = std::abs(sp.n_ * light_ray.dir_);
			//if(trShad) col += aoCol * scol * surfCol * cos * W;
			col += ao_col_ * surf_col * cos * w;
		}
	}
	return col / static_cast<float>(n);
}

Rgb MonteCarloIntegrator::sampleAmbientOcclusionClayLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f);
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	light_ray.dir_ = Vec3(0.f);
	float mask_obj_index = 0.f, mask_mat_index = 0.f;
	int n = ao_samples_;
	if(render_data.ray_division_ > 1) n = std::max(1, n / render_data.ray_division_);
	const unsigned int offs = n * render_data.pixel_sample_ + render_data.sampling_offs_;
	Halton hal_2(2, offs - 1);
	Halton hal_3(3, offs - 1);
	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();
		if(render_data.ray_division_ > 1)
		{
			s_1 = math::addMod1(s_1, render_data.dc_1_);
			s_2 = math::addMod1(s_2, render_data.dc_2_);
		}
		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
		else light_ray.tmin_ = scene_->shadow_bias_;
		light_ray.tmax_ = ao_dist_;
		float w = 0.f;
		Sample s(s_1, s_2, BsdfFlags::All);
		const Rgb surf_col = material->sampleClay(render_data, sp, wo, light_ray.dir_, s, w);
		s.pdf_ = 1.f;
		if(material->getFlags().hasAny(BsdfFlags::Emit))
		{
			col += material->emit(render_data, sp, wo) * s.pdf_;
		}
		const bool shadowed = scene_->isShadowed(render_data, light_ray, mask_obj_index, mask_mat_index);
		if(!shadowed)
		{
			float cos = std::abs(sp.n_ * light_ray.dir_);
			//if(trShad) col += aoCol * scol * surfCol * cos * W;
			col += ao_col_ * surf_col * cos * w;
		}
	}
	return col / static_cast<float>(n);
}

END_YAFARAY
