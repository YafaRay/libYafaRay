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

BEGIN_YAFARAY

unsigned int Material::material_index_highest_ = 1;
unsigned int Material::material_index_auto_ = 0;
float Material::highest_sampling_factor_ = 1.f;

std::unique_ptr<Material> Material::factory(ParamMap &params, std::list<ParamMap> &eparams, Scene &scene)
{
	if(Y_LOG_HAS_DEBUG)
	{
		Y_DEBUG PRTEXT(**Material) PREND;
		params.printDebug();
	}
	std::string type;
	params.getParam("type", type);
	if(type == "blend_mat") return BlendMaterial::factory(params, eparams, scene);
	else if(type == "coated_glossy") return CoatedGlossyMaterial::factory(params, eparams, scene);
	else if(type == "glass") return GlassMaterial::factory(params, eparams, scene);
	else if(type == "mirror") return MirrorMaterial::factory(params, eparams, scene);
	else if(type == "null") return NullMaterial::factory(params, eparams, scene);
	else if(type == "glossy") return GlossyMaterial::factory(params, eparams, scene);
	else if(type == "rough_glass") return RoughGlassMaterial::factory(params, eparams, scene);
	else if(type == "shinydiffusemat") return ShinyDiffuseMaterial::factory(params, eparams, scene);
	else if(type == "light_mat") return LightMaterial::factory(params, eparams, scene);
	else if(type == "mask_mat") return MaskMaterial::factory(params, eparams, scene);
	else return nullptr;
}

Material::Material()
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

Rgb Material::sampleClay(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const {
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	wi = sample::cosHemisphere(n, sp.nu_, sp.nv_, s.s_1_, s.s_2_);
	s.pdf_ = std::abs(wi * n);
	w = (std::abs(wi * sp.n_)) / (s.pdf_ * 0.99f + 0.01f);
	return Rgb(1.0f);	//Clay color White 100%
}


void Material::applyWireFrame(float &value, float wire_frame_amount, const SurfacePoint &sp) const
{
	if(wire_frame_amount > 0.f && wireframe_thickness_ > 0.f)
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
}

void Material::applyWireFrame(Rgb &col, float wire_frame_amount, const SurfacePoint &sp) const
{
	if(wire_frame_amount > 0.f && wireframe_thickness_ > 0.f)
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
}

void Material::applyWireFrame(Rgba &col, float wire_frame_amount, const SurfacePoint &sp) const
{
	if(wire_frame_amount > 0.f && wireframe_thickness_ > 0.f)
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
			col.a_ = wire_frame_amount;
		}
	}
}

bool Material::scatterPhoton(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s) const
{
	float w = 0.f;
	const Rgb scol = sample(render_data, sp, wi, wo, s, w);
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

Rgb Material::getReflectivity(const RenderData &render_data, const SurfacePoint &sp, BsdfFlags flags) const
{
	if(!flags.hasAny((BsdfFlags::Transmit | BsdfFlags::Reflect) & bsdf_flags_)) return Rgb(0.f);
	Rgb total(0.f);
	for(int i = 0; i < 16; ++i)
	{
		const float s_1 = 0.03125f + 0.0625f * static_cast<float>(i); // (1.f/32.f) + (1.f/16.f)*(float)i;
		const float s_2 = sample::riVdC(i);
		const float s_3 = Halton::lowDiscrepancySampling(2, i);
		const float s_4 = Halton::lowDiscrepancySampling(3, i);
		const Vec3 wo = sample::cosHemisphere(sp.n_, sp.nu_, sp.nv_, s_1, s_2);
		Vec3 wi;
		Sample s(s_3, s_4, flags);
		float w = 0.f;
		const Rgb col = sample(render_data, sp, wo, wi, s, w);
		total += col * w;
	}
	return total * 0.0625f; //total / 16.f
}

void Material::applyBump(SurfacePoint &sp, float df_dnu, float df_dnv) const
{
	sp.nu_ += df_dnu * sp.n_;
	sp.nv_ += df_dnv * sp.n_;
	sp.n_ = (sp.nu_ ^ sp.nv_).normalize();
	sp.nu_.normalize();
	sp.nv_ = (sp.n_ ^ sp.nu_).normalize();
}

END_YAFARAY
