#pragma once
/****************************************************************************
 *      simplemats.cc: a collection of simple materials
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

#ifndef YAFARAY_MATERIAL_SIMPLE_H
#define YAFARAY_MATERIAL_SIMPLE_H

#include "material/material.h"

/*=============================================================
a material intended for visible light sources, i.e. it has no
other properties than emiting light in conformance to uniform
surface light sources (area, sphere, mesh lights...)
=============================================================*/

BEGIN_YAFARAY

class LightMaterial final : public Material
{
	public:
		static Material *factory(ParamMap &params, std::list< ParamMap > &eparans, RenderEnvironment &env);

	private:
		LightMaterial(Rgb light_c, bool ds = false);
		virtual void initBsdf(const RenderState &state, SurfacePoint &sp, BsdfFlags &bsdf_types) const { bsdf_types = bsdf_flags_; }
		virtual Rgb eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const {return Rgb(0.0);}
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const;
		virtual Rgb emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual float pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const;

		Rgb light_col_;
		bool double_sided_;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_SIMPLE_H