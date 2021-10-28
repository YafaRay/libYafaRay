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
		virtual size_t getSizeBytes() const override { return sizeof(RoughGlassMaterialData); }
};

class RoughGlassMaterial final : public NodeMaterial
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &);

	private:
		RoughGlassMaterial(Logger &logger, float ior, Rgb filt_c, const Rgb &srcol, bool fake_s, float alpha, float disp_pow, Visibility e_visibility = Visibility::NormalVisible);
		virtual std::unique_ptr<MaterialData> createMaterialData() const override { return std::unique_ptr<RoughGlassMaterialData>(new RoughGlassMaterialData()); };
		virtual void initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const override;
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const override;
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w) const override;
		virtual Rgb eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval = false) const override { return 0.f; }
		virtual float pdf(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override { return 0.f; }
		virtual bool isTransparent() const override { return fake_shadow_; }
		virtual Rgb getTransparency(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual float getAlpha(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual float getMatIor() const override;
		virtual Rgb getGlossyColor(const RenderData &render_data) const override;
		virtual Rgb getTransColor(const RenderData &render_data) const override;
		virtual Rgb getMirrorColor(const RenderData &render_data) const override;

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
