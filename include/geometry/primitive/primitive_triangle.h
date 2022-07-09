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

#ifndef YAFARAY_PRIMITIVE_TRIANGLE_H
#define YAFARAY_PRIMITIVE_TRIANGLE_H

#include "primitive_face.h"
#include "geometry/shape/shape_triangle.h"

namespace yafaray {

class TrianglePrimitive : public FacePrimitive
{
	public:
		TrianglePrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object);

	private:
		std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time) const override;
		std::pair<float, Uv<float>> intersect(const Point3f &from, const Vec3f &dir, float time, const Matrix4f &obj_to_world) const override;
		bool clippingSupport() const override { return true; }
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const override;
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3d, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4f &obj_to_world) const override;
		Bound getBound() const override;
		Bound getBound(const Matrix4f &obj_to_world) const override;
		Vec3f getGeometricNormal(const Uv<float> &uv, float time, bool) const override;
		Vec3f getGeometricNormal(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4f &obj_to_world) const override;
		template<typename T=bool> std::unique_ptr<const SurfacePoint> getSurfaceTriangle(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const T &obj_to_world = {}) const;
		float surfaceArea(float time) const override;
		float surfaceArea(float time, const Matrix4f &obj_to_world) const override;
		std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time) const override;
		std::pair<Point3f, Vec3f> sample(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const override;
		float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3f> &dp_abs) const override { return ShapeTriangle::getDistToNearestEdge(uv, dp_abs); }
		Vec3f getGeometricNormal(const Matrix4f &obj_to_world) const;
		Vec3f getGeometricNormal(bool = false) const;
		template <typename T=bool> std::array<Point3f, 3> getVerticesAsArray(int time_step, const T &obj_to_world = {}) const;
		inline std::array<Point3f, 3> getOrcoVertices(int time_step) const;
		template <typename T=bool> std::array<Vec3f, 3> getVerticesNormals(int time_step, const Vec3f &surface_normal_world, const T &obj_to_world = {}) const;
		inline std::array<Uv<float>, 3> getUvs() const;
		Vec3f face_normal_geometric_;
};

inline TrianglePrimitive::TrianglePrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object) :  FacePrimitive{std::move(vertices_indices), std::move(vertices_uv_indices), mesh_object},
	face_normal_geometric_{ ShapeTriangle{getVerticesAsArray(0)}.calculateFaceNormal()}
{
}

template <typename T>
inline std::array<Point3f, 3> TrianglePrimitive::getVerticesAsArray(int time_step, const T &obj_to_world) const
{
	return { getVertex(0, time_step, obj_to_world), getVertex(1, time_step, obj_to_world), getVertex(2, time_step, obj_to_world) };
}

inline std::array<Point3f, 3> TrianglePrimitive::getOrcoVertices(int time_step) const
{
	return { getOrcoVertex(0, time_step), getOrcoVertex(1, time_step), getOrcoVertex(2, time_step) };
}

template <typename T>
inline std::array<Vec3f, 3> TrianglePrimitive::getVerticesNormals(int time_step, const Vec3f &surface_normal_world, const T &obj_to_world) const
{
	return { getVertexNormal(0, surface_normal_world, time_step, obj_to_world), getVertexNormal(1, surface_normal_world, time_step, obj_to_world), getVertexNormal(2, surface_normal_world, time_step, obj_to_world) };
}

inline std::array<Uv<float>, 3> TrianglePrimitive::getUvs() const
{
	return { getVertexUv(0), getVertexUv(1), getVertexUv(2) };
}

inline std::pair<float, Uv<float>> TrianglePrimitive::intersect(const Point3f &from, const Vec3f &dir, float time) const
{
	return ShapeTriangle{getVerticesAsArray(0)}.intersect(from, dir);
}

inline std::pair<float, Uv<float>> TrianglePrimitive::intersect(const Point3f &from, const Vec3f &dir, float time, const Matrix4f &obj_to_world) const
{
	return ShapeTriangle{getVerticesAsArray(0, obj_to_world)}.intersect(from, dir);
}

inline float TrianglePrimitive::surfaceArea(float time) const
{
	return ShapeTriangle{getVerticesAsArray(0)}.surfaceArea();
}

inline float TrianglePrimitive::surfaceArea(float time, const Matrix4f &obj_to_world) const
{
	return ShapeTriangle{getVerticesAsArray(0, obj_to_world)}.surfaceArea();
}

inline std::pair<Point3f, Vec3f> TrianglePrimitive::sample(const Uv<float> &uv, float time) const
{
	return {ShapeTriangle{getVerticesAsArray(0)}.sample(uv), getGeometricNormal() };
}

inline std::pair<Point3f, Vec3f> TrianglePrimitive::sample(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const
{
	return {ShapeTriangle{getVerticesAsArray(0, obj_to_world)}.sample(uv), getGeometricNormal(obj_to_world) };
}

inline Bound TrianglePrimitive::getBound() const
{
	return FacePrimitive::getBound(getVerticesAsVector(0));
}

inline Bound TrianglePrimitive::getBound(const Matrix4f &obj_to_world) const
{
	return FacePrimitive::getBound(getVerticesAsVector(0, obj_to_world));
}

inline Vec3f TrianglePrimitive::getGeometricNormal(bool) const
{
	return face_normal_geometric_;
}

inline Vec3f TrianglePrimitive::getGeometricNormal(const Uv<float> &, float, bool) const
{
	return getGeometricNormal();
}

inline Vec3f TrianglePrimitive::getGeometricNormal(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const
{
	return getGeometricNormal(obj_to_world);
}

inline Vec3f TrianglePrimitive::getGeometricNormal(const Matrix4f &obj_to_world) const
{
	return (obj_to_world * face_normal_geometric_).normalize();
}

} //namespace yafaray

#endif //YAFARAY_PRIMITIVE_TRIANGLE_H
