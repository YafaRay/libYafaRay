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

#include "geometry/primitive_instance.h"
#include "geometry/object_instance.h"
#include "geometry/surface.h"
#include "geometry/matrix4.h"
#include "geometry/bound.h"

BEGIN_YAFARAY

Bound PrimitiveInstance::getBound(const Matrix4 *) const
{
	return base_primitive_->getBound(base_object_.getObjToWorldMatrix());
}

bool PrimitiveInstance::intersectsBound(const ExBound &b, const Matrix4 *) const
{
	return base_primitive_->intersectsBound(b, base_object_.getObjToWorldMatrix());
}

bool PrimitiveInstance::clipToBound(const std::array<std::array<double, 3>, 2> (&bound), int axis, Bound &clipped, const void *d_old, void *d_new, const Matrix4 *) const
{
	return base_primitive_->clipToBound(bound, axis, clipped, d_old, d_new, base_object_.getObjToWorldMatrix());
}

IntersectData PrimitiveInstance::intersect(const Ray &ray, const Matrix4 *) const
{
	return base_primitive_->intersect(ray, base_object_.getObjToWorldMatrix());
}

SurfacePoint PrimitiveInstance::getSurface(const Point3 &hit_point, const IntersectData &intersect_data, const Matrix4 *) const
{
	return base_primitive_->getSurface(hit_point, intersect_data, base_object_.getObjToWorldMatrix());
}

float PrimitiveInstance::surfaceArea(const Matrix4 *) const
{
	return base_primitive_->surfaceArea(base_object_.getObjToWorldMatrix());
}

Vec3 PrimitiveInstance::getGeometricNormal(const Matrix4 *, float u, float v) const
{
	return base_primitive_->getGeometricNormal(base_object_.getObjToWorldMatrix(), u, v);
}

void PrimitiveInstance::sample(float s_1, float s_2, Point3 &p, Vec3 &n, const Matrix4 *) const
{
	return base_primitive_->sample(s_1, s_2, p, n, base_object_.getObjToWorldMatrix());
}

END_YAFARAY
