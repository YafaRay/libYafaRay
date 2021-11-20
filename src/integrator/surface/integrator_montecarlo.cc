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
#include "light/light.h"
#include "color/spectrum.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "render/progress_bar.h"
#include "photon/photon.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "render/render_data.h"
#include "accelerator/accelerator.h"

BEGIN_YAFARAY

constexpr int MonteCarloIntegrator::loffs_delta_;

//Constructor and destructor defined here to avoid issues with std::unique_ptr<Pdf1D> being Pdf1D incomplete in the header (forward declaration)
MonteCarloIntegrator::MonteCarloIntegrator(Logger &logger) : TiledIntegrator(logger)
{
	caustic_map_ = std::unique_ptr<PhotonMap>(new PhotonMap(logger));
	caustic_map_->setName("Caustic Photon Map");
}

MonteCarloIntegrator::~MonteCarloIntegrator() = default;

Rgb MonteCarloIntegrator::estimateAllDirectLight(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col{0.f};
	unsigned int loffs = 0;
	for(const auto &l : lights_)
	{
		col += doLightEstimation(accelerator, chromatic_enabled, wavelength, l, sp, wo, loffs, ray_division, color_layers, camera, random_generator, pixel_sampling_data);
		++loffs;
	}
	if(color_layers && color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(Layer::Shadow)) *color_layer *= 1.f / static_cast<float>(loffs);
	}
	return col;
}

Rgb MonteCarloIntegrator::estimateOneDirectLight(const Accelerator &accelerator, int thread_id, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, int n, const RayDivision &ray_division, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const
{
	const int num_lights = lights_.size();
	if(num_lights == 0) return {0.f};
	Halton hal_2(2, scene_->getImageFilm()->getBaseSamplingOffset() + correlative_sample_number_[thread_id] - 1); //Probably with this change the parameter "n" is no longer necessary, but I will keep it just in case I have to revert this change!
	const int lnum = std::min(static_cast<int>(hal_2.getNext() * static_cast<float>(num_lights)), num_lights - 1);
	++correlative_sample_number_[thread_id];
	return doLightEstimation(accelerator, chromatic_enabled, wavelength, lights_[lnum], sp, wo, lnum, ray_division, nullptr, camera, random_generator, pixel_sampling_data) * num_lights;
}

Rgb MonteCarloIntegrator::diracLight(const Accelerator &accelerator, const Light *light, const Vec3 &wo, const SurfacePoint &sp, RandomGenerator &random_generator, bool cast_shadows, const Camera *camera, ColorLayers *color_layers) const
{
	Ray light_ray;
	Rgb lcol;
	if(light->illuminate(sp, lcol, light_ray))
	{
		Rgb col{0.f};
		const Material *material = sp.material_;
		light_ray.from_ = sp.p_;
		Rgba *color_layer_shadow = nullptr;
		Rgba *color_layer_diffuse = nullptr;
		Rgba *color_layer_diffuse_no_shadow = nullptr;
		Rgba *color_layer_glossy = nullptr;
		if(color_layers)
		{
			if(color_layers->getFlags().hasAny(Layer::Flags::DiffuseLayers))
			{
				color_layer_diffuse = color_layers->find(Layer::Diffuse);
				color_layer_diffuse_no_shadow = color_layers->find(Layer::DiffuseNoShadow);
			}
			if(color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
			{
				color_layer_glossy = color_layers->find(Layer::Glossy);
				color_layer_shadow = color_layers->find(Layer::Shadow);
			}
		}
		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, sp.p_.length());
		else light_ray.tmin_ = scene_->shadow_bias_;
		float mask_obj_index = 0.f, mask_mat_index = 0.f;
		Rgb scol{0.f};
		bool shadowed = false;
		if(cast_shadows)
		{
			if(tr_shad_) shadowed = accelerator.isShadowed(light_ray, s_depth_, scol, mask_obj_index, mask_mat_index, scene_->getShadowBias(), camera);
			else shadowed = accelerator.isShadowed(light_ray, mask_obj_index, mask_mat_index, scene_->getShadowBias());
		}
		const float angle_light_normal = material->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_);	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal
		if(!shadowed || color_layer_diffuse_no_shadow)
		{
			if(!shadowed && color_layer_shadow) *color_layer_shadow += Rgb(1.f);
			const Rgb surf_col = material->eval(sp.mat_data_.get(), sp, wo, light_ray.dir_, BsdfFlags::All);
			const Rgb transmit_col = scene_->vol_integrator_ ? scene_->vol_integrator_->transmittance(random_generator, light_ray) : 1.f;
			const Rgba tmp_col_no_shadow = surf_col * lcol * angle_light_normal * transmit_col;
			if(tr_shad_ && cast_shadows) lcol *= scol;
			if(color_layers)
			{
				if(color_layer_diffuse_no_shadow) *color_layer_diffuse_no_shadow += tmp_col_no_shadow;
				if(!shadowed)
				{
					if(color_layer_diffuse) *color_layer_diffuse += material->eval(sp.mat_data_.get(), sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * lcol * angle_light_normal * transmit_col;
					if(color_layer_glossy) *color_layer_glossy += material->eval(sp.mat_data_.get(), sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * lcol * angle_light_normal * transmit_col;
				}
			}
			if(!shadowed) col += surf_col * lcol * angle_light_normal * transmit_col;
		}
		if(color_layers)
		{
			if(color_layers->getFlags().hasAny(Layer::Flags::IndexLayers))
			{
				if(shadowed)
				{
					const MaskParams &mask_params = scene_->getMaskParams();
					Rgba *color_layer_mat_index_mask_shadow = color_layers->find(Layer::MatIndexMaskShadow);
					Rgba *color_layer_obj_index_mask_shadow = color_layers->find(Layer::ObjIndexMaskShadow);
					if(color_layer_mat_index_mask_shadow && mask_mat_index == mask_params.mat_index_) *color_layer_mat_index_mask_shadow += Rgb(1.f);
					if(color_layer_obj_index_mask_shadow && mask_obj_index == mask_params.obj_index_) *color_layer_obj_index_mask_shadow += Rgb(1.f);
				}
			}
			if(color_layers->getFlags().hasAny(Layer::Flags::DebugLayers))
			{
				if(Rgba *color_layer = color_layers->find(Layer::DebugLightEstimationLightDirac)) *color_layer += col;
			}
		}
		return col;
	}
	else return {0.f};
}

Rgb MonteCarloIntegrator::areaLightSampleLight(const Accelerator &accelerator, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, Halton &hal_2, Halton &hal_3, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator) const
{
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	Rgb col{0.f};
	Rgba *color_layer_shadow = nullptr;
	Rgba *color_layer_mat_index_mask_shadow = nullptr;
	Rgba *color_layer_obj_index_mask_shadow = nullptr;
	Rgba *color_layer_diffuse = nullptr;
	Rgba *color_layer_diffuse_no_shadow = nullptr;
	Rgba *color_layer_glossy = nullptr;
	Rgba col_shadow{0.f}, col_shadow_obj_mask{0.f}, col_shadow_mat_mask{0.f}, col_diff_dir{0.f}, col_diff_no_shadow{0.f}, col_glossy_dir{0.f};
	if(color_layers)
	{
		if(color_layers->getFlags().hasAny(Layer::Flags::IndexLayers))
		{
			color_layer_mat_index_mask_shadow = color_layers->find(Layer::MatIndexMaskShadow);
			color_layer_obj_index_mask_shadow = color_layers->find(Layer::ObjIndexMaskShadow);
		}
		if(color_layers->getFlags().hasAny(Layer::Flags::DiffuseLayers))
		{
			color_layer_diffuse = color_layers->find(Layer::Diffuse);
			color_layer_diffuse_no_shadow = color_layers->find(Layer::DiffuseNoShadow);
		}
		if(color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
		{
			color_layer_shadow = color_layers->find(Layer::Shadow);
			color_layer_glossy = color_layers->find(Layer::Glossy);
		}
	}
	LSample ls;
	Rgb scol{0.f};
	for(unsigned int i = 0; i < num_samples; ++i)
	{
		// ...get sample val...
		ls.s_1_ = hal_2.getNext();
		ls.s_2_ = hal_3.getNext();
		if(light->illumSample(sp, ls, light_ray))
		{
			if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, sp.p_.length());
			else light_ray.tmin_ = scene_->shadow_bias_;
			float mask_obj_index = 0.f, mask_mat_index = 0.f;
			bool shadowed = false;
			if(cast_shadows)
			{
				if(tr_shad_) shadowed = accelerator.isShadowed(light_ray, s_depth_, scol, mask_obj_index, mask_mat_index, scene_->getShadowBias(), camera);
				else accelerator.isShadowed(light_ray, mask_obj_index, mask_mat_index, scene_->getShadowBias());
			}
			if((!shadowed && ls.pdf_ > 1e-6f) || color_layer_diffuse_no_shadow)
			{
				const Rgb ls_col_no_shadow = ls.col_;
				if(tr_shad_ && cast_shadows) ls.col_ *= scol;
				if(scene_->vol_integrator_)
				{
					const Rgb transmit_col = scene_->vol_integrator_->transmittance(random_generator, light_ray);
					ls.col_ *= transmit_col;
				}
				const Rgb surf_col = material->eval(sp.mat_data_.get(), sp, wo, light_ray.dir_, BsdfFlags::All);
				if(color_layer_shadow && !shadowed && ls.pdf_ > 1e-6f) col_shadow += Rgb(1.f);
				const float angle_light_normal = material->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_);	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal
				float w = 1.f;
				if(light->canIntersect())
				{
					const float m_pdf = material->pdf(sp.mat_data_.get(), sp, wo, light_ray.dir_, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
					if(m_pdf > 1e-6f)
					{
						const float l_2 = ls.pdf_ * ls.pdf_;
						const float m_2 = m_pdf * m_pdf;
						w = l_2 / (l_2 + m_2);
					}
				}
				if(color_layers)
				{
					if(color_layer_diffuse || color_layer_diffuse_no_shadow)
					{
						const Rgb tmp_col_no_light_color = material->eval(sp.mat_data_.get(), sp, wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal * w / ls.pdf_;
						if(color_layer_diffuse_no_shadow) col_diff_no_shadow += tmp_col_no_light_color * ls_col_no_shadow;
						if(color_layer_diffuse && !shadowed && ls.pdf_ > 1e-6f) col_diff_dir += tmp_col_no_light_color * ls.col_;
					}
					if(color_layer_glossy)
					{
						const Rgb tmp_col = material->eval(sp.mat_data_.get(), sp, wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal * w / ls.pdf_;
						if(!shadowed && ls.pdf_ > 1e-6f) col_glossy_dir += tmp_col;
					}
				}
				if(!shadowed && ls.pdf_ > 1e-6f) col += surf_col * ls.col_ * angle_light_normal * w / ls.pdf_;
			}
			if(color_layers && (shadowed || ls.pdf_ <= 1e-6f))
			{
				const MaskParams &mask_params = scene_->getMaskParams();
				if(color_layer_mat_index_mask_shadow && mask_mat_index == mask_params.mat_index_) col_shadow_mat_mask += Rgb(1.f);
				if(color_layer_obj_index_mask_shadow && mask_obj_index == mask_params.obj_index_) col_shadow_obj_mask += Rgb(1.f);
			}
		}
	}
	const Rgb col_result = col * inv_num_samples;
	if(color_layers)
	{
		if(color_layers->getFlags().hasAny(Layer::Flags::IndexLayers))
		{
			if(color_layer_mat_index_mask_shadow) *color_layer_mat_index_mask_shadow += col_shadow_mat_mask * inv_num_samples;
			if(color_layer_obj_index_mask_shadow) *color_layer_obj_index_mask_shadow += col_shadow_obj_mask * inv_num_samples;
		}
		if(color_layers->getFlags().hasAny(Layer::Flags::DiffuseLayers))
		{
			if(color_layer_diffuse) *color_layer_diffuse += col_diff_dir * inv_num_samples;
			if(color_layer_diffuse_no_shadow) *color_layer_diffuse_no_shadow += col_diff_no_shadow * inv_num_samples;
		}
		if(color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
		{
			if(color_layer_shadow) *color_layer_shadow += col_shadow * inv_num_samples;
			if(color_layer_glossy) *color_layer_glossy += col_glossy_dir * inv_num_samples;
		}
		if(color_layers->getFlags().hasAny(Layer::Flags::DebugLayers))
		{
			if(Rgba *color_layer = color_layers->find(Layer::DebugLightEstimationLightSampling)) *color_layer += col_result;
		}
	}
	return col_result;
}

Rgb MonteCarloIntegrator::areaLightSampleMaterial(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, Halton &hal_2, Halton &hal_3, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator) const
{
	if(light->canIntersect()) // sample from BSDF to complete MIS
	{
		Rgb col_result{0.f};
		const Material *material = sp.material_;
		Rgba *color_layer_diffuse = nullptr;
		Rgba *color_layer_diffuse_no_shadow = nullptr;
		Rgba *color_layer_glossy = nullptr;
		Rgba col_diff_dir{0.f}, col_diff_no_shadow{0.f}, col_glossy_dir{0.f};
		if(color_layers)
		{
			if(color_layers->getFlags().hasAny(Layer::Flags::DiffuseLayers))
			{
				color_layer_diffuse = color_layers->find(Layer::Diffuse);
				color_layer_diffuse_no_shadow = color_layers->find(Layer::DiffuseNoShadow);
			}
			if(color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
			{
				color_layer_glossy = color_layers->find(Layer::Glossy);
			}
		}
		Ray light_ray;
		light_ray.from_ = sp.p_;
		Rgb col{0.f};
		Rgb lcol;
		Ray b_ray;
		for(unsigned int i = 0; i < num_samples; ++i)
		{
			if(scene_->ray_min_dist_auto_) b_ray.tmin_ = scene_->ray_min_dist_ * std::max(1.f, sp.p_.length());
			else b_ray.tmin_ = scene_->ray_min_dist_;
			b_ray.from_ = sp.p_;
			const float s_1 = hal_2.getNext();
			const float s_2 = hal_3.getNext();
			float W = 0.f;
			Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
			const Rgb surf_col = material->sample(sp.mat_data_.get(), sp, wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera);
			float light_pdf;
			if(s.pdf_ > 1e-6f && light->intersect(b_ray, b_ray.tmax_, lcol, light_pdf))
			{
				float mask_obj_index = 0.f, mask_mat_index = 0.f;
				Rgb scol{0.f};
				bool shadowed = false;
				if(cast_shadows)
				{
					if(tr_shad_) shadowed = accelerator.isShadowed(b_ray, s_depth_, scol, mask_obj_index, mask_mat_index, scene_->getShadowBias(), camera);
					else accelerator.isShadowed(b_ray, mask_obj_index, mask_mat_index, scene_->getShadowBias());
				}
				if((!shadowed && light_pdf > 1e-6f) || color_layer_diffuse_no_shadow)
				{
					if(tr_shad_ && cast_shadows) lcol *= scol;
					if(scene_->vol_integrator_)
					{
						const Rgb transmit_col = scene_->vol_integrator_->transmittance(random_generator, b_ray);
						lcol *= transmit_col;
					}
					const float l_pdf = 1.f / light_pdf;
					const float l_2 = l_pdf * l_pdf;
					const float m_2 = s.pdf_ * s.pdf_;
					const float w = m_2 / (l_2 + m_2);
					if(color_layers)
					{
						if(color_layer_diffuse || color_layer_diffuse_no_shadow)
						{
							const Rgb tmp_col = material->sample(sp.mat_data_.get(), sp, wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera) * lcol * w * W;
							if(color_layer_diffuse_no_shadow) col_diff_no_shadow += tmp_col;
							if(color_layer_diffuse && !shadowed && light_pdf > 1e-6f && s.sampled_flags_.hasAny(BsdfFlags::Diffuse)) col_diff_dir += tmp_col;
						}
						if(color_layer_glossy)
						{
							const Rgb tmp_col = material->sample(sp.mat_data_.get(), sp, wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera) * lcol * w * W;
							if(!shadowed && light_pdf > 1e-6f && s.sampled_flags_.hasAny(BsdfFlags::Glossy)) col_glossy_dir += tmp_col;
						}
					}
					if(!shadowed && light_pdf > 1e-6f) col += surf_col * lcol * w * W;
				}
			}
		}
		col_result = col * inv_num_samples;
		if(color_layers)
		{
			if(color_layers->getFlags().hasAny(Layer::Flags::DiffuseLayers))
			{
				if(color_layer_diffuse) *color_layer_diffuse += col_diff_dir * inv_num_samples;
				if(color_layer_diffuse_no_shadow) *color_layer_diffuse_no_shadow += col_diff_no_shadow * inv_num_samples;
			}
			if(color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
			{
				if(color_layer_glossy) *color_layer_glossy += col_glossy_dir * inv_num_samples;
			}
			if(color_layers->getFlags().hasAny(Layer::Flags::DebugLayers))
			{
				if(Rgba *color_layer = color_layers->find(Layer::DebugLightEstimationMatSampling)) *color_layer += col_result;
			}
		}
		return col_result;
	}
	else return {0.f};
}

Rgb MonteCarloIntegrator::doLightEstimation(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const Light *light, const SurfacePoint &sp, const Vec3 &wo, unsigned int loffs, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col{0.f};
	const bool cast_shadows = light->castShadows() && sp.material_->getReceiveShadows();
	if(light->diracLight()) // handle lights with delta distribution, e.g. point and directional lights
	{
		col += diracLight(accelerator, light, wo, sp, random_generator, cast_shadows, camera, color_layers);
	}
	else // area light and suchlike
	{
		const unsigned int l_offs = loffs * loffs_delta_;
		int num_samples = static_cast<int>(ceilf(light->nSamples() * aa_light_sample_multiplier_));
		if(ray_division.division_ > 1) num_samples = std::max(1, num_samples / ray_division.division_);
		const float inv_num_samples = 1.f / static_cast<float>(num_samples);
		const unsigned int offs = num_samples * pixel_sampling_data.sample_ + pixel_sampling_data.offset_ + l_offs;
		Halton hal_2(2, offs - 1);
		Halton hal_3(3, offs - 1);
		col += areaLightSampleLight(accelerator, light, wo, sp, cast_shadows, num_samples, inv_num_samples, hal_2, hal_3, color_layers, camera, random_generator);
		hal_2.setStart(offs - 1);
		hal_3.setStart(offs - 1);
		col += areaLightSampleMaterial(accelerator, chromatic_enabled, wavelength, light, wo, sp, cast_shadows, num_samples, inv_num_samples, hal_2, hal_3, color_layers, camera, random_generator);
	}
	return col;
}

Rgb MonteCarloIntegrator::causticPhotons(const Ray &ray, ColorLayers *color_layers, const SurfacePoint &sp, const Vec3 &wo, float clamp_indirect, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search)
{
	Rgb col = MonteCarloIntegrator::estimateCausticPhotons(sp, wo, caustic_map, caustic_radius, n_caus_search);
	if(clamp_indirect > 0.f) col.clampProportionalRgb(clamp_indirect);
	if(color_layers && color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(Layer::Indirect)) *color_layer += col;
	}
	return col;
}

void MonteCarloIntegrator::causticWorker(const Accelerator &accelerator, PhotonMap *caustic_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, const Timer &timer, unsigned int n_caus_photons, Pdf1D *light_power_d, int num_lights, const std::vector<const Light *> &caus_lights, int caus_depth, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot)
{
	bool done = false;
	float s_1, s_2, s_3, s_4, s_5, s_6, s_7, s_l;
	const float f_num_lights = (float)num_lights;
	float light_num_pdf, light_pdf;

	unsigned int curr = 0;
	const unsigned int n_caus_photons_thread = 1 + ((n_caus_photons - 1) / scene->getNumThreadsPhotons());

	std::vector<Photon> local_caustic_photons;

	SurfacePoint hit_curr, hit_prev;
	hit_prev.initializeAllZero(); //Just to avoid compiler warnings

	Ray ray;
	local_caustic_photons.clear();
	local_caustic_photons.reserve(n_caus_photons_thread);

	while(!done)
	{
		const unsigned int haltoncurr = curr + n_caus_photons_thread * thread_id;
		const float wavelength = sample::riS(haltoncurr);

		s_1 = sample::riVdC(haltoncurr);
		s_2 = Halton::lowDiscrepancySampling(2, haltoncurr);
		s_3 = Halton::lowDiscrepancySampling(3, haltoncurr);
		s_4 = Halton::lowDiscrepancySampling(4, haltoncurr);

		s_l = float(haltoncurr) / float(n_caus_photons);

		const int light_num = light_power_d->dSample(logger_, s_l, &light_num_pdf);

		if(light_num >= num_lights)
		{
			logger_.logError(getName(), ": lightPDF sample error! ", s_l, "/", light_num);
			return;
		}

		Rgb pcol = caus_lights[light_num]->emitPhoton(s_1, s_2, s_3, s_4, ray, light_pdf);
		ray.tmin_ = scene->ray_min_dist_;
		ray.tmax_ = -1.f;
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= n_caus_photons_thread);
			continue;
		}
		else if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
		{
			logger_.logWarning(getName(), ": NaN (photon color)");
			continue;
		}
		int n_bounces = 0;
		bool caustic_photon = false;
		bool direct_photon = true;
		const Material *material_prev = nullptr;
		BsdfFlags mat_bsdfs_prev = BsdfFlags::None;
		bool chromatic_enabled = true;
		while(accelerator.intersect(ray, hit_curr, render_view->getCamera()))
		{
			// check for volumetric effects, based on the material from the previous photon bounce
			Rgb transm(1.f);
			if(material_prev && mat_bsdfs_prev.hasAny(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = material_prev->getVolumeHandler(hit_prev.ng_ * ray.dir_ < 0))
				{
					transm = vol->transmittance(ray);
				}
			}
			const Vec3 wi = -ray.dir_;
			const Material *material = hit_curr.material_;
			const BsdfFlags &mat_bsdfs = hit_curr.mat_data_->bsdf_flags_;
			if(mat_bsdfs.hasAny((BsdfFlags::Diffuse | BsdfFlags::Glossy)))
			{
				//deposit caustic photon on surface
				if(caustic_photon)
				{
					Photon np(wi, hit_curr.p_, pcol);
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
			Vec3 wo;
			bool scattered = material->scatterPhoton(hit_curr.mat_data_.get(), hit_curr, wi, wo, sample, chromatic_enabled, wavelength, render_view->getCamera());
			if(!scattered) break; //photon was absorped.
			pcol = sample.color_;
			// hm...dispersive is not really a scattering qualifier like specular/glossy/diffuse or the special case filter...
			caustic_photon = (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
			// light through transparent materials can be calculated by direct lighting, so still consider them direct!
			direct_photon = sample.sampled_flags_.hasAny(BsdfFlags::Filter) && direct_photon;
			// caustic-only calculation can be stopped if:
			if(!(caustic_photon || direct_photon)) break;

			if(chromatic_enabled && sample.sampled_flags_.hasAny(BsdfFlags::Dispersive))
			{
				chromatic_enabled = false;
				pcol *= spectrum::wl2Rgb(wavelength);
			}
			ray.from_ = hit_curr.p_;
			ray.dir_ = wo;
			ray.tmin_ = scene->ray_min_dist_;
			ray.tmax_ = -1.f;
			material_prev = material;
			mat_bsdfs_prev = mat_bsdfs;
			std::swap(hit_prev, hit_curr);
			++n_bounces;
		}
		++curr;
		if(curr % pb_step == 0)
		{
			pb->update();
			if(render_control.canceled()) { return; }
		}
		done = (curr >= n_caus_photons_thread);
	}
	caustic_map->mutx_.lock();
	caustic_map->appendVector(local_caustic_photons, curr);
	total_photons_shot += curr;
	caustic_map->mutx_.unlock();
}

bool MonteCarloIntegrator::createCausticMap(const RenderView *render_view, const RenderControl &render_control, const Timer &timer)
{
	const Accelerator *accelerator = scene_->getAccelerator();
	if(!accelerator) return false;
	std::shared_ptr<ProgressBar> pb;
	if(intpb_) pb = intpb_;
	else pb = std::make_shared<ConsoleProgressBar>(80);

	if(photon_map_processing_ == PhotonsLoad)
	{
		pb->setTag("Loading caustic photon map from file...");
		const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_caustic.photonmap";
		logger_.logInfo(getName(), ": Loading caustic photon map from: ", filename, ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
		if(caustic_map_->load(filename))
		{
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic map loaded.");
			return true;
		}
		else
		{
			photon_map_processing_ = PhotonsGenerateAndSave;
			logger_.logWarning(getName(), ": photon map loading failed, changing to Generate and Save mode.");
		}
	}

	if(photon_map_processing_ == PhotonsReuse)
	{
		logger_.logInfo(getName(), ": Reusing caustics photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
		if(caustic_map_->nPhotons() == 0)
		{
			photon_map_processing_ = PhotonsGenerateOnly;
			logger_.logWarning(getName(), ": One of the photon maps in memory was empty, they cannot be reused: changing to Generate mode.");
		}
		else return true;
	}

	caustic_map_->clear();
	caustic_map_->setNumPaths(0);
	caustic_map_->reserveMemory(n_caus_photons_);
	caustic_map_->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

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

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for caustics map:");

		for(int i = 0; i < num_lights; ++i)
		{
			Rgb pcol(0.f);
			pcol = caus_lights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			light_num_pdf = light_power_d->func_[i] * light_power_d->inv_integral_;
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", i + 1, "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}

		int pb_step;
		logger_.logInfo(getName(), ": Building caustics photon map...");
		pb->init(128, logger_.getConsoleLogColorsEnabled());
		pb_step = std::max(1U, n_caus_photons_ / 128);
		pb->setTag("Building caustics photon map...");

		unsigned int curr = 0;

		const int n_threads = scene_->getNumThreadsPhotons();

		n_caus_photons_ = std::max((unsigned int) n_threads, (n_caus_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons, so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		logger_.logParams(getName(), ": Shooting ", n_caus_photons_, " photons across ", n_threads, " threads (", (n_caus_photons_ / n_threads), " photons/thread)");

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&MonteCarloIntegrator::causticWorker, this, std::ref(*accelerator), caustic_map_.get(), i, scene_, render_view, std::ref(render_control), std::ref(timer), n_caus_photons_, light_power_d.get(), num_lights, caus_lights, caus_depth_, pb.get(), pb_step, std::ref(curr)));
		for(auto &t : threads) t.join();

		pb->done();
		pb->setTag("Caustic photon map built.");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		logger_.logInfo(getName(), ": Shot ", curr, " caustic photons from ", num_lights, " light(s).");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored caustic photons: ", caustic_map_->nPhotons());

		if(caustic_map_->nPhotons() > 0)
		{
			pb->setTag("Building caustic photons kd-tree...");
			caustic_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}

		if(photon_map_processing_ == PhotonsGenerateAndSave)
		{
			pb->setTag("Saving caustic photon map to file...");
			std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_caustic.photonmap";
			logger_.logInfo(getName(), ": Saving caustic photon map to: ", filename);
			if(caustic_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic map saved.");
		}
	}
	else if(logger_.isVerbose()) logger_.logVerbose(getName(), ": No caustic source lights found, skiping caustic map building...");
	return true;
}

Rgb MonteCarloIntegrator::estimateCausticPhotons(const SurfacePoint &sp, const Vec3 &wo, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search)
{
	if(!caustic_map->ready()) return {0.f};
	const auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_caus_search]);//(foundPhoton_t *)alloca(nCausSearch * sizeof(foundPhoton_t));
	float g_radius_square = caustic_radius * caustic_radius;
	const int n_gathered = caustic_map->gather(sp.p_, gathered.get(), n_caus_search, g_radius_square);
	g_radius_square = 1.f / g_radius_square;
	Rgb sum {0.f};
	if(n_gathered > 0)
	{
		const Material *material = sp.material_;
		for(int i = 0; i < n_gathered; ++i)
		{
			const Photon *photon = gathered[i].photon_;
			const Rgb surf_col = material->eval(sp.mat_data_.get(), sp, wo, photon->direction(), BsdfFlags::All);
			const float k = sample::kernel(gathered[i].dist_square_, g_radius_square);
			sum += surf_col * k * photon->color();
		}
		sum *= 1.f / static_cast<float>(caustic_map->nPaths());
	}
	return sum;
}

std::pair<Rgb, float> MonteCarloIntegrator::dispersive(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const
{
	const int ray_samples_dispersive = ray_division.division_ > 1 ?
									   std::max(1, initial_ray_samples_dispersive_ / ray_division.division_) :
									   initial_ray_samples_dispersive_;
	RayDivision ray_division_new {ray_division};
	ray_division_new.division_ *= ray_samples_dispersive;
	int branch = ray_division_new.division_ * ray_division.offset_;
	const float d_1 = 1.f / static_cast<float>(ray_samples_dispersive);
	const float ss_1 = sample::riS(pixel_sampling_data.sample_ + pixel_sampling_data.offset_);
	Rgb dcol(0.f);
	float w = 0.f;

	Rgb dcol_trans_accum;
	float alpha_accum = 0.f;
	std::unique_ptr<Ray> ref_ray_chromatic_volume; //Reference ray used for chromatic/dispersive volume color calculation only. FIXME: it only uses one of the sampled reference rays for volume calculations, not sure if this is ok??
	for(int ns = 0; ns < ray_samples_dispersive; ++ns)
	{
		float wavelength_dispersive;
		if(chromatic_enabled)
		{
			wavelength_dispersive = (ns + ss_1) * d_1;
			if(ray_division.division_ > 1) wavelength_dispersive = math::addMod1(wavelength_dispersive, ray_division.decorrelation_1_);
		}
		else wavelength_dispersive = 0.f;

		ray_division_new.decorrelation_1_ = Halton::lowDiscrepancySampling(2 * ray_level + 1, branch + pixel_sampling_data.offset_);
		ray_division_new.decorrelation_2_ = Halton::lowDiscrepancySampling(2 * ray_level + 2, branch + pixel_sampling_data.offset_);
		ray_division_new.offset_ = branch;
		++branch;
		Sample s(0.5f, 0.5f, BsdfFlags::Reflect | BsdfFlags::Transmit | BsdfFlags::Dispersive);
		Vec3 wi;
		const Rgb mcol = material->sample(sp.mat_data_.get(), sp, wo, wi, s, w, chromatic_enabled, wavelength_dispersive, camera);

		if(s.pdf_ > 1.0e-6f && s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
		{
			const Rgb wl_col = spectrum::wl2Rgb(wavelength_dispersive);
			Ray ref_ray(sp.p_, wi, scene_->ray_min_dist_);
			auto integ = integrate(accelerator, thread_id, ray_level, false, wavelength_dispersive, ref_ray, additional_depth, ray_division_new, nullptr, camera, random_generator, pixel_sampling_data);
			integ.first *= mcol * wl_col * w;
			dcol += integ.first;
			if(color_layers) dcol_trans_accum += integ.first;
			alpha_accum += integ.second;
			if(!ref_ray_chromatic_volume) ref_ray_chromatic_volume = std::unique_ptr<Ray>(new Ray(ref_ray, Ray::DifferentialsCopy::No));
		}
	}
	if(ref_ray_chromatic_volume && bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray_chromatic_volume->dir_ < 0))
		{
			dcol *= vol->transmittance(*ref_ray_chromatic_volume);
		}
	}
	if(color_layers && color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(Layer::Trans))
		{
			dcol_trans_accum *= d_1;
			*color_layer += dcol_trans_accum;
		}
	}
	return {dcol * d_1, alpha_accum * d_1};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossy(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &mat_bsdfs, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const
{
	const int ray_samples_glossy = ray_division.division_ > 1 ?
								   std::max(1, initial_ray_samples_glossy_ / ray_division.division_) :
								   initial_ray_samples_glossy_;
	RayDivision ray_division_new {ray_division};
	ray_division_new.division_ *= ray_samples_glossy;
	int branch = ray_division_new.division_ * ray_division.offset_;
	unsigned int offs = ray_samples_glossy * pixel_sampling_data.sample_ + pixel_sampling_data.offset_;
	const float inverse_ray_samples_glossy = 1.f / static_cast<float>(ray_samples_glossy);
	Rgb gcol(0.f);

	Halton hal_2(2, offs);
	Halton hal_3(3, offs);

	Rgb gcol_indirect_accum;
	Rgb gcol_reflect_accum;
	Rgb gcol_transmit_accum;
	float alpha_accum = 0.f;

	for(int ns = 0; ns < ray_samples_glossy; ++ns)
	{
		ray_division_new.decorrelation_1_ = Halton::lowDiscrepancySampling(2 * ray_level + 1, branch + pixel_sampling_data.offset_);
		ray_division_new.decorrelation_2_ = Halton::lowDiscrepancySampling(2 * ray_level + 2, branch + pixel_sampling_data.offset_);
		ray_division_new.offset_ = branch;
		++offs;
		++branch;

		const float s_1 = hal_2.getNext();
		const float s_2 = hal_3.getNext();

		if(mat_bsdfs.hasAny(BsdfFlags::Glossy))
		{
			if(mat_bsdfs.hasAny(BsdfFlags::Reflect) && !mat_bsdfs.hasAny(BsdfFlags::Transmit))
			{
				const auto result = glossyReflectNoTransmit(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, material, bsdfs, wo, additional_depth, camera, pixel_sampling_data, ray_division_new, s_1, s_2, random_generator);
				gcol += result.first;
				if(color_layers) gcol_indirect_accum += result.first;
				alpha_accum += result.second;
			}
			else if(mat_bsdfs.hasAny(BsdfFlags::Reflect) && mat_bsdfs.hasAny(BsdfFlags::Transmit))
			{
				Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::AllGlossy);
				Rgb mcol[2];
				float w[2];
				Vec3 dir[2];

				mcol[0] = material->sample(sp.mat_data_.get(), sp, wo, dir, mcol[1], s, w, chromatic_enabled, wavelength);

				if(s.sampled_flags_.hasAny(BsdfFlags::Reflect) && !s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
				{
					const auto result = glossyReflectDispersive(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, material, bsdfs, additional_depth, color_layers, camera, pixel_sampling_data, ray_division_new, mcol[0], w[0], dir[0], random_generator);
					gcol += result.first;
					if(color_layers) gcol_reflect_accum += result.first;
					alpha_accum += result.second;
				}
				if(s.sampled_flags_.hasAny(BsdfFlags::Transmit))
				{
					const auto result = glossyTransmit(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, material, bsdfs, additional_depth, color_layers, camera, pixel_sampling_data, ray_division_new, mcol[1], w[1], dir[1], random_generator);
					gcol += result.first;
					if(color_layers) gcol_transmit_accum += result.first;
					alpha_accum += result.second;
				}
			}
		}
	}

	if(color_layers && color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(Layer::GlossyIndirect))
		{
			gcol_indirect_accum *= inverse_ray_samples_glossy;
			*color_layer += gcol_indirect_accum;
		}
		if(Rgba *color_layer = color_layers->find(Layer::Trans))
		{
			gcol_reflect_accum *= inverse_ray_samples_glossy;
			*color_layer += gcol_reflect_accum;
		}
		if(Rgba *color_layer = color_layers->find(Layer::GlossyIndirect))
		{
			gcol_transmit_accum *= inverse_ray_samples_glossy;
			*color_layer += gcol_transmit_accum;
		}
	}
	return {gcol * inverse_ray_samples_glossy, alpha_accum * inverse_ray_samples_glossy};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyReflectDispersive(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, int additional_depth, const ColorLayers *color_layers, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &reflect_color, float w, const Vec3 &dir, RandomGenerator &random_generator) const
{
	Ray ref_ray = Ray(sp.p_, dir, scene_->ray_min_dist_);
	if(ray.differentials_) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
	auto integ = integrate(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ref_ray, additional_depth, ray_division_new, nullptr, camera, random_generator, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ.first *= vol->transmittance(ref_ray);
		}
	}
	integ.first *= reflect_color * w;
	return {integ.first, integ.second};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyTransmit(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, int additional_depth, const ColorLayers *color_layers, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &transmit_col, float w, const Vec3 &dir, RandomGenerator &random_generator) const
{
	Ray ref_ray = Ray(sp.p_, dir, scene_->ray_min_dist_);
	if(ray.differentials_) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, material->getMatIor());
	auto integ = integrate(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ref_ray, additional_depth, ray_division_new, nullptr, camera, random_generator, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ.first *= vol->transmittance(ref_ray);
		}
	}
	integ.first *= transmit_col * w;
	return integ;
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyReflectNoTransmit(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const float s_1, const float s_2, RandomGenerator &random_generator) const
{
	float w = 0.f;
	Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Reflect);
	Vec3 wi;
	const Rgb mcol = material->sample(sp.mat_data_.get(), sp, wo, wi, s, w, chromatic_enabled, wavelength, camera);
	Ray ref_ray(sp.p_, wi, scene_->ray_min_dist_);
	if(ray.differentials_)
	{
		if(s.sampled_flags_.hasAny(BsdfFlags::Reflect)) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
		else if(s.sampled_flags_.hasAny(BsdfFlags::Transmit)) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, material->getMatIor());
	}
	auto integ = integrate(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ref_ray, additional_depth, ray_division_new, nullptr, camera, random_generator, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ.first *= vol->transmittance(ref_ray);
		}
	}
	integ.first *= mcol * w;
	return integ;
}

std::pair<Rgb, float> MonteCarloIntegrator::specularReflect(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *reflect_data, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data, const Camera *camera) const
{
	Ray ref_ray(sp.p_, reflect_data->dir_, scene_->ray_min_dist_);
	if(ray.differentials_) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
	auto integ = integrate(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ref_ray, additional_depth, ray_division, nullptr, camera, random_generator, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ.first *= vol->transmittance(ref_ray);
		}
	}
	integ.first *= reflect_data->col_;
	if(color_layers && color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(Layer::ReflectPerfect)) *color_layer += integ.first;
	}
	return integ;
}

std::pair<Rgb, float> MonteCarloIntegrator::specularRefract(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *refract_data, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data, const Camera *camera) const
{
	Ray ref_ray;
	float transp_bias_factor = material->getTransparentBiasFactor();
	if(transp_bias_factor > 0.f)
	{
		const bool transpbias_multiply_raydepth = material->getTransparentBiasMultiplyRayDepth();
		if(transpbias_multiply_raydepth) transp_bias_factor *= ray_level;
		ref_ray = Ray(sp.p_ + refract_data->dir_ * transp_bias_factor, refract_data->dir_, scene_->ray_min_dist_);
	}
	else ref_ray = Ray(sp.p_, refract_data->dir_, scene_->ray_min_dist_);

	if(ray.differentials_) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, material->getMatIor());
	auto integ = integrate(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ref_ray, additional_depth, ray_division, nullptr, camera, random_generator, pixel_sampling_data);

	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ.first *= vol->transmittance(ref_ray);
		}
	}
	integ.first *= refract_data->col_;
	if(color_layers && color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(Layer::RefractPerfect)) *color_layer += integ.first;
	}
	return integ;
}

std::pair<Rgb, float> MonteCarloIntegrator::recursiveRaytrace(const Accelerator &accelerator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const BsdfFlags &bsdfs, SurfacePoint &sp, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col {0.f};
	float alpha = 0.f;
	int alpha_count = 0;
	if(ray_level <= (r_depth_ + additional_depth))
	{
		const Material *material = sp.material_;
		const BsdfFlags &mat_bsdfs = sp.mat_data_->bsdf_flags_;
		// dispersive effects with recursive raytracing:
		if(bsdfs.hasAny(BsdfFlags::Dispersive) && chromatic_enabled)
		{
			const auto result = dispersive(accelerator, thread_id, ray_level, chromatic_enabled, sp, material, bsdfs, wo, additional_depth, ray_division, color_layers, camera, random_generator, pixel_sampling_data);
			col += result.first;
			alpha += result.second;
			++alpha_count;
		}
		if(ray_level < 20 && bsdfs.hasAny(BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter))
		{
			// glossy reflection with recursive raytracing:
			if(bsdfs.hasAny(BsdfFlags::Glossy))
			{
				const auto result = glossy(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, material, mat_bsdfs, bsdfs, wo, additional_depth, ray_division, color_layers, camera, random_generator, pixel_sampling_data);
				col += result.first;
				alpha += result.second;
				++alpha_count;
			}
			//...perfect specular reflection/refraction with recursive raytracing...
			if(bsdfs.hasAny((BsdfFlags::Specular | BsdfFlags::Filter)))
			{
				const Specular specular = material->getSpecular(ray_level, sp.mat_data_.get(), sp, wo, chromatic_enabled, wavelength);
				if(specular.reflect_)
				{
					const auto result = specularReflect(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, material, bsdfs, specular.reflect_.get(), additional_depth, ray_division, color_layers, random_generator, pixel_sampling_data, camera);
					col += result.first;
					alpha += result.second;
					++alpha_count;
				}
				if(specular.refract_)
				{
					const auto result = specularRefract(accelerator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, material, bsdfs, specular.refract_.get(), additional_depth, ray_division, color_layers, random_generator, pixel_sampling_data, camera);
					col += result.first;
					alpha += result.second;
					++alpha_count;
				}
			}
		}
	}
	return {col, (alpha_count > 0 ? alpha / alpha_count : 1.f)};
}

END_YAFARAY
