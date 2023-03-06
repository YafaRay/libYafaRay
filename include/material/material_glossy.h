#pragma once
/****************************************************************************
 *      glossy_mat.cc: a glossy material based on Ashikhmin&Shirley's Paper
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

#ifndef LIBYAFARAY_MATERIAL_GLOSSY_H
#define LIBYAFARAY_MATERIAL_GLOSSY_H

#include "common/logger.h"
#include "material/material_node.h"
#include "material/material_data.h"

namespace yafaray {

class GlossyMaterialData final : public MaterialData
{
	public:
		GlossyMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<GlossyMaterialData>(*this); }
		float m_diffuse_, m_glossy_, p_diffuse_;
};

class GlossyMaterial final : public NodeMaterial
{
		using ThisClassType_t = GlossyMaterial; using ParentClassType_t = NodeMaterial; using MaterialData_t = GlossyMaterialData;

	public:
		inline static std::string getClassName() { return "GlossyMaterial"; }
		static std::pair<std::unique_ptr<Material>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		GlossyMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials);

	private:
		[[nodiscard]] Type type() const override { return Type::Glossy; }
		struct ShaderNodeType : public Enum<ShaderNodeType, size_t>
		{
			enum : ValueType_t { Bump, Wireframe, Diffuse, Glossy, GlossyReflect, Exponent, SigmaOrenNayar, DiffuseReflect, Size }; //Always leave the Size entry at the end!!
			inline static const EnumMap<ValueType_t> map_{{
					{"bump_shader", Bump, ""},
					{"wireframe_shader", Wireframe, "Shader node for wireframe shading (float)"},
					{"diffuse_shader", Diffuse, ""},
					{"glossy_shader", Glossy, ""},
					{"glossy_reflect_shader", GlossyReflect, ""},
					{"exponent_shader", Exponent, ""},
					{"sigma_oren_shader", SigmaOrenNayar, "Shader node for sigma in Oren Nayar material (float)"},
					{"diffuse_refl_shader", DiffuseReflect, "Shader node for diffuse reflection strength (float)"},
				}};
			bool isBump() const { return value() == Bump; }
		};
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(Rgb, glossy_color_, Rgb{1.f}, "color", "");
			PARAM_DECL(Rgb, diffuse_color_, Rgb{1.f}, "diffuse_color", "");
			PARAM_DECL(float, diffuse_reflect_, 0.f, "diffuse_reflect", "");
			PARAM_DECL(float, glossy_reflect_, 1.f, "glossy_reflect", "");
			PARAM_DECL(bool , as_diffuse_, true, "as_diffuse", "");
			PARAM_DECL(float, exponent_, 50.f, "exponent", ""); // This comment was in the old factory code, not sure what it means: "wild guess, do sth better"
			PARAM_DECL(bool , anisotropic_, false, "anisotropic", "");
			PARAM_DECL(float, exp_u_, 50.f, "exp_u", "");
			PARAM_DECL(float, exp_v_, 50.f, "exp_v", "");
			PARAM_ENUM_DECL(DiffuseBrdf, diffuse_brdf_, DiffuseBrdf::Lambertian, "diffuse_brdf", "");
			PARAM_DECL(float, sigma_, 0.1f, "sigma", "Oren-Nayar sigma factor, used if diffuse BRDF is set to Oren-Nayar");
			inline static const auto shader_node_names_meta_{ParamMeta::enumToParamMetaArray<ShaderNodeType>()};
		} params_;
		std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const override;
		Rgb getDiffuseColor(const NodeTreeData &node_tree_data) const override;
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;

		void initOrenNayar(double sigma);
		float orenNayar(const Vec3f &wi, const Vec3f &wo, const Vec3f &n, bool use_texture_sigma, double texture_sigma) const;

		std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> shaders_{initShaderArray<ShaderNodeType::Size>()};
		bool with_diffuse_ = false;
		float oren_a_ = 0.f, oren_b_ = 0.f;
};

} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_GLOSSY_H