/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "material/material.h"
#include "material/material_blend.h"
#include "material/material_mask.h"
#include "material/material_rough_glass.h"
#include "material/material_shiny_diffuse.h"
#include "material/material_glass.h"
#include "material/material_glossy.h"
#include "material/material_coated_glossy.h"
#include "material/material_light.h"
#include "material/material_mirror.h"
#include "material/material_null.h"
#include "common/items.h"
#include "geometry/surface.h"
#include "param/param.h"
#include "sampler/sample.h"
#include "sampler/halton.h"
#include "common/logger.h"
#include "math/interpolation.h"
#include "scene/scene.h"
#include "material/sample.h"
#include "photon/photon_sample.h"
#include "volume/handler/volume_handler.h"

namespace yafaray {

Material::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(receive_shadows_);
	PARAM_LOAD(flat_material_);
	PARAM_ENUM_LOAD(visibility_);
	PARAM_LOAD(mat_pass_index_);
	PARAM_LOAD(additional_depth_);
	PARAM_LOAD(transparent_bias_factor_);
	PARAM_LOAD(transparent_bias_multiply_raydepth_);
	PARAM_LOAD(sampling_factor_);
	PARAM_LOAD(wireframe_amount_);
	PARAM_LOAD(wireframe_thickness_);
	PARAM_LOAD(wireframe_exponent_);
	PARAM_LOAD(wireframe_color_);
}

ParamMap Material::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(receive_shadows_);
	PARAM_SAVE(flat_material_);
	PARAM_ENUM_SAVE(visibility_);
	PARAM_SAVE(mat_pass_index_);
	PARAM_SAVE(additional_depth_);
	PARAM_SAVE(transparent_bias_factor_);
	PARAM_SAVE(transparent_bias_multiply_raydepth_);
	PARAM_SAVE(sampling_factor_);
	PARAM_SAVE(wireframe_amount_);
	PARAM_SAVE(wireframe_thickness_);
	PARAM_SAVE(wireframe_exponent_);
	PARAM_SAVE(wireframe_color_);
	PARAM_SAVE_END;
}

ParamMap Material::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<std::unique_ptr<Material>, ParamResult> Material::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	std::pair<std::unique_ptr<Material>, ParamResult> result{nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	switch(type.value())
	{
		case Type::Blend: result = BlendMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::CoatedGlossy: result = CoatedGlossyMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::Glass: result = GlassMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::Mirror: result = MirrorMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::Null: result = NullMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::Glossy: result = GlossyMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::RoughGlass: result = RoughGlassMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::ShinyDiffuse: result = ShinyDiffuseMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::Light: result = LightMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		case Type::Mask: result = MaskMaterial::factory(logger, scene, name, param_map, nodes_param_maps); break;
		default: break;
	}
	return result;
}

Material::Material(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) :
		params_{param_result, param_map}, materials_{materials}, logger_{logger}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " " + getName() + " params_:\n" + params_.getAsParamMap(true).print());
}

Material::~Material() = default; //Destructor definition done here and not in material.h to avoid compilation problems with std::unique_ptr

Rgb Material::sampleClay(const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w)
{
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	wi = sample::cosHemisphere(n, sp.uvn_, s.s_1_, s.s_2_);
	s.pdf_ = std::abs(wi * n);
	w = (std::abs(wi * sp.n_)) / (s.pdf_ * 0.99f + 0.01f);
	return Rgb{1.f};	//Clay color White 100%
}


void Material::applyWireFrame(float &value, float wire_frame_amount, const SurfacePoint &sp) const
{
	const float dist = sp.getDistToNearestEdge();
	if(dist <= params_.wireframe_thickness_)
	{
		if(params_.wireframe_exponent_ > 0.f)
		{
			wire_frame_amount *= math::pow((params_.wireframe_thickness_ - dist) / params_.wireframe_thickness_, params_.wireframe_exponent_);
		}
		value = value * (1.f - wire_frame_amount);
	}
}

void Material::applyWireFrame(Rgb &col, float wire_frame_amount, const SurfacePoint &sp) const
{
	const float dist = sp.getDistToNearestEdge();
	if(dist <= params_.wireframe_thickness_)
	{
		const Rgb wire_frame_col = params_.wireframe_color_ * wire_frame_amount;
		if(params_.wireframe_exponent_ > 0.f)
		{
			wire_frame_amount *= math::pow((params_.wireframe_thickness_ - dist) / params_.wireframe_thickness_, params_.wireframe_exponent_);
		}
		col.blend(wire_frame_col, wire_frame_amount);
	}
}

void Material::applyWireFrame(Rgba &col, float wire_frame_amount, const SurfacePoint &sp) const
{
	const float dist = sp.getDistToNearestEdge();
	if(dist <= params_.wireframe_thickness_)
	{
		const Rgb wire_frame_col = params_.wireframe_color_ * wire_frame_amount;
		if(params_.wireframe_exponent_ > 0.f)
		{
			wire_frame_amount *= math::pow((params_.wireframe_thickness_ - dist) / params_.wireframe_thickness_, params_.wireframe_exponent_);
		}
		col.blend(Rgba{wire_frame_col}, wire_frame_amount);
		col.a_ = wire_frame_amount;
	}
}

bool Material::scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wi, Vec3f &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
{
	float w = 0.f;
	const Rgb scol = sample(mat_data, sp, wi, wo, s, w, chromatic, wavelength, camera);
	if(s.pdf_ > 1.0e-6f)
	{
		const Rgb cnew = s.lcol_ * s.alpha_ * scol * w;
		const float new_max = cnew.maximum();
		const float old_max = s.lcol_.maximum();
		const float prob = std::min(1.f, new_max / old_max);
		if(s.s_3_ <= prob && prob > 1e-4f)
		{
			s.color_ = cnew / prob;
			return true;
		}
	}
	return false;
}

Rgb Material::getReflectivity(FastRandom &fast_random, const MaterialData *mat_data, const SurfacePoint &sp, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const
{
	if(!flags.has((BsdfFlags::Transmit | BsdfFlags::Reflect) & bsdf_flags_)) return Rgb{0.f};
	Rgb total(0.f);
	for(int i = 0; i < 16; ++i)
	{
		const float s_1 = 0.03125f + 0.0625f * static_cast<float>(i); // (1.f/32.f) + (1.f/16.f)*(float)i;
		const float s_2 = sample::riVdC(i);
		const float s_3 = Halton::lowDiscrepancySampling(fast_random, 2, i);
		const float s_4 = Halton::lowDiscrepancySampling(fast_random, 3, i);
		const Vec3f wo{sample::cosHemisphere(sp.n_, sp.uvn_, s_1, s_2)};
		Vec3f wi;
		Sample s(s_3, s_4, flags);
		float w = 0.f;
		const Rgb col = sample(mat_data, sp, wo, wi, s, w, chromatic, wavelength, camera);
		total += col * w;
	}
	return total * 0.0625f; //total / 16.f
}

void Material::applyBump(SurfacePoint &sp, const DuDv &du_dv)
{
	sp.uvn_.u_ += du_dv.getDu() * sp.n_;
	sp.uvn_.v_ += du_dv.getDv() * sp.n_;
	sp.n_ = (sp.uvn_.u_ ^ sp.uvn_.v_).normalize();
	sp.uvn_.u_.normalize();
	sp.uvn_.v_ = (sp.n_ ^ sp.uvn_.u_).normalize();
}

std::string Material::getName() const
{
	return materials_.findNameFromId(id_).first;
}

void Material::setId(size_t id)
{
	id_ = id;
	index_auto_color_ = Rgb::pseudoRandomDistinctFromIndex(id + 1);
}

} //namespace yafaray
