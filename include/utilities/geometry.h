#pragma once

#ifndef YAFARAY_GEOMETRY_H
#define YAFARAY_GEOMETRY_H

#include <yafray_constants.h>
#include <core_api/ray.h>

BEGIN_YAFRAY

struct Plane
{
	Vec3 p_;
	Vec3 n_;
};

inline float rayPlaneIntersection__(Ray const &ray, Plane const &plane)
{
	return plane.n_ * (plane.p_ - Vec3(ray.from_)) / (ray.dir_ * plane.n_);
}

END_YAFRAY

#endif // YAFARAY_GEOMETRY_H
