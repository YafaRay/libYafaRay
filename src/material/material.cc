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
#include "sampler/halton_scr.h"
#include "common/logger.h"

BEGIN_YAFARAY

float Material::material_index_highest_ = 1.f;	//Initially this class shared variable will be 1.f
unsigned int Material::material_index_auto_ = 0;	//Initially this class shared variable will be 0
float Material::highest_sampling_factor_ = 1.f;	//Initially this class shared variable will be 1.f


Material *Material::factory(ParamMap &params, std::list<ParamMap> &eparams, Scene &scene)
{
	Y_DEBUG PRTEXT(**Material) PREND; params.printDebug();
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

Material::Material() : bsdf_flags_(BsdfFlags::None), visibility_(Material::Visibility::NormalVisible), receive_shadows_(true), req_mem_(0), vol_i_(nullptr), vol_o_(nullptr), additional_depth_(0)
{
	material_index_auto_++;
	srand(material_index_auto_);
	float r, g, b;
	do
	{
		r = (float)(rand() % 8) / 8.f;
		g = (float)(rand() % 8) / 8.f;
		b = (float)(rand() % 8) / 8.f;
	}
	while(r + g + b < 0.5f);
	material_index_auto_color_ = Rgb(r, g, b);
	material_index_auto_number_ = material_index_auto_;
}

Rgb Material::sampleClay(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const {
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	wi = sample::cosHemisphere(n, sp.nu_, sp.nv_, s.s_1_, s.s_2_);
	s.pdf_ = std::abs(wi * n);
	w = (std::abs(wi * sp.n_)) / (s.pdf_ * 0.99f + 0.01f);
	return Rgb(1.0f);	//Clay color White 100%
}


void Material::applyWireFrame(float &value, float wire_frame_amount, const SurfacePoint &sp) const
{
	if(wire_frame_amount > 0.f && wireframe_thickness_ > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
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
		float dist = sp.getDistToNearestEdge();
		if(dist <= wireframe_thickness_)
		{
			Rgb wire_frame_col = wireframe_color_ * wire_frame_amount;
			if(wireframe_exponent_ > 0.f)
			{
				wire_frame_amount *= math::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
			}
			col.blend(wire_frame_col, wire_frame_amount);
		}
	}
}

void Material::applyWireFrame(Rgb *const col, float wire_frame_amount, const SurfacePoint &sp) const
{
	if(wire_frame_amount > 0.f && wireframe_thickness_ > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= wireframe_thickness_)
		{
			Rgb wire_frame_col = wireframe_color_ * wire_frame_amount;
			if(wireframe_exponent_ > 0.f)
			{
				wire_frame_amount *= math::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
			}
			col[0].blend(wire_frame_col, wire_frame_amount);
			col[1].blend(wire_frame_col, wire_frame_amount);
		}
	}
}

void Material::applyWireFrame(Rgba &col, float wire_frame_amount, const SurfacePoint &sp) const
{
	if(wire_frame_amount > 0.f && wireframe_thickness_ > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= wireframe_thickness_)
		{
			Rgb wire_frame_col = wireframe_color_ * wire_frame_amount;
			if(wireframe_exponent_ > 0.f)
			{
				wire_frame_amount *= math::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
			}
			col.blend(wire_frame_col, wire_frame_amount);
			col.a_ = wire_frame_amount;
		}
	}
}

void Material::applyWireFrame(Rgba *const col, float wire_frame_amount, const SurfacePoint &sp) const
{
	if(wire_frame_amount > 0.f && wireframe_thickness_ > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= wireframe_thickness_)
		{
			Rgb wire_frame_col = wireframe_color_ * wire_frame_amount;
			if(wireframe_exponent_ > 0.f)
			{
				wire_frame_amount *= math::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
			}
			col[0].blend(wire_frame_col, wire_frame_amount);
			col[0].a_ = wire_frame_amount;
			col[1].blend(wire_frame_col, wire_frame_amount);
			col[1].a_ = wire_frame_amount;
		}
	}
}

bool Material::scatterPhoton(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s) const
{
	float w = 0.f;
	Rgb scol = sample(render_data, sp, wi, wo, s, w);
	if(s.pdf_ > 1.0e-6f)
	{
		Rgb cnew = s.lcol_ * s.alpha_ * scol * w;
		float new_max = cnew.maximum();
		float old_max = s.lcol_.maximum();
		float prob = std::min(1.f, new_max / old_max);
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
	if(flags.hasAny((BsdfFlags::Transmit | BsdfFlags::Reflect) & bsdf_flags_)) return Rgb(0.f);
	float s_1, s_2, s_3, s_4, w = 0.f;
	Rgb total(0.f), col;
	Vec3 wi, wo;
	for(int i = 0; i < 16; ++i)
	{
		s_1 = 0.03125 + 0.0625 * (float)i; // (1.f/32.f) + (1.f/16.f)*(float)i;
		s_2 = sample::riVdC(i);
		s_3 = scrHalton__(2, i);
		s_4 = scrHalton__(3, i);
		wo = sample::cosHemisphere(sp.n_, sp.nu_, sp.nv_, s_1, s_2);
		Sample s(s_3, s_4, flags);
		col = sample(render_data, sp, wo, wi, s, w);
		total += col * w;
	}
	return total * 0.0625; //total / 16.f
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
