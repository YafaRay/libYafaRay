#pragma once

#ifndef Y_RAY_H
#define Y_RAY_H

#include <yafray_constants.h>
#include "vector3d.h"

__BEGIN_YAFRAY

class ray_t
{
public:
	ray_t(): tmin(0), tmax(-1.0), time(0.0) {}
	ray_t(const point3d_t &f, const vector3d_t &d, float start=0.0, float end=-1.0, float ftime=0.0):
		from(f), dir(d), tmin(start), tmax(end), time(ftime) { }

	point3d_t from;
	vector3d_t dir;
	mutable float tmin, tmax;
	float time; //!< relative frame time (values between [0;1]) at which ray was generated
};

class diffRay_t: public ray_t
{
	public:
		diffRay_t(): ray_t(), hasDifferentials(false) {}
		diffRay_t(const ray_t &r): ray_t(r), hasDifferentials(false) {}
		diffRay_t(const point3d_t &f, const vector3d_t &d, float start=0.0, float end=-1.0, float ftime=0.0):
			ray_t(f, d, start, end, ftime), hasDifferentials(false) {}
		bool hasDifferentials;
		point3d_t xfrom, yfrom;
		vector3d_t xdir, ydir;
};

__END_YAFRAY

#endif //Y_RAY_H
