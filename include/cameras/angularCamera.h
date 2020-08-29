#pragma once

#ifndef YAFARAY_ANGULARCAMERA_H
#define YAFARAY_ANGULARCAMERA_H

#include <yafray_constants.h>
#include <core_api/camera.h>

BEGIN_YAFRAY

class ParamMap;
class RenderEnvironment;

enum class AngularProjection : int  //Fish Eye Projections as defined in https://en.wikipedia.org/wiki/Fisheye_lens
{
	Equidistant = 0, //Default and used traditionally in YafaRay
	Orthographic = 1,  //Orthographic projection where the centre of the image is enlarged/more defined at the cost of much more distorted edges
	Stereographic = 2,
	EquisolidAngle = 3,
	Rectilinear = 4,
};

class AngularCamera final : public Camera
{
	public:
		static Camera *factory(ParamMap &params, RenderEnvironment &render);

	private:
		AngularCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
					  int resx, int resy, float aspect, float angle, float max_angle, bool circ, const AngularProjection &projection,
					  float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
		virtual void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz) override;
		virtual Ray shootRay(float px, float py, float lu, float lv, float &wt) const override;
		virtual Point3 screenproject(const Point3 &p) const override;

		float aspect_;
		float focal_length_;
		float max_radius_;
		bool circular_;
		AngularProjection projection_;
};


END_YAFRAY

#endif // YAFARAY_ANGULARCAMERA_H
