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

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_TRIANGLE_H
