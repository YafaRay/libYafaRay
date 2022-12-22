#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef LIBYAFARAY_MATERIAL_MASK_H
#define LIBYAFARAY_MATERIAL_MASK_H

#include "material/material_node.h"
#include "material/material_data.h"
#include "common/items.h"

namespace yafaray {

class Texture;
class Scene;

class MaskMaterialData final : public MaterialData
{
	public:
		MaskMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<MaskMaterialData>(*this); }
		MaskMaterialData(const MaskMaterialData &mask_material_data)
			: MaterialData{mask_material_data.bsdf_flags_, mask_material_data.node_tree_data_},
			  select_mat_2_{mask_material_data.select_mat_2_},
			  mat_1_data_{mask_material_data.mat_1_data_->clone()},
			  mat_2_data_{mask_material_data.mat_2_data_->clone()} { }
		bool select_mat_2_;
		std::unique_ptr<const MaterialData> mat_1_data_;
		std::unique_ptr<const MaterialData> mat_2_data_;
};

class MaskMaterial final : public NodeMaterial
{
		using ThisClassType_t = MaskMaterial; using ParentClassType_t = NodeMaterial; using MaterialData_t = MaskMaterialData;

	public:
		inline static std::string getClassName() { return "MaskMaterial"; }
		static std::pair<std::unique_ptr<Material>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		MaskMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, size_t material_1_id, size_t material_2_id, const Items<Material> &materials);

	private:
		[[nodiscard]] Type type() const override { return Type::Mask; }
		struct ShaderNodeType : public Enum<ShaderNodeType>
		{
			enum : ValueType_t { Mask, Size }; //Always leave the Size entry at the end!!
			inline static const EnumMap<ValueType_t> map_{{
				{"mask_shader", Mask, "Shader node for mask value (float)"},
			}};
			bool isBump() { return false; }
		};
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(std::string, material_1_name_, "", "material1", "Name of the first material, must be specified or the blend material exits with an error");
			PARAM_DECL(std::string, material_2_name_, "", "material2", "Name of the second material, must be specified or the blend material exits with an error");
			PARAM_DECL(float, threshold_, 0.5f, "threshold", "");
			PARAM_SHADERS_DECL;
		} params_;
		std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const override;
		bool isTransparent() const override;
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const override;
		Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;

		size_t material_1_id_{0};
		size_t material_2_id_{0};
		const Items<Material> &materials_;
		std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> shaders_{initShaderArray<ShaderNodeType::Size>()};
};

} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_MASK_H
