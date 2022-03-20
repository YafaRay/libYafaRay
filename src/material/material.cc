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
#include "material/material_simple.h"
#include "geometry/surface.h"
#include "common/param.h"
#include "sampler/sample.h"
#include "sampler/halton.h"
#include "common/logger.h"
#include "volume/volume.h"
#include "math/interpolation.h"

BEGIN_YAFARAY

unsigned int Material::material_index_highest_ = 1;
unsigned int Material::material_index_auto_ = 0;
float Material::highest_sampling_factor_ = 1.f;

Material * Material::factory(Logger &logger, ParamMap &params, std::list<ParamMap> &nodes_params, const Scene &scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Material");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	if(type == "blend_mat") return BlendMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "coated_glossy") return CoatedGlossyMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "glass") return GlassMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "mirror") return MirrorMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "null") return NullMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "glossy") return GlossyMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "rough_glass") return RoughGlassMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "shinydiffusemat") return ShinyDiffuseMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "light_mat") return LightMaterial::factory(logger, params, nodes_params, scene);
	else if(type == "mask_mat") return MaskMaterial::factory(logger, params, nodes_params, scene);
	else return nullptr;
}

Material::Material(Logger &logger) : logger_(logger)
{
	material_index_auto_++;
	srand(material_index_auto_);
	float r, g, b;
	do
	{
		r = static_cast<float>(rand() % 8) / 8.f;
		g = static_cast<float>(rand() % 8) / 8.f;
		b = static_cast<float>(rand() % 8) / 8.f;
	}
	while(r + g + b < 0.5f);
	material_index_auto_color_ = Rgb(r, g, b);
}

Material::~Material()
{
	resetMaterialIndex();
}

Rgb Material::sampleClay(const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	const Vec3 n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	wi = sample::cosHemisphere(n, sp.nu_, sp.nv_, s.s_1_, s.s_2_);
	s.pdf_ = std::abs(wi * n);
	w = (std::abs(wi * sp.n_)) / (s.pdf_ * 0.99f + 0.01f);
	return Rgb{1.f};	//Clay color White 100%
}


void Material::applyWireFrame(float &value, float wire_frame_amount, const SurfacePoint &sp) const
{
	const float dist = sp.getDistToNearestEdge();
	if(dist <= wireframe_thickness_)
	{
		if(wireframe_exponent_ > 0.f)
		{
			wire_frame_amount *= math::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
		}
		value = value * (1.f - wire_frame_amount);
	}
}

void Material::applyWireFrame(Rgb &col, float wire_frame_amount, const SurfacePoint &sp) const
{
	const float dist = sp.getDistToNearestEdge();
	if(dist <= wireframe_thickness_)
	{
		const Rgb wire_frame_col = wireframe_color_ * wire_frame_amount;
		if(wireframe_exponent_ > 0.f)
		{
			wire_frame_amount *= math::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
		}
		col.blend(wire_frame_col, wire_frame_amount);
	}
}

void Material::applyWireFrame(Rgba &col, float wire_frame_amount, const SurfacePoint &sp) const
{
	const float dist = sp.getDistToNearestEdge();
	if(dist <= wireframe_thickness_)
	{
		const Rgb wire_frame_col = wireframe_color_ * wire_frame_amount;
		if(wireframe_exponent_ > 0.f)
		{
			wire_frame_amount *= math::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
		}
		col.blend(Rgba{wire_frame_col}, wire_frame_amount);
		col.a_ = wire_frame_amount;
	}
}

bool Material::scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
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

Rgb Material::getReflectivity(const MaterialData *mat_data, const SurfacePoint &sp, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const
{
	if(!flags.hasAny((BsdfFlags::Transmit | BsdfFlags::Reflect) & bsdf_flags_)) return Rgb{0.f};
	Rgb total(0.f);
	for(int i = 0; i < 16; ++i)
	{
		const float s_1 = 0.03125f + 0.0625f * static_cast<float>(i); // (1.f/32.f) + (1.f/16.f)*(float)i;
		const float s_2 = sample::riVdC(i);
		const float s_3 = Halton::lowDiscrepancySampling(2, i);
		const float s_4 = Halton::lowDiscrepancySampling(3, i);
		const Vec3 wo{sample::cosHemisphere(sp.n_, sp.nu_, sp.nv_, s_1, s_2)};
		Vec3 wi;
		Sample s(s_3, s_4, flags);
		float w = 0.f;
		const Rgb col = sample(mat_data, sp, wo, wi, s, w, chromatic, wavelength, camera);
		total += col * w;
	}
	return total * 0.0625f; //total / 16.f
}

void Material::applyBump(SurfacePoint &sp, const DuDv &du_dv) const
{
	sp.nu_ += du_dv.getDu() * sp.n_;
	sp.nv_ += du_dv.getDv() * sp.n_;
	sp.n_ = (sp.nu_ ^ sp.nv_).normalize();
	sp.nu_.normalize();
	sp.nv_ = (sp.n_ ^ sp.nu_).normalize();
}

std::unique_ptr<DirectionColor> DirectionColor::blend(std::unique_ptr<DirectionColor> direction_color_1, std::unique_ptr<DirectionColor> direction_color_2, float blend_val)
{
	if(blend_val <= 0.f) return direction_color_1;
	else if(blend_val >= 1.f) return direction_color_2;
	if(direction_color_1 && direction_color_2)
	{
		std::unique_ptr<DirectionColor> direction_color_blend = std::unique_ptr<DirectionColor>(new DirectionColor());
		direction_color_blend->col_ = math::lerp(direction_color_1->col_, direction_color_2->col_, blend_val);
		direction_color_blend->dir_ = (direction_color_1->dir_ + direction_color_2->dir_).normalize();
		return direction_color_blend;
	}
	else if(direction_color_1)
	{
		std::unique_ptr<DirectionColor> direction_color_blend = std::unique_ptr<DirectionColor>(new DirectionColor());
		direction_color_blend->col_ = math::lerp(direction_color_1->col_, Rgb{0.f}, blend_val);
		direction_color_blend->dir_ = direction_color_1->dir_.normalize();
		return direction_color_blend;
	}
	else if(direction_color_2)
	{
		std::unique_ptr<DirectionColor> direction_color_blend = std::unique_ptr<DirectionColor>(new DirectionColor());
		direction_color_blend->col_ = math::lerp(Rgb{0.f}, direction_color_2->col_, blend_val);
		direction_color_blend->dir_ = direction_color_2->dir_.normalize();
		return direction_color_blend;
	}
	return nullptr;
}

END_YAFARAY
