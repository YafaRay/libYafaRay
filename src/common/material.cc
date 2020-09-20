/****************************************************************************
 *      material.cc: default implementations for materials
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
#include "common/surface.h"
#include "shader/shader_node.h"
#include "scene/scene.h"
#include "utility/util_sample.h"
#include "utility/util_mcqmc.h"
#include "common/scr_halton.h"

BEGIN_YAFARAY


float Material::material_index_highest_ = 1.f;	//Initially this class shared variable will be 1.f
unsigned int Material::material_index_auto_ = 0;	//Initially this class shared variable will be 0
float Material::highest_sampling_factor_ = 1.f;	//Initially this class shared variable will be 1.f


bool Material::scatterPhoton(const RenderState &state, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s) const
{
	float w = 0.f;
	Rgb scol = sample(state, sp, wi, wo, s, w);
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

Rgb Material::getReflectivity(const RenderState &state, const SurfacePoint &sp, BsdfFlags flags) const
{
	if(!Material::hasFlag(flags, (BsdfFlags::Transmit | BsdfFlags::Reflect) & bsdf_flags_)) return Rgb(0.f);
	float s_1, s_2, s_3, s_4, w = 0.f;
	Rgb total(0.f), col;
	Vec3 wi, wo;
	for(int i = 0; i < 16; ++i)
	{
		s_1 = 0.03125 + 0.0625 * (float)i; // (1.f/32.f) + (1.f/16.f)*(float)i;
		s_2 = riVdC__(i);
		s_3 = scrHalton__(2, i);
		s_4 = scrHalton__(3, i);
		wo = sampleCosHemisphere__(sp.n_, sp.nu_, sp.nv_, s_1, s_2);
		Sample s(s_3, s_4, flags);
		col = sample(state, sp, wo, wi, s, w);
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
