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

#ifndef YAFARAY_PRIMITIVE_QUAD_H
#define YAFARAY_PRIMITIVE_QUAD_H

#include "primitive_face.h"
#include "geometry/shape/shape_quad.h"
#include "geometry/shape/shape_triangle.h"
#include <array>

BEGIN_YAFARAY

class QuadPrimitive : public FacePrimitive
{
	public:
		QuadPrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object);

	private:
		IntersectData intersect(const Ray &ray) const override;
		IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const override;
		bool intersectsBound(const ExBound &ex_bound, const Matrix4 *obj_to_world) const override;
		bool clippingSupport() const override { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const override;
		float surfaceArea(const Matrix4 *obj_to_world) const override;
		std::pair<Point3, Vec3> sample(float s_1, float s_2, const Matrix4 *obj_to_world) const override;
		void calculateGeometricFaceNormal() override;
		float getDistToNearestEdge(float u, float v, const Vec3 &dp_du_abs, const Vec3 &dp_dv_abs) const override { return ShapeQuad::getDistToNearestEdge(u, v, dp_du_abs, dp_dv_abs); }
};

inline QuadPrimitive::QuadPrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object) : FacePrimitive(vertices_indices, vertices_uv_indices, mesh_object)
{
	calculateGeometricFaceNormal();
}

inline IntersectData QuadPrimitive::intersect(const Ray &ray) const
{
	return ShapeQuad{ {getVertex(0), getVertex(1), getVertex(2), getVertex(3)} }.intersect(ray);
}

inline IntersectData QuadPrimitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	return ShapeQuad{{ getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world), getVertex(3, obj_to_world) }}.intersect(ray);
}

inline bool QuadPrimitive::intersectsBound(const ExBound &ex_bound, const Matrix4 *obj_to_world) const
{
	return ShapeQuad{{ getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world), getVertex(3, obj_to_world) }}.intersectsBound(ex_bound);
}

inline void QuadPrimitive::calculateGeometricFaceNormal()
{
	//Assuming quad is planar, having same normal as the first triangle
	face_normal_geometric_ = ShapeTriangle{{ getVertex(0), getVertex(1), getVertex(2) }}.calculateFaceNormal();
}

inline float QuadPrimitive::surfaceArea(const Matrix4 *obj_to_world) const
{
	return ShapeQuad{{ getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world), getVertex(3, obj_to_world) }}.surfaceArea();
}

inline std::pair<Point3, Vec3> QuadPrimitive::sample(float s_1, float s_2, const Matrix4 *obj_to_world) const
{
	return {
			ShapeQuad{{getVertex(0, obj_to_world), getVertex(1, obj_to_world), getVertex(2, obj_to_world), getVertex(3, obj_to_world)}}.sample(s_1, s_2),
			Primitive::getGeometricFaceNormal(obj_to_world)
	};
}

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_QUAD_H
