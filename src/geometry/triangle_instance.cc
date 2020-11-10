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

#include "geometry/triangle_instance.h"
#include "geometry/object_triangle_instance.h"
#include "geometry/ray.h"
#include "geometry/surface.h"
#include "geometry/bound.h"
#include "geometry/uv.h"

BEGIN_YAFARAY

Point3 TriangleInstance::getVertex(size_t index) const
{
	return triangle_object_instance_->getVertex(triangle_->getPointId(index));
}

Point3 TriangleInstance::getOrcoVertex(size_t index) const
{
	return triangle_->getOrcoVertex(index);
}

Point3 TriangleInstance::getVertexNormal(size_t index, const Vec3 &surface_normal) const
{
	const int normal_id = triangle_->getNormalId(index);
	return (normal_id >= 0) ? triangle_object_instance_->getVertexNormal(normal_id) : surface_normal;
}

Uv TriangleInstance::getVertexUv(size_t index) const
{
	const size_t uvi = getSelfIndex() * 3;
	const TriangleObject *base_triangle_object = triangle_object_instance_->getBaseTriangleObject();
	return base_triangle_object->getUvValues()[base_triangle_object->getUvOffsets()[uvi]];
}

Vec3 TriangleInstance::getNormal() const
{
	return Vec3(triangle_object_instance_->getObjToWorldMatrix() * triangle_->getNormal()).normalize();
}

const TriangleObject *TriangleInstance::getMesh() const
{
	return triangle_object_instance_;
}

void TriangleInstance::calculateShadingSpace(SurfacePoint &sp) const
{
	// transform dPdU and dPdV in shading space
	Vec3 u_vec, v_vec;
	Vec3::createCs(sp.ng_, u_vec, v_vec);
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = u_vec * sp.dp_du_;
	sp.ds_du_.y_ = v_vec * sp.dp_du_;
	sp.ds_du_.z_ = sp.ng_ * sp.dp_du_;
	sp.ds_dv_.x_ = u_vec * sp.dp_dv_;
	sp.ds_dv_.y_ = v_vec * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.ng_ * sp.dp_dv_;
	sp.ds_du_.normalize();
	sp.ds_dv_.normalize();
}

void TriangleInstance::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	Triangle::sample(s_1, s_2, p, getVertices());
	n = getNormal();
}

END_YAFARAY

