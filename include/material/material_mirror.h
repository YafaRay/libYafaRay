#pragma once
/****************************************************************************
 *      material_mirror.h: a simple mirror mat
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

#ifndef LIBYAFARAY_MATERIAL_MIRROR_H
#define LIBYAFARAY_MATERIAL_MIRROR_H

#include "material/material_node.h"
#include "material/material_data.h"

namespace yafaray {

class MirrorMaterialData final : public MaterialData
{
	public:
		MirrorMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<MirrorMaterialData>(*this); }
};

class MirrorMaterial final : public Material
{
		using ThisClassType_t = MirrorMaterial; using ParentClassType_t = Material; using MaterialData_t = MirrorMaterialData;

	public:
		inline static std::string getClassName() { return "MirrorMaterial"; }
		static std::pair<std::unique_ptr<Material>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps, size_t id);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		MirrorMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map, size_t id);

	private:
		[[nodiscard]] Type type() const override { return Type::Mirror; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float , reflect_, 1.f, "reflect", "");
		} params_;
		std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override { return std::make_unique<MaterialData_t>(bsdf_flags_, 0); }
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const override {return Rgb(0.0);}
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const override;
		const Rgb ref_col_{params_.color_ * std::min(params_.reflect_, 1.f)};
};

} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_MIRROR_H