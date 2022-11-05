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

#include "geometry/matrix.h"
#include "geometry/object/object.h"
#include "geometry/instance.h"
#include "geometry/primitive/primitive_instance.h"
#include "scene/scene.h"

namespace yafaray {

void Instance::addObject(size_t object_id)
{
	base_ids_.emplace_back(BaseId{static_cast<int>(object_id), false});
}

void Instance::addInstance(size_t instance_id)
{
	//if(instance_id == id_) return; //FIXME DAVID: do proper error handling and better even, do proper graph management of dependencies!
	base_ids_.emplace_back(BaseId{static_cast<int>(instance_id), true});
}

std::vector<const PrimitiveInstance *> Instance::getPrimitives() const
{
	std::vector<const PrimitiveInstance *> result;
	for(const auto &primitive_instance : primitives_) result.emplace_back(primitive_instance.get());
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

void Instance::updatePrimitives(const Scene &scene)
{
	primitives_.clear();
	for(const auto &base_id : base_ids_)
	{
		if(base_id.is_instance_)
		{
			const auto [instance, instance_result]{scene.getInstance(base_id.id_)};
			if(!instance) continue;
			const auto &primitives{instance->getPrimitives()};
			for(const auto &primitive: primitives)
			{
				if(primitive) primitives_.emplace_back(std::make_unique<PrimitiveInstance>(*primitive, *this));
			}
			//auto primitives{instance->getPrimitives()};
			//result.insert(result.end(),std::make_move_iterator(primitives.begin()), std::make_move_iterator(primitives.end())); //To append std::vector of std::unique_ptr if needed...
		}
		else
		{
			const auto [object, object_result]{scene.getObject(base_id.id_)};
			if(!object) continue;
			const auto &primitives{object->getPrimitives()};
			for(const auto &primitive: primitives)
			{
				if(primitive) primitives_.emplace_back(std::make_unique<PrimitiveInstance>(*primitive, *this));
			}
		}
	}
}

} //namespace yafaray
