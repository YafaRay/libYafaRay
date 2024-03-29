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

#include "geometry/object/object.h"
#include "geometry/object/object_mesh.h"
#include "geometry/object/object_curve.h"
#include "geometry/object/object_primitive.h"
#include "geometry/primitive/primitive_sphere.h"
#include "scene/scene.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> Object::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(light_name_);
	PARAM_META(visibility_);
	PARAM_META(is_base_object_);
	PARAM_META(object_index_);
	PARAM_META(motion_blur_bezier_);
	PARAM_META(time_range_start_);
	PARAM_META(time_range_end_);
	return param_meta_map;
}

Object::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(light_name_);
	PARAM_ENUM_LOAD(visibility_);
	PARAM_LOAD(is_base_object_);
	PARAM_LOAD(object_index_);
	PARAM_LOAD(motion_blur_bezier_);
	PARAM_LOAD(time_range_start_);
	PARAM_LOAD(time_range_end_);
}

ParamMap Object::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	param_map.setParam(Params::light_name_meta_, lights_.findNameFromId(light_id_).first);
	PARAM_ENUM_SAVE(visibility_);
	PARAM_SAVE(is_base_object_);
	PARAM_SAVE(object_index_);
	PARAM_SAVE(motion_blur_bezier_);
	PARAM_SAVE(time_range_start_);
	PARAM_SAVE(time_range_end_);
	return param_map;
}

std::pair<std::unique_ptr<Object>, ParamResult> Object::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	std::pair<std::unique_ptr<Object>, ParamResult> result {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
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
			ParamResult param_result;
			auto primitive_object{std::make_unique<PrimitiveObject>(param_result, param_map, scene.getObjects(), scene.getMaterials(), scene.getLights())};
			auto [primitive, primitive_param_result]{SpherePrimitive::factory(logger, scene, name, param_map, *primitive_object)};
			param_result.merge(primitive_param_result);
			primitive_object->setPrimitive(std::move(primitive));
			result = {std::move(primitive_object), param_result};
			break;
		}
	}
	if(result.first) return result;
	else return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_TYPE_UNKNOWN_PARAM}};
}

Object::Object(ParamResult &param_result, const ParamMap &param_map, const Items <Object> &objects, const Items<Material> &materials, const Items<Light> &lights) : params_{param_result, param_map}, objects_{objects}, materials_{materials}, lights_{lights}
{
	//if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

void Object::setId(size_t id)
{
	id_ = id;
	index_auto_color_ = Rgb::pseudoRandomDistinctFromIndex(id + 1);
}

std::string Object::getName() const
{
	return objects_.findNameFromId(id_).first;
}

} //namespace yafaray
