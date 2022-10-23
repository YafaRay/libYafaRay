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

#ifndef LIBYAFARAY_MATERIAL_BLEND_H
#define LIBYAFARAY_MATERIAL_BLEND_H

#include "material/material_node.h"
#include "material/material_data.h"
#include "scene/scene_items.h"

namespace yafaray {

/*! A material that blends the properties of two materials
	note: if both materials have specular reflection or specular transmission
	components, recursive raytracing will not work properly!
	Sampling will still work, but possibly be inefficient
	Outdated info... DarkTide
*/

class BlendMaterialData final : public MaterialData
{
	public:
		BlendMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		BlendMaterialData(const BlendMaterialData &blend_material_data)
			: MaterialData{blend_material_data.bsdf_flags_, blend_material_data.node_tree_data_},
			  mat_1_data_{blend_material_data.mat_1_data_->clone()},
			  mat_2_data_{blend_material_data.mat_2_data_->clone()} { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<BlendMaterialData>(*this); }
		std::unique_ptr<const MaterialData> mat_1_data_;
		std::unique_ptr<const MaterialData> mat_2_data_;
};

class BlendMaterial final : public NodeMaterial
{
		using ThisClassType_t = BlendMaterial; using ParentClassType_t = NodeMaterial; using MaterialData_t = BlendMaterialData;

	public:
		inline static std::string getClassName() { return "BlendMaterial"; }
		static std::pair<std::unique_ptr<Material>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		BlendMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map, size_t material_1_id, size_t material_2_id, const SceneItems<Material> &materials);

	private:
		[[nodiscard]] Type type() const override { return Type::Blend; }
		struct ShaderNodeType : public Enum<ShaderNodeType>
		{
			enum : ValueType_t { Blend, Wireframe, Size }; //Always leave the Size entry at the end!!
			inline static const EnumMap<ValueType_t> map_{{
				{"blend_shader", Blend, "Shader node for blend value (float)"},
				{"wireframe_shader", Wireframe, "Shader node for wireframe shading (float)"},
			}};
			bool isBump() { return false; }
		};
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(std::string, material_1_name_, "", "material1", "Name of the first material, must be specified or the blend material exits with an error");
			PARAM_DECL(std::string, material_2_name_, "", "material2", "Name of the second material, must be specified or the blend material exits with an error");
			PARAM_DECL(float, blend_value_, 0.5f, "blend_value", "");
			PARAM_SHADERS_DECL;
		} params_;
		std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f *dir, Rgb &tcol, Sample &s, float *w, bool chromatic, float wavelength) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const override;
		float getMatIor() const override;
		bool isTransparent() const override;
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		bool scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wi, Vec3f &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const override;
		const VolumeHandler *getVolumeHandler(bool inside) const override;
		float getBlendVal(const NodeTreeData &node_tree_data) const;

		size_t material_1_id_{0};
		size_t material_2_id_{0};
		const SceneItems<Material> &materials_;
		std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> shaders_{initShaderArray<ShaderNodeType::Size>()};
		float blended_ior_{1.f};
};


} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_BLEND_H
