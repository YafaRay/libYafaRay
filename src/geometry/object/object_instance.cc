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

#include "common/visibility.h"
#include "geometry/matrix.h"
#include "geometry/instance.h"
#include "geometry/primitive/primitive_instance.h"

namespace yafaray {

void Instance::addPrimitives(const std::vector<const Primitive *> &base_primitives)
{
	primitive_instances_.reserve(base_primitives.size());
	for(const auto &primitive : base_primitives)
	{
		if(primitive) primitive_instances_.emplace_back(std::make_unique<PrimitiveInstance>(*primitive, *this));
	}
}

std::vector<const Primitive *> Instance::getPrimitives() const
{
	std::vector<const Primitive *> result;
	for(const auto &primitive_instance : primitive_instances_) result.emplace_back(primitive_instance.get());
	return result;
}

std::vector<const Matrix4f *> Instance::getObjToWorldMatrices() const
{
	std::vector<const Matrix4f *> result;
	for(const auto &time_step : time_steps_)
	{
		result.emplace_back(&time_step.obj_to_world_);
	}
	return result;
}

} //namespace yafaray
