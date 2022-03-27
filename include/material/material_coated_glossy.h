#pragma once
/****************************************************************************
 *      coatedglossy.cc: a glossy material with specular coating
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
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef YAFARAY_MATERIAL_COATED_GLOSSY_H
#define YAFARAY_MATERIAL_COATED_GLOSSY_H

#include <common/logger.h>
#include "material/material_node.h"

BEGIN_YAFARAY

/*! Coated Glossy Material.
	A Material with Phong/Anisotropic Phong microfacet base layer and a layer of
	(dielectric) perfectly specular coating. This is to simulate surfaces like
	metallic paint */

class CoatedGlossyMaterialData final : public MaterialData
{
	public:
		CoatedGlossyMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		float diffuse_, glossy_, p_diffuse_;
};

class CoatedGlossyMaterial final : public NodeMaterial
{
	public:
		static const Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		CoatedGlossyMaterial(Logger &logger, const Rgb &col, const Rgb &dcol, const Rgb &mir_col, float mirror_strength, float reflect, float diff, float ior, float expo, bool as_diff, Visibility e_visibility = Visibility::NormalVisible);
		MaterialData * createMaterialData(size_t number_of_nodes) const override { return new CoatedGlossyMaterialData(bsdf_flags_, number_of_nodes); };
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		Rgb getDiffuseColor(const NodeTreeData &node_tree_data) const override;
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;

		void initOrenNayar(double sigma);
		float orenNayar(const Vec3 &wi, const Vec3 &wo, const Vec3 &n, bool use_texture_sigma, double texture_sigma) const;

		const ShaderNode *diffuse_shader_ = nullptr;
		const ShaderNode *glossy_shader_ = nullptr;
		const ShaderNode *glossy_reflection_shader_ = nullptr;
		const ShaderNode *bump_shader_ = nullptr;
		const ShaderNode *ior_shader_ = nullptr;
		const ShaderNode *exponent_shader_ = nullptr;
		const ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		const ShaderNode *mirror_shader_ = nullptr;        //!< Shader node for specular reflection strength (float)
		const ShaderNode *mirror_color_shader_ = nullptr;   //!< Shader node for specular reflection color
		const ShaderNode *sigma_oren_shader_ = nullptr;     //!< Shader node for sigma in Oren Nayar material
		const ShaderNode *diffuse_reflection_shader_ = nullptr;   //!< Shader node for diffuse reflection strength (float)
		Rgb gloss_color_, diff_color_, mirror_color_; //!< color of glossy base
		float mirror_strength_;              //!< BSDF Specular reflection component strength when not textured
		float ior_;
		float exponent_, exp_u_, exp_v_;
		float reflectivity_;
		float diffuse_;
		bool as_diffuse_, with_diffuse_ = false, anisotropic_ = false;
		BsdfFlags c_flags_[3];
		int n_bsdf_;
		bool oren_nayar_;
		float oren_a_, oren_b_;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_COATED_GLOSSY_H