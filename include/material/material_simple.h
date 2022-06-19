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

namespace yafaray {

class LightMaterialData final : public MaterialData
{
	public:
		LightMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<LightMaterialData>(*this); }
};

class LightMaterial final : public Material
{
	public:
		static Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		LightMaterial(Logger &logger, Rgb light_c, bool ds = false);
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override { return new LightMaterialData(bsdf_flags_, 0); }
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const override { return Rgb{0.0}; }
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;

		Rgb light_col_;
		bool double_sided_;
};

} //namespace yafaray

#endif // YAFARAY_MATERIAL_SIMPLE_H