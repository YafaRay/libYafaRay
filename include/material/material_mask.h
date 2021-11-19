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

#ifndef YAFARAY_MATERIAL_MASK_H
#define YAFARAY_MATERIAL_MASK_H

#include <common/logger.h>
#include "material/material_node.h"

BEGIN_YAFARAY

class Texture;
class Scene;

class MaskMaterialData final : public MaterialData
{
	public:
		MaskMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		bool select_mat_2_;
		std::unique_ptr<const MaterialData> mat_1_data_;
		std::unique_ptr<const MaterialData> mat_2_data_;
};

class MaskMaterial final : public NodeMaterial
{
	public:
		static std::unique_ptr<Material> factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &);

	private:
		MaskMaterial(Logger &logger, const Material *m_1, const Material *m_2, float thresh, Visibility visibility = Visibility::NormalVisible);
		virtual std::unique_ptr<MaterialData> createMaterialData(size_t number_of_nodes) const override { return std::unique_ptr<MaskMaterialData>(new MaskMaterialData(bsdf_flags_, number_of_nodes)); };
		virtual std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		virtual Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval = false) const override;
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		virtual float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const override;
		virtual bool isTransparent() const override;
		virtual Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;
		virtual Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const override;
		virtual Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const override;

		const Material *mat_1_ = nullptr;
		const Material *mat_2_ = nullptr;
		ShaderNode *mask_ = nullptr;
		float threshold_;
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_MASK_H
