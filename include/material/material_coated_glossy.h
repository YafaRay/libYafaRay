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

#include "material/material_node.h"

BEGIN_YAFARAY

/*! Coated Glossy Material.
	A Material with Phong/Anisotropic Phong microfacet base layer and a layer of
	(dielectric) perfectly specular coating. This is to simulate surfaces like
	metallic paint */

class CoatedGlossyMaterial final : public NodeMaterial
{
	public:
		static Material *factory(ParamMap &, std::list< ParamMap > &, RenderEnvironment &);

	private:
		CoatedGlossyMaterial(const Rgb &col, const Rgb &dcol, const Rgb &mir_col, float mirror_strength, float reflect, float diff, float ior, float expo, bool as_diff, Visibility e_visibility = Material::Visibility::NormalVisible);
		virtual void initBsdf(const RenderState &state, SurfacePoint &sp, BsdfFlags &bsdf_types) const override;
		virtual Rgb eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval = false) const override;
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const override;
		virtual float pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;
		virtual void getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
								 bool &refl, bool &refr, Vec3 *const dir, Rgb *const col) const override;
		virtual Rgb getDiffuseColor(const RenderState &state) const override;
		virtual Rgb getGlossyColor(const RenderState &state) const override;
		virtual Rgb getMirrorColor(const RenderState &state) const override;

		struct MDat
		{
			float m_diffuse_, m_glossy_, p_diffuse_;
			void *stack_;
		};

		void initOrenNayar(double sigma);
		float orenNayar(const Vec3 &wi, const Vec3 &wo, const Vec3 &n, bool use_texture_sigma, double texture_sigma) const;

		ShaderNode *diffuse_shader_ = nullptr;
		ShaderNode *glossy_shader_ = nullptr;
		ShaderNode *glossy_reflection_shader_ = nullptr;
		ShaderNode *bump_shader_ = nullptr;
		ShaderNode *ior_shader_ = nullptr;
		ShaderNode *exponent_shader_ = nullptr;
		ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		ShaderNode *mirror_shader_ = nullptr;        //!< Shader node for specular reflection strength (float)
		ShaderNode *mirror_color_shader_ = nullptr;   //!< Shader node for specular reflection color
		ShaderNode *sigma_oren_shader_ = nullptr;     //!< Shader node for sigma in Oren Nayar material
		ShaderNode *diffuse_reflection_shader_ = nullptr;   //!< Shader node for diffuse reflection strength (float)
		Rgb gloss_color_, diff_color_, mirror_color_; //!< color of glossy base
		float mirror_strength_;              //!< BSDF Specular reflection component strength when not textured
		float ior_;
		float exponent_, exp_u_, exp_v_;
		float reflectivity_;
		float diffuse_;
		bool as_diffuse_, with_diffuse_ = false, anisotropic_ = false;
		BsdfFlags spec_flags_;
		BsdfFlags c_flags_[3];
		int n_bsdf_;
		bool oren_nayar_;
		float oren_a_, oren_b_;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_COATED_GLOSSY_H