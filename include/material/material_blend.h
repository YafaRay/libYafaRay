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
		BlendMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<const MaterialData> mat_1_data_;
		std::unique_ptr<const MaterialData> mat_2_data_;
};

class BlendMaterial final : public NodeMaterial
{
	public:
		static const Material *factory(Logger &logger, const ParamMap &params, const std::list<ParamMap> &nodes_params, const Scene &scene);

	private:
		BlendMaterial(Logger &logger, const std::unique_ptr<const Material> *material_1, const std::unique_ptr<const Material> *material_2, float blendv, Visibility visibility = Visibility::NormalVisible);
		MaterialData * createMaterialData(size_t number_of_nodes) const override { return new BlendMaterialData(bsdf_flags_, number_of_nodes); };
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *dir, Rgb &tcol, Sample &s, float *w, bool chromatic, float wavelength) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;
		float getMatIor() const override;
		bool isTransparent() const override;
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		bool scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const override;
		const VolumeHandler *getVolumeHandler(bool inside) const override;
		float getBlendVal(const NodeTreeData &node_tree_data) const;

		const std::unique_ptr<const Material> *mat_1_ = nullptr, *mat_2_ = nullptr;
		const ShaderNode *blend_shader_ = nullptr; //!< the shader node used for blending the materials
		const ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		float blend_val_;
		bool recalc_blend_;
		float blended_ior_;
};


END_YAFARAY

#endif // YAFARAY_MATERIAL_BLEND_H
