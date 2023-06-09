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

#include "geometry/object/object_primitive.h"
#include "geometry/primitive/primitive.h"

namespace yafaray {

PrimitiveObject::PrimitiveObject(ParamResult &param_result, const ParamMap &param_map, const Items <Object> &objects, const Items<Material> &materials, const Items<Light> &lights) : Object{param_result, param_map, objects, materials, lights}
{
	//Empty
}

std::string PrimitiveObject::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<object>" << std::endl;
	ss << std::string(indent_level, '\t') << "<parameters name=\"" << getName() << "\">" << std::endl;
	ss << param_map.exportMap(indent_level + 1, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level, '\t') << "</parameters>" << std::endl;
	for(const auto primitive : getPrimitives()) ss << primitive->exportToString(indent_level, container_export_type, only_export_non_default_parameters);
	ss << std::string(indent_level, '\t') << "</object>" << std::endl;
	return ss.str();
}

} //namespace yafaray