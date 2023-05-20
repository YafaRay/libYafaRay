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
	base_ids_.emplace_back(BaseId{object_id, BaseId::Type::Object});
}

void Instance::addInstance(size_t instance_id)
{
	base_ids_.emplace_back(BaseId{instance_id, BaseId::Type::Instance});
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

bool Instance::updatePrimitives(const Scene &scene)
{
	primitives_.clear();
	bool result{true};
	for(const auto &base_id : base_ids_)
	{
		if(base_id.base_id_type_ == BaseId::Type::Object)
		{
			const auto [object, object_result]{scene.getObject(base_id.id_)};
			if(!object) continue;
			const auto &primitives{object->getPrimitives()};
			for(const auto &primitive: primitives)
			{
				if(primitive) primitives_.emplace_back(std::make_unique<PrimitiveInstance>(*primitive, *this));
			}
		}
		else
		{
			const auto [instance, instance_result]{scene.getInstance(base_id.id_)};
			if(!instance || instance == this)
			{
				result = false;
				continue; //FIXME DAVID: do proper recursion error handling and better even, do proper graph management of dependencies!
			}
			const auto &primitives{instance->getPrimitives()};
			for(const auto &primitive: primitives)
			{
				if(primitive) primitives_.emplace_back(std::make_unique<PrimitiveInstance>(*primitive, *this));
			}
			//auto primitives{instance->getPrimitives()};
			//result.insert(result.end(),std::make_move_iterator(primitives.begin()), std::make_move_iterator(primitives.end())); //To append std::vector of std::unique_ptr if needed...
		}
	}
	return result;
}

std::string Instance::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, const Scene &scene) const
{
	const auto &objects{scene.getObjects()};
	std::stringstream ss;
	ss << std::string(indent_level, '\t') << "<instance>" << std::endl;
	for(const auto &base_id : base_ids_)
	{
		if(base_id.base_id_type_ == BaseId::Type::Object)
		{
			ss << std::string(indent_level + 1, '\t') << "<object name=\"" << objects.findNameFromId(base_id.id_).first << "\"/>" << std::endl;
		}
		else if(base_id.base_id_type_ == BaseId::Type::Instance)
		{
			ss << std::string(indent_level + 1, '\t') << "<instance id=\"" << base_id.id_ << "\"/>" << std::endl;
		}
	}
	for(const auto &time_step : time_steps_)
	{
		ss << std::string(indent_level + 1, '\t') << "<matrix time=\"" << time_step.time_ << "\" ";
		ss << time_step.obj_to_world_.exportToString(container_export_type);
		ss << "/>" << std::endl;
	}
	ss << std::string(indent_level, '\t') << "</instance>" << std::endl;
	return ss.str();
}

} //namespace yafaray
