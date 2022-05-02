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
#include "geometry/vector.h"
#include <array>

BEGIN_YAFARAY

class TrianglePrimitive final : public FacePrimitive
{
	public:
		TrianglePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object);

	private:
		IntersectData intersect(const Ray &ray) const override;
		IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const override;
		bool intersectsBound(const ExBound &eb, const Matrix4 *obj_to_world) const override;
		bool clippingSupport() const override { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const override;
		float surfaceArea(const Matrix4 *obj_to_world) const override;
		std::pair<Point3, Vec3> sample(float s_1, float s_2, const Matrix4 *obj_to_world) const override;
		void calculateGeometricFaceNormal() override;
		static void calculateShadingSpace(SurfacePoint &sp);
		static IntersectData intersect(const Ray &ray, const std::array<Point3, 3> &vertices);
		static bool intersectsBound(const ExBound &ex_bound, const std::array<Point3, 3> &vertices);
		static Vec3 calculateFaceNormal(const std::array<Point3, 3> &vertices);
		static float surfaceArea(const std::array<Point3, 3> &vertices);
		static Point3 sample(float s_1, float s_2, const std::array<Point3, 3> &vertices);
		//! triBoxOverlap and related functions are based on "AABB-triangle overlap test code" by Tomas Akenine-Möller
		static bool triBoxOverlap(const Vec3Double &boxcenter, const Vec3Double &boxhalfsize, const std::array<Vec3Double, 3> &triverts);
};

inline TrianglePrimitive::TrianglePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object) : FacePrimitive(vertices_indices, vertices_uv_indices, mesh_object)
{
	calculateGeometricFaceNormal();
}

inline IntersectData TrianglePrimitive::intersect(const Ray &ray) const
{
	return TrianglePrimitive::intersect(ray, { getVertex(0), getVertex(1), getVertex(2) });
}

inline IntersectData TrianglePrimitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	return TrianglePrimitive::intersect(ray, { getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) });
}

inline bool TrianglePrimitive::intersectsBound(const ExBound &ex_bound, const Matrix4 *obj_to_world) const
{
	return TrianglePrimitive::intersectsBound(ex_bound, { getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) });
}

inline bool TrianglePrimitive::intersectsBound(const ExBound &ex_bound, const std::array<Point3, 3> &vertices)
{
	std::array<Vec3Double, 3> t_points;
	for(size_t i = 0; i < 3; ++i)
		for(size_t j = 0; j < 3; ++j)
			t_points[j][i] = vertices[j][i];
	return triBoxOverlap(ex_bound.center_, ex_bound.half_size_, t_points);
}

inline void TrianglePrimitive::calculateGeometricFaceNormal()
{
	face_normal_geometric_ = calculateFaceNormal({getVertex(0), getVertex(1), getVertex(2)});
}

inline Vec3 TrianglePrimitive::calculateFaceNormal(const std::array<Point3, 3> &vertices)
{
	return ((vertices[1] - vertices[0]) ^ (vertices[2] - vertices[0])).normalize();
}

inline float TrianglePrimitive::surfaceArea(const std::array<Point3, 3> &vertices)
{
	const Vec3 vec_0_1{vertices[1] - vertices[0]};
	const Vec3 vec_0_2{vertices[2] - vertices[0]};
	return 0.5f * (vec_0_1 ^ vec_0_2).length();
}

inline float TrianglePrimitive::surfaceArea(const Matrix4 *obj_to_world) const
{
	return surfaceArea({ getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world) });
}

inline std::pair<Point3, Vec3> TrianglePrimitive::sample(float s_1, float s_2, const Matrix4 *obj_to_world) const
{
	return {
			TrianglePrimitive::sample(s_1, s_2, {getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world)}),
			Primitive::getGeometricFaceNormal(obj_to_world)
	};
}

inline Point3 TrianglePrimitive::sample(float s_1, float s_2, const std::array<Point3, 3> &vertices)
{
	const float su_1 = math::sqrt(s_1);
	const float u = 1.f - su_1;
	const float v = s_2 * su_1;
	return u * vertices[0] + v * vertices[1] + (1.f - u - v) * vertices[2];
}

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_TRIANGLE_H
