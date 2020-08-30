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

#include <core_api/material.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
#include <core_api/params.h>
#include <materials/maskmat.h>
//#include <yafraycore/spectrum.h>


/*=============================================================
a material intended for visible light sources, i.e. it has no
other properties than emiting light in conformance to uniform
surface light sources (area, sphere, mesh lights...)
=============================================================*/

BEGIN_YAFRAY

class LightMaterial: public Material
{
	public:
		LightMaterial(Rgb light_c, bool ds = false);
		virtual void initBsdf(const RenderState &state, SurfacePoint &sp, unsigned int &bsdf_types) const { bsdf_types = bsdf_flags_; }
		virtual Rgb eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, Bsdf_t bsdfs, bool force_eval = false) const {return Rgb(0.0);}
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const;
		virtual Rgb emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual float pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs) const;
		static Material *factory(ParamMap &params, std::list< ParamMap > &eparans, RenderEnvironment &env);
	protected:
		Rgb light_col_;
		bool double_sided_;
};

LightMaterial::LightMaterial(Rgb light_c, bool ds): light_col_(light_c), double_sided_(ds)
{
	bsdf_flags_ = BsdfEmit;
}

Rgb LightMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	s.pdf_ = 0.f;
	w = 0.f;
	return Rgb(0.f);
}

Rgb LightMaterial::emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	if(!state.include_lights_) return Rgb(0.f);
	if(double_sided_) return light_col_;

	float angle = wo * sp.n_;
	return (angle > 0) ? light_col_ : Rgb(0.f);
}

float LightMaterial::pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs) const
{
	return 0.f;
}
Material *LightMaterial::factory(ParamMap &params, std::list< ParamMap > &eparans, RenderEnvironment &env)
{
	Rgb col(1.0);
	double power = 1.0;
	bool ds = false;

	params.getParam("color", col);
	params.getParam("power", power);
	params.getParam("double_sided", ds);
	return new LightMaterial(col * (float)power, ds);
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("light_mat", LightMaterial::factory);
		render.registerFactory("mask_mat", MaskMaterial::factory);
	}
}

END_YAFRAY
