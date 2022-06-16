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

#ifndef YAFARAY_SHAPE_TRIANGLE_H
#define YAFARAY_SHAPE_TRIANGLE_H

#include "geometry/vector.h"
#include "geometry/uv.h"
#include <array>

BEGIN_YAFARAY

class ShapeTriangle final
{
	public:
		ShapeTriangle(const ShapeTriangle &triangle) = default;
		ShapeTriangle(ShapeTriangle &&triangle) = default;
		explicit ShapeTriangle(std::array<Point3, 3> &&vertices) : vertices_(std::move(vertices)) { }
		std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir) const;
		Vec3 calculateFaceNormal() const;
		float surfaceArea() const;
		Point3 sample(const Uv<float> &uv) const;
		//UV <-> Barycentric UVW relationship is not obvious, interesting explanation in: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
		static std::array<float, 3> getBarycentricUVW(const Uv<float> &uv) { return { 1.f - uv.u_ - uv.v_, uv.u_, uv.v_ }; }
		static float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs);

	private:
		alignas(8) std::array<Point3, 3> vertices_;
};

inline Vec3 ShapeTriangle::calculateFaceNormal() const
{
	return ((vertices_[1] - vertices_[0]) ^ (vertices_[2] - vertices_[0])).normalize();
}

inline float ShapeTriangle::surfaceArea() const
{
	const Vec3 vec_0_1{vertices_[1] - vertices_[0]};
	const Vec3 vec_0_2{vertices_[2] - vertices_[0]};
	return 0.5f * (vec_0_1 ^ vec_0_2).length();
}

inline Point3 ShapeTriangle::sample(const Uv<float> &uv) const
{
	const float su_1 = math::sqrt(uv.u_);
	const float u = 1.f - su_1;
	const float v = uv.v_ * su_1;
	return u * vertices_[0] + v * vertices_[1] + (1.f - u - v) * vertices_[2];
}

inline float ShapeTriangle::getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs)
{
	const auto [barycentric_u, barycentric_v, barycentric_w] = ShapeTriangle::getBarycentricUVW(uv);
	const float u_dist_rel = 0.5f - std::abs(barycentric_u - 0.5f);
	const float u_dist_abs = u_dist_rel * dp_abs.u_.length();
	const float v_dist_rel = 0.5f - std::abs(barycentric_v - 0.5f);
	const float v_dist_abs = v_dist_rel * dp_abs.v_.length();
	const float w_dist_rel = 0.5f - std::abs(barycentric_w - 0.5f);
	const float w_dist_abs = w_dist_rel * (dp_abs.v_ - dp_abs.u_).length();
	return math::min(u_dist_abs, v_dist_abs, w_dist_abs);
}

inline std::pair<float, Uv<float>> ShapeTriangle::intersect(const Point3 &from, const Vec3 &dir) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec3 edge_1{vertices_[1] - vertices_[0]};
	const Vec3 edge_2{vertices_[2] - vertices_[0]};
	const float epsilon = 0.1f * min_raydist_global * std::max(edge_1.length(), edge_2.length());
	const Vec3 pvec{dir ^ edge_2};
	const float det = edge_1 * pvec;
	if(det <= -epsilon || det >= epsilon)
	{
		const float inv_det = 1.f / det;
		const Vec3 tvec{from - vertices_[0]};
		const float u = (tvec * pvec) * inv_det;
		if(u >= 0.f && u <= 1.f)
		{
			const Vec3 qvec{tvec ^ edge_1};
			const float v = (dir * qvec) * inv_det;
			if(v >= 0.f && (u + v) <= 1.f)
			{
				const float t = edge_2 * qvec * inv_det;
				if(t >= epsilon) return {t, {u, v}};
			}
		}
	}
	return {};
}

END_YAFARAY

#endif //YAFARAY_SHAPE_TRIANGLE_H
