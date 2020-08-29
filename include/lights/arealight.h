#pragma once

#ifndef YAFARAY_AREALIGHT_H
#define YAFARAY_AREALIGHT_H

#include <core_api/light.h>
#include <core_api/environment.h>

BEGIN_YAFRAY

class AreaLight final : public Light
{
	public:
		static Light *factory(ParamMap &params, RenderEnvironment &render);

	private:
		AreaLight(const Point3 &c, const Vec3 &v_1, const Vec3 &v_2,
				  const Rgb &col, float inte, int nsam, bool light_enabled = true, bool cast_shadows = true);
		virtual void init(Scene &scene) override;
		virtual Rgb totalEnergy() const override;
		virtual Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		virtual Rgb emitSample(Vec3 &wo, LSample &s) const override;
		virtual bool diracLight() const override { return false; }
		virtual bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const override;
		virtual bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const override { return false; }
		virtual bool canIntersect() const override { return true; }
		virtual bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		virtual float illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const override;
		virtual void emitPdf(const SurfacePoint &sp, const Vec3 &wi, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		virtual int nSamples() const override { return samples_; }

		Point3 corner_, c_2_, c_3_, c_4_;
		Vec3 to_x_, to_y_, normal_, fnormal_;
		Vec3 du_, dv_; //!< directions for hemisampler (emitting photons)
		Rgb color_; //!< includes intensity amplification! so...
		int samples_;
		unsigned int object_id_;
		float intensity_; //!< ...this is actually redundant.
		float area_, inv_area_;
};

END_YAFRAY

#endif //YAFARAY_AREALIGHT_H
