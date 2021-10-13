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

#include "geometry/object/object_instance.h"
#include "geometry/primitive/primitive_instance.h"
#include "geometry/matrix4.h"

BEGIN_YAFARAY

ObjectInstance::ObjectInstance(const Object &base_object, const Matrix4 &obj_to_world) : base_object_(base_object), obj_to_world_(new Matrix4(obj_to_world))
{
	const std::vector<const Primitive *> primitives = base_object_.getPrimitives();
	primitive_instances_.reserve(base_object.numPrimitives());
	for(const auto &primitive : primitives)
	{
		primitive_instances_.emplace_back(new PrimitiveInstance(primitive, *this));
	}
}

const std::vector<const Primitive *> ObjectInstance::getPrimitives() const
{
	std::vector<const Primitive *> result;
	for(const auto &primitive_instance : primitive_instances_) result.emplace_back(primitive_instance.get());
	return result;
}

/*void ObjectInstance::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	base_->sample(s_1, s_2, p, n);
}*/

END_YAFARAY
