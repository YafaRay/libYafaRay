#pragma once

#ifndef YAFARAY_EQUIRECTANGULARCAMERA_H
#define YAFARAY_EQUIRECTANGULARCAMERA_H

#include <yafray_constants.h>
#include <core_api/camera.h>

BEGIN_YAFRAY

class ParamMap;
class RenderEnvironment;

class EquirectangularCamera final : public Camera
{
	public:
		static Camera *factory(ParamMap &params, RenderEnvironment &render);

	private:
		EquirectangularCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
							  int resx, int resy, float aspect,
							  float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
		virtual void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz) override;
		virtual Ray shootRay(float px, float py, float lu, float lv, float &wt) const override;
		virtual Point3 screenproject(const Point3 &p) const override;

		float aspect_;
};


END_YAFRAY

#endif // YAFARAY_EQUIRECTANGULARCAMERA_H
