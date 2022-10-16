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

#ifndef LIBYAFARAY_MATERIAL_SHINY_DIFFUSE_H
#define LIBYAFARAY_MATERIAL_SHINY_DIFFUSE_H

#include "common/logger.h"
#include "material/material_node.h"
#include "material/material_data.h"

namespace yafaray {

/*! A general purpose material for basic diffuse and specular reflecting
    surfaces with transparency and translucency support.
    Parameter definitions are as follows:
    Of the incoming Light, the specular reflected part is substracted.
        l' = l*(1.0 - specular_refl)
    Of the remaining light (l') the specular transmitted light is substracted.
        l" = l'*(1.0 - specular_transmit)
    Of the remaining light (l") the diffuse transmitted light (translucency)
    is substracted.
        l"' =  l"*(1.0 - translucency)
    The remaining (l"') light is either reflected diffuse or absorbed.
*/

class ShinyDiffuseMaterialData final : public MaterialData
{
	public:
		ShinyDiffuseMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<ShinyDiffuseMaterialData>(*this); }
		std::array<float, 4> components_{{0.f, 0.f, 0.f, 0.f}};
};

class ShinyDiffuseMaterial final : public NodeMaterial
{
		using ThisClassType_t = ShinyDiffuseMaterial; using ParentClassType_t = NodeMaterial;

	public:
		inline static std::string getClassName() { return "ShinyDiffuseMaterial"; }
		static std::pair<std::unique_ptr<Material>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		ShinyDiffuseMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::ShinyDiffuse; }
		struct ShaderNodeType : public Enum<ShaderNodeType>
		{
			enum : ValueType_t { Bump, Wireframe, Diffuse, Glossy, Transparency, Translucency, Ior, Mirror, SigmaOrenNayar, DiffuseReflect, MirrorColor, Size }; //Always leave the Size entry at the end!!
			inline static const EnumMap<ValueType_t> map_{{
					{"bump_shader", Bump, ""},
					{"wireframe_shader", Wireframe, "Shader node for wireframe shading (float)"},
					{"diffuse_shader", Diffuse, ""},
					{"IOR_shader", Ior, ""},
					{"mirror_shader", Mirror, "Shader node for specular reflection strength (float)"},
					{"sigma_oren_shader", SigmaOrenNayar, "Shader node for sigma in Oren Nayar material (float)"},
					{"diffuse_refl_shader", DiffuseReflect, "Shader node for diffuse reflection strength (float)"},
					{"mirror_color_shader", MirrorColor, "Shader node for specular reflection color"},
					{"transparency_shader", Transparency, "Shader node for transparency strength (float)"},
					{"translucency_shader", Translucency, "Shader node for translucency strength (float)"},
				}};
			bool isBump() const { return value() == Bump; }
		};
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Rgb, diffuse_color_, Rgb{1.f}, "color", "BSDF Diffuse component color");
			PARAM_DECL(Rgb, mirror_color_, Rgb{1.f}, "mirror_color", "BSDF Mirror component color");
			PARAM_DECL(float, transparency_, 0.f, "transparency", "BSDF Transparency component strength when not textured");
			PARAM_DECL(float, translucency_, 0.f, "translucency", "BSDF Translucency component strength when not textured");
			PARAM_DECL(float, diffuse_reflect_, 1.f, "diffuse_reflect", "BSDF Diffuse component strength when not textured");
			PARAM_DECL(float, specular_reflect_, 0.f, "specular_reflect", "Mirror strength. BSDF Specular reflection component strength when not textured");
			PARAM_DECL(float, emit_, 0.f, "emit", "Light emission strength");
			PARAM_DECL(bool, fresnel_effect_, false, "fresnel_effect", "To enable/disable the Fresnel specular effect");
			PARAM_DECL(float, ior_, 1.33f, "IOR", "Index of refraction, used if the Fresnel effect is enabled");
			PARAM_DECL(float, transmit_filter_, 1.f, "transmit_filter", "Determines how strong light passing through material gets tinted");
			PARAM_ENUM_DECL(DiffuseBrdf, diffuse_brdf_, DiffuseBrdf::Lambertian, "diffuse_brdf", "");
			PARAM_DECL(float, sigma_, 0.1f, "sigma", "Oren-Nayar sigma factor, used if diffuse BRDF is set to Oren-Nayar");
			PARAM_SHADERS_DECL;
		} params_;
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const override;
		bool isTransparent() const override { return is_transparent_; }
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const override; // { return emitCol; }
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		Rgb getDiffuseColor(const NodeTreeData &node_tree_data) const override;
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		Rgb getTransColor(const NodeTreeData &node_tree_data) const override;
		Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;
		Rgb getSubSurfaceColor(const NodeTreeData &node_tree_data) const override;

		void config();
		std::array<float, 4> getComponents(const std::array<bool, 4> &use_nodes, const NodeTreeData &node_tree_data) const;
		float getFresnelKr(const Vec3f &wo, const Vec3f &n, float current_ior_squared) const;
		void initOrenNayar(double sigma);
		float orenNayar(const Vec3f &wi, const Vec3f &wo, const Vec3f &n, bool use_texture_sigma, double texture_sigma) const;
		static std::array<float, 4> accumulate(const std::array<float, 4> &components, float kr);

		bool is_transparent_ = false;                  //!< Boolean value which is true if you have transparent component
		bool is_translucent_ = false;                  //!< Boolean value which is true if you have translucent component
		bool is_mirror_ = false;                       //!< Boolean value which is true if you have specular reflection component
		bool is_diffuse_ = false;                      //!< Boolean value which is true if you have diffuse component

		const float ior_squared_{params_.ior_ * params_.ior_}; //!< Squared IOR

		std::array<bool, 4> components_view_independent_{{false, false, false, false}};
		std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> shaders_{initShaderArray<ShaderNodeType::Size>()};

		const Rgb emit_color_{params_.emit_ * params_.diffuse_color_}; //!< Emit color
		float oren_nayar_a_ = 0.f;           //!< Oren Nayar A coefficient
		float oren_nayar_b_ = 0.f;           //!< Oren Nayar B coefficient
		int n_bsdf_ = 0;
		std::array<BsdfFlags, 4> c_flags_{BsdfFlags::None, BsdfFlags::None, BsdfFlags::None, BsdfFlags::None}; //!< list the BSDF components that are present
		std::array<int, 4> c_index_; //!< list the index of the BSDF components (0=specular reflection, 1=specular transparency, 2=translucency, 3=diffuse reflection)
};


} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_SHINY_DIFFUSE_H
