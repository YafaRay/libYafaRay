#pragma once
/****************************************************************************
 *      glass.cc: a dielectric material with dispersion, two trivial mats
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

#ifndef YAFARAY_MATERIAL_GLASS_H
#define YAFARAY_MATERIAL_GLASS_H

#include "material/material_node.h"

BEGIN_YAFARAY

class GlassMaterial final : public NodeMaterial
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &);

	private:
		GlassMaterial(Logger &logger, float ior, Rgb filt_c, const Rgb &srcol, double disp_pow, bool fake_s, Visibility e_visibility = Visibility::NormalVisible);
		virtual void initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const;
		virtual Rgb eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const {return Rgb(0.0);}
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const;
		virtual float pdf(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const {return 0.f;}
		virtual bool isTransparent() const { return fake_shadow_; }
		virtual Rgb getTransparency(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual float getAlpha(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual Specular getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual float getMatIor() const;
		virtual Rgb getGlossyColor(const RenderData &render_data) const;
		virtual Rgb getTransColor(const RenderData &render_data) const;
		virtual Rgb getMirrorColor(const RenderData &render_data) const;

		ShaderNode *bump_shader_ = nullptr;
		ShaderNode *mirror_color_shader_ = nullptr;
		ShaderNode *filter_color_shader_ = nullptr;
		ShaderNode *ior_shader_ = nullptr;
		ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		Rgb filter_color_, specular_reflection_color_;
		Rgb beer_sigma_a_;
		float ior_;
		bool absorb_ = false, disperse_ = false, fake_shadow_;
		BsdfFlags tm_flags_;
		float dispersion_power_;
		float cauchy_a_, cauchy_b_;
};

/*====================================
a simple mirror mat
==================================*/

class MirrorMaterial final : public Material
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &);

	private:
		MirrorMaterial(Logger &logger, Rgb r_col, float ref_val): Material(logger), ref_(ref_val)
		{
			if(ref_ > 1.0) ref_ = 1.0;
			ref_col_ = r_col * ref_val;
			bsdf_flags_ = BsdfFlags::Specular;
		}
		virtual void initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const override { bsdf_types = bsdf_flags_; }
		virtual Rgb eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const override {return Rgb(0.0);}
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const override;
		virtual Specular getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		Rgb ref_col_;
		float ref_;
};

/*=============================================================
a "dummy" material, useful e.g. to keep photons from getting
stored on surfaces that don't affect the scene
=============================================================*/

class NullMaterial final : public Material
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &);

	private:
		NullMaterial(Logger &logger) : Material(logger) { }
		virtual void initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const override { bsdf_types = BsdfFlags::None; }
		virtual Rgb eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const override {return Rgb(0.0);}
		virtual Rgb sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const override;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_GLASS_H