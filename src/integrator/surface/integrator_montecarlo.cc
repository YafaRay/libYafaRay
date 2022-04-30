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

#include <memory>
#include "geometry/surface.h"
#include "common/layers.h"
#include "color/color_layers.h"
#include "common/logger.h"
#include "material/material.h"
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
#include "geometry/primitive/primitive.h"
#include "geometry/object/object.h"

BEGIN_YAFARAY

//Constructor and destructor defined here to avoid issues with std::unique_ptr<Pdf1D> being Pdf1D incomplete in the header (forward declaration)
MonteCarloIntegrator::MonteCarloIntegrator(RenderControl &render_control, Logger &logger) : TiledIntegrator(render_control, logger)
{
	caustic_map_ = std::make_unique<PhotonMap>(logger);
	caustic_map_->setName("Caustic Photon Map");
}

MonteCarloIntegrator::~MonteCarloIntegrator() = default;

Rgb MonteCarloIntegrator::estimateAllDirectLight(RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col{0.f};
	unsigned int loffs = 0;
	for(const auto &light : lights_)
	{
		col += doLightEstimation(random_generator, color_layers, chromatic_enabled, wavelength, light, sp, wo, loffs, ray_division, pixel_sampling_data);
		++loffs;
	}
	if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::Shadow)) *color_layer *= 1.f / static_cast<float>(loffs);
	}
	return col;
}

Rgb MonteCarloIntegrator::estimateOneDirectLight(RandomGenerator &random_generator, int thread_id, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, int n, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	const int num_lights = lights_.size();
	if(num_lights == 0) return Rgb{0.f};
	Halton hal_2(2, image_film_->getBaseSamplingOffset() + correlative_sample_number_[thread_id] - 1); //Probably with this change the parameter "n" is no longer necessary, but I will keep it just in case I have to revert this change!
	const int lnum = std::min(static_cast<int>(hal_2.getNext() * static_cast<float>(num_lights)), num_lights - 1);
	++correlative_sample_number_[thread_id];
	return doLightEstimation(random_generator, nullptr, chromatic_enabled, wavelength, lights_[lnum], sp, wo, lnum, ray_division, pixel_sampling_data) * num_lights;
}

Rgb MonteCarloIntegrator::diracLight(RandomGenerator &random_generator, ColorLayers *color_layers, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows) const
{
	Ray light_ray;
	Rgb lcol;
	if(light->illuminate(sp, lcol, light_ray))
	{
		Rgb col{0.f};
		light_ray.from_ = sp.p_;
		Rgba *color_layer_shadow = nullptr;
		Rgba *color_layer_diffuse = nullptr;
		Rgba *color_layer_diffuse_no_shadow = nullptr;
		Rgba *color_layer_glossy = nullptr;
		if(color_layers)
		{
			if(color_layers->getFlags().hasAny(LayerDef::Flags::DiffuseLayers))
			{
				color_layer_diffuse = color_layers->find(LayerDef::Diffuse);
				color_layer_diffuse_no_shadow = color_layers->find(LayerDef::DiffuseNoShadow);
			}
			if(color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
			{
				color_layer_glossy = color_layers->find(LayerDef::Glossy);
				color_layer_shadow = color_layers->find(LayerDef::Shadow);
			}
		}
		if(shadow_bias_auto_) light_ray.tmin_ = shadow_bias_ * std::max(1.f, sp.p_.length());
		else light_ray.tmin_ = shadow_bias_;
		Rgb scol{0.f};
		bool shadowed = false;
		const Primitive *shadow_casting_primitive = nullptr;
		if(cast_shadows)
		{
			if(tr_shad_) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator_->isShadowed(light_ray, s_depth_, shadow_bias_, camera_);
			else std::tie(shadowed, shadow_casting_primitive) = accelerator_->isShadowed(light_ray, shadow_bias_);
		}
		const float angle_light_normal = sp.material_->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_);	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal
		if(!shadowed || color_layer_diffuse_no_shadow)
		{
			if(!shadowed && color_layer_shadow) *color_layer_shadow += Rgba{1.f};
			const Rgb surf_col = sp.eval(wo, light_ray.dir_, BsdfFlags::All);
			const Rgb transmit_col = vol_integrator_ ? vol_integrator_->transmittance(random_generator, light_ray) : Rgb{1.f};
			const Rgba tmp_col_no_shadow{surf_col * lcol * angle_light_normal * transmit_col};
			if(tr_shad_ && cast_shadows) lcol *= scol;
			if(color_layers)
			{
				if(color_layer_diffuse_no_shadow) *color_layer_diffuse_no_shadow += tmp_col_no_shadow;
				if(!shadowed)
				{
					if(color_layer_diffuse) *color_layer_diffuse += Rgba{sp.eval(wo, light_ray.dir_, BsdfFlags::Diffuse) * lcol * angle_light_normal * transmit_col};
					if(color_layer_glossy) *color_layer_glossy += Rgba{sp.eval(wo, light_ray.dir_, BsdfFlags::Glossy, true) * lcol * angle_light_normal * transmit_col};
				}
			}
			if(!shadowed) col += surf_col * lcol * angle_light_normal * transmit_col;
		}
		if(color_layers)
		{
			if(shadowed && color_layers->getFlags().hasAny(LayerDef::Flags::IndexLayers) && shadow_casting_primitive)
			{
				Rgba *color_layer_mat_index_mask_shadow = color_layers->find(LayerDef::MatIndexMaskShadow);
				Rgba *color_layer_obj_index_mask_shadow = color_layers->find(LayerDef::ObjIndexMaskShadow);
				float mask_obj_index = 0.f, mask_mat_index = 0.f;
				if(const Object *casting_object = shadow_casting_primitive->getObject()) mask_obj_index = casting_object->getAbsObjectIndex();    //Object index of the object casting the shadow
				if(const Material *casting_material = shadow_casting_primitive->getMaterial()) mask_mat_index = casting_material->getAbsMaterialIndex();    //Material index of the object casting the shadow
				if(color_layer_mat_index_mask_shadow && mask_mat_index == mask_params_.mat_index_) *color_layer_mat_index_mask_shadow += Rgba{1.f};
				if(color_layer_obj_index_mask_shadow && mask_obj_index == mask_params_.obj_index_) *color_layer_obj_index_mask_shadow += Rgba{1.f};
			}
			if(color_layers->getFlags().hasAny(LayerDef::Flags::DebugLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugLightEstimationLightDirac)) *color_layer += Rgba{col};
			}
		}
		return col;
	}
	else return Rgb{0.f};
}

Rgb MonteCarloIntegrator::areaLightSampleLight(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples) const
{
	Ray light_ray;
	light_ray.from_ = sp.p_;
	Rgb col{0.f};
	std::unique_ptr<ColorLayerAccum> layer_shadow;
	std::unique_ptr<ColorLayerAccum> layer_mat_index_mask_shadow;
	std::unique_ptr<ColorLayerAccum> layer_obj_index_mask_shadow;
	std::unique_ptr<ColorLayerAccum> layer_diffuse;
	std::unique_ptr<ColorLayerAccum> layer_diffuse_no_shadow;
	std::unique_ptr<ColorLayerAccum> layer_glossy;
	if(color_layers)
	{
		if(color_layers->getFlags().hasAny(LayerDef::Flags::IndexLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexMaskShadow))
				layer_mat_index_mask_shadow = std::make_unique<ColorLayerAccum>(color_layer);
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexMaskShadow))
				layer_obj_index_mask_shadow = std::make_unique<ColorLayerAccum>(color_layer);
		}
		if(color_layers->getFlags().hasAny(LayerDef::Flags::DiffuseLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Diffuse))
				layer_diffuse = std::make_unique<ColorLayerAccum>(color_layer);
			if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseNoShadow))
				layer_diffuse_no_shadow = std::make_unique<ColorLayerAccum>(color_layer);
		}
		if(color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Shadow))
				layer_shadow = std::make_unique<ColorLayerAccum>(color_layer);
			if(Rgba *color_layer = color_layers->find(LayerDef::Glossy))
				layer_glossy = std::make_unique<ColorLayerAccum>(color_layer);
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
			if(shadow_bias_auto_) light_ray.tmin_ = shadow_bias_ * std::max(1.f, sp.p_.length());
			else light_ray.tmin_ = shadow_bias_;
			bool shadowed = false;
			const Primitive *shadow_casting_primitive = nullptr;
			if(cast_shadows)
			{
				if(tr_shad_) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator_->isShadowed(light_ray, s_depth_, shadow_bias_, camera_);
				else std::tie(shadowed, shadow_casting_primitive) = accelerator_->isShadowed(light_ray, shadow_bias_);
			}
			if((!shadowed && ls.pdf_ > 1e-6f) || layer_diffuse_no_shadow)
			{
				const Rgb ls_col_no_shadow = ls.col_;
				if(tr_shad_ && cast_shadows) ls.col_ *= scol;
				if(vol_integrator_)
				{
					const Rgb transmit_col = vol_integrator_->transmittance(random_generator, light_ray);
					ls.col_ *= transmit_col;
				}
				const Rgb surf_col = sp.eval(wo, light_ray.dir_, BsdfFlags::All);
				if(layer_shadow && !shadowed && ls.pdf_ > 1e-6f) layer_shadow->accum_ += Rgba{1.f};
				const float angle_light_normal = sp.material_->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_);    //If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal
				float w = 1.f;
				if(light->canIntersect())
				{
					const float m_pdf = sp.pdf(wo, light_ray.dir_, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
					if(m_pdf > 1e-6f)
					{
						const float l_2 = ls.pdf_ * ls.pdf_;
						const float m_2 = m_pdf * m_pdf;
						w = l_2 / (l_2 + m_2);
					}
				}
				if(color_layers)
				{
					if(layer_diffuse || layer_diffuse_no_shadow)
					{
						const Rgb tmp_col_no_light_color = sp.eval(wo, light_ray.dir_, BsdfFlags::Diffuse) * angle_light_normal * w / ls.pdf_;
						if(layer_diffuse_no_shadow) layer_diffuse_no_shadow->accum_ += Rgba{tmp_col_no_light_color * ls_col_no_shadow};
						if(layer_diffuse && !shadowed && ls.pdf_ > 1e-6f) layer_diffuse->accum_ += Rgba{tmp_col_no_light_color * ls.col_};
					}
					if(layer_glossy)
					{
						const Rgb tmp_col = sp.eval(wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal * w / ls.pdf_;
						if(!shadowed && ls.pdf_ > 1e-6f) layer_glossy->accum_ += Rgba{tmp_col};
					}
				}
				if(!shadowed && ls.pdf_ > 1e-6f) col += surf_col * ls.col_ * angle_light_normal * w / ls.pdf_;
			}
			if(color_layers && (shadowed || ls.pdf_ <= 1e-6f) && color_layers->getFlags().hasAny(LayerDef::Flags::IndexLayers) && shadow_casting_primitive)
			{
				float mask_obj_index = 0.f, mask_mat_index = 0.f;
				if(const Object *casting_object = shadow_casting_primitive->getObject()) mask_obj_index = casting_object->getAbsObjectIndex();    //Object index of the object casting the shadow
				if(const Material *casting_material = shadow_casting_primitive->getMaterial()) mask_mat_index = casting_material->getAbsMaterialIndex();    //Material index of the object casting the shadow
				if(layer_mat_index_mask_shadow && mask_mat_index == mask_params_.mat_index_) layer_mat_index_mask_shadow->accum_ += Rgba{1.f};
				if(layer_obj_index_mask_shadow && mask_obj_index == mask_params_.obj_index_) layer_obj_index_mask_shadow->accum_ += Rgba{1.f};
			}
		}
	}
	const Rgb col_result = col * inv_num_samples;
	if(color_layers)
	{
		if(color_layers->getFlags().hasAny(LayerDef::Flags::IndexLayers))
		{
			if(layer_mat_index_mask_shadow) *layer_mat_index_mask_shadow->color_ += layer_mat_index_mask_shadow->accum_ * inv_num_samples;
			if(layer_obj_index_mask_shadow) *layer_obj_index_mask_shadow->color_ += layer_obj_index_mask_shadow->accum_ * inv_num_samples;
		}
		if(color_layers->getFlags().hasAny(LayerDef::Flags::DiffuseLayers))
		{
			if(layer_diffuse) *layer_diffuse->color_ += layer_diffuse->accum_ * inv_num_samples;
			if(layer_diffuse_no_shadow) *layer_diffuse_no_shadow->color_ += layer_diffuse_no_shadow->accum_ * inv_num_samples;
		}
		if(color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
		{
			if(layer_shadow) *layer_shadow->color_ += layer_shadow->accum_ * inv_num_samples;
			if(layer_glossy) *layer_glossy->color_ += layer_glossy->accum_ * inv_num_samples;
		}
		if(color_layers->getFlags().hasAny(LayerDef::Flags::DebugLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugLightEstimationLightSampling)) *color_layer += Rgba{col_result};
		}
	}
	return col_result;
}

Rgb MonteCarloIntegrator::areaLightSampleMaterial(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const Light *light, const Vec3 &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples) const
{
	if(light->canIntersect()) // sample from BSDF to complete MIS
	{
		Rgb col_result{0.f};
		std::unique_ptr<ColorLayerAccum> layer_diffuse;
		std::unique_ptr<ColorLayerAccum> layer_diffuse_no_shadow;
		std::unique_ptr<ColorLayerAccum> layer_glossy;
		if(color_layers)
		{
			if(color_layers->getFlags().hasAny(LayerDef::Flags::DiffuseLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Diffuse))
					layer_diffuse = std::make_unique<ColorLayerAccum>(color_layer);
				if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseNoShadow))
					layer_diffuse_no_shadow = std::make_unique<ColorLayerAccum>(color_layer);
			}
			if(color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Glossy))
					layer_glossy = std::make_unique<ColorLayerAccum>(color_layer);
			}
		}
		Ray light_ray;
		light_ray.from_ = sp.p_;
		Rgb col{0.f};
		Rgb lcol;
		Ray b_ray;
		for(unsigned int i = 0; i < num_samples; ++i)
		{
			if(ray_min_dist_auto_) b_ray.tmin_ = ray_min_dist_ * std::max(1.f, sp.p_.length());
			else b_ray.tmin_ = ray_min_dist_;
			b_ray.from_ = sp.p_;
			const float s_1 = hal_2.getNext();
			const float s_2 = hal_3.getNext();
			float W = 0.f;
			Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
			const Rgb surf_col = sp.sample(wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera_);
			float light_pdf;
			if(s.pdf_ > 1e-6f && light->intersect(b_ray, b_ray.tmax_, lcol, light_pdf))
			{
				Rgb scol{0.f};
				bool shadowed = false;
				const Primitive *shadow_casting_primitive = nullptr;
				if(cast_shadows)
				{
					if(tr_shad_) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator_->isShadowed(b_ray, s_depth_, shadow_bias_, camera_);
					else std::tie(shadowed, shadow_casting_primitive) = accelerator_->isShadowed(b_ray, shadow_bias_);
				}
				if((!shadowed && light_pdf > 1e-6f) || layer_diffuse_no_shadow)
				{
					if(tr_shad_ && cast_shadows) lcol *= scol;
					if(vol_integrator_)
					{
						const Rgb transmit_col = vol_integrator_->transmittance(random_generator, b_ray);
						lcol *= transmit_col;
					}
					const float l_pdf = 1.f / light_pdf;
					const float l_2 = l_pdf * l_pdf;
					const float m_2 = s.pdf_ * s.pdf_;
					const float w = m_2 / (l_2 + m_2);
					if(color_layers)
					{
						if(layer_diffuse || layer_diffuse_no_shadow)
						{
							const Rgb tmp_col = sp.sample(wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera_) * lcol * w * W;
							if(layer_diffuse_no_shadow) layer_diffuse_no_shadow->accum_ += Rgba{tmp_col};
							if(layer_diffuse && !shadowed && light_pdf > 1e-6f && s.sampled_flags_.hasAny(BsdfFlags::Diffuse)) layer_diffuse->accum_ += Rgba{tmp_col};
						}
						if(layer_glossy)
						{
							const Rgb tmp_col = sp.sample(wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera_) * lcol * w * W;
							if(!shadowed && light_pdf > 1e-6f && s.sampled_flags_.hasAny(BsdfFlags::Glossy)) layer_glossy->accum_ += Rgba{tmp_col};
						}
					}
					if(!shadowed && light_pdf > 1e-6f) col += surf_col * lcol * w * W;
				}
			}
		}
		col_result = col * inv_num_samples;
		if(color_layers)
		{
			if(color_layers->getFlags().hasAny(LayerDef::Flags::DiffuseLayers))
			{
				if(layer_diffuse) *layer_diffuse->color_ += layer_diffuse->accum_ * inv_num_samples;
				if(layer_diffuse_no_shadow) *layer_diffuse_no_shadow->color_ += layer_diffuse_no_shadow->accum_ * inv_num_samples;
			}
			if(color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
			{
				if(layer_glossy) *layer_glossy->color_ += layer_glossy->accum_ * inv_num_samples;
			}
			if(color_layers->getFlags().hasAny(LayerDef::Flags::DebugLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugLightEstimationMatSampling)) *color_layer += Rgba{col_result};
			}
		}
		return col_result;
	}
	else return Rgb{0.f};
}

Rgb MonteCarloIntegrator::doLightEstimation(RandomGenerator &random_generator, ColorLayers *color_layers, bool chromatic_enabled, float wavelength, const Light *light, const SurfacePoint &sp, const Vec3 &wo, unsigned int loffs, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col{0.f};
	const bool cast_shadows = light->castShadows() && sp.material_->getReceiveShadows();
	if(light->diracLight()) // handle lights with delta distribution, e.g. point and directional lights
	{
		col += diracLight(random_generator, color_layers, light, wo, sp, cast_shadows);
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
		col += areaLightSampleLight(hal_2, hal_3, random_generator, color_layers, light, wo, sp, cast_shadows, num_samples, inv_num_samples);
		hal_2.setStart(offs - 1);
		hal_3.setStart(offs - 1);
		col += areaLightSampleMaterial(hal_2, hal_3, random_generator, color_layers, chromatic_enabled, wavelength, light, wo, sp, cast_shadows, num_samples, inv_num_samples);
	}
	return col;
}

Rgb MonteCarloIntegrator::causticPhotons(ColorLayers *color_layers, const Ray &ray, const SurfacePoint &sp, const Vec3 &wo, float clamp_indirect, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search)
{
	Rgb col = MonteCarloIntegrator::estimateCausticPhotons(sp, wo, caustic_map, caustic_radius, n_caus_search);
	if(clamp_indirect > 0.f) col.clampProportionalRgb(clamp_indirect);
	if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::Indirect)) *color_layer += Rgba{col};
	}
	return col;
}

void MonteCarloIntegrator::causticWorker(FastRandom &fast_random, unsigned int &total_photons_shot, int thread_id, const Pdf1D *light_power_d_caustic, const std::vector<const Light *> &lights_caustic, int pb_step)
{
	bool done = false;
	const int num_lights_caustic = lights_caustic.size();
	const auto f_num_lights = static_cast<float>(num_lights_caustic);
	unsigned int curr = 0;
	const unsigned int n_caus_photons_thread = 1 + ((n_caus_photons_ - 1) / num_threads_photons_);
	std::vector<Photon> local_caustic_photons;
	std::unique_ptr<const SurfacePoint> hit_prev, hit_curr;
	local_caustic_photons.clear();
	local_caustic_photons.reserve(n_caus_photons_thread);
	while(!done)
	{
		const unsigned int haltoncurr = curr + n_caus_photons_thread * thread_id;
		const float wavelength = sample::riS(haltoncurr);
		const float s_1 = sample::riVdC(haltoncurr);
		const float s_2 = Halton::lowDiscrepancySampling(fast_random, 2, haltoncurr);
		const float s_3 = Halton::lowDiscrepancySampling(fast_random, 3, haltoncurr);
		const float s_4 = Halton::lowDiscrepancySampling(fast_random, 4, haltoncurr);
		const float s_l = static_cast<float>(haltoncurr) / static_cast<float>(n_caus_photons_);
		float light_num_pdf;
		const int light_num = light_power_d_caustic->dSample(s_l, light_num_pdf);
		if(light_num >= num_lights_caustic)
		{
			logger_.logError(getName(), ": lightPDF sample error! ", s_l, "/", light_num);
			return;
		}
		Ray ray;
		float light_pdf;
		Rgb pcol = lights_caustic[light_num]->emitPhoton(s_1, s_2, s_3, s_4, ray, light_pdf);
		ray.tmin_ = ray_min_dist_;
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
		while(true)
		{
			std::tie(hit_curr, ray.tmax_) = accelerator_->intersect(ray, camera_);
			if(!hit_curr) break;
			// check for volumetric effects, based on the material from the previous photon bounce
			Rgb transm(1.f);
			if(material_prev && hit_prev && mat_bsdfs_prev.hasAny(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = material_prev->getVolumeHandler(hit_prev->ng_ * ray.dir_ < 0))
				{
					transm = vol->transmittance(ray);
				}
			}
			const Vec3 wi{-ray.dir_};
			const BsdfFlags &mat_bsdfs = hit_curr->mat_data_->bsdf_flags_;
			if(mat_bsdfs.hasAny((BsdfFlags::Diffuse | BsdfFlags::Glossy)))
			{
				//deposit caustic photon on surface
				if(caustic_photon)
				{
					Photon np(wi, hit_curr->p_, pcol);
					local_caustic_photons.emplace_back(np);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == caus_depth_) break;
			// scatter photon
			const int d_5 = 3 * n_bounces + 5;
			//int d6 = d5 + 1;

			const float s_5 = Halton::lowDiscrepancySampling(fast_random, d_5, haltoncurr);
			const float s_6 = Halton::lowDiscrepancySampling(fast_random, d_5 + 1, haltoncurr);
			const float s_7 = Halton::lowDiscrepancySampling(fast_random, d_5 + 2, haltoncurr);

			PSample sample(s_5, s_6, s_7, BsdfFlags::AllSpecular | BsdfFlags::Glossy | BsdfFlags::Filter | BsdfFlags::Dispersive, pcol, transm);
			Vec3 wo;
			bool scattered = hit_curr->scatterPhoton(wi, wo, sample, chromatic_enabled, wavelength, camera_);
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
			ray.from_ = hit_curr->p_;
			ray.dir_ = wo;
			ray.tmin_ = ray_min_dist_;
			ray.tmax_ = -1.f;
			material_prev = hit_curr->material_;
			mat_bsdfs_prev = mat_bsdfs;
			std::swap(hit_prev, hit_curr);
			++n_bounces;
		}
		++curr;
		if(curr % pb_step == 0)
		{
			intpb_->update();
			if(render_control_.canceled()) { return; }
		}
		done = (curr >= n_caus_photons_thread);
	}
	caustic_map_->mutx_.lock();
	caustic_map_->appendVector(local_caustic_photons, curr);
	total_photons_shot += curr;
	caustic_map_->mutx_.unlock();
}

bool MonteCarloIntegrator::createCausticMap(FastRandom &fast_random)
{
	if(photon_map_processing_ == PhotonsLoad)
	{
		intpb_->setTag("Loading caustic photon map from file...");
		const std::string filename = image_film_->getFilmSavePath() + "_caustic.photonmap";
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
	caustic_map_->setNumThreadsPkDtree(num_threads_photons_);

	const std::vector<const Light *> lights_caustic = render_view_->getLightsEmittingCausticPhotons();
	if(!lights_caustic.empty())
	{
		const int num_lights_caustic = lights_caustic.size();
		const auto f_num_lights_caustic = static_cast<float>(num_lights_caustic);
		std::vector<float> energies_caustic(num_lights_caustic);
		for(int i = 0; i < num_lights_caustic; ++i) energies_caustic[i] = lights_caustic[i]->totalEnergy().energy();
		auto light_power_d_caustic = std::make_unique<Pdf1D>(energies_caustic);

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for caustics map:");

		for(int i = 0; i < num_lights_caustic; ++i)
		{
			Ray ray;
			float light_pdf;
			Rgb pcol = lights_caustic[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			const float light_num_pdf = light_power_d_caustic->function(i) * light_power_d_caustic->invIntegral();
			pcol *= f_num_lights_caustic * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", lights_caustic[i]->getName(), "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}

		logger_.logInfo(getName(), ": Building caustics photon map...");
		intpb_->init(128, logger_.getConsoleLogColorsEnabled());
		const int pb_step = std::max(1U, n_caus_photons_ / 128);
		intpb_->setTag("Building caustics photon map...");

		unsigned int curr = 0;

		n_caus_photons_ = std::max(static_cast<unsigned int>(num_threads_photons_), (n_caus_photons_ / num_threads_photons_) * num_threads_photons_); //rounding the number of diffuse photons, so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		logger_.logParams(getName(), ": Shooting ", n_caus_photons_, " photons across ", num_threads_photons_, " threads (", (n_caus_photons_ / num_threads_photons_), " photons/thread)");

		std::vector<std::thread> threads;
		threads.reserve(num_threads_photons_);
		for(int i = 0; i < num_threads_photons_; ++i) threads.emplace_back(&MonteCarloIntegrator::causticWorker, this, std::ref(fast_random), std::ref(curr), i, light_power_d_caustic.get(), lights_caustic, pb_step);
		for(auto &t : threads) t.join();

		intpb_->done();
		intpb_->setTag("Caustic photon map built.");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		logger_.logInfo(getName(), ": Shot ", curr, " caustic photons from ", num_lights_caustic, " light(s).");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored caustic photons: ", caustic_map_->nPhotons());

		if(caustic_map_->nPhotons() > 0)
		{
			intpb_->setTag("Building caustic photons kd-tree...");
			caustic_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}

		if(photon_map_processing_ == PhotonsGenerateAndSave)
		{
			intpb_->setTag("Saving caustic photon map to file...");
			std::string filename = image_film_->getFilmSavePath() + "_caustic.photonmap";
			logger_.logInfo(getName(), ": Saving caustic photon map to: ", filename);
			if(caustic_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic map saved.");
		}
	}
	else if(logger_.isVerbose()) logger_.logVerbose(getName(), ": No caustic source lights found, skiping caustic map building...");
	return true;
}

Rgb MonteCarloIntegrator::estimateCausticPhotons(const SurfacePoint &sp, const Vec3 &wo, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search)
{
	if(!caustic_map->ready()) return Rgb{0.f};
	const auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_caus_search]);//(foundPhoton_t *)alloca(nCausSearch * sizeof(foundPhoton_t));
	float g_radius_square = caustic_radius * caustic_radius;
	const int n_gathered = caustic_map->gather(sp.p_, gathered.get(), n_caus_search, g_radius_square);
	g_radius_square = 1.f / g_radius_square;
	Rgb sum {0.f};
	if(n_gathered > 0)
	{
		for(int i = 0; i < n_gathered; ++i)
		{
			const Photon *photon = gathered[i].photon_;
			const Rgb surf_col = sp.eval(wo, photon->direction(), BsdfFlags::All);
			const float k = sample::kernel(gathered[i].dist_square_, g_radius_square);
			sum += surf_col * k * photon->color();
		}
		sum *= 1.f / static_cast<float>(caustic_map->nPaths());
	}
	return sum;
}

std::pair<Rgb, float> MonteCarloIntegrator::dispersive(FastRandom &fast_random, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
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

		ray_division_new.decorrelation_1_ = Halton::lowDiscrepancySampling(fast_random, 2 * ray_level + 1, branch + pixel_sampling_data.offset_);
		ray_division_new.decorrelation_2_ = Halton::lowDiscrepancySampling(fast_random, 2 * ray_level + 2, branch + pixel_sampling_data.offset_);
		ray_division_new.offset_ = branch;
		++branch;
		Sample s(0.5f, 0.5f, BsdfFlags::Reflect | BsdfFlags::Transmit | BsdfFlags::Dispersive);
		Vec3 wi;
		const Rgb mcol = sp.sample(wo, wi, s, w, chromatic_enabled, wavelength_dispersive, camera_);

		if(s.pdf_ > 1.0e-6f && s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
		{
			const Rgb wl_col = spectrum::wl2Rgb(wavelength_dispersive);
			Ray ref_ray(sp.p_, wi, ray_min_dist_);
			auto [integ_col, integ_alpha] = integrate(ref_ray, fast_random, random_generator, nullptr, thread_id, ray_level, false, wavelength_dispersive, additional_depth, ray_division_new, pixel_sampling_data);
			integ_col *= mcol * wl_col * w;
			dcol += integ_col;
			if(color_layers) dcol_trans_accum += integ_col;
			alpha_accum += integ_alpha;
			if(!ref_ray_chromatic_volume) ref_ray_chromatic_volume = std::make_unique<Ray>(ref_ray, Ray::DifferentialsCopy::No);
		}
	}
	if(ref_ray_chromatic_volume && bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray_chromatic_volume->dir_ < 0))
		{
			dcol *= vol->transmittance(*ref_ray_chromatic_volume);
		}
	}
	if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::Trans))
		{
			dcol_trans_accum *= d_1;
			*color_layer += Rgba{dcol_trans_accum};
		}
	}
	return {dcol * d_1, alpha_accum * d_1};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossy(FastRandom &fast_random, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	if(!sp.mat_data_->bsdf_flags_.hasAny(BsdfFlags::Reflect)) return {Rgb{0.f}, -1.f}; //If alpha is -1.f we consider the result invalid and do not use it to accumulate alpha
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
		ray_division_new.decorrelation_1_ = Halton::lowDiscrepancySampling(fast_random, 2 * ray_level + 1, branch + pixel_sampling_data.offset_);
		ray_division_new.decorrelation_2_ = Halton::lowDiscrepancySampling(fast_random, 2 * ray_level + 2, branch + pixel_sampling_data.offset_);
		ray_division_new.offset_ = branch;
		++offs;
		++branch;
		const float s_1 = hal_2.getNext();
		const float s_2 = hal_3.getNext();
		if(sp.mat_data_->bsdf_flags_.hasAny(BsdfFlags::Transmit))
		{
			Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::AllGlossy);
			Rgb mcol[2];
			float w[2];
			Vec3 dir[2];

			mcol[0] = sp.sample(wo, dir, mcol[1], s, w, chromatic_enabled, wavelength);

			if(!s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
			{
				const auto [reflect_col, reflect_alpha] = glossyReflect(fast_random, random_generator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, bsdfs, additional_depth, pixel_sampling_data, ray_division_new, mcol[0], w[0], dir[0]);
				gcol += reflect_col;
				if(color_layers) gcol_reflect_accum += reflect_col;
				alpha_accum += reflect_alpha;
			}
			if(s.sampled_flags_.hasAny(BsdfFlags::Transmit))
			{
				const auto [transmit_col, transmit_alpha] = glossyTransmit(fast_random, random_generator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, bsdfs, additional_depth, pixel_sampling_data, ray_division_new, mcol[1], w[1], dir[1]);
				gcol += transmit_col;
				if(color_layers) gcol_transmit_accum += transmit_col;
				alpha_accum += transmit_alpha;
			}
		}
		else
		{
			const auto [reflect_col, reflect_alpha] = glossyReflectNoTransmit(fast_random, random_generator, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, bsdfs, wo, additional_depth, pixel_sampling_data, ray_division_new, s_1, s_2);
			gcol += reflect_col;
			if(color_layers) gcol_indirect_accum += reflect_col;
			alpha_accum += reflect_alpha;
		}
	}

	if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::GlossyIndirect))
		{
			gcol_indirect_accum *= inverse_ray_samples_glossy;
			*color_layer += Rgba{gcol_indirect_accum};
		}
		if(Rgba *color_layer = color_layers->find(LayerDef::Trans))
		{
			gcol_reflect_accum *= inverse_ray_samples_glossy;
			*color_layer += Rgba{gcol_reflect_accum};
		}
		if(Rgba *color_layer = color_layers->find(LayerDef::GlossyIndirect))
		{
			gcol_transmit_accum *= inverse_ray_samples_glossy;
			*color_layer += Rgba{gcol_transmit_accum};
		}
	}
	return {gcol * inverse_ray_samples_glossy, alpha_accum * inverse_ray_samples_glossy};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyReflect(FastRandom &fast_random, RandomGenerator &random_generator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &reflect_color, float w, const Vec3 &dir) const
{
	Ray ref_ray = Ray(sp.p_, dir, ray_min_dist_);
	if(ray.differentials_) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
	auto [integ_col, integ_alpha] = integrate(ref_ray, fast_random, random_generator, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division_new, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = sp.material_->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= reflect_color * w;
	return {integ_col, integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyTransmit(FastRandom &fast_random, RandomGenerator &random_generator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &transmit_col, float w, const Vec3 &dir) const
{
	Ray ref_ray = Ray(sp.p_, dir, ray_min_dist_);
	if(ray.differentials_) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp.material_->getMatIor());
	auto [integ_col, integ_alpha] = integrate(ref_ray, fast_random, random_generator, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division_new, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = sp.material_->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= transmit_col * w;
	return {integ_col, integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyReflectNoTransmit(FastRandom &fast_random, RandomGenerator &random_generator, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const BsdfFlags &bsdfs, const Vec3 &wo, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const float s_1, const float s_2) const
{
	float w = 0.f;
	Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Reflect);
	Vec3 wi;
	const Rgb mcol = sp.sample(wo, wi, s, w, chromatic_enabled, wavelength, camera_);
	Ray ref_ray(sp.p_, wi, ray_min_dist_);
	if(ray.differentials_)
	{
		if(s.sampled_flags_.hasAny(BsdfFlags::Reflect)) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
		else if(s.sampled_flags_.hasAny(BsdfFlags::Transmit)) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp.material_->getMatIor());
	}
	auto [integ_col, integ_alpha] = integrate(ref_ray, fast_random, random_generator, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division_new, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = sp.material_->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= mcol * w;
	return {integ_col, integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::specularReflect(FastRandom &fast_random, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *reflect_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Ray ref_ray(sp.p_, reflect_data->dir_, ray_min_dist_);
	if(ray.differentials_) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
	auto [integ_col, integ_alpha] = integrate(ref_ray, fast_random, random_generator, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division, pixel_sampling_data);
	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= reflect_data->col_;
	if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::ReflectPerfect)) *color_layer += Rgba{integ_col};
	}
	return {integ_col, integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::specularRefract(FastRandom &fast_random, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, const BsdfFlags &bsdfs, const DirectionColor *refract_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Ray ref_ray;
	float transp_bias_factor = material->getTransparentBiasFactor();
	if(transp_bias_factor > 0.f)
	{
		const bool transpbias_multiply_raydepth = material->getTransparentBiasMultiplyRayDepth();
		if(transpbias_multiply_raydepth) transp_bias_factor *= ray_level;
		ref_ray = Ray(sp.p_ + refract_data->dir_ * transp_bias_factor, refract_data->dir_, ray_min_dist_);
	}
	else ref_ray = Ray(sp.p_, refract_data->dir_, ray_min_dist_);

	if(ray.differentials_) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, material->getMatIor());
	auto [integ_col, integ_alpha] = integrate(ref_ray, fast_random, random_generator, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division, pixel_sampling_data);

	if(bsdfs.hasAny(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= refract_data->col_;
	if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::RefractPerfect)) *color_layer += Rgba{integ_col};
	}
	return {integ_col, integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::recursiveRaytrace(FastRandom &fast_random, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, const BsdfFlags &bsdfs, const SurfacePoint &sp, const Vec3 &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col {0.f};
	float alpha = 0.f;
	int alpha_count = 0;
	if(ray_level <= (r_depth_ + additional_depth))
	{
		// dispersive effects with recursive raytracing:
		if(bsdfs.hasAny(BsdfFlags::Dispersive) && chromatic_enabled)
		{
			const auto [dispersive_col, dispersive_alpha] = dispersive(fast_random, random_generator, color_layers, thread_id, ray_level, chromatic_enabled, sp, sp.material_, bsdfs, wo, additional_depth, ray_division, pixel_sampling_data);
			col += dispersive_col;
			alpha += dispersive_alpha;
			++alpha_count;
		}
		if(ray_level < 20 && bsdfs.hasAny(BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter))
		{
			// glossy reflection with recursive raytracing:
			if(bsdfs.hasAny(BsdfFlags::Glossy))
			{
				const auto [glossy_col, glossy_alpha] = glossy(fast_random, random_generator, color_layers, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, bsdfs, wo, additional_depth, ray_division, pixel_sampling_data);
				if(alpha != -1.f) //If alpha is -1.f we consider the result invalid and do not use it to accumulate alpha
				{
					col += glossy_col;
					alpha += glossy_alpha;
					++alpha_count;
				}
			}
			//...perfect specular reflection/refraction with recursive raytracing...
			if(bsdfs.hasAny((BsdfFlags::Specular | BsdfFlags::Filter)))
			{
				const Specular specular = sp.getSpecular(ray_level, wo, chromatic_enabled, wavelength);
				if(specular.reflect_)
				{
					const auto [specular_col, specular_alpha] = specularReflect(fast_random, random_generator, color_layers, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, sp.material_, bsdfs, specular.reflect_.get(), additional_depth, ray_division, pixel_sampling_data);
					col += specular_col;
					alpha += specular_alpha;
					++alpha_count;
				}
				if(specular.refract_)
				{
					const auto [specular_col, specular_alpha] = specularRefract(fast_random, random_generator, color_layers, thread_id, ray_level, chromatic_enabled, wavelength, ray, sp, sp.material_, bsdfs, specular.refract_.get(), additional_depth, ray_division, pixel_sampling_data);
					col += specular_col;
					alpha += specular_alpha;
					++alpha_count;
				}
			}
		}
	}
	return {col, (alpha_count > 0 ? alpha / alpha_count : 1.f)};
}

END_YAFARAY
