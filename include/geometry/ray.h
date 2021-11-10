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

#include "common/yafaray_common.h"
#include "geometry/vector.h"
#include <memory>

BEGIN_YAFARAY

struct RayDifferentials
{
	RayDifferentials() = default;
	RayDifferentials(const Point3 &xfrom, const Vec3 &xdir, const Point3 &yfrom, const Vec3 &ydir) : xfrom_(xfrom), xdir_(xdir), yfrom_(yfrom), ydir_(ydir) { }
	Point3 xfrom_, yfrom_;
	Vec3 xdir_, ydir_;
};

class Ray
{
	public:
		enum class DifferentialsAssignment : int { Ignore, Copy };
		Ray() = default;
		Ray(const Ray &ray, DifferentialsAssignment differentials_assignment);
		Ray(Ray &&ray) = default;
		Ray(const Point3 &f, const Vec3 &d, float start = 0.f, float end = -1.f, float ftime = 0.f):
				from_(f), dir_(d), tmin_(start), tmax_(end), time_(ftime) { }
		Ray& operator=(Ray&& ray) = default;
		Point3 from_;
		Vec3 dir_;
		mutable float tmin_ = 0.f, tmax_ = -1.f;
		float time_ = 0.f; //!< relative frame time (values between [0;1]) at which ray was generated
		std::unique_ptr<RayDifferentials> differentials_;
};

inline Ray::Ray(const Ray &ray, DifferentialsAssignment differentials_assignment) : Ray{ray.from_, ray.dir_, ray.tmin_, ray.tmax_, ray.time_}
{
	if(differentials_assignment == DifferentialsAssignment::Copy && ray.differentials_)
	{
		differentials_ = std::unique_ptr<RayDifferentials>(new RayDifferentials(ray.differentials_->xfrom_, ray.differentials_->xdir_, ray.differentials_->yfrom_, ray.differentials_->ydir_));
	}
}

END_YAFARAY

#endif //YAFARAY_RAY_H
