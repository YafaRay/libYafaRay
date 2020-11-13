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

#include "material/material_node.h"

BEGIN_YAFARAY

class GlossyMaterial final : public NodeMaterial
{
	public:
		static Material *factory(ParamMap &, std::list< ParamMap > &, Scene &);

	private:
		GlossyMaterial(const Rgb &col, const Rgb &dcol, float reflect, float diff, float expo, bool as_diffuse, Visibility e_visibility = Visibility::NormalVisible);
		virtual void initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const override;
		virtual Rgb eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval = false) const override;
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const override;
		virtual float pdf(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;

		struct MDat
		{
			float m_diffuse_, m_glossy_, p_diffuse_;
			void *stack_;
		};

		void initOrenNayar(double sigma);

		virtual Rgb getDiffuseColor(const RenderData &render_data) const override;
		virtual Rgb getGlossyColor(const RenderData &render_data) const override;

		float orenNayar(const Vec3 &wi, const Vec3 &wo, const Vec3 &n, bool use_texture_sigma, double texture_sigma) const;

		ShaderNode *diffuse_shader_ = nullptr;
		ShaderNode *glossy_shader_ = nullptr;
		ShaderNode *glossy_reflection_shader_ = nullptr;
		ShaderNode *bump_shader_ = nullptr;
		ShaderNode *exponent_shader_ = nullptr;
		ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		ShaderNode *sigma_oren_shader_ = nullptr;     //!< Shader node for sigma in Oren Nayar material
		ShaderNode *diffuse_reflection_shader_ = nullptr;   //!< Shader node for diffuse reflection strength (float)
		Rgb gloss_color_, diff_color_;
		float exponent_, exp_u_, exp_v_;
		float reflectivity_;
		float diffuse_;
		bool as_diffuse_, with_diffuse_ = false, anisotropic_ = false;
		bool oren_nayar_;
		float oren_a_, oren_b_;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_GLOSSY_H