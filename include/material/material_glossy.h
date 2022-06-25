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

#ifndef YAFARAY_MATERIAL_GLOSSY_H
#define YAFARAY_MATERIAL_GLOSSY_H

#include <common/logger.h>
#include "material/material_node.h"

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
	public:
		static Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		GlossyMaterial(Logger &logger, const Rgb &col, const Rgb &dcol, float reflect, float diff, float expo, bool as_diffuse, Visibility e_visibility = Visibility::NormalVisible);
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, BsdfFlags bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, BsdfFlags bsdfs) const override;
		Rgb getDiffuseColor(const NodeTreeData &node_tree_data) const override;
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;

		void initOrenNayar(double sigma);
		float orenNayar(const Vec3 &wi, const Vec3 &wo, const Vec3 &n, bool use_texture_sigma, double texture_sigma) const;

		const ShaderNode *diffuse_shader_ = nullptr;
		const ShaderNode *glossy_shader_ = nullptr;
		const ShaderNode *glossy_reflection_shader_ = nullptr;
		const ShaderNode *bump_shader_ = nullptr;
		const ShaderNode *exponent_shader_ = nullptr;
		const ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		const ShaderNode *sigma_oren_shader_ = nullptr;     //!< Shader node for sigma in Oren Nayar material
		const ShaderNode *diffuse_reflection_shader_ = nullptr;   //!< Shader node for diffuse reflection strength (float)
		Rgb gloss_color_, diff_color_;
		float exponent_, exp_u_, exp_v_;
		float reflectivity_;
		float diffuse_;
		bool as_diffuse_, with_diffuse_ = false, anisotropic_ = false;
		bool oren_nayar_;
		float oren_a_, oren_b_;
};

} //namespace yafaray

#endif // YAFARAY_MATERIAL_GLOSSY_H