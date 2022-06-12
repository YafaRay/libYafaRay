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

#ifndef YAFARAY_SHAPE_QUAD_H
#define YAFARAY_SHAPE_QUAD_H

#include "geometry/vector.h"
#include "geometry/uv.h"
#include <array>

BEGIN_YAFARAY

class Ray;
class IntersectData;

class ShapeQuad final
{
	public:
		ShapeQuad(const ShapeQuad &quad) = default;
		ShapeQuad(ShapeQuad &&quad) = default;
		explicit ShapeQuad(std::array<Point3, 4> &&vertices) : vertices_(std::move(vertices)) { }
		IntersectData intersect(const Ray &ray) const;
		Vec3 calculateFaceNormal() const;
		float surfaceArea() const;
		Point3 sample(const Uv<float> &uv) const;
		template <class T> static T interpolate(const Uv<float> &uv, const std::array<T, 4> &t);
		static float getDistToNearestEdge(const Uv<float> &uv, const Vec3 &dp_du_abs, const Vec3 &dp_dv_abs);

	private:
		alignas(8) std::array<Point3, 4> vertices_;
};

inline Vec3 ShapeQuad::calculateFaceNormal() const
{
	//Assuming quad is planar, having same normal as the first triangle
	return ((vertices_[1] - vertices_[0]) ^ (vertices_[2] - vertices_[0])).normalize();
}

inline Point3 ShapeQuad::sample(const Uv<float> &uv) const
{
	return ShapeQuad::interpolate(uv, vertices_);
}

template <class T>
inline T ShapeQuad::interpolate(const Uv<float> &uv, const std::array<T, 4> &t)
{
	return (1.f - uv.v_) * ((1.f - uv.u_) * t[0] + uv.u_ * t[1]) + uv.v_ * ((1.f - uv.u_) * t[3] + uv.u_ * t[2]);
}

inline float ShapeQuad::getDistToNearestEdge(const Uv<float> &uv, const Vec3 &dp_du_abs, const Vec3 &dp_dv_abs)
{
	const float u_dist_rel = 0.5f - std::abs(uv.u_ - 0.5f);
	const float u_dist_abs = u_dist_rel * dp_du_abs.length();
	const float v_dist_rel = 0.5f - std::abs(uv.v_ - 0.5f);
	const float v_dist_abs = v_dist_rel * dp_dv_abs.length();
	return std::min(u_dist_abs, v_dist_abs);
}

END_YAFARAY

#endif //YAFARAY_SHAPE_QUAD_H
