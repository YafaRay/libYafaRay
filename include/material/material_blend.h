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

#ifndef YAFARAY_MATERIAL_BLEND_H
#define YAFARAY_MATERIAL_BLEND_H

#include "material/material_node.h"

BEGIN_YAFARAY

/*! A material that blends the properties of two materials
	note: if both materials have specular reflection or specular transmission
	components, recursive raytracing will not work properly!
	Sampling will still work, but possibly be inefficient
	Outdated info... DarkTide
*/

class BlendMaterial final : public NodeMaterial
{
	public:
		static std::unique_ptr<Material> factory(ParamMap &params, std::list<ParamMap> &eparams, Scene &scene);
		virtual ~BlendMaterial() override;

	private:
		BlendMaterial(const Material *m_1, const Material *m_2, float blendv, Visibility visibility = Visibility::NormalVisible);
		virtual void initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const override;
		virtual Rgb eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const override;
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const override;
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w) const override;
		virtual float pdf(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;
		virtual float getMatIor() const override;
		virtual bool isTransparent() const override;
		virtual Rgb getTransparency(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual Rgb emit(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual Specular getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual float getAlpha(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual bool scatterPhoton(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s) const override;
		virtual const VolumeHandler *getVolumeHandler(bool inside) const override;
		float getBlendVal(const RenderData &render_data, const SurfacePoint &sp) const;

		const Material *mat_1_ = nullptr, *mat_2_ = nullptr;
		ShaderNode *blend_shader_ = nullptr; //!< the shader node used for blending the materials
		ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		float blend_val_;
		size_t mmem_1_;
		bool recalc_blend_;
		float blended_ior_;
		mutable BsdfFlags mat_1_flags_, mat_2_flags_;
};


END_YAFARAY

#endif // YAFARAY_MATERIAL_BLEND_H
