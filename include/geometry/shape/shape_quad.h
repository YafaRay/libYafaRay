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

BEGIN_YAFARAY

class ShapeQuad final
{
	public:
		ShapeQuad(const ShapeQuad &quad) = default;
		ShapeQuad(ShapeQuad &&quad) = default;
		explicit ShapeQuad(std::array<Point3, 4> &&vertices) : vertices_(std::move(vertices)) { }
		IntersectData intersect(const Point3 &from, const Vec3 &dir) const;
		Vec3 calculateFaceNormal() const;
		float surfaceArea() const;
		Point3 sample(const Uv<float> &uv) const;
		template <class T> static T interpolate(const Uv<float> &uv, const std::array<T, 4> &t);
		static float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs);

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

inline float ShapeQuad::getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs)
{
	const float u_dist_rel = 0.5f - std::abs(uv.u_ - 0.5f);
	const float u_dist_abs = u_dist_rel * dp_abs.u_.length();
	const float v_dist_rel = 0.5f - std::abs(uv.v_ - 0.5f);
	const float v_dist_abs = v_dist_rel * dp_abs.v_.length();
	return std::min(u_dist_abs, v_dist_abs);
}

inline IntersectData ShapeQuad::intersect(const Point3 &from, const Vec3 &dir) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec3 edge_1{vertices_[1] - vertices_[0]};
	const Vec3 edge_2{vertices_[2] - vertices_[0]};
	const float edge_1_length = edge_1.length();
	const float edge_2_length = edge_2.length();
	const float epsilon_1_2 = 0.1f * min_raydist_global * std::max(edge_1_length, edge_2_length);
	const Vec3 pvec_2{dir ^ edge_2};
	const float det_1_2 = edge_1 * pvec_2;
	if(det_1_2 <= -epsilon_1_2 || det_1_2 >= epsilon_1_2) //Test first triangle in the quad
	{
		const float inv_det_1_2 = 1.f / det_1_2;
		const Vec3 tvec{from - vertices_[0]};
		float u = (tvec * pvec_2) * inv_det_1_2;
		if(u >= 0.f && u <= 1.f)
		{
			const Vec3 qvec_1{tvec ^ edge_1};
			const float v = (dir * qvec_1) * inv_det_1_2;
			if(v >= 0.f && (u + v) <= 1.f)
			{
				const float t = edge_2 * qvec_1 * inv_det_1_2;
				if(t >= epsilon_1_2) return IntersectData{t, {u + v, v}};
			}
		}
		else //Test second triangle in the quad
		{
			const Vec3 edge_3{vertices_[3] - vertices_[0]};
			const float edge_3_length = edge_3.length();
			const float epsilon_2_3 = 0.1f * min_raydist_global * std::max(edge_2_length, edge_3_length);
			const Vec3 pvec_3{dir ^ edge_3};
			const float det_2_3 = edge_2 * pvec_3;
			if(det_2_3 <= -epsilon_2_3 || det_2_3 >= epsilon_2_3)
			{
				const float inv_det_2_3 = 1.f / det_2_3;
				u = (tvec * pvec_3) * inv_det_2_3;
				if(u >= 0.f && u <= 1.f)
				{
					const Vec3 qvec_2{tvec ^ edge_2};
					const float v = (dir * qvec_2) * inv_det_2_3;
					if(v >= 0.f && (u + v) <= 1.f)
					{
						const float t = edge_3 * qvec_2 * inv_det_2_3;
						if(t >= epsilon_2_3) return IntersectData{t, {u, u + v}};
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

END_YAFARAY

#endif //YAFARAY_SHAPE_QUAD_H
