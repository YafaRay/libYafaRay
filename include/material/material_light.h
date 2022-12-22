#pragma once
/****************************************************************************
 *      material_light.h: a material intended for visible light sources, i.e.
 *      it has no other properties than emiting light in conformance to uniform
 *      surface light sources (area, sphere, mesh lights...)
 *
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

#ifndef LIBYAFARAY_MATERIAL_LIGHT_H
#define LIBYAFARAY_MATERIAL_LIGHT_H

#include "material/material.h"
#include "material/material_data.h"

namespace yafaray {

class LightMaterialData final : public MaterialData
{
	public:
		LightMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<LightMaterialData>(*this); }
};

class LightMaterial final : public Material
{
		using ThisClassType_t = LightMaterial; using ParentClassType_t = Material; using MaterialData_t = LightMaterialData;

	public:
		inline static std::string getClassName() { return "LightMaterial"; }
		static std::pair<std::unique_ptr<Material>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		LightMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items<Material> &materials);

	private:
		[[nodiscard]] Type type() const override { return Type::Light; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(bool , double_sided_, false, "double_sided", "");
		} params_;
		std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override { return std::make_unique<MaterialData_t>(bsdf_flags_, 0); }
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const override { return Rgb{0.0}; }
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const override;

		const Rgb light_col_{params_.color_ * params_.power_};
};

} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_LIGHT_H