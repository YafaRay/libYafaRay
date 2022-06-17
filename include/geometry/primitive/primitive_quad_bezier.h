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

#ifndef YAFARAY_PRIMITIVE_QUAD_BEZIER_H
#define YAFARAY_PRIMITIVE_QUAD_BEZIER_H

#include "primitive_face.h"
#include "geometry/shape/shape_quad.h"

BEGIN_YAFARAY

class QuadBezierPrimitive : public FacePrimitive
{
	public:
		QuadBezierPrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object);

	private:
		std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time) const override;
		std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const override;
		bool clippingSupport() const override { return false; }
		Bound getBound() const override;
		Bound getBound(const Matrix4 &obj_to_world) const override;
		Vec3 getGeometricNormal(const Uv<float> &uv, float time, bool) const override;
		Vec3 getGeometricNormal(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4 &obj_to_world) const override;
		template<typename T=bool> std::unique_ptr<const SurfacePoint> getSurfaceQuadBezier(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const T &obj_to_world = {}) const;
		float surfaceArea(float time) const override;
		float surfaceArea(float time, const Matrix4 &obj_to_world) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const override;
		float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs) const override { return ShapeQuad::getDistToNearestEdge(uv, dp_abs); }
		template <typename T=bool> ShapeQuad getShapeAtTime(float time, const T &obj_to_world = {}) const;
		template <typename T=bool> std::array<Point3, 4> getVerticesAsArray(int time_step, const T &obj_to_world = {}) const;
		template <typename T=bool> std::array<Point3, 4> getVerticesAsArray(const std::array<float, 3> &bezier_factors, const T &obj_to_world = {}) const;
		inline std::array<Point3, 4> getOrcoVertices(int time_step) const;
		template <typename T=bool> std::array<Vec3, 4> getVerticesNormals(int time_step, const Vec3 &surface_normal_world, const T &obj_to_world = {}) const;
		inline std::array<Uv<float>, 4> getUvs() const;
};

inline QuadBezierPrimitive::QuadBezierPrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object) : FacePrimitive{std::move(vertices_indices), std::move(vertices_uv_indices), mesh_object}
{
}

template <typename T>
inline std::array<Point3, 4> QuadBezierPrimitive::getVerticesAsArray(int time_step, const T &obj_to_world) const
{
	return { getVertex(0, time_step, obj_to_world), getVertex(1, time_step, obj_to_world), getVertex(2, time_step, obj_to_world), getVertex(3, time_step, obj_to_world) };
}

template <typename T>
inline std::array<Point3, 4> QuadBezierPrimitive::getVerticesAsArray(const std::array<float, 3> &bezier_factors, const T &obj_to_world) const
{
	return { getVertex(0, bezier_factors, obj_to_world), getVertex(1, bezier_factors, obj_to_world), getVertex(2, bezier_factors, obj_to_world), getVertex(3, bezier_factors, obj_to_world) };
}

inline std::array<Point3, 4> QuadBezierPrimitive::getOrcoVertices(int time_step) const
{
	return { getOrcoVertex(0, time_step), getOrcoVertex(1, time_step), getOrcoVertex(2, time_step), getOrcoVertex(3, time_step) };
}

template <typename T>
inline std::array<Vec3, 4> QuadBezierPrimitive::getVerticesNormals(int time_step, const Vec3 &surface_normal_world, const T &obj_to_world) const
{
	return { getVertexNormal(0, surface_normal_world, time_step, obj_to_world), getVertexNormal(1, surface_normal_world, time_step, obj_to_world), getVertexNormal(2, surface_normal_world, time_step, obj_to_world), getVertexNormal(3, surface_normal_world, time_step, obj_to_world) };
}

inline std::array<Uv<float>, 4> QuadBezierPrimitive::getUvs() const
{
	return { getVertexUv(0), getVertexUv(1), getVertexUv(2), getVertexUv(3) };
}

inline std::pair<float, Uv<float>> QuadBezierPrimitive::intersect(const Point3 &from, const Vec3 &dir, float time) const
{
	return getShapeAtTime(time).intersect(from, dir);
}

inline std::pair<float, Uv<float>> QuadBezierPrimitive::intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const
{
	return getShapeAtTime(time, obj_to_world).intersect(from, dir);
}

inline float QuadBezierPrimitive::surfaceArea(float time) const
{
	return getShapeAtTime(time).surfaceArea();
}

inline float QuadBezierPrimitive::surfaceArea(float time, const Matrix4 &obj_to_world) const
{
	return getShapeAtTime(time, obj_to_world).surfaceArea();
}

inline std::pair<Point3, Vec3> QuadBezierPrimitive::sample(const Uv<float> &uv, float time) const
{
	const auto quad = getShapeAtTime(time);
	return {
			quad.sample(uv),
			quad.calculateFaceNormal()
	};
}

inline std::pair<Point3, Vec3> QuadBezierPrimitive::sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const
{
	const auto quad = getShapeAtTime(time, obj_to_world);
	return {
			quad.sample(uv),
			quad.calculateFaceNormal()
	};
}

inline Vec3 QuadBezierPrimitive::getGeometricNormal(const Uv<float> &uv, float time, bool) const
{
	return getShapeAtTime(time).calculateFaceNormal();
}

inline Vec3 QuadBezierPrimitive::getGeometricNormal(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const
{
	const Vec3 normal {getShapeAtTime(time, obj_to_world).calculateFaceNormal()};
	return (obj_to_world * normal).normalize();
}

inline Bound QuadBezierPrimitive::getBound() const
{
	return getBoundTimeSteps();
}

inline Bound QuadBezierPrimitive::getBound(const Matrix4 &obj_to_world) const
{
	return getBoundTimeSteps(obj_to_world);
}

template <typename T>
inline ShapeQuad QuadBezierPrimitive::getShapeAtTime(float time, const T &obj_to_world) const
{
	const float time_start = base_mesh_object_.getTimeRangeStart();
	const float time_end = base_mesh_object_.getTimeRangeEnd();

	if(time <= time_start)
		return ShapeQuad {getVerticesAsArray(0, obj_to_world)};
	else if(time >= time_end)
		return ShapeQuad {getVerticesAsArray(2, obj_to_world)};
	else
	{
		const float time_mapped = math::lerpSegment(time, 0.f, time_start, 1.f, time_end); //time_mapped must be in range [0.f-1.f]
		const auto bezier = math::bezierCalculateFactors(time_mapped);
		return ShapeQuad{getVerticesAsArray(bezier, obj_to_world)};
	}
}

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_QUAD_BEZIER_H
