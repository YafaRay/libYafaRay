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

#include "integrator/integrator_montecarlo.h"
#include "common/surface.h"
#include "common/logging.h"
#include "common/renderpasses.h"
#include "material/material.h"
#include "scene/scene.h"
#include "volume/volume.h"
#include "common/session.h"
#include "light/light.h"
#include "common/scr_halton.h"
#include "common/spectrum.h"
#include "utility/util_mcqmc.h"
#include "common/imagefilm.h"
#include "common/monitor.h"
#include "photon/photon.h"
#include "utility/util_sample.h"

#ifdef __clang__
#define inline  // aka inline removal
#endif

BEGIN_YAFARAY

static constexpr int loffs_delta__ = 4567; //just some number to have different sequences per light...and it's a prime even...

Rgb MonteCarloIntegrator::estimateAllDirectLight(RenderState &state, const SurfacePoint &sp, const Vec3 &wo, IntPasses *int_passes) const
{
	const bool int_passes_used = state.raylevel_ == 0 && int_passes && int_passes->size() > 1;

	Rgb col;
	unsigned int loffs = 0;
	for(auto l = lights_.begin(); l != lights_.end(); ++l)
	{
		col += doLightEstimation(state, (*l), sp, wo, loffs, int_passes);
		loffs++;
	}

	if(int_passes_used)
	{
		if(Rgba *color_pass = int_passes->find(PassShadow)) *color_pass *= (1.f / (float) loffs);
	}

	return col;
}

Rgb MonteCarloIntegrator::estimateOneDirectLight(RenderState &state, const SurfacePoint &sp, Vec3 wo, int n) const
{
	const int light_num = lights_.size();

	if(light_num == 0) return Rgb(0.f); //??? if you get this far the lights must be >= 1 but, what the hell... :)

	Halton hal_2(2);

	hal_2.setStart(image_film_->getBaseSamplingOffset() + correlative_sample_number_[state.thread_id_] - 1); //Probably with this change the parameter "n" is no longer necessary, but I will keep it just in case I have to revert back this change!
	int lnum = std::min((int)(hal_2.getNext() * (float)light_num), light_num - 1);

	++correlative_sample_number_[state.thread_id_];

	return doLightEstimation(state, lights_[lnum], sp, wo, lnum) * light_num;
}

Rgb MonteCarloIntegrator::doLightEstimation(RenderState &state, Light *light, const SurfacePoint &sp, const Vec3 &wo, const unsigned int  &loffs, IntPasses *int_passes) const
{
	const bool int_passes_used = state.raylevel_ == 0 && int_passes && int_passes->size() > 1;

	Rgb col(0.f);
	bool shadowed;
	unsigned int l_offs = loffs * loffs_delta__;
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	Rgb lcol(0.f), scol;
	float light_pdf;
	float mask_obj_index = 0.f, mask_mat_index = 0.f;
	const PassesSettings *passes_settings = scene_->getPassesSettings();
	const PassMaskParams mask_params = passes_settings->passMaskParams();

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

			if(cast_shadows) shadowed = (tr_shad_) ? scene_->isShadowed(state, light_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(state, light_ray, mask_obj_index, mask_mat_index);
			else shadowed = false;

			const float angle_light_normal = (material->isFlat() ? 1.f : std::fabs(sp.n_ * light_ray.dir_));	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal

			if(!shadowed || (int_passes_used && int_passes->enabled(PassDiffuseNoShadow)))
			{
				if(!shadowed && int_passes_used && int_passes->enabled(PassShadow)) col_shadow += Rgb(1.f);

				const Rgb surf_col = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::All);
				const Rgb transmit_col = scene_->vol_integrator_->transmittance(state, light_ray);
				const Rgba tmp_col_no_shadow = surf_col * lcol * angle_light_normal * transmit_col;
				if(tr_shad_ && cast_shadows) lcol *= scol;

				if(int_passes_used)
				{
					if(int_passes->enabled(PassDiffuse) || int_passes->enabled(PassDiffuseNoShadow))
					{
						col_diff_no_shadow += tmp_col_no_shadow;
						if(!shadowed) col_diff_dir += material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * lcol * angle_light_normal * transmit_col;
					}

					if(int_passes->enabled(PassGlossy))
					{
						if(!shadowed) col_glossy_dir += material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * lcol * angle_light_normal * transmit_col;
					}
				}

				if(!shadowed) col += surf_col * lcol * angle_light_normal * transmit_col;
			}

			if(shadowed && int_passes_used)
			{
				if(int_passes->enabled(PassMatIndexMaskShadow) && mask_mat_index == mask_params.mat_index_) col_shadow_mat_mask += Rgb(1.f);
				if(int_passes->enabled(PassObjIndexMaskShadow) && mask_obj_index == mask_params.obj_index_) col_shadow_obj_mask += Rgb(1.f);
			}
		}
		if(int_passes_used)
		{
			if(Rgba *color_pass = int_passes->find(PassShadow)) *color_pass += col_shadow;
			if(Rgba *color_pass = int_passes->find(PassMatIndexMaskShadow)) *color_pass += col_shadow_mat_mask;
			if(Rgba *color_pass = int_passes->find(PassObjIndexMaskShadow)) *color_pass += col_shadow_obj_mask;
			if(Rgba *color_pass = int_passes->find(PassDiffuse)) *color_pass += col_diff_dir;
			if(Rgba *color_pass = int_passes->find(PassDiffuseNoShadow)) *color_pass += col_diff_no_shadow;
			if(Rgba *color_pass = int_passes->find(PassGlossy)) *color_pass += col_glossy_dir;
			if(Rgba *color_pass = int_passes->find(PassDebugLightEstimationLightDirac)) *color_pass += col;
		}
	}
	else // area light and suchlike
	{
		Halton hal_2(2);
		Halton hal_3(3);
		int n = (int) ceilf(light->nSamples() * aa_light_sample_multiplier_);
		if(state.ray_division_ > 1) n = std::max(1, n / state.ray_division_);
		const float inv_ns = 1.f / (float)n;
		const unsigned int offs = n * state.pixel_sample_ + state.sampling_offs_ + l_offs;
		const bool can_intersect = light->canIntersect();
		Rgb ccol(0.0);
		LSample ls;

		hal_2.setStart(offs - 1);
		hal_3.setStart(offs - 1);

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

				if(cast_shadows) shadowed = (tr_shad_) ? scene_->isShadowed(state, light_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(state, light_ray, mask_obj_index, mask_mat_index);
				else shadowed = false;

				if((!shadowed && ls.pdf_ > 1e-6f)  || (int_passes_used && int_passes->enabled(PassDiffuseNoShadow)))
				{
					const Rgb ls_col_no_shadow = ls.col_;
					if(tr_shad_ && cast_shadows) ls.col_ *= scol;
					const Rgb transmit_col = scene_->vol_integrator_->transmittance(state, light_ray);
					ls.col_ *= transmit_col;
					const Rgb surf_col = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::All);

					if(int_passes_used && (!shadowed && ls.pdf_ > 1e-6f) && int_passes->enabled(PassShadow)) col_shadow += Rgb(1.f);

					const float angle_light_normal = (material->isFlat() ? 1.f : std::fabs(sp.n_ * light_ray.dir_));	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal

					if(can_intersect)
					{
						const float m_pdf = material->pdf(state, sp, wo, light_ray.dir_, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
						if(m_pdf > 1e-6f)
						{
							const float l_2 = ls.pdf_ * ls.pdf_;
							const float m_2 = m_pdf * m_pdf;
							const float w = l_2 / (l_2 + m_2);

							if(int_passes_used)
							{
								if(int_passes->enabled(PassDiffuse) || int_passes->enabled(PassDiffuseNoShadow))
								{
									const Rgb tmp_col_no_light_color = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal * w / ls.pdf_;
									col_diff_no_shadow += tmp_col_no_light_color * ls_col_no_shadow;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_diff_dir += tmp_col_no_light_color * ls.col_;
								}
								if(int_passes->enabled(PassGlossy))
								{
									const Rgb tmp_col = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal * w / ls.pdf_;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_glossy_dir += tmp_col;
								}
							}

							if((!shadowed && ls.pdf_ > 1e-6f)) ccol += surf_col * ls.col_ * angle_light_normal * w / ls.pdf_;
						}
						else
						{
							if(int_passes_used)
							{
								if(int_passes->enabled(PassDiffuse) || int_passes->enabled(PassDiffuseNoShadow))
								{
									const Rgb tmp_col_no_light_color = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal / ls.pdf_;
									col_diff_no_shadow += tmp_col_no_light_color * ls_col_no_shadow;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_diff_dir += tmp_col_no_light_color * ls.col_;
								}

								if(int_passes->enabled(PassGlossy))
								{
									const Rgb tmp_col = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal / ls.pdf_;
									if((!shadowed && ls.pdf_ > 1e-6f)) col_glossy_dir += tmp_col;
								}
							}

							if((!shadowed && ls.pdf_ > 1e-6f)) ccol += surf_col * ls.col_ * angle_light_normal / ls.pdf_;
						}
					}
					else
					{
						if(int_passes_used)
						{
							if(int_passes->enabled(PassDiffuse) || int_passes->enabled(PassDiffuseNoShadow))
							{
								const Rgb tmp_col_no_light_color = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal / ls.pdf_;
								col_diff_no_shadow += tmp_col_no_light_color * ls_col_no_shadow;
								if((!shadowed && ls.pdf_ > 1e-6f)) col_diff_dir += tmp_col_no_light_color * ls.col_;
							}

							if(int_passes->enabled(PassGlossy))
							{
								const Rgb tmp_col = material->eval(state, sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal / ls.pdf_;
								if((!shadowed && ls.pdf_ > 1e-6f)) col_glossy_dir += tmp_col;
							}
						}

						if((!shadowed && ls.pdf_ > 1e-6f)) ccol += surf_col * ls.col_ * angle_light_normal / ls.pdf_;
					}
				}

				if(int_passes_used && (shadowed || ls.pdf_ <= 1e-6f))
				{
					if(int_passes->enabled(PassMatIndexMaskShadow) && mask_mat_index == mask_params.mat_index_) col_shadow_mat_mask += Rgb(1.f);
					if(int_passes->enabled(PassObjIndexMaskShadow) && mask_obj_index == mask_params.obj_index_) col_shadow_obj_mask += Rgb(1.f);
				}
			}
		}

		col += ccol * inv_ns;

		if(int_passes_used)
		{
			if(Rgba *color_pass = int_passes->find(PassDebugLightEstimationLightSampling)) *color_pass += ccol * inv_ns;
			if(Rgba *color_pass = int_passes->find(PassShadow)) *color_pass += col_shadow * inv_ns;
			if(Rgba *color_pass = int_passes->find(PassMatIndexMaskShadow)) *color_pass += col_shadow_mat_mask * inv_ns;
			if(Rgba *color_pass = int_passes->find(PassObjIndexMaskShadow)) *color_pass += col_shadow_obj_mask * inv_ns;
			if(Rgba *color_pass = int_passes->find(PassDiffuse)) *color_pass += col_diff_dir * inv_ns;
			if(Rgba *color_pass = int_passes->find(PassDiffuseNoShadow)) *color_pass += col_diff_no_shadow * inv_ns;
			if(Rgba *color_pass = int_passes->find(PassGlossy)) *color_pass += col_glossy_dir * inv_ns;
		}

		if(can_intersect) // sample from BSDF to complete MIS
		{
			Rgb ccol_2(0.f);

			if(int_passes_used)
			{
				if(int_passes->enabled(PassDiffuse) || int_passes->enabled(PassDiffuseNoShadow))
				{
					col_diff_no_shadow = Rgba(0.f);
					col_diff_dir = Rgba(0.f);
				}
				if(int_passes->enabled(PassGlossy)) col_glossy_dir = Rgba(0.f);
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
				const Rgb surf_col = material->sample(state, sp, wo, b_ray.dir_, s, W);
				if(s.pdf_ > 1e-6f && light->intersect(b_ray, b_ray.tmax_, lcol, light_pdf))
				{
					if(cast_shadows) shadowed = (tr_shad_) ? scene_->isShadowed(state, b_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(state, b_ray, mask_obj_index, mask_mat_index);
					else shadowed = false;

					if((!shadowed && light_pdf > 1e-6f) || (int_passes_used && int_passes->enabled(PassDiffuseNoShadow)))
					{
						if(tr_shad_ && cast_shadows) lcol *= scol;
						const Rgb transmit_col = scene_->vol_integrator_->transmittance(state, light_ray);
						lcol *= transmit_col;
						const float l_pdf = 1.f / light_pdf;
						const float l_2 = l_pdf * l_pdf;
						const float m_2 = s.pdf_ * s.pdf_;
						const float w = m_2 / (l_2 + m_2);

						if(int_passes_used)
						{
							if(int_passes->enabled(PassDiffuse) || int_passes->enabled(PassDiffuseNoShadow))
							{
								const Rgb tmp_col = material->sample(state, sp, wo, b_ray.dir_, s, W) * lcol * w * W;
								col_diff_no_shadow += tmp_col;
								if((!shadowed && light_pdf > 1e-6f) && Material::hasFlag(s.sampled_flags_, BsdfFlags::Diffuse)) col_diff_dir += tmp_col;
							}

							if(int_passes->enabled(PassGlossy))
							{
								const Rgb tmp_col = material->sample(state, sp, wo, b_ray.dir_, s, W) * lcol * w * W;
								if((!shadowed && light_pdf > 1e-6f) && Material::hasFlag(s.sampled_flags_, BsdfFlags::Glossy)) col_glossy_dir += tmp_col;
							}
						}

						if((!shadowed && light_pdf > 1e-6f)) ccol_2 += surf_col * lcol * w * W;
					}
				}
			}

			col += ccol_2 * inv_ns;

			if(int_passes_used)
			{
				if(Rgba *color_pass = int_passes->find(PassDebugLightEstimationMatSampling)) *color_pass += ccol_2 * inv_ns;
				if(Rgba *color_pass = int_passes->find(PassDiffuse)) *color_pass += col_diff_dir * inv_ns;
				if(Rgba *color_pass = int_passes->find(PassDiffuseNoShadow)) *color_pass += col_diff_no_shadow * inv_ns;
				if(Rgba *color_pass = int_passes->find(PassGlossy)) *color_pass += col_glossy_dir * inv_ns;
			}
		}
	}

	return col;
}

void MonteCarloIntegrator::causticWorker(PhotonMap *caustic_map, int thread_id, const Scene *scene, unsigned int n_caus_photons, Pdf1D *light_power_d, int num_lights, const std::vector<Light *> &caus_lights, int caus_depth, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot)
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

	RenderState state;
	state.cam_ = scene->getCamera();
	alignas (16) unsigned char userdata[user_data_size__];
	state.userdata_ = static_cast<void *>(userdata);

	local_caustic_photons.clear();
	local_caustic_photons.reserve(n_caus_photons_thread);

	while(!done)
	{
		const unsigned int haltoncurr = curr + n_caus_photons_thread * thread_id;

		state.chromatic_ = true;
		state.wavelength_ = riS__(haltoncurr);

		s_1 = riVdC__(haltoncurr);
		s_2 = scrHalton__(2, haltoncurr);
		s_3 = scrHalton__(3, haltoncurr);
		s_4 = scrHalton__(4, haltoncurr);

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
				if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(hit->ng_ * ray.dir_ < 0)))
				{
					vol->transmittance(state, ray, vcol);
					transm = vcol;
				}
			}
			std::swap(hit, hit_2);
			Vec3 wi = -ray.dir_, wo;
			material = hit->material_;
			material->initBsdf(state, *hit, bsdfs);
			if(Material::hasFlag(bsdfs, (BsdfFlags::Diffuse | BsdfFlags::Glossy)))
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

			s_5 = scrHalton__(d_5, haltoncurr);
			s_6 = scrHalton__(d_5 + 1, haltoncurr);
			s_7 = scrHalton__(d_5 + 2, haltoncurr);

			PSample sample(s_5, s_6, s_7, BsdfFlags::AllSpecular | BsdfFlags::Glossy | BsdfFlags::Filter | BsdfFlags::Dispersive, pcol, transm);
			bool scattered = material->scatterPhoton(state, *hit, wi, wo, sample);
			if(!scattered) break; //photon was absorped.
			pcol = sample.color_;
			// hm...dispersive is not really a scattering qualifier like specular/glossy/diffuse or the special case filter...
			caustic_photon = (Material::hasFlag(sample.sampled_flags_, (BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (Material::hasFlag(sample.sampled_flags_, (BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
			// light through transparent materials can be calculated by direct lighting, so still consider them direct!
			direct_photon = Material::hasFlag(sample.sampled_flags_, BsdfFlags::Filter) && direct_photon;
			// caustic-only calculation can be stopped if:
			if(!(caustic_photon || direct_photon)) break;

			if(state.chromatic_ && Material::hasFlag(sample.sampled_flags_, BsdfFlags::Dispersive))
			{
				state.chromatic_ = false;
				Rgb wl_col;
				wl2Rgb__(state.wavelength_, wl_col);
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
			if(scene->getSignals() & Y_SIG_ABORT) { return; }
		}
		done = (curr >= n_caus_photons_thread);
	}
	caustic_map->mutx_.lock();
	caustic_map->appendVector(local_caustic_photons, curr);
	total_photons_shot += curr;
	caustic_map->mutx_.unlock();
}

bool MonteCarloIntegrator::createCausticMap()
{
	ProgressBar *pb;
	if(intpb_) pb = intpb_;
	else pb = new ConsoleProgressBar(80);

	if(photon_map_processing_ == PhotonsLoad)
	{
		pb->setTag("Loading caustic photon map from file...");
		const std::string filename = session__.getPathImageOutput() + "_caustic.photonmap";
		Y_INFO << getName() << ": Loading caustic photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
		if(session__.caustic_map_->load(filename))
		{
			Y_VERBOSE << getName() << ": Caustic map loaded." << YENDL;
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
		if(session__.caustic_map_->nPhotons() == 0)
		{
			photon_map_processing_ = PhotonsGenerateOnly;
			Y_WARNING << getName() << ": One of the photon maps in memory was empty, they cannot be reused: changing to Generate mode." << YENDL;
		}
		else return true;
	}

	session__.caustic_map_->clear();
	session__.caustic_map_->setNumPaths(0);
	session__.caustic_map_->reserveMemory(n_caus_photons_);
	session__.caustic_map_->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

	Ray ray;
	std::vector<Light *> caus_lights;

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
		float *energies = new float[num_lights];
		for(int i = 0; i < num_lights; ++i) energies[i] = caus_lights[i]->totalEnergy().energy();
		Pdf1D *light_power_d = new Pdf1D(energies, num_lights);

		Y_VERBOSE << getName() << ": Light(s) photon color testing for caustics map:" << YENDL;

		for(int i = 0; i < num_lights; ++i)
		{
			Rgb pcol(0.f);
			pcol = caus_lights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			light_num_pdf = light_power_d->func_[i] * light_power_d->inv_integral_;
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			Y_VERBOSE << getName() << ": Light [" << i + 1 << "] Photon col:" << pcol << " | lnpdf: " << light_num_pdf << YENDL;
		}

		delete[] energies;

		int pb_step;
		Y_INFO << getName() << ": Building caustics photon map..." << YENDL;
		pb->init(128);
		pb_step = std::max(1U, n_caus_photons_ / 128);
		pb->setTag("Building caustics photon map...");

		unsigned int curr = 0;

		const int n_threads = scene_->getNumThreadsPhotons();

		n_caus_photons_ = std::max((unsigned int) n_threads, (n_caus_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		Y_PARAMS << getName() << ": Shooting " << n_caus_photons_ << " photons across " << n_threads << " threads (" << (n_caus_photons_ / n_threads) << " photons/thread)" << YENDL;

		if(n_threads >= 2)
		{
			std::vector<std::thread> threads;
			for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&MonteCarloIntegrator::causticWorker, this, session__.caustic_map_, i, scene_, n_caus_photons_, light_power_d, num_lights, caus_lights, caus_depth_, pb, pb_step, std::ref(curr)));
			for(auto &t : threads) t.join();
		}
		else
		{
			bool done = false;
			float s_1, s_2, s_3, s_4, s_5, s_6, s_7, s_l;
			SurfacePoint sp_1, sp_2;
			SurfacePoint *hit = &sp_1, *hit_2 = &sp_2;

			RenderState state;
			state.cam_ = scene_->getCamera();
			alignas (16) unsigned char userdata[user_data_size__];
			state.userdata_ = static_cast<void *>(userdata);

			while(!done)
			{
				if(scene_->getSignals() & Y_SIG_ABORT) { pb->done(); if(!intpb_) delete pb; return false; }
				state.chromatic_ = true;
				state.wavelength_ = riS__(curr);
				s_1 = riVdC__(curr);
				s_2 = scrHalton__(2, curr);
				s_3 = scrHalton__(3, curr);
				s_4 = scrHalton__(4, curr);

				s_l = float(curr) / float(n_caus_photons_);

				const int light_num = light_power_d->dSample(s_l, &light_num_pdf);

				if(light_num >= num_lights)
				{
					Y_ERROR << getName() << ": lightPDF sample error! " << s_l << "/" << light_num << YENDL;
					delete light_power_d;
					return false;
				}

				Rgb pcol = caus_lights[light_num]->emitPhoton(s_1, s_2, s_3, s_4, ray, light_pdf);
				ray.tmin_ = scene_->ray_min_dist_;
				ray.tmax_ = -1.0;
				pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of th pdf, hence *=...
				if(pcol.isBlack())
				{
					++curr;
					done = (curr >= n_caus_photons_);
					continue;
				}
				BsdfFlags bsdfs = BsdfFlags::None;
				int n_bounces = 0;
				bool caustic_photon = false;
				bool direct_photon = true;
				const Material *material = nullptr;
				const VolumeHandler *vol = nullptr;

				while(scene_->intersect(ray, *hit_2))
				{
					if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
					{
						Y_WARNING << getName() << ": NaN (photon color)" << YENDL;
						break;
					}
					Rgb transm(1.f), vcol;
					// check for volumetric effects
					if(material)
					{
						if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(hit->ng_ * ray.dir_ < 0)))
						{
							vol->transmittance(state, ray, vcol);
							transm = vcol;
						}

					}
					std::swap(hit, hit_2);
					Vec3 wi = -ray.dir_, wo;
					material = hit->material_;
					material->initBsdf(state, *hit, bsdfs);
					if(Material::hasFlag(bsdfs, BsdfFlags::Diffuse | BsdfFlags::Glossy))
					{
						//deposit caustic photon on surface
						if(caustic_photon)
						{
							Photon np(wi, hit->p_, pcol);
							session__.caustic_map_->pushPhoton(np);
							session__.caustic_map_->setNumPaths(curr);
						}
					}
					// need to break in the middle otherwise we scatter the photon and then discard it => redundant
					if(n_bounces == caus_depth_) break;
					// scatter photon
					const int d_5 = 3 * n_bounces + 5;
					//int d6 = d5 + 1;

					s_5 = scrHalton__(d_5, curr);
					s_6 = scrHalton__(d_5 + 1, curr);
					s_7 = scrHalton__(d_5 + 2, curr);

					PSample sample(s_5, s_6, s_7, BsdfFlags::AllSpecular | BsdfFlags::Glossy | BsdfFlags::Filter | BsdfFlags::Dispersive, pcol, transm);
					const bool scattered = material->scatterPhoton(state, *hit, wi, wo, sample);
					if(!scattered) break; //photon was absorped.
					pcol = sample.color_;
					// hm...dispersive is not really a scattering qualifier like specular/glossy/diffuse or the special case filter...
					caustic_photon = (Material::hasFlag(sample.sampled_flags_, (BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
									 (Material::hasFlag(sample.sampled_flags_, (BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
					// light through transparent materials can be calculated by direct lighting, so still consider them direct!
					direct_photon = Material::hasFlag(sample.sampled_flags_, BsdfFlags::Filter) && direct_photon;
					// caustic-only calculation can be stopped if:
					if(!(caustic_photon || direct_photon)) break;

					if(state.chromatic_ && Material::hasFlag(sample.sampled_flags_, BsdfFlags::Dispersive))
					{
						state.chromatic_ = false;
						Rgb wl_col;
						wl2Rgb__(state.wavelength_, wl_col);
						pcol *= wl_col;
					}
					ray.from_ = hit->p_;
					ray.dir_ = wo;
					ray.tmin_ = scene_->ray_min_dist_;
					ray.tmax_ = -1.0;
					++n_bounces;
				}
				++curr;
				if(curr % pb_step == 0) pb->update();
				done = (curr >= n_caus_photons_);
			}
		}

		pb->done();
		pb->setTag("Caustic photon map built.");
		Y_VERBOSE << getName() << ": Done." << YENDL;
		Y_INFO << getName() << ": Shot " << curr << " caustic photons from " << num_lights << " light(s)." << YENDL;
		Y_VERBOSE << getName() << ": Stored caustic photons: " << session__.caustic_map_->nPhotons() << YENDL;

		delete light_power_d;

		if(session__.caustic_map_->nPhotons() > 0)
		{
			pb->setTag("Building caustic photons kd-tree...");
			session__.caustic_map_->updateTree();
			Y_VERBOSE << getName() << ": Done." << YENDL;
		}

		if(photon_map_processing_ == PhotonsGenerateAndSave)
		{
			pb->setTag("Saving caustic photon map to file...");
			std::string filename = session__.getPathImageOutput() + "_caustic.photonmap";
			Y_INFO << getName() << ": Saving caustic photon map to: " << filename << YENDL;
			if(session__.caustic_map_->save(filename)) Y_VERBOSE << getName() << ": Caustic map saved." << YENDL;
		}
	}
	else
	{
		Y_VERBOSE << getName() << ": No caustic source lights found, skiping caustic map building..." << YENDL;
	}

	if(!intpb_) delete pb;
	return true;
}

Rgb MonteCarloIntegrator::estimateCausticPhotons(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	if(!session__.caustic_map_->ready()) return Rgb(0.f);

	FoundPhoton *gathered = new FoundPhoton[n_caus_search_];//(foundPhoton_t *)alloca(nCausSearch * sizeof(foundPhoton_t));
	int n_gathered = 0;

	float g_radius_square = caus_radius_ * caus_radius_;

	n_gathered = session__.caustic_map_->gather(sp.p_, gathered, n_caus_search_, g_radius_square);

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
			surf_col = material->eval(state, sp, wo, photon->direction(), BsdfFlags::All);
			k = kernel__(gathered[i].dist_square_, g_radius_square);
			sum += surf_col * k * photon->color();
		}
		sum *= 1.f / (float(session__.caustic_map_->nPaths()));
	}

	delete [] gathered;

	return sum;
}

void MonteCarloIntegrator::recursiveRaytrace(RenderState &state, DiffRay &ray, BsdfFlags bsdfs, SurfacePoint &sp, Vec3 &wo, Rgb &col, float &alpha, int additional_depth, IntPasses *int_passes) const
{
	const bool int_passes_used = state.raylevel_ == 0 && int_passes && int_passes->size() > 1;

	const Material *material = sp.material_;
	SpDifferentials sp_diff(sp, ray);

	state.raylevel_++;

	if(state.raylevel_ <= (r_depth_ + additional_depth))
	{
		Halton hal_2(2);
		Halton hal_3(3);

		// dispersive effects with recursive raytracing:
		if(Material::hasFlag(bsdfs, BsdfFlags::Dispersive) && state.chromatic_)
		{
			state.include_lights_ = false; //debatable...
			int dsam = 8;
			const int old_division = state.ray_division_;
			const int old_offset = state.ray_offset_;
			const float old_dc_1 = state.dc_1_, old_dc_2 = state.dc_2_;
			if(state.ray_division_ > 1) dsam = std::max(1, dsam / old_division);
			state.ray_division_ *= dsam;
			int branch = state.ray_division_ * old_offset;
			const float d_1 = 1.f / (float)dsam;
			const float ss_1 = riS__(state.pixel_sample_ + state.sampling_offs_);
			Rgb dcol(0.f), vcol(1.f);
			Vec3 wi;
			const VolumeHandler *vol;
			DiffRay ref_ray;
			float w = 0.f;

			Rgb dcol_trans_accum;
			for(int ns = 0; ns < dsam; ++ns)
			{
				state.wavelength_ = (ns + ss_1) * d_1;
				state.dc_1_ = scrHalton__(2 * state.raylevel_ + 1, branch + state.sampling_offs_);
				state.dc_2_ = scrHalton__(2 * state.raylevel_ + 2, branch + state.sampling_offs_);
				if(old_division > 1) state.wavelength_ = addMod1__(state.wavelength_, old_dc_1);
				state.ray_offset_ = branch;
				++branch;
				Sample s(0.5f, 0.5f, BsdfFlags::Reflect | BsdfFlags::Transmit | BsdfFlags::Dispersive);
				const Rgb mcol = material->sample(state, sp, wo, wi, s, w);

				if(s.pdf_ > 1.0e-6f && Material::hasFlag(s.sampled_flags_, BsdfFlags::Dispersive))
				{
					state.chromatic_ = false;
					Rgb wl_col;
					wl2Rgb__(state.wavelength_, wl_col);
					ref_ray = DiffRay(sp.p_, wi, scene_->ray_min_dist_);

					const Rgb dcol_trans = (Rgb) integrate(state, ref_ray, additional_depth, nullptr) * mcol * wl_col * w;
					dcol += dcol_trans;
					if(int_passes_used) dcol_trans_accum += dcol_trans;

					state.chromatic_ = true;
				}
			}

			if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
			{
				vol->transmittance(state, ref_ray, vcol);
				dcol *= vcol;
				//if(tmpColorPasses.size() > 1) tmpColorPasses *= vcol; //FIXME DAVID??
			}

			col += dcol * d_1;
			if(int_passes_used)
			{
				if(Rgba *color_pass = int_passes->find(PassTrans))
				{
					dcol_trans_accum *= d_1;
					*color_pass += dcol_trans_accum;
				}
			}

			state.ray_division_ = old_division;
			state.ray_offset_ = old_offset;
			state.dc_1_ = old_dc_1;
			state.dc_2_ = old_dc_2;
		}

		// glossy reflection with recursive raytracing:
		if(Material::hasFlag(bsdfs, BsdfFlags::Glossy) && state.raylevel_ < 20)
		{
			state.include_lights_ = true;
			int gsam = 8;
			const int old_division = state.ray_division_;
			const int old_offset = state.ray_offset_;
			const float old_dc_1 = state.dc_1_, old_dc_2 = state.dc_2_;
			if(state.ray_division_ > 1) gsam = std::max(1, gsam / old_division);
			state.ray_division_ *= gsam;
			int branch = state.ray_division_ * old_offset;
			unsigned int offs = gsam * state.pixel_sample_ + state.sampling_offs_;
			const float d_1 = 1.f / (float)gsam;
			Rgb gcol(0.f), vcol(1.f);
			Vec3 wi;
			const VolumeHandler *vol;
			DiffRay ref_ray;

			hal_2.setStart(offs);
			hal_3.setStart(offs);

			Rgb gcol_indirect_accum;
			Rgb gcol_reflect_accum;
			Rgb gcol_transmit_accum;

			for(int ns = 0; ns < gsam; ++ns)
			{
				state.dc_1_ = scrHalton__(2 * state.raylevel_ + 1, branch + state.sampling_offs_);
				state.dc_2_ = scrHalton__(2 * state.raylevel_ + 2, branch + state.sampling_offs_);
				state.ray_offset_ = branch;
				++offs;
				++branch;

				const float s_1 = hal_2.getNext();
				const float s_2 = hal_3.getNext();

				if(Material::hasFlag(material->getFlags(), BsdfFlags::Glossy))
				{
					if(Material::hasFlag(material->getFlags(), BsdfFlags::Reflect) && !Material::hasFlag(material->getFlags(), BsdfFlags::Transmit))
					{
						float w = 0.f;

						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Reflect);
						const Rgb mcol = material->sample(state, sp, wo, wi, s, w);
						Rgba integ = 0.f;
						ref_ray = DiffRay(sp.p_, wi, scene_->ray_min_dist_);
						if(diff_rays_enabled_)
						{
							if(Material::hasFlag(s.sampled_flags_, BsdfFlags::Reflect)) sp_diff.reflectedRay(ray, ref_ray);
							else if(Material::hasFlag(s.sampled_flags_, BsdfFlags::Transmit)) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
						}
						integ = (Rgb) integrate(state, ref_ray, additional_depth, nullptr);

						if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
						{
							if(vol->transmittance(state, ref_ray, vcol)) integ *= vcol;
						}

						const Rgb g_ind_col = (Rgb) integ * mcol * w;
						gcol += g_ind_col;
						if(int_passes_used) gcol_indirect_accum += g_ind_col;
					}
					else if(Material::hasFlag(material->getFlags(), BsdfFlags::Reflect) && Material::hasFlag(material->getFlags(), BsdfFlags::Transmit))
					{
						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::AllGlossy);
						Rgb mcol[2];
						float w[2];
						Vec3 dir[2];

						mcol[0] = material->sample(state, sp, wo, dir, mcol[1], s, w);
						Rgba integ = 0.f;

						if(Material::hasFlag(s.sampled_flags_, BsdfFlags::Reflect) && !Material::hasFlag(s.sampled_flags_, BsdfFlags::Dispersive))
						{
							ref_ray = DiffRay(sp.p_, dir[0], scene_->ray_min_dist_);
							if(diff_rays_enabled_) sp_diff.reflectedRay(ray, ref_ray);
							integ = integrate(state, ref_ray, additional_depth, nullptr);
							if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
							{
								if(vol->transmittance(state, ref_ray, vcol)) integ *= vcol;
							}
							const Rgb col_reflect_factor = mcol[0] * w[0];
							const Rgb g_ind_col = (Rgb) integ * col_reflect_factor;
							gcol += g_ind_col;
							if(int_passes_used) gcol_reflect_accum += g_ind_col;
						}

						if(Material::hasFlag(s.sampled_flags_, BsdfFlags::Transmit))
						{
							ref_ray = DiffRay(sp.p_, dir[1], scene_->ray_min_dist_);
							if(diff_rays_enabled_) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
							integ = integrate(state, ref_ray, additional_depth, nullptr);
							if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
							{
								if(vol->transmittance(state, ref_ray, vcol)) integ *= vcol;
							}

							const Rgb col_transmit_factor = mcol[1] * w[1];
							const Rgb g_ind_col = (Rgb) integ * col_transmit_factor;
							gcol += g_ind_col;
							if(int_passes_used) gcol_transmit_accum += g_ind_col;
							alpha = integ.a_;
						}
					}
				}
			}

			col += gcol * d_1;

			if(int_passes_used)
			{
				if(Rgba *color_pass = int_passes->find(PassGlossyIndirect))
				{
					gcol_indirect_accum *= d_1;
					*color_pass += gcol_indirect_accum;
				}
				if(Rgba *color_pass = int_passes->find(PassTrans))
				{
					gcol_reflect_accum *= d_1;
					*color_pass += gcol_reflect_accum;
				}
				if(Rgba *color_pass = int_passes->find(PassGlossyIndirect))
				{
					gcol_transmit_accum *= d_1;
					*color_pass += gcol_transmit_accum;
				}
			}

			state.ray_division_ = old_division;
			state.ray_offset_ = old_offset;
			state.dc_1_ = old_dc_1;
			state.dc_2_ = old_dc_2;
		}

		//...perfect specular reflection/refraction with recursive raytracing...
		if(Material::hasFlag(bsdfs, (BsdfFlags::Specular | BsdfFlags::Filter)) && state.raylevel_ < 20)
		{
			state.include_lights_ = true;
			bool reflect = false, refract = false;
			Vec3 dir[2];
			Rgb rcol[2], vcol;
			material->getSpecular(state, sp, wo, reflect, refract, dir, rcol);
			const VolumeHandler *vol;
			if(reflect)
			{
				DiffRay ref_ray(sp.p_, dir[0], scene_->ray_min_dist_);
				if(diff_rays_enabled_) sp_diff.reflectedRay(ray, ref_ray);
				Rgb integ = integrate(state, ref_ray, additional_depth, nullptr);

				if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
				{
					if(vol->transmittance(state, ref_ray, vcol)) integ *= vcol;
				}

				const Rgb col_ind = (Rgb) integ * rcol[0];
				col += col_ind;
				if(int_passes_used)
				{
					if(Rgba *color_pass = int_passes->find(PassReflectPerfect)) *color_pass += col_ind;
				}
			}
			if(refract)
			{
				DiffRay ref_ray;
				float transp_bias_factor = material->getTransparentBiasFactor();

				if(transp_bias_factor > 0.f)
				{
					const bool transpbias_multiply_raydepth = material->getTransparentBiasMultiplyRayDepth();
					if(transpbias_multiply_raydepth) transp_bias_factor *= state.raylevel_;
					ref_ray = DiffRay(sp.p_ + dir[1] * transp_bias_factor, dir[1], scene_->ray_min_dist_);
				}
				else ref_ray = DiffRay(sp.p_, dir[1], scene_->ray_min_dist_);

				if(diff_rays_enabled_) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
				Rgba integ = integrate(state, ref_ray, additional_depth, nullptr);

				if(Material::hasFlag(bsdfs, BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
				{
					if(vol->transmittance(state, ref_ray, vcol)) integ *= vcol;
				}

				const Rgb col_ind = (Rgb) integ * rcol[1];
				col += col_ind;
				if(int_passes_used)
				{
					if(Rgba *color_pass = int_passes->find(PassRefractPerfect)) *color_pass += col_ind;
				}

				alpha = integ.a_;
			}
		}
	}
	--state.raylevel_;
}

Rgb MonteCarloIntegrator::sampleAmbientOcclusion(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f), surf_col(0.f), scol(0.f);
	bool shadowed;
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	light_ray.dir_ = Vec3(0.f);
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	int n = ao_samples_;//(int) ceilf(aoSamples*getSampleMultiplier());
	if(state.ray_division_ > 1) n = std::max(1, n / state.ray_division_);

	const unsigned int offs = n * state.pixel_sample_ + state.sampling_offs_;

	Halton hal_2(2);
	Halton hal_3(3);

	hal_2.setStart(offs - 1);
	hal_3.setStart(offs - 1);

	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();

		if(state.ray_division_ > 1)
		{
			s_1 = addMod1__(s_1, state.dc_1_);
			s_2 = addMod1__(s_2, state.dc_2_);
		}

		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
		else light_ray.tmin_ = scene_->shadow_bias_;

		light_ray.tmax_ = ao_dist_;

		float w = 0.f;

		Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Reflect);
		surf_col = material->sample(state, sp, wo, light_ray.dir_, s, w);

		if(Material::hasFlag(material->getFlags(), BsdfFlags::Emit))
		{
			col += material->emit(state, sp, wo) * s.pdf_;
		}

		shadowed = (tr_shad_) ? scene_->isShadowed(state, light_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(state, light_ray, mask_obj_index, mask_mat_index);

		if(!shadowed)
		{
			const float cos = std::fabs(sp.n_ * light_ray.dir_);
			if(tr_shad_) col += ao_col_ * scol * surf_col * cos * w;
			else col += ao_col_ * surf_col * cos * w;
		}
	}

	return col / (float)n;
}

Rgb MonteCarloIntegrator::sampleAmbientOcclusionPass(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f), surf_col(0.f), scol(0.f);
	bool shadowed;
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	light_ray.dir_ = Vec3(0.f);
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	int n = ao_samples_;//(int) ceilf(aoSamples*getSampleMultiplier());
	if(state.ray_division_ > 1) n = std::max(1, n / state.ray_division_);

	const unsigned int offs = n * state.pixel_sample_ + state.sampling_offs_;

	Halton hal_2(2);
	Halton hal_3(3);

	hal_2.setStart(offs - 1);
	hal_3.setStart(offs - 1);

	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();

		if(state.ray_division_ > 1)
		{
			s_1 = addMod1__(s_1, state.dc_1_);
			s_2 = addMod1__(s_2, state.dc_2_);
		}

		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
		else light_ray.tmin_ = scene_->shadow_bias_;

		light_ray.tmax_ = ao_dist_;

		float w = 0.f;

		Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Reflect);
		surf_col = material->sample(state, sp, wo, light_ray.dir_, s, w);

		if(Material::hasFlag(material->getFlags(), BsdfFlags::Emit))
		{
			col += material->emit(state, sp, wo) * s.pdf_;
		}

		shadowed = scene_->isShadowed(state, light_ray, mask_obj_index, mask_mat_index);

		if(!shadowed)
		{
			float cos = std::fabs(sp.n_ * light_ray.dir_);
			//if(trShad) col += aoCol * scol * surfCol * cos * W;
			col += ao_col_ * surf_col * cos * w;
		}
	}

	return col / (float)n;
}


Rgb MonteCarloIntegrator::sampleAmbientOcclusionPassClay(RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f), surf_col(0.f), scol(0.f);
	bool shadowed;
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	light_ray.dir_ = Vec3(0.f);
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	int n = ao_samples_;
	if(state.ray_division_ > 1) n = std::max(1, n / state.ray_division_);

	const unsigned int offs = n * state.pixel_sample_ + state.sampling_offs_;

	Halton hal_2(2);
	Halton hal_3(3);

	hal_2.setStart(offs - 1);
	hal_3.setStart(offs - 1);

	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();

		if(state.ray_division_ > 1)
		{
			s_1 = addMod1__(s_1, state.dc_1_);
			s_2 = addMod1__(s_2, state.dc_2_);
		}

		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
		else light_ray.tmin_ = scene_->shadow_bias_;

		light_ray.tmax_ = ao_dist_;

		float w = 0.f;

		Sample s(s_1, s_2, BsdfFlags::All);
		surf_col = material->sampleClay(state, sp, wo, light_ray.dir_, s, w);
		s.pdf_ = 1.f;

		if(Material::hasFlag(material->getFlags(), BsdfFlags::Emit))
		{
			col += material->emit(state, sp, wo) * s.pdf_;
		}

		shadowed = scene_->isShadowed(state, light_ray, mask_obj_index, mask_mat_index);

		if(!shadowed)
		{
			float cos = std::fabs(sp.n_ * light_ray.dir_);
			//if(trShad) col += aoCol * scol * surfCol * cos * W;
			col += ao_col_ * surf_col * cos * w;
		}
	}

	return col / (float)n;
}

END_YAFARAY
