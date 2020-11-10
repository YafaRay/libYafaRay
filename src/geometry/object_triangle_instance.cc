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

#include "geometry/object_triangle_instance.h"
#include "geometry/triangle_instance.h"
#include "geometry/uv.h"

BEGIN_YAFARAY

TriangleObjectInstance::TriangleObjectInstance(const TriangleObject *base, Matrix4 obj_2_world)
{
	obj_to_world_ = obj_2_world;
	triangle_object_ = base;
	visible_ = true;
	is_base_object_ = false;
	triangle_instances_.reserve(triangle_object_->getTriangles().size());
	for(const auto &triangle : triangle_object_->getTriangles())
	{
		triangle_instances_.push_back(TriangleInstance(&triangle, this));
	}
}

int TriangleObjectInstance::getPrimitives(const Triangle **prims) const
{
	for(size_t i = 0; i < triangle_instances_.size(); i++) prims[i] = &triangle_instances_[i];
	return triangle_instances_.size();
}

void TriangleObjectInstance::finish()
{
	// Empty
}

END_YAFARAY