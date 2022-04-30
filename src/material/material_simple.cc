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

#include "material/material_simple.h"
#include "geometry/surface.h"
#include "scene/scene.h"
#include "common/param.h"
#include "render/render_data.h"
#include "photon/photon.h"

/*=============================================================
a material intended for visible light sources, i.e. it has no
other properties than emiting light in conformance to uniform
surface light sources (area, sphere, mesh lights...)
=============================================================*/

BEGIN_YAFARAY

LightMaterial::LightMaterial(Logger &logger, Rgb light_c, bool ds): Material(logger), light_col_(light_c), double_sided_(ds)
{
	bsdf_flags_ = BsdfFlags::Emit;
}

Rgb LightMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	s.pdf_ = 0.f;
	w = 0.f;
	return Rgb{0.f};
}

Rgb LightMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	if(double_sided_) return light_col_;
	const float angle = wo * sp.n_;
	return angle > 0 ? light_col_ : Rgb(0.f);
}

float LightMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	return 0.f;
}

Material *LightMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params)
{
	Rgb col(1.0);
	double power = 1.0;
	bool ds = false;

	params.getParam("color", col);
	params.getParam("power", power);
	params.getParam("double_sided", ds);
	return new LightMaterial(logger, col * static_cast<float>(power), ds);
}

END_YAFARAY
