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
#include "material/material_mask.h"
#include "material/material_simple.h"
//#include "material/material_beer.h"
#include "common/environment.h"
#include "common/param.h"

BEGIN_YAFARAY

Material *Material::factory(ParamMap &params, std::list<ParamMap> &eparams, RenderEnvironment &render)
{
	std::string type;
	params.getParam("type", type);
	if(type == "blend_mat") return BlendMaterial::factory(params, eparams, render);
	else if(type == "coated_glossy") return CoatedGlossyMaterial::factory(params, eparams, render);
	else if(type == "glass") return GlassMaterial::factory(params, eparams, render);
	else if(type == "mirror") return MirrorMaterial::factory(params, eparams, render);
	else if(type == "null") return NullMaterial::factory(params, eparams, render);
	else if(type == "glossy") return GlossyMaterial::factory(params, eparams, render);
	else if(type == "rough_glass") return RoughGlassMaterial::factory(params, eparams, render);
	else if(type == "shinydiffusemat") return ShinyDiffuseMaterial::factory(params, eparams, render);
	else if(type == "light_mat") return LightMaterial::factory(params, eparams, render);
	else if(type == "mask_mat") return MaskMaterial::factory(params, eparams, render);
	else return nullptr;
}

Material::Material() : bsdf_flags_(BsdfNone), visibility_(NormalVisible), receive_shadows_(true), req_mem_(0), vol_i_(nullptr), vol_o_(nullptr), additional_depth_(0)
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

Rgb Material::sampleClay(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const {
	Vec3 n = FACE_FORWARD(sp.ng_, sp.n_, wo);
	wi = sampleCosHemisphere__(n, sp.nu_, sp.nv_, s.s_1_, s.s_2_);
	s.pdf_ = std::fabs(wi * n);
	w = (std::fabs(wi * sp.n_)) / (s.pdf_ * 0.99f + 0.01f);
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
				wire_frame_amount *= std::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
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
				wire_frame_amount *= std::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
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
				wire_frame_amount *= std::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
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
				wire_frame_amount *= std::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
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
				wire_frame_amount *= std::pow((wireframe_thickness_ - dist) / wireframe_thickness_, wireframe_exponent_);
			}
			col[0].blend(wire_frame_col, wire_frame_amount);
			col[0].a_ = wire_frame_amount;
			col[1].blend(wire_frame_col, wire_frame_amount);
			col[1].a_ = wire_frame_amount;
		}
	}
}


END_YAFARAY
