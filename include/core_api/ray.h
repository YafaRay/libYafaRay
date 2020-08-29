#pragma once

#ifndef YAFARAY_RAY_H
#define YAFARAY_RAY_H

#include <yafray_constants.h>
#include "vector3d.h"

BEGIN_YAFRAY

class Ray
{
	public:
		Ray(): tmin_(0), tmax_(-1.0), time_(0.0) {}
		Ray(const Point3 &f, const Vec3 &d, float start = 0.0, float end = -1.0, float ftime = 0.0):
				from_(f), dir_(d), tmin_(start), tmax_(end), time_(ftime) { }

		Point3 from_;
		Vec3 dir_;
		mutable float tmin_, tmax_;
		float time_; //!< relative frame time (values between [0;1]) at which ray was generated
};

class DiffRay: public Ray
{
	public:
		DiffRay(): Ray(), has_differentials_(false) {}
		DiffRay(const Ray &r): Ray(r), has_differentials_(false) {}
		DiffRay(const Point3 &f, const Vec3 &d, float start = 0.0, float end = -1.0, float ftime = 0.0):
				Ray(f, d, start, end, ftime), has_differentials_(false) {}
		bool has_differentials_;
		Point3 xfrom_, yfrom_;
		Vec3 xdir_, ydir_;
};

END_YAFRAY

#endif //YAFARAY_RAY_H
