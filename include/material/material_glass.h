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

class GlassMaterialData final : public MaterialData
{
	public:
		GlassMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
};

class GlassMaterial final : public NodeMaterial
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &);

	private:
		GlassMaterial(Logger &logger, float ior, Rgb filt_c, const Rgb &srcol, double disp_pow, bool fake_s, Visibility e_visibility = Visibility::NormalVisible);
		virtual std::unique_ptr<MaterialData> createMaterialData(size_t number_of_nodes) const override { return std::unique_ptr<GlassMaterialData>(new GlassMaterialData(bsdf_flags_, number_of_nodes)); };
		virtual std::unique_ptr<MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		virtual Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const override {return Rgb(0.0);}
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		virtual float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override {return 0.f;}
		virtual bool isTransparent() const override { return fake_shadow_; }
		virtual Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		virtual float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		virtual Specular getSpecular(int raylevel, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		virtual float getMatIor() const override;
		virtual Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		virtual Rgb getTransColor(const NodeTreeData &node_tree_data) const override;
		virtual Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;

		const ShaderNode *bump_shader_ = nullptr;
		const ShaderNode *mirror_color_shader_ = nullptr;
		const ShaderNode *filter_color_shader_ = nullptr;
		const ShaderNode *ior_shader_ = nullptr;
		const ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
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

class MirrorMaterialData final : public MaterialData
{
	public:
		MirrorMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
};

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
		virtual std::unique_ptr<MaterialData> createMaterialData(size_t number_of_nodes) const override { return std::unique_ptr<MirrorMaterialData>(new MirrorMaterialData(bsdf_flags_, number_of_nodes)); };
		virtual std::unique_ptr<MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override { return createMaterialData(0); }
		virtual Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const override {return Rgb(0.0);}
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		virtual Specular getSpecular(int raylevel, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		Rgb ref_col_;
		float ref_;
};

/*=============================================================
a "dummy" material, useful e.g. to keep photons from getting
stored on surfaces that don't affect the scene
=============================================================*/

class NullMaterialData final : public MaterialData
{
	public:
		NullMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
};

class NullMaterial final : public Material
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &);

	private:
		NullMaterial(Logger &logger) : Material(logger) { }
		virtual std::unique_ptr<MaterialData> createMaterialData(size_t number_of_nodes) const override { return std::unique_ptr<NullMaterialData>(new NullMaterialData(bsdf_flags_, number_of_nodes)); };
		virtual std::unique_ptr<MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override { return createMaterialData(0); }
		virtual Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const override {return Rgb(0.0);}
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_GLASS_H