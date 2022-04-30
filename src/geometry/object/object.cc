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
#include "common/param.h"
#include "common/logger.h"

BEGIN_YAFARAY

Object * Object::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("Object::factory");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	Object *object = nullptr;
	if(type == "mesh") object = MeshObject::factory(logger, scene, name, params);
	else if(type == "curve") object = CurveObject::factory(logger, scene, name, params);
	else if(type == "sphere")
	{
		auto primitive_object = new PrimitiveObject;
		primitive_object->setPrimitive(SpherePrimitive::factory(params, scene, *primitive_object));
		object = primitive_object;
	}
	object->setIndexAuto(scene.getObjectIndexAuto());
	return object;
}

END_YAFARAY
