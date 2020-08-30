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

#ifndef YAFARAY_UTIL_GEOMETRY_H
#define YAFARAY_UTIL_GEOMETRY_H

#include "constants.h"
#include "common/ray.h"

BEGIN_YAFARAY

struct Plane
{
	Vec3 p_;
	Vec3 n_;
};

inline float rayPlaneIntersection__(Ray const &ray, Plane const &plane)
{
	return plane.n_ * (plane.p_ - Vec3(ray.from_)) / (ray.dir_ * plane.n_);
}

END_YAFARAY

#endif // YAFARAY_UTIL_GEOMETRY_H
