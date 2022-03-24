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

#include <common/logger.h>
#include "geometry/primitive/primitive_instance.h"
#include "geometry/object/object_instance.h"
#include "geometry/surface.h"
#include "geometry/matrix4.h"
#include "geometry/bound.h"

BEGIN_YAFARAY

Bound PrimitiveInstance::getBound(const Matrix4 *) const
{
	return base_primitive_->getBound(base_instance_.getObjToWorldMatrix());
}

bool PrimitiveInstance::intersectsBound(const ExBound &b, const Matrix4 *) const
{
	return base_primitive_->intersectsBound(b, base_instance_.getObjToWorldMatrix());
}

PolyDouble::ClipResultWithBound PrimitiveInstance::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 *obj_to_world) const
{
	return base_primitive_->clipToBound(logger, bound, clip_plane, poly, base_instance_.getObjToWorldMatrix());
}

IntersectData PrimitiveInstance::intersect(const Ray &ray, const Matrix4 *) const
{
	return base_primitive_->intersect(ray, base_instance_.getObjToWorldMatrix());
}

std::unique_ptr<const SurfacePoint> PrimitiveInstance::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const
{
	return base_primitive_->getSurface(ray_differentials, hit, intersect_data, base_instance_.getObjToWorldMatrix(), camera);
}

float PrimitiveInstance::surfaceArea(const Matrix4 *) const
{
	return base_primitive_->surfaceArea(base_instance_.getObjToWorldMatrix());
}

Vec3 PrimitiveInstance::getGeometricNormal(const Matrix4 *, float u, float v) const
{
	return base_primitive_->getGeometricNormal(base_instance_.getObjToWorldMatrix(), u, v);
}

std::pair<Point3, Vec3> PrimitiveInstance::sample(float s_1, float s_2, const Matrix4 *) const
{
	return base_primitive_->sample(s_1, s_2, base_instance_.getObjToWorldMatrix());
}

END_YAFARAY
