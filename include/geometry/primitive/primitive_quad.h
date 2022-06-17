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
#include "geometry/shape/shape_triangle.h"
#include "geometry/shape/shape_quad.h"

BEGIN_YAFARAY

class QuadPrimitive : public FacePrimitive
{
	public:
		QuadPrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object);

	private:
		std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time) const override;
		std::pair<float, Uv<float>> intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const override;
		bool clippingSupport() const override { return true; }
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const override;
		PolyDouble::ClipResultWithBound clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 &obj_to_world) const override;
		Bound getBound() const override;
		Bound getBound(const Matrix4 &obj_to_world) const override;
		Vec3 getGeometricNormal(const Uv<float> &uv, float time, bool) const override;
		Vec3 getGeometricNormal(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const override;
		std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4 &obj_to_world) const override;
		template<typename T=bool> std::unique_ptr<const SurfacePoint> getSurfaceQuad(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const T &obj_to_world = {}) const;
		float surfaceArea(float time) const override;
		float surfaceArea(float time, const Matrix4 &obj_to_world) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time) const override;
		std::pair<Point3, Vec3> sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const override;
		float getDistToNearestEdge(const Uv<float> &uv, const Uv<Vec3> &dp_abs) const override { return ShapeQuad::getDistToNearestEdge(uv, dp_abs); }
		Vec3 getGeometricNormal(const Matrix4 &obj_to_world) const;
		Vec3 getGeometricNormal(bool = false) const;
		Vec3 face_normal_geometric_;
};

inline QuadPrimitive::QuadPrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object) : FacePrimitive{std::move(vertices_indices), std::move(vertices_uv_indices), mesh_object},
	face_normal_geometric_{ ShapeQuad{{
		getVertex(0, 0),
		getVertex(1, 0),
		getVertex(2, 0),
		getVertex(3, 0),
	}}.calculateFaceNormal()}
{
}

inline std::pair<float, Uv<float>> QuadPrimitive::intersect(const Point3 &from, const Vec3 &dir, float time) const
{
	return ShapeQuad{{
		getVertex(0, 0),
		getVertex(1, 0),
		getVertex(2, 0),
		getVertex(3, 0),
	 }}.intersect(from, dir);
}

inline std::pair<float, Uv<float>> QuadPrimitive::intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const
{
	return ShapeQuad{{
		getVertex(0, 0, obj_to_world),
		getVertex(1, 0, obj_to_world),
		getVertex(2, 0, obj_to_world),
		getVertex(3, 0, obj_to_world),
	 }}.intersect(from, dir);
}

inline float QuadPrimitive::surfaceArea(float time) const
{
	return ShapeQuad{{
		getVertex(0, 0),
		getVertex(1, 0),
		getVertex(2, 0),
		getVertex(3, 0),
	}}.surfaceArea();
}

inline float QuadPrimitive::surfaceArea(float time, const Matrix4 &obj_to_world) const
{
	return ShapeQuad{{
		getVertex(0, 0, obj_to_world),
		getVertex(1, 0, obj_to_world),
		getVertex(2, 0, obj_to_world),
		getVertex(3, 0, obj_to_world),
	}}.surfaceArea();
}

inline std::pair<Point3, Vec3> QuadPrimitive::sample(const Uv<float> &uv, float time) const
{
	return {
			ShapeQuad{{
							  getVertex(0, 0),
							  getVertex(1, 0),
							  getVertex(2, 0),
							  getVertex(3, 0),
					  }}.sample(uv),
		getGeometricNormal()
	};
}

inline std::pair<Point3, Vec3> QuadPrimitive::sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const
{
	return {
			ShapeQuad{{
							  getVertex(0, 0, obj_to_world),
							  getVertex(1, 0, obj_to_world),
							  getVertex(2, 0, obj_to_world),
							  getVertex(3, 0, obj_to_world),
					  }}.sample(uv),
		getGeometricNormal(obj_to_world)
	};
}

inline Bound QuadPrimitive::getBound() const
{
	return FacePrimitive::getBound(getVertices(0));
}

inline Bound QuadPrimitive::getBound(const Matrix4 &obj_to_world) const
{
	return FacePrimitive::getBound(getVertices(0, obj_to_world));
}

inline Vec3 QuadPrimitive::getGeometricNormal(bool) const
{
	return face_normal_geometric_;
}

inline Vec3 QuadPrimitive::getGeometricNormal(const Uv<float> &, float, bool) const
{
	return getGeometricNormal();
}

inline Vec3 QuadPrimitive::getGeometricNormal(const Uv<float> &, float, const Matrix4 &obj_to_world) const
{
	return getGeometricNormal(obj_to_world);
}

inline Vec3 QuadPrimitive::getGeometricNormal(const Matrix4 &obj_to_world) const
{
	return (obj_to_world * face_normal_geometric_).normalize();
}

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_QUAD_H
