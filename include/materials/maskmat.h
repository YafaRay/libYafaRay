#pragma once

#ifndef YAFARAY_MASKMAT_H
#define YAFARAY_MASKMAT_H

#include <yafray_constants.h>
#include <yafraycore/nodematerial.h>
#include <core_api/color_ramp.h>

BEGIN_YAFRAY

class Texture;
class RenderEnvironment;

class MaskMaterial: public NodeMaterial
{
	public:
		MaskMaterial(const Material *m_1, const Material *m_2, float thresh, Visibility visibility = NormalVisible);
		virtual void initBsdf(const RenderState &state, SurfacePoint &sp, Bsdf_t &bsdf_types) const;
		virtual Rgb eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs, bool force_eval = false) const;
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const;
		virtual float pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs) const;
		virtual bool isTransparent() const;
		virtual Rgb getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual void getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
								 bool &reflect, bool &refract, Vec3 *const dir, Rgb *const col) const;
		virtual Rgb emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual float getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		static Material *factory(ParamMap &, std::list< ParamMap > &, RenderEnvironment &);

	protected:
		const Material *mat_1_;
		const Material *mat_2_;
		ShaderNode *mask_;
		float threshold_;
		//const texture_t *mask;
};

END_YAFRAY

#endif // YAFARAY_MASKMAT_H
