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

#include "geometry/vector.h"
#include <memory>

namespace yafaray {

struct RayDifferentials
{
	RayDifferentials() = default;
	RayDifferentials(const Point3f &xfrom, const Vec3f &xdir, const Point3f &yfrom, const Vec3f &ydir) : xfrom_(xfrom), yfrom_(yfrom), xdir_(xdir), ydir_(ydir) { }
	Point3f xfrom_, yfrom_;
	Vec3f xdir_, ydir_;
};

class Ray
{
	public:
		enum class DifferentialsCopy : unsigned char { No, FullCopy };
		Ray() = default;
		Ray(const Ray &ray, DifferentialsCopy differentials_copy);
		Ray(Ray &&ray) = default;
		Ray(const Point3f &f, const Vec3f &d, float time, float tmin = 0.f, float tmax = -1.f) :
				from_{f}, dir_{d}, tmin_{tmin}, tmax_{tmax}, time_{time} { }
		Ray& operator=(Ray&& ray) = default;
		alignas(8) Point3f from_;
		Vec3f dir_;
		float tmin_ = 0.f, tmax_ = -1.f;
		float time_ = 0.f; //!< relative frame time (values between [0;1]) at which ray was generated
		std::unique_ptr<RayDifferentials> differentials_;
};

inline Ray::Ray(const Ray &ray, DifferentialsCopy differentials_copy) : Ray{ray.from_, ray.dir_, ray.time_, ray.tmin_, ray.tmax_}
{
	if(differentials_copy == DifferentialsCopy::FullCopy && ray.differentials_)
	{
		differentials_ = std::make_unique<RayDifferentials>(*ray.differentials_);
	}
}

} //namespace yafaray

#endif //YAFARAY_RAY_H
