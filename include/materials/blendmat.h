#pragma once

#ifndef YAFARAY_BLENDMAT_H
#define YAFARAY_BLENDMAT_H

#include <yafray_constants.h>
#include <yafraycore/nodematerial.h>
#include <core_api/shader.h>
#include <core_api/environment.h>
#include <core_api/color_ramp.h>

BEGIN_YAFRAY

/*! A material that blends the properties of two materials
	note: if both materials have specular reflection or specular transmission
	components, recursive raytracing will not work properly!
	Sampling will still work, but possibly be inefficient
	Outdated info... DarkTide
*/

class BlendMaterial final : public NodeMaterial
{
	public:
		static Material *factory(ParamMap &params, std::list<ParamMap> &eparams, RenderEnvironment &render);

	private:
		BlendMaterial(const Material *m_1, const Material *m_2, float blendv, Visibility visibility = NormalVisible);
		virtual ~BlendMaterial() override;
		virtual void initBsdf(const RenderState &state, SurfacePoint &sp, Bsdf_t &bsdf_types) const override;
		virtual Rgb eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, Bsdf_t bsdfs, bool force_eval = false) const override;
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const override;
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w) const override;
		virtual float pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs) const override;
		virtual float getMatIor() const override;
		virtual bool isTransparent() const override;
		virtual Rgb getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual Rgb emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual void getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
								 bool &reflect, bool &refract, Vec3 *const dir, Rgb *const col) const override;
		virtual float getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const override;
		virtual bool scatterPhoton(const RenderState &state, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s) const override;
		virtual const VolumeHandler *getVolumeHandler(bool inside) const override;
		void getBlendVal(const RenderState &state, const SurfacePoint &sp, float &val, float &ival) const;

		const Material *mat_1_ = nullptr, *mat_2_ = nullptr;
		ShaderNode *blend_s_ = nullptr; //!< the shader node used for blending the materials
		ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		float blend_val_;
		float min_thres_;
		float max_thres_;
		size_t mmem_1_;
		bool recalc_blend_;
		float blended_ior_;
		mutable Bsdf_t mat_1_flags_, mat_2_flags_;
};


END_YAFRAY

#endif // YAFARAY_BLENDMAT_H
