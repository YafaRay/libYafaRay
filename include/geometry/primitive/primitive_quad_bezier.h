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
		IntersectData intersect(const Ray &ray) const override;
		IntersectData intersect(const Ray &ray, const Matrix4 &obj_to_world) const override;
		bool clippingSupport() const override { return false; }
		Bound getBound() const override;
		Bound getBound(const Matrix4 &obj_to_world) const override;
		Vec3 getGeometricNormal(const Uv<float> &uv, float time) const override;
		Vec3 getGeometricNormal(const Matrix4 &obj_to_world, const Uv<float> &uv, float time) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 &obj_to_world, const Camera *camera) const override;
		float surfaceArea(float time) const override;
		float surfaceArea(const Matrix4 &obj_to_world, float time) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, const Matrix4 &obj_to_world, float time) const override;
		float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs) const override { return ShapeQuad::getDistToNearestEdge(uv, dp_abs); }
		ShapeQuad getShapeQuad(int time_step) const;
		ShapeQuad getShapeQuad(const Matrix4 &obj_to_world, int time_step) const;
		ShapeQuad getShapeQuadAtTime(float time) const;
		ShapeQuad getShapeQuadAtTime(const Matrix4 &obj_to_world, float time) const;
};

inline QuadBezierPrimitive::QuadBezierPrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object) : FacePrimitive{std::move(vertices_indices), std::move(vertices_uv_indices), mesh_object}
{
}

inline IntersectData QuadBezierPrimitive::intersect(const Ray &ray) const
{
	return getShapeQuadAtTime(ray.time_).intersect(ray);
}

inline IntersectData QuadBezierPrimitive::intersect(const Ray &ray, const Matrix4 &obj_to_world) const
{
	return getShapeQuadAtTime(obj_to_world, ray.time_).intersect(ray);
}

inline float QuadBezierPrimitive::surfaceArea(float time) const
{
	return getShapeQuadAtTime(time).surfaceArea();
}

inline float QuadBezierPrimitive::surfaceArea(const Matrix4 &obj_to_world, float time) const
{
	return getShapeQuadAtTime(obj_to_world, time).surfaceArea();
}

inline std::pair<Point3, Vec3> QuadBezierPrimitive::sample(const Uv<float> &uv, float time) const
{
	const auto quad = getShapeQuadAtTime(time);
	return {
			quad.sample(uv),
			quad.calculateFaceNormal()
	};
}

inline std::pair<Point3, Vec3> QuadBezierPrimitive::sample(const Uv<float> &uv, const Matrix4 &obj_to_world, float time) const
{
	const auto quad = getShapeQuadAtTime(obj_to_world, time);
	return {
			quad.sample(uv),
			quad.calculateFaceNormal()
	};
}

inline Vec3 QuadBezierPrimitive::getGeometricNormal(const Uv<float> &uv, float time) const
{
	return getShapeQuadAtTime(time).calculateFaceNormal();
}

inline Vec3 QuadBezierPrimitive::getGeometricNormal(const Matrix4 &obj_to_world, const Uv<float> &uv, float time) const
{
	const Vec3 normal {getShapeQuadAtTime(obj_to_world, time).calculateFaceNormal()};
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

inline ShapeQuad QuadBezierPrimitive::getShapeQuad(int time_step) const
{
	return ShapeQuad {{
			getVertex(0, time_step),
			getVertex(1, time_step),
			getVertex(2, time_step),
			getVertex(3, time_step)
	}};
}

inline ShapeQuad QuadBezierPrimitive::getShapeQuad(const Matrix4 &obj_to_world, int time_step) const
{
	return ShapeQuad {{
			getVertex(0, obj_to_world, time_step),
			getVertex(1, obj_to_world, time_step),
			getVertex(2, obj_to_world, time_step),
			getVertex(3, obj_to_world, time_step)
	}};
}

inline ShapeQuad QuadBezierPrimitive::getShapeQuadAtTime(float time) const
{
	const float time_start = base_mesh_object_.getTimeRangeStart();
	const float time_end = base_mesh_object_.getTimeRangeEnd();

	if(time <= time_start)
		return getShapeQuad(0);
	else if(time >= time_end)
		return getShapeQuad(2);
	else
	{
		const float time_mapped = math::lerpSegment(time, 0.f, time_start, 1.f, time_end); //time_mapped must be in range [0.f-1.f]
		const auto bezier = math::bezierCalculateFactors(time_mapped);
		return ShapeQuad{{
				getVertex(0, bezier),
				getVertex(1, bezier),
				getVertex(2, bezier),
				getVertex(3, bezier),
		}};
	}
}

inline ShapeQuad QuadBezierPrimitive::getShapeQuadAtTime(const Matrix4 &obj_to_world, float time) const
{
	const float time_start = base_mesh_object_.getTimeRangeStart();
	const float time_end = base_mesh_object_.getTimeRangeEnd();

	if(time <= time_start)
		return getShapeQuad(obj_to_world, 0);
	else if(time >= time_end)
		return getShapeQuad(obj_to_world, 2);
	else
	{
		const float time_mapped = math::lerpSegment(time, 0.f, time_start, 1.f, time_end); //time_mapped must be in range [0.f-1.f]
		const auto bezier = math::bezierCalculateFactors(time_mapped);
		return ShapeQuad{{
				getVertex(0, obj_to_world, bezier),
				getVertex(1, obj_to_world, bezier),
				getVertex(2, obj_to_world, bezier),
				getVertex(3, obj_to_world, bezier),
		}};
	}
}

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_QUAD_BEZIER_H
