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

#ifndef LIBYAFARAY_SHAPE_POLYGON_H
#define LIBYAFARAY_SHAPE_POLYGON_H

#include "geometry/vector.h"
#include "geometry/uv.h"
#include <array>

namespace yafaray {

template <typename T, size_t N>
class ShapePolygon final
{
	static_assert(N >= 3 && N <= 4, "This class can only be instantiated with 3 or 4 edges");
	static_assert(std::is_arithmetic_v<T>, "This class can only be instantiated for arithmetic types like int, T, etc");

	public:
		ShapePolygon<T, N>(const ShapePolygon<T, N> &polygon) = default;
		ShapePolygon<T, N>(ShapePolygon<T, N> &&polygon) = default;
		explicit ShapePolygon<T, N>(std::array<Point<T, 3>, N> &&vertices) : vertices_(std::move(vertices)) { }
		explicit ShapePolygon<T, N>(const std::array<Point<T, 3>, N> &vertices) : vertices_(vertices) { }
		ShapePolygon<T, N> &operator=(const ShapePolygon<T, N> &polygon) = default;
		ShapePolygon<T, N> &operator=(ShapePolygon<T, N> &&polygon) = default;
		Point<T, 3> &operator[](int vertex_number) { return vertices_[vertex_number]; }
		const Point<T, 3> &operator[](int vertex_number) const { return vertices_[vertex_number]; }
		std::pair<T, Uv<T>> intersect(const Point<T, 3> &from, const Vec<T, 3> &dir) const;
		Vec<T, 3> calculateFaceNormal() const;
		T surfaceArea() const;
		Point<T, 3> sample(const Uv<T> &uv) const;
		//UV <-> Barycentric UVW relationship is not obvious, interesting explanation in: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
		static std::tuple<T, T, T> getTriangleBarycentricUVW(const Uv<T> &uv) { return { T{1} - uv.u_ - uv.v_, uv.u_, uv.v_ }; }
		static T getDistToNearestEdge(const Uv<T> &uv, const Uv<Vec<T, 3>> &dp_abs);
		template <typename K> static K interpolate(const Uv<T> &uv, const std::array<K, N> &t);

	private:
		alignas(std::max(size_t{8}, sizeof(T))) std::array<Point<T, 3>, N> vertices_;
};

template <typename T, size_t N>
inline Vec<T, 3> ShapePolygon<T, N>::calculateFaceNormal() const
{
	//Assuming polygon is planar, having same normal as the first triangle
	return ((vertices_[1] - vertices_[0]) ^ (vertices_[2] - vertices_[0])).normalize();
}

template <typename T, size_t N>
inline T ShapePolygon<T, N>::surfaceArea() const
{
	const Vec<T, 3> vec_0_1{vertices_[1] - vertices_[0]};
	const Vec<T, 3> vec_0_2{vertices_[2] - vertices_[0]};
	T result{T{0.5} * (vec_0_1 ^ vec_0_2).length()};
	if constexpr (N == 4)
	{
		const Vec<T, 3> vec_0_3{vertices_[3] - vertices_[0]};
		result += T{0.5} * (vec_0_2 ^ vec_0_3).length();
	}
	return result;
}

template <typename T, size_t N>
inline Point<T, 3> ShapePolygon<T, N>::sample(const Uv<T> &uv) const
{
	if constexpr(N == 3)
	{
		const T su_1{math::sqrt(uv.u_)};
		const T u{T{1} - su_1};
		const T v{uv.v_ * su_1};
		return u * vertices_[0] + v * vertices_[1] + (T{1} - u - v) * vertices_[2];
	}
	else
	{
		return interpolate(uv, vertices_);
	}
}

template <typename T, size_t N>
template <typename K>
inline K ShapePolygon<T, N>::interpolate(const Uv<T> &uv, const std::array<K, N> &t)
{
	static_assert(N == 4, "This method can only be instantiated for quads");
	return (T{1} - uv.v_) * ((T{1} - uv.u_) * t[0] + uv.u_ * t[1]) + uv.v_ * ((T{1} - uv.u_) * t[3] + uv.u_ * t[2]);
}

template <typename T, size_t N>
inline T ShapePolygon<T, N>::getDistToNearestEdge(const Uv<T> &uv, const Uv<Vec<T, 3>> &dp_abs)
{
	if constexpr(N == 3)
	{
		const auto [barycentric_u, barycentric_v, barycentric_w]{ShapePolygon<T, N>::getTriangleBarycentricUVW(uv)};
		const T u_dist_rel{T{0.5} - std::abs(barycentric_u - T{0.5})};
		const T u_dist_abs{u_dist_rel * dp_abs.u_.length()};
		const T v_dist_rel{T{0.5} - std::abs(barycentric_v - T{0.5})};
		const T v_dist_abs{v_dist_rel * dp_abs.v_.length()};
		const T w_dist_rel{T{0.5} - std::abs(barycentric_w - T{0.5})};
		const T w_dist_abs{w_dist_rel * (dp_abs.v_ - dp_abs.u_).length()};
		return math::min(u_dist_abs, v_dist_abs, w_dist_abs);
	}
	else
	{
		const T u_dist_rel{T{0.5} - std::abs(uv.u_ - T{0.5})};
		const T u_dist_abs{u_dist_rel * dp_abs.u_.length()};
		const T v_dist_rel{T{0.5} - std::abs(uv.v_ - T{0.5})};
		const T v_dist_abs{v_dist_rel * dp_abs.v_.length()};
		return std::min(u_dist_abs, v_dist_abs);
	}
}

template <typename T, size_t N>
inline std::pair<T, Uv<T>> ShapePolygon<T, N>::intersect(const Point<T, 3> &from, const Vec<T, 3> &dir) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec<T, 3> edge_1{vertices_[1] - vertices_[0]};
	const Vec<T, 3> edge_2{vertices_[2] - vertices_[0]};
	const Vec<T, 3> pvec_2{dir ^ edge_2};
	const T det_1_2{edge_1 * pvec_2};
	if(det_1_2 != T{0}) //Test first triangle in the quad
	{
		const T inv_det_1_2{T{1} / det_1_2};
		const Vec<T, 3> tvec{from - vertices_[0]};
		T u{(tvec * pvec_2) * inv_det_1_2};
		if(u >= T{0} && u <= T{1})
		{
			const Vec<T, 3> qvec_1{tvec ^ edge_1};
			const T v{(dir * qvec_1) * inv_det_1_2};
			if(v >= T{0} && (u + v) <= T{1})
			{
				const T t{edge_2 * qvec_1 * inv_det_1_2};
				if(t > T{0})
				{
					if constexpr(N == 3) return {t, {u, v}};
					else return {t, {u + v, v}};
				}
			}
		}
		else if constexpr(N == 4) //Test second triangle in the quad
		{
			const Vec<T, 3> edge_3{vertices_[3] - vertices_[0]};
			const Vec<T, 3> pvec_3{dir ^ edge_3};
			const T det_2_3{edge_2 * pvec_3};
			if(det_2_3 != T{0})
			{
				const T inv_det_2_3{T{1} / det_2_3};
				u = (tvec * pvec_3) * inv_det_2_3;
				if(u >= T{0} && u <= T{1})
				{
					const Vec<T, 3> qvec_2{tvec ^ edge_2};
					const T v{(dir * qvec_2) * inv_det_2_3};
					if(v >= T{0} && (u + v) <= T{1})
					{
						const T t{edge_3 * qvec_2 * inv_det_2_3};
						if(t > T{0}) return {t, {u, u + v}};
					}
				}
			}
		}
	}
	return {};
}

} //namespace yafaray

#endif //LIBYAFARAY_SHAPE_POLYGON_H
