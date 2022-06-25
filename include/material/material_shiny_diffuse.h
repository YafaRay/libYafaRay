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

#ifndef YAFARAY_MATERIAL_SHINY_DIFFUSE_H
#define YAFARAY_MATERIAL_SHINY_DIFFUSE_H

#include <common/logger.h>
#include "material/material_node.h"

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
	public:
		static Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		ShinyDiffuseMaterial(Logger &logger, const Rgb &diffuse_color, const Rgb &mirror_color, float diffuse_strength, float transparency_strength = 0.0, float translucency_strength = 0.0, float mirror_strength = 0.0, float emit_strength = 0.0, float transmit_filter_strength = 1.0, Visibility visibility = Visibility::NormalVisible);
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, BsdfFlags bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, BsdfFlags bsdfs) const override;
		bool isTransparent() const override { return is_transparent_; }
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo) const override; // { return emitCol; }
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		Rgb getDiffuseColor(const NodeTreeData &node_tree_data) const override;
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		Rgb getTransColor(const NodeTreeData &node_tree_data) const override;
		Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;
		Rgb getSubSurfaceColor(const NodeTreeData &node_tree_data) const override;

		void config();
		std::array<float, 4> getComponents(const std::array<bool, 4> &use_nodes, const NodeTreeData &node_tree_data) const;
		float getFresnelKr(const Vec3 &wo, const Vec3 &n, float current_ior_squared) const;
		void initOrenNayar(double sigma);
		float orenNayar(const Vec3 &wi, const Vec3 &wo, const Vec3 &n, bool use_texture_sigma, double texture_sigma) const;
		static std::array<float, 4> accumulate(const std::array<float, 4> &components, float kr);

		bool is_transparent_ = false;                  //!< Boolean value which is true if you have transparent component
		bool is_translucent_ = false;                  //!< Boolean value which is true if you have translucent component
		bool is_mirror_ = false;                       //!< Boolean value which is true if you have specular reflection component
		bool is_diffuse_ = false;                      //!< Boolean value which is true if you have diffuse component

		bool has_fresnel_effect_ = false;               //!< Boolean value which is true if you have Fresnel specular effect
		float ior_ = 1.f;                              //!< IOR
		float ior_squared_ = 1.f;                     //!< Squared IOR

		std::array<bool, 4> components_view_independent_{{false, false, false, false}};
		const ShaderNode *diffuse_shader_ = nullptr;       //!< Shader node for diffuse color
		const ShaderNode *bump_shader_ = nullptr;          //!< Shader node for bump
		const ShaderNode *transparency_shader_ = nullptr;  //!< Shader node for transparency strength (float)
		const ShaderNode *translucency_shader_ = nullptr;  //!< Shader node for translucency strength (float)
		const ShaderNode *mirror_shader_ = nullptr;        //!< Shader node for specular reflection strength (float)
		const ShaderNode *mirror_color_shader_ = nullptr;   //!< Shader node for specular reflection color
		const ShaderNode *sigma_oren_shader_ = nullptr;     //!< Shader node for sigma in Oren Nayar material
		const ShaderNode *diffuse_refl_shader_ = nullptr;   //!< Shader node for diffuse reflection strength (float)
		const ShaderNode *ior_shader_ = nullptr;                 //!< Shader node for IOR value (float)
		const ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)

		Rgb diffuse_color_;              //!< BSDF Diffuse component color
		Rgb emit_color_;                 //!< Emit color
		Rgb mirror_color_;               //!< BSDF Mirror component color
		float mirror_strength_;              //!< BSDF Specular reflection component strength when not textured
		float transparency_strength_;        //!< BSDF Transparency component strength when not textured
		float translucency_strength_;        //!< BSDF Translucency component strength when not textured
		float diffuse_strength_;             //!< BSDF Diffuse component strength when not textured
		float emit_strength_;                //!< Emit strength
		float transmit_filter_strength_;      //!< determines how strong light passing through material gets tinted

		bool use_oren_nayar_ = false;         //!< Use Oren Nayar reflectance (default Lambertian)
		float oren_nayar_a_ = 0.f;           //!< Oren Nayar A coefficient
		float oren_nayar_b_ = 0.f;           //!< Oren Nayar B coefficient

		int n_bsdf_ = 0;

		BsdfFlags c_flags_[4];                   //!< list the BSDF components that are present
		int c_index_[4];                      //!< list the index of the BSDF components (0=specular reflection, 1=specular transparency, 2=translucency, 3=diffuse reflection)
};


} //namespace yafaray

#endif // YAFARAY_MATERIAL_SHINY_DIFFUSE_H
