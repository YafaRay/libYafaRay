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

#include "geometry/object/object_base.h"
#include "geometry/object/object_mesh.h"
#include "geometry/object/object_curve.h"
#include "geometry/object/object_primitive.h"
#include "geometry/primitive/primitive_sphere.h"
#include "scene/scene.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

ObjectBase::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(light_name_);
	PARAM_ENUM_LOAD(visibility_);
	PARAM_LOAD(is_base_object_);
	PARAM_LOAD(object_index_);
	PARAM_LOAD(motion_blur_bezier_);
	PARAM_LOAD(time_range_start_);
	PARAM_LOAD(time_range_end_);
}

ParamMap ObjectBase::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(light_name_);
	PARAM_ENUM_SAVE(visibility_);
	PARAM_SAVE(is_base_object_);
	PARAM_SAVE(object_index_);
	PARAM_SAVE(motion_blur_bezier_);
	PARAM_SAVE(time_range_start_);
	PARAM_SAVE(time_range_end_);
	PARAM_SAVE_END;
}

ParamMap ObjectBase::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<std::unique_ptr<Object>, ParamError> ObjectBase::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	std::pair<std::unique_ptr<Object>, ParamError> result {nullptr, ParamError{ResultFlags::ErrorWhileCreating}};
	switch(type.value())
	{
		case Type::Mesh:
			result = MeshObject::factory(logger, scene, name, param_map);
			break;
		case Type::Curve:
			result = CurveObject::factory(logger, scene, name, param_map);
			break;
		case Type::Sphere:
		{
			//FIXME DAVID: probably will need to work on a better parameter error handling considering that both an object and a primitive are involved and the check needs to be done for both
			ParamError param_error;
			auto primitive_object{std::make_unique<PrimitiveObject>(param_error, param_map, scene.getMaterials())};
			auto [primitive, primitive_param_error]{SpherePrimitive::factory(logger, scene, name, param_map, *primitive_object)};
			param_error.merge(primitive_param_error);
			primitive_object->setPrimitive(std::move(primitive));
			result = {std::move(primitive_object), param_error};
			break;
		}
	}
	if(result.first)
	{
		result.first->setIndexAuto(scene.getObjectIndexAuto());
		return result;
	}
	else return {nullptr, ParamError{ResultFlags::ErrorTypeUnknownParam}};
}

ObjectBase::ObjectBase(ParamError &param_error, const ParamMap &param_map, const SceneItems<Material> &materials) : params_{param_error, param_map}, materials_{materials}
{
	//if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void ObjectBase::setIndexAuto(unsigned int new_obj_index)
{
	index_auto_ = new_obj_index;
	srand(index_auto_);
	float r, g, b;
	do
	{
		r = static_cast<float>(rand() % 8) / 8.f;
		g = static_cast<float>(rand() % 8) / 8.f;
		b = static_cast<float>(rand() % 8) / 8.f;
	}
	while(r + g + b < 0.5f);
	index_auto_color_ = Rgb(r, g, b);
}

} //namespace yafaray
