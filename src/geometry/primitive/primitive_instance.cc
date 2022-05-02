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

std::unique_ptr<const SurfacePoint> PrimitiveInstance::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const
{
	return base_primitive_->getSurface(ray_differentials, hit, intersect_data, base_instance_.getObjToWorldMatrix(), camera);
}

END_YAFARAY
