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
#include "integrator/volume/integrator_volume.h"
#include "param/param.h"
#include <memory>
#include "geometry/surface.h"
#include "common/layers.h"
#include "color/color_layers.h"
#include "common/logger.h"
#include "material/material.h"
#include "light/light.h"
#include "color/spectrum.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "sampler/sample.h"
#include "render/render_data.h"
#include "geometry/primitive/primitive.h"
#include "material/sample.h"
#include "volume/handler/volume_handler.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> MonteCarloIntegrator::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(r_depth_);
	PARAM_META(transparent_shadows_);
	PARAM_META(shadow_depth_);
	PARAM_META(ao_);
	PARAM_META(ao_samples_);
	PARAM_META(ao_distance_);
	PARAM_META(ao_color_);
	PARAM_META(transparent_background_);
	PARAM_META(transparent_background_refraction_);
	return param_meta_map;
}

MonteCarloIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(r_depth_);
	PARAM_LOAD(transparent_shadows_);
	PARAM_LOAD(shadow_depth_);
	PARAM_LOAD(ao_);
	PARAM_LOAD(ao_samples_);
	PARAM_LOAD(ao_distance_);
	PARAM_LOAD(ao_color_);
	PARAM_LOAD(transparent_background_);
	PARAM_LOAD(transparent_background_refraction_);
}

ParamMap MonteCarloIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	PARAM_SAVE(r_depth_);
	PARAM_SAVE(transparent_shadows_);
	PARAM_SAVE(shadow_depth_);
	PARAM_SAVE(ao_);
	PARAM_SAVE(ao_samples_);
	PARAM_SAVE(ao_distance_);
	PARAM_SAVE(ao_color_);
	PARAM_SAVE(transparent_background_);
	PARAM_SAVE(transparent_background_refraction_);
	return param_map;
}

//Constructor and destructor defined here to avoid issues with std::unique_ptr<Pdf1D> being Pdf1D incomplete in the header (forward declaration)
MonteCarloIntegrator::MonteCarloIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : ParentClassType_t(logger, param_result, name, param_map), params_{param_result, param_map}
{
}

MonteCarloIntegrator::~MonteCarloIntegrator() = default;

Rgb MonteCarloIntegrator::estimateAllDirectLight(RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, bool chromatic_enabled, float wavelength, float aa_light_sample_multiplier, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col{0.f};
	unsigned int loffs = 0;
	for(const auto &light : getLights())
	{
		col += doLightEstimation(random_generator, color_layers, camera, chromatic_enabled, wavelength, light, sp, wo, loffs, aa_light_sample_multiplier, ray_division, pixel_sampling_data);
		++loffs;
	}
	if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::Shadow)) *color_layer *= 1.f / static_cast<float>(loffs);
	}
	return col;
}

Rgb MonteCarloIntegrator::estimateOneDirectLight(RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, unsigned int base_sampling_offset, int thread_id, const Camera *camera, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, int n, float aa_light_sample_multiplier, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	const int num_lights = numLights();
	if(num_lights == 0) return Rgb{0.f};
	Halton hal_2(2, base_sampling_offset + correlative_sample_number[thread_id] - 1); //Probably with this change the parameter "n" is no longer necessary, but I will keep it just in case I have to revert this change!
	const int lnum = std::min(static_cast<int>(hal_2.getNext() * static_cast<float>(num_lights)), num_lights - 1);
	++correlative_sample_number[thread_id];
	return doLightEstimation(random_generator, nullptr, camera, chromatic_enabled, wavelength, getLight(lnum), sp, wo, lnum, aa_light_sample_multiplier, ray_division, pixel_sampling_data) * num_lights;
}

Rgb MonteCarloIntegrator::diracLight(RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, float time) const
{
	if(auto[hit, light_ray, lcol]{light->illuminate(sp.p_, time)}; hit)
	{
		Rgb col{0.f};
		light_ray.from_ = sp.p_;
		Rgba *color_layer_shadow = nullptr;
		Rgba *color_layer_diffuse = nullptr;
		Rgba *color_layer_diffuse_no_shadow = nullptr;
		Rgba *color_layer_glossy = nullptr;
		if(color_layers)
		{
			if(color_layers->getFlags().has(LayerDef::Flags::DiffuseLayers))
			{
				color_layer_diffuse = color_layers->find(LayerDef::Diffuse);
				color_layer_diffuse_no_shadow = color_layers->find(LayerDef::DiffuseNoShadow);
			}
			if(color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
			{
				color_layer_glossy = color_layers->find(LayerDef::Glossy);
				color_layer_shadow = color_layers->find(LayerDef::Shadow);
			}
		}
		if(SurfaceIntegrator::params_.shadow_bias_auto_) light_ray.tmin_ = shadow_bias_ * std::max(1.f, sp.p_.length());
		else light_ray.tmin_ = shadow_bias_;
		Rgb scol{0.f};
		bool shadowed = false;
		const Primitive *shadow_casting_primitive = nullptr;
		if(cast_shadows)
		{
			if(params_.transparent_shadows_) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator_->isShadowedTransparentShadow(light_ray, params_.shadow_depth_, camera);
			else std::tie(shadowed, shadow_casting_primitive) = accelerator_->isShadowed(light_ray);
		}
		const float angle_light_normal = sp.getMaterial()->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_);	//If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal
		if(!shadowed || color_layer_diffuse_no_shadow)
		{
			if(!shadowed && color_layer_shadow) *color_layer_shadow += Rgba{1.f};
			const Rgb surf_col = sp.eval(wo, light_ray.dir_, BsdfFlags::All);
			const Rgb transmit_col = vol_integrator_ ? vol_integrator_->transmittance(random_generator, light_ray) : Rgb{1.f};
			const Rgba tmp_col_no_shadow{surf_col * lcol * angle_light_normal * transmit_col};
			if(params_.transparent_shadows_ && cast_shadows) lcol *= scol;
			if(color_layers)
			{
				if(color_layer_diffuse_no_shadow) *color_layer_diffuse_no_shadow += tmp_col_no_shadow;
				if(!shadowed)
				{
					if(color_layer_diffuse) *color_layer_diffuse += sp.eval(wo, light_ray.dir_, BsdfFlags::Diffuse) * lcol * angle_light_normal * transmit_col;
					if(color_layer_glossy) *color_layer_glossy += sp.eval(wo, light_ray.dir_, BsdfFlags::Glossy, true) * lcol * angle_light_normal * transmit_col;
				}
			}
			if(!shadowed) col += surf_col * lcol * angle_light_normal * transmit_col;
		}
		if(color_layers)
		{
			if(shadowed && color_layers->getFlags().has(LayerDef::Flags::IndexLayers) && shadow_casting_primitive)
			{
				Rgba *color_layer_mat_index_mask_shadow = color_layers->find(LayerDef::MatIndexMaskShadow);
				Rgba *color_layer_obj_index_mask_shadow = color_layers->find(LayerDef::ObjIndexMaskShadow);
				float mask_obj_index = 0.f, mask_mat_index = 0.f;
				mask_obj_index = shadow_casting_primitive->getObjectIndex();    //Object index of the object casting the shadow
				if(const Material *casting_material = shadow_casting_primitive->getMaterial()) mask_mat_index = casting_material->getPassIndex();    //Material index of the object casting the shadow
				if(color_layer_mat_index_mask_shadow && mask_mat_index == mask_params_.mat_index_) *color_layer_mat_index_mask_shadow += Rgba{1.f};
				if(color_layer_obj_index_mask_shadow && mask_obj_index == mask_params_.obj_index_) *color_layer_obj_index_mask_shadow += Rgba{1.f};
			}
			if(color_layers->getFlags().has(LayerDef::Flags::DebugLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugLightEstimationLightDirac)) *color_layer += col;
			}
		}
		return col;
	}
	else return Rgb{0.f};
}

Rgb MonteCarloIntegrator::areaLightSampleLight(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples, float time) const
{
	Rgb col{0.f};
	std::unique_ptr<ColorLayerAccum> layer_shadow;
	std::unique_ptr<ColorLayerAccum> layer_mat_index_mask_shadow;
	std::unique_ptr<ColorLayerAccum> layer_obj_index_mask_shadow;
	std::unique_ptr<ColorLayerAccum> layer_diffuse;
	std::unique_ptr<ColorLayerAccum> layer_diffuse_no_shadow;
	std::unique_ptr<ColorLayerAccum> layer_glossy;
	if(color_layers)
	{
		if(color_layers->getFlags().has(LayerDef::Flags::IndexLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexMaskShadow))
				layer_mat_index_mask_shadow = std::make_unique<ColorLayerAccum>(color_layer);
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexMaskShadow))
				layer_obj_index_mask_shadow = std::make_unique<ColorLayerAccum>(color_layer);
		}
		if(color_layers->getFlags().has(LayerDef::Flags::DiffuseLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Diffuse))
				layer_diffuse = std::make_unique<ColorLayerAccum>(color_layer);
			if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseNoShadow))
				layer_diffuse_no_shadow = std::make_unique<ColorLayerAccum>(color_layer);
		}
		if(color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Shadow))
				layer_shadow = std::make_unique<ColorLayerAccum>(color_layer);
			if(Rgba *color_layer = color_layers->find(LayerDef::Glossy))
				layer_glossy = std::make_unique<ColorLayerAccum>(color_layer);
		}
	}
	for(unsigned int i = 0; i < num_samples; ++i)
	{
		// ...get sample val...
		LSample ls;
		ls.s_1_ = hal_2.getNext();
		ls.s_2_ = hal_3.getNext();
		if(auto[hit, light_ray]{light->illumSample(sp.p_, ls, time)}; hit)
		{
			if(SurfaceIntegrator::params_.shadow_bias_auto_) light_ray.tmin_ = shadow_bias_ * std::max(1.f, sp.p_.length());
			else light_ray.tmin_ = shadow_bias_;
			bool shadowed = false;
			const Primitive *shadow_casting_primitive = nullptr;
			Rgb scol{0.f};
			if(cast_shadows)
			{
				if(params_.transparent_shadows_) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator_->isShadowedTransparentShadow(light_ray, params_.shadow_depth_, camera);
				else std::tie(shadowed, shadow_casting_primitive) = accelerator_->isShadowed(light_ray);
			}
			if((!shadowed && ls.pdf_ > 1e-6f) || layer_diffuse_no_shadow)
			{
				const Rgb ls_col_no_shadow = ls.col_;
				if(params_.transparent_shadows_ && cast_shadows) ls.col_ *= scol;
				if(vol_integrator_)
				{
					const Rgb transmit_col = vol_integrator_->transmittance(random_generator, light_ray);
					ls.col_ *= transmit_col;
				}
				const Rgb surf_col = sp.eval(wo, light_ray.dir_, BsdfFlags::All);
				if(layer_shadow && !shadowed && ls.pdf_ > 1e-6f) layer_shadow->accum_ += Rgba{1.f};
				const float angle_light_normal = sp.getMaterial()->isFlat() ? 1.f : std::abs(sp.n_ * light_ray.dir_);    //If the material has the special attribute "isFlat()" then we will not multiply the surface reflection by the cosine of the angle between light and normal
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
						if(layer_diffuse_no_shadow) layer_diffuse_no_shadow->accum_ += tmp_col_no_light_color * ls_col_no_shadow;
						if(layer_diffuse && !shadowed && ls.pdf_ > 1e-6f) layer_diffuse->accum_ += tmp_col_no_light_color * ls.col_;
					}
					if(layer_glossy)
					{
						const Rgb tmp_col = sp.eval(wo, light_ray.dir_, BsdfFlags::Glossy, true) * ls.col_ * angle_light_normal * w / ls.pdf_;
						if(!shadowed && ls.pdf_ > 1e-6f) layer_glossy->accum_ += tmp_col;
					}
				}
				if(!shadowed && ls.pdf_ > 1e-6f) col += surf_col * ls.col_ * angle_light_normal * w / ls.pdf_;
			}
			if(color_layers && (shadowed || ls.pdf_ <= 1e-6f) && color_layers->getFlags().has(LayerDef::Flags::IndexLayers) && shadow_casting_primitive)
			{
				float mask_obj_index = 0.f, mask_mat_index = 0.f;
				mask_obj_index = shadow_casting_primitive->getObjectIndex();    //Object index of the object casting the shadow
				if(const Material *casting_material = shadow_casting_primitive->getMaterial()) mask_mat_index = casting_material->getPassIndex();    //Material index of the object casting the shadow
				if(layer_mat_index_mask_shadow && mask_mat_index == mask_params_.mat_index_) layer_mat_index_mask_shadow->accum_ += Rgba{1.f};
				if(layer_obj_index_mask_shadow && mask_obj_index == mask_params_.obj_index_) layer_obj_index_mask_shadow->accum_ += Rgba{1.f};
			}
		}
	}
	const Rgb col_result = col * inv_num_samples;
	if(color_layers)
	{
		if(color_layers->getFlags().has(LayerDef::Flags::IndexLayers))
		{
			if(layer_mat_index_mask_shadow) *layer_mat_index_mask_shadow->color_ += layer_mat_index_mask_shadow->accum_ * inv_num_samples;
			if(layer_obj_index_mask_shadow) *layer_obj_index_mask_shadow->color_ += layer_obj_index_mask_shadow->accum_ * inv_num_samples;
		}
		if(color_layers->getFlags().has(LayerDef::Flags::DiffuseLayers))
		{
			if(layer_diffuse) *layer_diffuse->color_ += layer_diffuse->accum_ * inv_num_samples;
			if(layer_diffuse_no_shadow) *layer_diffuse_no_shadow->color_ += layer_diffuse_no_shadow->accum_ * inv_num_samples;
		}
		if(color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
		{
			if(layer_shadow) *layer_shadow->color_ += layer_shadow->accum_ * inv_num_samples;
			if(layer_glossy) *layer_glossy->color_ += layer_glossy->accum_ * inv_num_samples;
		}
		if(color_layers->getFlags().has(LayerDef::Flags::DebugLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugLightEstimationLightSampling)) *color_layer += col_result;
		}
	}
	return col_result;
}

Rgb MonteCarloIntegrator::areaLightSampleMaterial(Halton &hal_2, Halton &hal_3, RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, bool chromatic_enabled, float wavelength, const Light *light, const Vec3f &wo, const SurfacePoint &sp, bool cast_shadows, unsigned int num_samples, float inv_num_samples) const
{
	if(light->canIntersect()) // sample from BSDF to complete MIS
	{
		Rgb col_result{0.f};
		std::unique_ptr<ColorLayerAccum> layer_diffuse;
		std::unique_ptr<ColorLayerAccum> layer_diffuse_no_shadow;
		std::unique_ptr<ColorLayerAccum> layer_glossy;
		if(color_layers)
		{
			if(color_layers->getFlags().has(LayerDef::Flags::DiffuseLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Diffuse))
					layer_diffuse = std::make_unique<ColorLayerAccum>(color_layer);
				if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseNoShadow))
					layer_diffuse_no_shadow = std::make_unique<ColorLayerAccum>(color_layer);
			}
			if(color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Glossy))
					layer_glossy = std::make_unique<ColorLayerAccum>(color_layer);
			}
		}
		Ray light_ray;
		light_ray.from_ = sp.p_;
		Rgb col{0.f};
		Ray b_ray;
		for(unsigned int i = 0; i < num_samples; ++i)
		{
			if(SurfaceIntegrator::params_.ray_min_dist_auto_) b_ray.tmin_ = ray_min_dist_ * std::max(1.f, sp.p_.length());
			else b_ray.tmin_ = ray_min_dist_;
			b_ray.from_ = sp.p_;
			const float s_1 = hal_2.getNext();
			const float s_2 = hal_3.getNext();
			float W = 0.f;
			Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Dispersive | BsdfFlags::Reflect | BsdfFlags::Transmit);
			const Rgb surf_col = sp.sample(wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera);
			if(auto [light_hit, light_pdf, lcol]{light->intersect(b_ray, b_ray.tmax_)}; light_hit && s.pdf_ > 1e-6f)
			{
				Rgb scol{0.f};
				bool shadowed = false;
				const Primitive *shadow_casting_primitive = nullptr;
				if(cast_shadows)
				{
					if(params_.transparent_shadows_) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator_->isShadowedTransparentShadow(b_ray, params_.shadow_depth_, camera);
					else std::tie(shadowed, shadow_casting_primitive) = accelerator_->isShadowed(b_ray);
				}
				if((!shadowed && light_pdf > 1e-6f) || layer_diffuse_no_shadow)
				{
					if(params_.transparent_shadows_ && cast_shadows) lcol *= scol;
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
							const Rgb tmp_col = sp.sample(wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera) * lcol * w * W;
							if(layer_diffuse_no_shadow) layer_diffuse_no_shadow->accum_ += tmp_col;
							if(layer_diffuse && !shadowed && light_pdf > 1e-6f && s.sampled_flags_.has(BsdfFlags::Diffuse)) layer_diffuse->accum_ += tmp_col;
						}
						if(layer_glossy)
						{
							const Rgb tmp_col = sp.sample(wo, b_ray.dir_, s, W, chromatic_enabled, wavelength, camera) * lcol * w * W;
							if(!shadowed && light_pdf > 1e-6f && s.sampled_flags_.has(BsdfFlags::Glossy)) layer_glossy->accum_ += tmp_col;
						}
					}
					if(!shadowed && light_pdf > 1e-6f) col += surf_col * lcol * w * W;
				}
			}
		}
		col_result = col * inv_num_samples;
		if(color_layers)
		{
			if(color_layers->getFlags().has(LayerDef::Flags::DiffuseLayers))
			{
				if(layer_diffuse) *layer_diffuse->color_ += layer_diffuse->accum_ * inv_num_samples;
				if(layer_diffuse_no_shadow) *layer_diffuse_no_shadow->color_ += layer_diffuse_no_shadow->accum_ * inv_num_samples;
			}
			if(color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
			{
				if(layer_glossy) *layer_glossy->color_ += layer_glossy->accum_ * inv_num_samples;
			}
			if(color_layers->getFlags().has(LayerDef::Flags::DebugLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugLightEstimationMatSampling)) *color_layer += col_result;
			}
		}
		return col_result;
	}
	else return Rgb{0.f};
}

Rgb MonteCarloIntegrator::doLightEstimation(RandomGenerator &random_generator, ColorLayers *color_layers, const Camera *camera, bool chromatic_enabled, float wavelength, const Light *light, const SurfacePoint &sp, const Vec3f &wo, unsigned int loffs, float aa_light_sample_multiplier, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col{0.f};
	const bool cast_shadows = light->castShadows() && sp.getMaterial()->getReceiveShadows();
	if(light->diracLight()) // handle lights with delta distribution, e.g. point and directional lights
	{
		col += diracLight(random_generator, color_layers, camera, light, wo, sp, cast_shadows, pixel_sampling_data.time_);
	}
	else // area light and suchlike
	{
		const unsigned int l_offs = loffs * loffs_delta_;
		int num_samples = static_cast<int>(ceilf(light->nSamples() * aa_light_sample_multiplier));
		if(ray_division.division_ > 1) num_samples = std::max(1, num_samples / ray_division.division_);
		const float inv_num_samples = 1.f / static_cast<float>(num_samples);
		const unsigned int offs = num_samples * pixel_sampling_data.sample_ + pixel_sampling_data.offset_ + l_offs;
		Halton hal_2(2, offs - 1);
		Halton hal_3(3, offs - 1);
		col += areaLightSampleLight(hal_2, hal_3, random_generator, color_layers, camera, light, wo, sp, cast_shadows, num_samples, inv_num_samples, pixel_sampling_data.time_);
		hal_2.setStart(offs - 1);
		hal_3.setStart(offs - 1);
		col += areaLightSampleMaterial(hal_2, hal_3, random_generator, color_layers, camera, chromatic_enabled, wavelength, light, wo, sp, cast_shadows, num_samples, inv_num_samples);
	}
	return col;
}

std::pair<Rgb, float> MonteCarloIntegrator::dispersive(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
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
		Vec3f wi;
		const Rgb mcol = sp.sample(wo, wi, s, w, chromatic_enabled, wavelength_dispersive, image_film->getCamera());

		if(s.pdf_ > 1.0e-6f && s.sampled_flags_.has(BsdfFlags::Dispersive))
		{
			const Rgb wl_col = spectrum::wl2Rgb(wavelength_dispersive);
			Ray ref_ray{sp.p_, wi, sp.time_, ray_min_dist_};
			auto [integ_col, integ_alpha] = integrate(image_film, ref_ray, fast_random, random_generator, correlative_sample_number, nullptr, thread_id, ray_level, false, wavelength_dispersive, additional_depth, ray_division_new, pixel_sampling_data, 0, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);
			integ_col *= mcol * wl_col * w;
			dcol += integ_col;
			if(color_layers) dcol_trans_accum += integ_col;
			alpha_accum += integ_alpha;
			if(!ref_ray_chromatic_volume) ref_ray_chromatic_volume = std::make_unique<Ray>(ref_ray, Ray::DifferentialsCopy::No);
		}
	}
	if(ref_ray_chromatic_volume && bsdfs.has(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray_chromatic_volume->dir_ < 0))
		{
			dcol *= vol->transmittance(*ref_ray_chromatic_volume);
		}
	}
	if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::Trans))
		{
			dcol_trans_accum *= d_1;
			*color_layer += dcol_trans_accum;
		}
	}
	return {dcol * d_1, alpha_accum * d_1};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossy(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	if(!sp.mat_data_->bsdf_flags_.has(BsdfFlags::Reflect)) return {Rgb{0.f}, -1.f}; //If alpha is -1.f we consider the result invalid and do not use it to accumulate alpha
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

	const auto camera{image_film->getCamera()};

	for(int ns = 0; ns < ray_samples_glossy; ++ns)
	{
		ray_division_new.decorrelation_1_ = Halton::lowDiscrepancySampling(fast_random, 2 * ray_level + 1, branch + pixel_sampling_data.offset_);
		ray_division_new.decorrelation_2_ = Halton::lowDiscrepancySampling(fast_random, 2 * ray_level + 2, branch + pixel_sampling_data.offset_);
		ray_division_new.offset_ = branch;
		++offs;
		++branch;
		const float s_1 = hal_2.getNext();
		const float s_2 = hal_3.getNext();
		if(sp.mat_data_->bsdf_flags_.has(BsdfFlags::Transmit))
		{
			Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::AllGlossy);
			Rgb mcol[2];
			float w[2];
			Vec3f dir[2];

			mcol[0] = sp.sample(wo, dir, mcol[1], s, w, chromatic_enabled, wavelength);

			if(!s.sampled_flags_.has(BsdfFlags::Dispersive))
			{
				const auto [reflect_col, reflect_alpha] = glossyReflect(image_film, fast_random, random_generator, correlative_sample_number, thread_id, ray_level, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, sp, bsdfs, additional_depth, pixel_sampling_data, ray_division_new, mcol[0], w[0], dir[0]);
				gcol += reflect_col;
				if(color_layers) gcol_reflect_accum += reflect_col;
				alpha_accum += reflect_alpha;
			}
			if(s.sampled_flags_.has(BsdfFlags::Transmit))
			{
				const auto [transmit_col, transmit_alpha] = glossyTransmit(image_film, fast_random, random_generator, correlative_sample_number, thread_id, ray_level, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, sp, bsdfs, additional_depth, pixel_sampling_data, ray_division_new, mcol[1], w[1], dir[1]);
				gcol += transmit_col;
				if(color_layers) gcol_transmit_accum += transmit_col;
				alpha_accum += transmit_alpha;
			}
		}
		else
		{
			const auto [reflect_col, reflect_alpha] = glossyReflectNoTransmit(image_film, fast_random, random_generator, correlative_sample_number, thread_id, ray_level, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, sp, bsdfs, wo, additional_depth, pixel_sampling_data, ray_division_new, s_1, s_2);
			gcol += reflect_col;
			if(color_layers) gcol_indirect_accum += reflect_col;
			alpha_accum += reflect_alpha;
		}
	}

	if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::GlossyIndirect))
		{
			gcol_indirect_accum *= inverse_ray_samples_glossy;
			*color_layer += gcol_indirect_accum;
		}
		if(Rgba *color_layer = color_layers->find(LayerDef::Trans))
		{
			gcol_reflect_accum *= inverse_ray_samples_glossy;
			*color_layer += gcol_reflect_accum;
		}
		if(Rgba *color_layer = color_layers->find(LayerDef::GlossyIndirect))
		{
			gcol_transmit_accum *= inverse_ray_samples_glossy;
			*color_layer += gcol_transmit_accum;
		}
	}
	return {gcol * inverse_ray_samples_glossy, alpha_accum * inverse_ray_samples_glossy};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyReflect(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &reflect_color, float w, const Vec3f &dir) const
{
	Ray ref_ray{sp.p_, dir, ray.time_, ray_min_dist_};
	if(ray.differentials_) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
	auto [integ_col, integ_alpha] = integrate(image_film, ref_ray, fast_random, random_generator, correlative_sample_number, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division_new, pixel_sampling_data, 0, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);
	if(bsdfs.has(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = sp.getMaterial()->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= reflect_color * w;
	return {std::move(integ_col), integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyTransmit(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const Rgb &transmit_col, float w, const Vec3f &dir) const
{
	Ray ref_ray{sp.p_, dir, ray.time_, ray_min_dist_};
	if(ray.differentials_) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp.getMaterial()->getMatIor());
	auto [integ_col, integ_alpha] = integrate(image_film, ref_ray, fast_random, random_generator, correlative_sample_number, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division_new, pixel_sampling_data, 0, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);
	if(bsdfs.has(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = sp.getMaterial()->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= transmit_col * w;
	return {std::move(integ_col), integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::glossyReflectNoTransmit(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, BsdfFlags bsdfs, const Vec3f &wo, int additional_depth, const PixelSamplingData &pixel_sampling_data, const RayDivision &ray_division_new, const float s_1, const float s_2) const
{
	float w = 0.f;
	Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Reflect);
	Vec3f wi;
	const Rgb mcol = sp.sample(wo, wi, s, w, chromatic_enabled, wavelength, image_film->getCamera());
	Ray ref_ray{sp.p_, wi, ray.time_, ray_min_dist_};
	if(ray.differentials_)
	{
		if(s.sampled_flags_.has(BsdfFlags::Reflect)) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
		else if(s.sampled_flags_.has(BsdfFlags::Transmit)) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp.getMaterial()->getMatIor());
	}
	auto [integ_col, integ_alpha] = integrate(image_film, ref_ray, fast_random, random_generator, correlative_sample_number, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division_new, pixel_sampling_data, 0, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);
	if(bsdfs.has(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = sp.getMaterial()->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= mcol * w;
	return {std::move(integ_col), integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::specularReflect(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const DirectionColor *reflect_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Ray ref_ray{sp.p_, reflect_data->dir_, ray.time_, ray_min_dist_};
	if(ray.differentials_) ref_ray.differentials_ = sp.reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
	auto [integ_col, integ_alpha] = integrate(image_film, ref_ray, fast_random, random_generator, correlative_sample_number, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division, pixel_sampling_data, 0, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);
	if(bsdfs.has(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= reflect_data->col_;
	if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::ReflectPerfect)) *color_layer += integ_col;
	}
	return {std::move(integ_col), integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::specularRefract(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, const SurfacePoint &sp, const Material *material, BsdfFlags bsdfs, const DirectionColor *refract_data, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Ray ref_ray;
	float transp_bias_factor = material->getTransparentBiasFactor();
	if(transp_bias_factor > 0.f)
	{
		const bool transpbias_multiply_raydepth = material->getTransparentBiasMultiplyRayDepth();
		if(transpbias_multiply_raydepth) transp_bias_factor *= ray_level;
		ref_ray = {sp.p_ + refract_data->dir_ * transp_bias_factor, refract_data->dir_, ray.time_, ray_min_dist_};
	}
	else ref_ray = {sp.p_, refract_data->dir_, ray.time_, ray_min_dist_};

	if(ray.differentials_) ref_ray.differentials_ = sp.refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, material->getMatIor());
	auto [integ_col, integ_alpha] = integrate(image_film, ref_ray, fast_random, random_generator, correlative_sample_number, nullptr, thread_id, ray_level, chromatic_enabled, wavelength, additional_depth, ray_division, pixel_sampling_data, 0, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);

	if(bsdfs.has(BsdfFlags::Volumetric))
	{
		if(const VolumeHandler *vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0))
		{
			integ_col *= vol->transmittance(ref_ray);
		}
	}
	integ_col *= refract_data->col_;
	if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::RefractPerfect)) *color_layer += integ_col;
	}
	return {std::move(integ_col), integ_alpha};
}

std::pair<Rgb, float> MonteCarloIntegrator::recursiveRaytrace(ImageFilm *image_film, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, float wavelength, const Ray &ray, BsdfFlags bsdfs, const SurfacePoint &sp, const Vec3f &wo, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb col {0.f};
	float alpha = 0.f;
	int alpha_count = 0;
	if(ray_level <= (params_.r_depth_ + additional_depth))
	{
		// dispersive effects with recursive raytracing:
		if(bsdfs.has(BsdfFlags::Dispersive) && chromatic_enabled)
		{
			const auto [dispersive_col, dispersive_alpha] = dispersive(image_film, fast_random, random_generator, correlative_sample_number, color_layers, thread_id, ray_level, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, sp, sp.getMaterial(), bsdfs, wo, additional_depth, ray_division, pixel_sampling_data);
			col += dispersive_col;
			alpha += dispersive_alpha;
			++alpha_count;
		}
		if(ray_level < 20 && bsdfs.has(BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter))
		{
			// glossy reflection with recursive raytracing:
			if(bsdfs.has(BsdfFlags::Glossy))
			{
				const auto [glossy_col, glossy_alpha] = glossy(image_film, fast_random, random_generator, correlative_sample_number, color_layers, thread_id, ray_level, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, sp, bsdfs, wo, additional_depth, ray_division, pixel_sampling_data);
				if(alpha != -1.f) //If alpha is -1.f we consider the result invalid and do not use it to accumulate alpha
				{
					col += glossy_col;
					alpha += glossy_alpha;
					++alpha_count;
				}
			}
			//...perfect specular reflection/refraction with recursive raytracing...
			if(bsdfs.has((BsdfFlags::Specular | BsdfFlags::Filter)))
			{
				const Specular specular = sp.getSpecular(ray_level, wo, chromatic_enabled, wavelength);
				if(specular.reflect_)
				{
					const auto [specular_col, specular_alpha] = specularReflect(image_film, fast_random, random_generator, correlative_sample_number, color_layers, thread_id, ray_level, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, sp, sp.getMaterial(), bsdfs, specular.reflect_.get(), additional_depth, ray_division, pixel_sampling_data);
					col += specular_col;
					alpha += specular_alpha;
					++alpha_count;
				}
				if(specular.refract_)
				{
					const auto [specular_col, specular_alpha] = specularRefract(image_film, fast_random, random_generator, correlative_sample_number, color_layers, thread_id, ray_level, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, sp, sp.getMaterial(), bsdfs, specular.refract_.get(), additional_depth, ray_division, pixel_sampling_data);
					col += specular_col;
					alpha += specular_alpha;
					++alpha_count;
				}
			}
		}
	}
	return {std::move(col), (alpha_count > 0 ? alpha / alpha_count : 1.f)};
}

} //namespace yafaray
