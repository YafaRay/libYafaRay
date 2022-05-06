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

#ifndef YAFARAY_PRIMITIVE_TRIANGLE_BEZIER_H
#define YAFARAY_PRIMITIVE_TRIANGLE_BEZIER_H

#include "primitive_triangle.h"

BEGIN_YAFARAY

class ShapeTriangle;

enum BezierTimeStep : int { Start = 0, Mid = 1, End = 2 };

/*! a triangle supporting time based deformation described by a quadratic bezier spline */
class TriangleBezierPrimitive final : public TrianglePrimitive
{
	public:
	TriangleBezierPrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object);

	private:
		ShapeTriangle getShapeTriangleAtTime(float time, const Matrix4 *obj_to_world) const;
		IntersectData intersect(const Ray &ray) const override { return TriangleBezierPrimitive::intersect(ray, nullptr); }
		IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const override;
		bool clippingSupport() const override { return false; }
		void calculateGeometricFaceNormal() override;
		float surfaceArea(const Matrix4 *obj_to_world) const override;
		std::pair<Point3, Vec3> sample(float s_1, float s_2, const Matrix4 *obj_to_world) const override;
		Bound getBound() const override { return getBound(nullptr); }
		Bound getBound(const Matrix4 *obj_to_world) const override;
		Point3 getVertex(BezierTimeStep time_step, size_t vertex_number, const Matrix4 *obj_to_world) const { if(obj_to_world) return (*obj_to_world) * getVertex(time_step, vertex_number); else return getVertex(time_step, vertex_number); }
		Point3 getVertex(BezierTimeStep time_step, size_t vertex_number) const; //!< Get face vertex
};

inline TriangleBezierPrimitive::TriangleBezierPrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object) : TrianglePrimitive(vertices_indices, vertices_uv_indices, mesh_object)
{
}

inline Bound TriangleBezierPrimitive::getBound(const Matrix4 *obj_to_world) const
{
	return FacePrimitive::getBound({
			getVertex(BezierTimeStep::Start, 0, obj_to_world), getVertex(BezierTimeStep::Mid, 0, obj_to_world), getVertex(BezierTimeStep::End, 0, obj_to_world),
			getVertex(BezierTimeStep::Start, 1, obj_to_world), getVertex(BezierTimeStep::Mid, 1, obj_to_world), getVertex(BezierTimeStep::End, 1, obj_to_world),
			getVertex(BezierTimeStep::Start, 2, obj_to_world), getVertex(BezierTimeStep::Mid, 2, obj_to_world), getVertex(BezierTimeStep::End, 2, obj_to_world)
	});
}

END_YAFARAY

#endif//YAFARAY_PRIMITIVE_TRIANGLE_BEZIER_H
