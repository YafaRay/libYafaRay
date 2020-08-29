#pragma once

#ifndef YAFARAY_ROUGHGLASS_H
#define YAFARAY_ROUGHGLASS_H

#include <yafray_constants.h>
#include <yafraycore/nodematerial.h>
#include <core_api/scene.h>

BEGIN_YAFRAY

class RoughGlassMaterial: public NodeMaterial
{
	public:
		RoughGlassMaterial(float ior, Rgb filt_c, const Rgb &srcol, bool fake_s, float alpha, float disp_pow, Visibility e_visibility = NormalVisible);
		virtual void initBsdf(const RenderState &state, SurfacePoint &sp, unsigned int &bsdf_types) const;
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const;
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w) const;
		virtual Rgb eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs, bool force_eval = false) const { return 0.f; }
		virtual float pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs) const { return 0.f; }
		virtual bool isTransparent() const { return fake_shadow_; }
		virtual Rgb getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual float getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const;
		virtual float getMatIor() const;
		static Material *factory(ParamMap &, std::list< ParamMap > &, RenderEnvironment &);
		virtual Rgb getGlossyColor(const RenderState &state) const
		{
			NodeStack stack(state.userdata_);
			return (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);
		}
		virtual Rgb getTransColor(const RenderState &state) const
		{
			NodeStack stack(state.userdata_);
			return (filter_col_shader_ ? filter_col_shader_->getColor(stack) : filter_color_);
		}
		virtual Rgb getMirrorColor(const RenderState &state) const
		{
			NodeStack stack(state.userdata_);
			return (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);
		}

	protected:
		ShaderNode *bump_shader_ = nullptr;
		ShaderNode *mirror_color_shader_ = nullptr;
		ShaderNode *roughness_shader_ = nullptr;
		ShaderNode *ior_shader_ = nullptr;
		ShaderNode *filter_col_shader_ = nullptr;
		ShaderNode *wireframe_shader_ = nullptr;     //!< Shader node for wireframe shading (float)
		Rgb filter_color_, specular_reflection_color_;
		Rgb beer_sigma_a_;
		float ior_;
		float a_2_;
		float a_;
		bool absorb_ = false, disperse_ = false, fake_shadow_;
		float dispersion_power_;
		float cauchy_a_, cauchy_b_;
};

END_YAFRAY

#endif // YAFARAY_ROUGHGLASS_H
