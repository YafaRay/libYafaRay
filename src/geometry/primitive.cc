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

#include "geometry/primitive.h"
#include "geometry/object.h"
#include "geometry/surface.h"
#include "scene/yafaray/primitive_triangle.h"
#include "scene/yafaray/primitive_sphere.h"
#include "geometry/bound.h"
#include "common/logger.h"
#include "common/param.h"

BEGIN_YAFARAY

Primitive *Primitive::factory(ParamMap &params, const Scene &scene)
{
	Y_DEBUG PRTEXT(**Primitive) PREND; params.printDebug();
	std::string type;
	params.getParam("type", type);
	if(type == "triangle") return TrianglePrimitive::factory(params, scene);
	else if(type == "sphere") return SpherePrimitive::factory(params, scene);
	else return nullptr;
}

Visibility Primitive::getVisibility() const
{
	return base_object_->getVisibility();
}

SurfacePoint Primitive::getSurface(const Point3 &hit, const IntersectData &data, const Matrix4 *obj_to_world) const
{
	return {};
}

IntersectData Primitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	return {};
}

END_YAFARAY
