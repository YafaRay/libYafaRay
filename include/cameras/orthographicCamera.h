#pragma once

#ifndef YAFARAY_ORTHOGRAPHICCAMERA_H
#define YAFARAY_ORTHOGRAPHICCAMERA_H

#include <yafray_constants.h>
#include <core_api/camera.h>

BEGIN_YAFRAY

class ParamMap;
class RenderEnvironment;

class OrthographicCamera: public Camera
{
	public:
		OrthographicCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
						   int resx, int resy, float aspect, float scale,
						   float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
		virtual void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz);
		virtual Ray shootRay(float px, float py, float lu, float lv, float &wt) const;
		virtual Point3 screenproject(const Point3 &p) const;

		static Camera *factory(ParamMap &params, RenderEnvironment &render);
	protected:
		float scale_;
		Point3 pos_;
};

END_YAFRAY

#endif // YAFARAY_ORTHOGRAPHICCAMERA_H
