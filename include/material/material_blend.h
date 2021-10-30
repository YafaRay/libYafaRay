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

class BlendMaterialData final : public MaterialData
{
	public:
		std::unique_ptr<MaterialData> mat_1_data_;
		std::unique_ptr<MaterialData> mat_2_data_;
};

class BlendMaterial final : public NodeMaterial
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &params, std::list<ParamMap> &nodes_params, const Scene &scene);
		virtual ~BlendMaterial() override;

	private:
		BlendMaterial(Logger &logger, const Material *m_1, const Material *m_2, float blendv, Visibility visibility = Visibility::NormalVisible);
		virtual std::unique_ptr<MaterialData> createMaterialData() const override { return std::unique_ptr<BlendMaterialData>(new BlendMaterialData()); };
		virtual std::unique_ptr<MaterialData> initBsdf(SurfacePoint &sp, BsdfFlags &bsdf_types, const Camera *camera) const override;
		virtual Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval = false) const override;
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const override;
		virtual float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;
		virtual float getMatIor() const override;
		virtual bool isTransparent() const override;
		virtual Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		virtual Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool lights_geometry_material_emit) const override;
		virtual Material::Specular getSpecular(int raylevel, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		virtual float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		virtual bool scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const override;
		virtual const VolumeHandler *getVolumeHandler(bool inside) const override;
		float getBlendVal(const MaterialData *mat_data, const SurfacePoint &sp) const;

		const Material *mat_1_ = nullptr, *mat_2_ = nullptr;
		ShaderNode *blend_shader_ = nullptr; //!< the shader node used for blending the materials
		ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		float blend_val_;
		bool recalc_blend_;
		float blended_ior_;
		mutable BsdfFlags mat_1_flags_, mat_2_flags_;
};


END_YAFARAY

#endif // YAFARAY_MATERIAL_BLEND_H
