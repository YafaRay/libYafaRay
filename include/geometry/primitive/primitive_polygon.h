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

#ifndef LIBYAFARAY_PRIMITIVE_POLYGON_H
#define LIBYAFARAY_PRIMITIVE_POLYGON_H

#include "geometry/primitive/primitive_face.h"
#include "geometry/shape/shape_polygon.h"

namespace yafaray {

template <typename T, size_t N, MotionBlurType MotionBlur>
class PrimitivePolygon final : public FacePrimitive
{
	static_assert(N >= 3 && N <= 4, "This class can only be instantiated with 3 or 4 edges");
	static_assert(std::is_arithmetic_v<T>, "This class can only be instantiated for arithmetic types like int, T, etc");

	public:
		PrimitivePolygon<T, N, MotionBlur>(const FaceIndices<int> &face_indices, const MeshObject &mesh_object);

	private:
		std::pair<T, Uv<T>> intersect(const Point<T, 3> &from, const Vec<T, 3> &dir, T time) const override;
		std::pair<T, Uv<T>> intersect(const Point<T, 3> &from, const Vec<T, 3> &dir, T time, const SquareMatrix<T, 4> &obj_to_world) const override;
		bool clippingSupport() const override { return !hasMotionBlur(); }
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const override;
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const SquareMatrix<T, 4> &obj_to_world) const override;
		Bound<T> getBound() const override;
		Bound<T> getBound(const SquareMatrix<T, 4> &obj_to_world) const override;
		Vec<T, 3> getGeometricNormal(const Uv<T> &uv, T time, bool) const override;
		Vec<T, 3> getGeometricNormal(const Uv<T> &uv, T time, const SquareMatrix<T, 4> &obj_to_world) const override;
		Vec<T, 3> getGeometricNormal(const SquareMatrix<T, 4> &obj_to_world) const;
		Vec<T, 3> getGeometricNormal(bool = false) const;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point<T, 3> &hit_point, T time, const Uv<T> &intersect_uv, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point<T, 3> &hit_point, T time, const Uv<T> &intersect_uv, const Camera *camera, const SquareMatrix<T, 4> &obj_to_world) const override;
		template<typename M=bool> std::unique_ptr<const SurfacePoint> getSurfacePolygon(const RayDifferentials *ray_differentials, const Point<T, 3> &hit_point, T time, const Uv<T> &intersect_uv, const Camera *camera, const M &obj_to_world = {}) const;
		T surfaceArea(T time) const override;
		T surfaceArea(T time, const SquareMatrix<T, 4> &obj_to_world) const override;
		std::pair<Point<T, 3>, Vec<T, 3>> sample(const Uv<T> &uv, T time) const override;
		std::pair<Point<T, 3>, Vec<T, 3>> sample(const Uv<T> &uv, T time, const SquareMatrix<T, 4> &obj_to_world) const override;
		T getDistToNearestEdge(const Uv<T> &uv, const Uv<Vec<T, 3>> &dp_abs) const override { return ShapePolygon<T, N>::getDistToNearestEdge(uv, dp_abs); }
		template <typename M=bool> std::array<Point<T, 3>, N> getVerticesAsArray(int time_step, const M &obj_to_world = {}) const;
		template <typename M=bool> std::array<Point<T, 3>, N> getVerticesAsArray(const std::array<T, 3> &bezier_factors, const M &obj_to_world = {}) const;
		std::array<Point<T, 3>, N> getOrcoVertices(int time_step) const;
		template <typename M=bool> std::array<Vec<T, 3>, N> getVerticesNormals(int time_step, const Vec<T, 3> &surface_normal_world, const M &obj_to_world = {}) const;
		std::array<Uv<T>, N> getVerticesUvs() const;
		template <typename M=bool> ShapePolygon<T, N> getShapeAtTime(T time, const M &obj_to_world = {}) const;
		Vec<T, 3> face_normal_geometric_;
};

template <typename T, size_t N, MotionBlurType MotionBlur>
inline PrimitivePolygon<T, N, MotionBlur>::PrimitivePolygon(const FaceIndices<int> &face_indices, const MeshObject &mesh_object) : FacePrimitive{face_indices, mesh_object},
	face_normal_geometric_{ ShapePolygon<T, N>{getVerticesAsArray(0)}.calculateFaceNormal()}
{
}

template <typename T, size_t N, MotionBlurType MotionBlur>
template <typename M>
inline std::array<Point<T, 3>, N> PrimitivePolygon<T, N, MotionBlur>::getVerticesAsArray(int time_step, const M &obj_to_world) const
{
	if constexpr(N == 3)
	{
		return {getVertex(0, time_step, obj_to_world), getVertex(1, time_step, obj_to_world), getVertex(2, time_step, obj_to_world)};
	}
	else
	{
		return {getVertex(0, time_step, obj_to_world), getVertex(1, time_step, obj_to_world), getVertex(2, time_step, obj_to_world), getVertex(3, time_step, obj_to_world)};
	}
}

template <typename T, size_t N, MotionBlurType MotionBlur>
template <typename M>
inline std::array<Point<T, 3>, N> PrimitivePolygon<T, N, MotionBlur>::getVerticesAsArray(const std::array<T, 3> &bezier_factors, const M &obj_to_world) const
{
	if constexpr(N == 3)
	{
		return { getVertex(0, bezier_factors, obj_to_world), getVertex(1, bezier_factors, obj_to_world), getVertex(2, bezier_factors, obj_to_world) };
	}
	else
	{
		return { getVertex(0, bezier_factors, obj_to_world), getVertex(1, bezier_factors, obj_to_world), getVertex(2, bezier_factors, obj_to_world), getVertex(3, bezier_factors, obj_to_world) };
	}
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline std::array<Point<T, 3>, N> PrimitivePolygon<T, N, MotionBlur>::getOrcoVertices(int time_step) const
{
	if constexpr(N == 3)
	{
		return {getOrcoVertex(0, time_step), getOrcoVertex(1, time_step), getOrcoVertex(2, time_step)};
	}
	else
	{
		return {getOrcoVertex(0, time_step), getOrcoVertex(1, time_step), getOrcoVertex(2, time_step), getOrcoVertex(3, time_step)};
	}
}

template <typename T, size_t N, MotionBlurType MotionBlur>
template <typename M>
inline std::array<Vec<T, 3>, N> PrimitivePolygon<T, N, MotionBlur>::getVerticesNormals(int time_step, const Vec<T, 3> &surface_normal_world, const M &obj_to_world) const
{
	if constexpr(N == 3)
	{
		return {getVertexNormal(0, surface_normal_world, time_step, obj_to_world), getVertexNormal(1, surface_normal_world, time_step, obj_to_world), getVertexNormal(2, surface_normal_world, time_step, obj_to_world)};
	}
	else
	{
		return {getVertexNormal(0, surface_normal_world, time_step, obj_to_world), getVertexNormal(1, surface_normal_world, time_step, obj_to_world), getVertexNormal(2, surface_normal_world, time_step, obj_to_world), getVertexNormal(3, surface_normal_world, time_step, obj_to_world)};
	}
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline std::array<Uv<T>, N> PrimitivePolygon<T, N, MotionBlur>::getVerticesUvs() const
{
	if constexpr(N == 3)
	{
		return {getVertexUv(0), getVertexUv(1), getVertexUv(2)};
	}
	else
	{
		return {getVertexUv(0), getVertexUv(1), getVertexUv(2), getVertexUv(3)};
	}
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline std::pair<T, Uv<T>> PrimitivePolygon<T, N, MotionBlur>::intersect(const Point<T, 3> &from, const Vec<T, 3> &dir, T time) const
{
	return getShapeAtTime(time).intersect(from, dir);
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline std::pair<T, Uv<T>> PrimitivePolygon<T, N, MotionBlur>::intersect(const Point<T, 3> &from, const Vec<T, 3> &dir, T time, const SquareMatrix<T, 4> &obj_to_world) const
{
	return getShapeAtTime(time, obj_to_world).intersect(from, dir);
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline T PrimitivePolygon<T, N, MotionBlur>::surfaceArea(T time) const
{
	return getShapeAtTime(time).surfaceArea();
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline T PrimitivePolygon<T, N, MotionBlur>::surfaceArea(T time, const SquareMatrix<T, 4> &obj_to_world) const
{
	return getShapeAtTime(time, obj_to_world).surfaceArea();
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline std::pair<Point<T, 3>, Vec<T, 3>> PrimitivePolygon<T, N, MotionBlur>::sample(const Uv<T> &uv, T time) const
{
	if constexpr(MotionBlur == MotionBlurType::Bezier)
	{
		const auto polygon{getShapeAtTime(time)};
		return {
				polygon.sample(uv),
				polygon.calculateFaceNormal()
		};
	}
	else return {ShapePolygon<T, N>{getVerticesAsArray(0)}.sample(uv), getGeometricNormal()};
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline std::pair<Point<T, 3>, Vec<T, 3>> PrimitivePolygon<T, N, MotionBlur>::sample(const Uv<T> &uv, T time, const SquareMatrix<T, 4> &obj_to_world) const
{
	if constexpr(MotionBlur == MotionBlurType::Bezier)
	{
		const auto polygon = getShapeAtTime(time, obj_to_world);
		return {
				polygon.sample(uv),
				polygon.calculateFaceNormal()
		};
	}
	else return {ShapePolygon<T, N>{getVerticesAsArray(0, obj_to_world)}.sample(uv), getGeometricNormal(obj_to_world) };
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline Bound<T> PrimitivePolygon<T, N, MotionBlur>::getBound() const
{
	if constexpr(MotionBlur == MotionBlurType::Bezier) return getBoundTimeSteps();
	else return FacePrimitive::getBound(getVerticesAsVector(0));
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline Bound<T> PrimitivePolygon<T, N, MotionBlur>::getBound(const SquareMatrix<T, 4> &obj_to_world) const
{
	if constexpr(MotionBlur == MotionBlurType::Bezier) return getBoundTimeSteps(obj_to_world);
	else return FacePrimitive::getBound(getVerticesAsVector(0, obj_to_world));
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline Vec<T, 3> PrimitivePolygon<T, N, MotionBlur>::getGeometricNormal(bool) const
{
	return face_normal_geometric_;
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline Vec<T, 3> PrimitivePolygon<T, N, MotionBlur>::getGeometricNormal(const Uv<T> &, T time, bool) const
{
	return getShapeAtTime(time).calculateFaceNormal();
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline Vec<T, 3> PrimitivePolygon<T, N, MotionBlur>::getGeometricNormal(const Uv<T> &, T time, const SquareMatrix<T, 4> &obj_to_world) const
{
	if constexpr(MotionBlur == MotionBlurType::Bezier)
	{
		const Vec<T, 3> normal {getShapeAtTime(time, obj_to_world).calculateFaceNormal()};
		return (obj_to_world * normal).normalize();
	}
	else return getGeometricNormal(obj_to_world);
}

template <typename T, size_t N, MotionBlurType MotionBlur>
inline Vec<T, 3> PrimitivePolygon<T, N, MotionBlur>::getGeometricNormal(const SquareMatrix<T, 4> &obj_to_world) const
{
	return (obj_to_world * face_normal_geometric_).normalize();
}

template <typename T, size_t N, MotionBlurType MotionBlur>
template <typename M>
inline ShapePolygon<T, N> PrimitivePolygon<T, N, MotionBlur>::getShapeAtTime(T time, const M &obj_to_world) const
{
	if constexpr(MotionBlur == MotionBlurType::Bezier)
	{
		const T time_start{base_mesh_object_.getTimeRangeStart()};
		const T time_end{base_mesh_object_.getTimeRangeEnd()};

		if(time <= time_start)
			return ShapePolygon<T, N>{getVerticesAsArray(0, obj_to_world)};
		else if(time >= time_end)
			return ShapePolygon<T, N>{getVerticesAsArray(2, obj_to_world)};
		else
		{
			const T time_mapped{math::lerpSegment(time, T{0}, time_start, T{1}, time_end)}; //time_mapped must be in range [0.f-1.f]
			const auto bezier{math::bezierCalculateFactors(time_mapped)};
			return ShapePolygon<T, N>{getVerticesAsArray(bezier, obj_to_world)};
		}
	}
	else return ShapePolygon<T, N>{getVerticesAsArray(0, obj_to_world)};
}

} //namespace yafaray

#endif //LIBYAFARAY_PRIMITIVE_POLYGON_H
