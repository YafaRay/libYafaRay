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

#ifndef YAFARAY_MATERIAL_ROUGH_GLASS_H
#define YAFARAY_MATERIAL_ROUGH_GLASS_H

#include "material/material_node.h"

BEGIN_YAFARAY

class RoughGlassMaterialData final : public MaterialData
{
	public:
		RoughGlassMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<RoughGlassMaterialData>(*this); }
};

class RoughGlassMaterial final : public NodeMaterial
{
	public:
		static Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		RoughGlassMaterial(Logger &logger, float ior, Rgb filt_c, const Rgb &srcol, bool fake_s, float alpha, float disp_pow, Visibility e_visibility = Visibility::NormalVisible);
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *dir, Rgb &tcol, Sample &s, float *w, bool chromatic, float wavelength) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval) const override { return Rgb{0.f}; }
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override { return 0.f; }
		bool isTransparent() const override { return fake_shadow_; }
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		float getMatIor() const override;
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		Rgb getTransColor(const NodeTreeData &node_tree_data) const override;
		Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;

		const ShaderNode *bump_shader_ = nullptr;
		const ShaderNode *mirror_color_shader_ = nullptr;
		const ShaderNode *roughness_shader_ = nullptr;
		const ShaderNode *ior_shader_ = nullptr;
		const ShaderNode *filter_col_shader_ = nullptr;
		const ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		Rgb filter_color_, specular_reflection_color_;
		Rgb beer_sigma_a_;
		float ior_;
		float a_2_;
		bool absorb_ = false, disperse_ = false, fake_shadow_;
		float dispersion_power_;
		float cauchy_a_, cauchy_b_;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_ROUGH_GLASS_H
