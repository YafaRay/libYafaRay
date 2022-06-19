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

#ifndef YAFARAY_PLANE_H
#define YAFARAY_PLANE_H

#include "common/yafaray_common.h"
#include "geometry/ray.h"

namespace yafaray {

class Plane final
{
	public:
		float rayIntersection(Ray const &ray) const;

		Vec3 p_;
		Vec3 n_;
};

inline float Plane::rayIntersection(Ray const &ray) const
{
	return n_ * (p_ - static_cast<Vec3>(ray.from_)) / (ray.dir_ * n_);
}

} //namespace yafaray

#endif // YAFARAY_PLANE_H
