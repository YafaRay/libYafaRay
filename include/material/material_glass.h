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
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<GlassMaterialData>(*this); }
};

class GlassMaterial final : public NodeMaterial
{
	public:
		static Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		GlassMaterial(Logger &logger, float ior, Rgb filt_c, const Rgb &srcol, double disp_pow, bool fake_s, Visibility e_visibility = Visibility::NormalVisible);
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const override {return Rgb(0.0);}
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override {return 0.f;}
		bool isTransparent() const override { return fake_shadow_; }
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		float getMatIor() const override;
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		Rgb getTransColor(const NodeTreeData &node_tree_data) const override;
		Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;

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
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<MirrorMaterialData>(*this); }
};

class MirrorMaterial final : public Material
{
	public:
		static Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		MirrorMaterial(Logger &logger, Rgb r_col, float ref_val): Material(logger), ref_(ref_val)
		{
			if(ref_ > 1.0) ref_ = 1.0;
			ref_col_ = r_col * ref_val;
			bsdf_flags_ = BsdfFlags::Specular;
		}
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override { return new MirrorMaterialData(bsdf_flags_, 0); }
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const override {return Rgb(0.0);}
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
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
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<NullMaterialData>(*this); }
};

class NullMaterial final : public Material
{
	public:
		static Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);

	private:
		explicit NullMaterial(Logger &logger) : Material(logger) { }
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override { return new NullMaterialData(bsdf_flags_, 0); }
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const override {return Rgb(0.0);}
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_GLASS_H