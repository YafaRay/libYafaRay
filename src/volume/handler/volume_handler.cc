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

#include "volume/handler/volume_handler_beer.h"
#include "volume/handler/volume_handler_sss.h"
#include "common/logger.h"
#include "param/param.h"
#include "volume/handler/volume_handler.h"


namespace yafaray {

std::map<std::string, const ParamMeta *> VolumeHandler::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	return param_meta_map;
}

VolumeHandler::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
}

ParamMap VolumeHandler::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	return param_map;
}

std::pair<std::unique_ptr<VolumeHandler>, ParamResult> VolumeHandler::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Beer: return BeerVolumeHandler::factory(logger, scene, name, param_map);
		case Type::Sss: return SssVolumeHandler::factory(logger, scene, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

VolumeHandler::VolumeHandler(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		params_{param_result, param_map}, logger_{logger}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

std::string VolumeHandler::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<volume_handler>" << std::endl;
	ss << param_map.exportMap(indent_level + 1, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level, '\t') << "</volume_handler>" << std::endl;
	return ss.str();
}

} //namespace yafaray

