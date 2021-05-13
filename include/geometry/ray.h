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

#ifndef YAFARAY_RAY_H
#define YAFARAY_RAY_H

#include "yafaray_conf.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class Ray
{
	public:
		Ray(): tmin_(0), tmax_(-1.f), time_(0.f) {}
		Ray(const Point3 &f, const Vec3 &d, float start = 0.f, float end = -1.f, float ftime = 0.f):
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
		DiffRay(const Point3 &f, const Vec3 &d, float start = 0.0, float end = -1.f, float ftime = 0.f):
				Ray(f, d, start, end, ftime), has_differentials_(false) {}
		bool has_differentials_;
		Point3 xfrom_, yfrom_;
		Vec3 xdir_, ydir_;
};

END_YAFARAY

#endif //YAFARAY_RAY_H
