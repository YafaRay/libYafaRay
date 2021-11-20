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

#ifndef YAFARAY_PRIMITIVE_TRIANGLE_BSPLINE_H
#define YAFARAY_PRIMITIVE_TRIANGLE_BSPLINE_H

#include "primitive_face.h"

BEGIN_YAFARAY

/*! a triangle supporting time based deformation described by a quadratic bezier spline */
class BsTrianglePrimitive final : public FacePrimitive
{
	public:
		BsTrianglePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object);

	private:
		virtual IntersectData intersect(const Ray &ray, const Matrix4 *obj_to_world) const override;
		virtual Bound getBound(const Matrix4 *obj_to_world) const override;
		virtual std::unique_ptr<const SurfacePoint> getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const override;
};

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_TRIANGLE_BSPLINE_H
