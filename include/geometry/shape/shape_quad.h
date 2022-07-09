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

#include "geometry/shape/shape_triangle.h"

namespace yafaray {

class ShapeQuad final
{
	public:
		ShapeQuad(const ShapeQuad &quad) = default;
		ShapeQuad(ShapeQuad &&quad) = default;
		explicit ShapeQuad(const std::array<Point3f, 4> &vertices) : vertices_(vertices) { }
		explicit ShapeQuad(std::array<Point3f, 4> &&vertices) : vertices_(std::move(vertices)) { }
		std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir) const;
		Vec3f calculateFaceNormal() const;
		float surfaceArea() const;
		Point3f sample(const Uv<float> &uv) const;
		template <typename T> static T interpolate(const Uv<float> &uv, const std::array<T, 4> &t);
		static float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3f> &dp_abs);

	private:
		alignas(8) std::array<Point3f, 4> vertices_;
};

inline Vec3f ShapeQuad::calculateFaceNormal() const
{
	//Assuming quad is planar, having same normal as the first triangle
	return ((vertices_[1] - vertices_[0]) ^ (vertices_[2] - vertices_[0])).normalize();
}

inline Point3f ShapeQuad::sample(const Uv<float> &uv) const
{
	return ShapeQuad::interpolate(uv, vertices_);
}

template <typename T>
inline T ShapeQuad::interpolate(const Uv<float> &uv, const std::array<T, 4> &t)
{
	return (1.f - uv.v_) * ((1.f - uv.u_) * t[0] + uv.u_ * t[1]) + uv.v_ * ((1.f - uv.u_) * t[3] + uv.u_ * t[2]);
}

inline float ShapeQuad::getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3f> &dp_abs)
{
	const float u_dist_rel = 0.5f - std::abs(uv.u_ - 0.5f);
	const float u_dist_abs = u_dist_rel * dp_abs.u_.length();
	const float v_dist_rel = 0.5f - std::abs(uv.v_ - 0.5f);
	const float v_dist_abs = v_dist_rel * dp_abs.v_.length();
	return std::min(u_dist_abs, v_dist_abs);
}

inline std::pair<float, Uv<float>> ShapeQuad::intersect(const Point3f &from, const Vec3f &dir) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec3f edge_1{vertices_[1] - vertices_[0]};
	const Vec3f edge_2{vertices_[2] - vertices_[0]};
	const Vec3f pvec_2{dir ^ edge_2};
	const float det_1_2 = edge_1 * pvec_2;
	if(det_1_2 != 0.f) //Test first triangle in the quad
	{
		const float inv_det_1_2 = 1.f / det_1_2;
		const Vec3f tvec{from - vertices_[0]};
		float u = (tvec * pvec_2) * inv_det_1_2;
		if(u >= 0.f && u <= 1.f)
		{
			const Vec3f qvec_1{tvec ^ edge_1};
			const float v = (dir * qvec_1) * inv_det_1_2;
			if(v >= 0.f && (u + v) <= 1.f)
			{
				const float t = edge_2 * qvec_1 * inv_det_1_2;
				if(t > 0.f) return {t, {u + v, v}};
			}
		}
		else //Test second triangle in the quad
		{
			const Vec3f edge_3{vertices_[3] - vertices_[0]};
			const Vec3f pvec_3{dir ^ edge_3};
			const float det_2_3 = edge_2 * pvec_3;
			if(det_2_3 != 0.f)
			{
				const float inv_det_2_3 = 1.f / det_2_3;
				u = (tvec * pvec_3) * inv_det_2_3;
				if(u >= 0.f && u <= 1.f)
				{
					const Vec3f qvec_2{tvec ^ edge_2};
					const float v = (dir * qvec_2) * inv_det_2_3;
					if(v >= 0.f && (u + v) <= 1.f)
					{
						const float t = edge_3 * qvec_2 * inv_det_2_3;
						if(t > 0.f) return {t, {u, u + v}};
					}
				}
			}
		}
	}
	return {};
}

inline float ShapeQuad::surfaceArea() const
{
	return ShapeTriangle{{vertices_[0], vertices_[1], vertices_[2]}}.surfaceArea() + ShapeTriangle{{vertices_[0], vertices_[2], vertices_[3]}}.surfaceArea();
}

} //namespace yafaray

#endif //YAFARAY_SHAPE_QUAD_H
