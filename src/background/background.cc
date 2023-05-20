/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "background/background.h"
#include "background/background_darksky.h"
#include "background/background_gradient.h"
#include "background/background_texture.h"
#include "background/background_constant.h"
#include "background/background_sunsky.h"
#include "light/light.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> Background::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(power_);
	PARAM_META(ibl_);
	PARAM_META(ibl_samples_);
	PARAM_META(with_caustic_);
	PARAM_META(with_diffuse_);
	PARAM_META(cast_shadows_);
	return param_meta_map;
}

Background::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(power_);
	PARAM_LOAD(ibl_);
	PARAM_LOAD(ibl_samples_);
	PARAM_LOAD(with_caustic_);
	PARAM_LOAD(with_diffuse_);
	PARAM_LOAD(cast_shadows_);
}

ParamMap Background::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(power_);
	PARAM_SAVE(ibl_);
	PARAM_SAVE(ibl_samples_);
	PARAM_SAVE(with_caustic_);
	PARAM_SAVE(with_diffuse_);
	PARAM_SAVE(cast_shadows_);
	return param_map;
}

std::pair<std::unique_ptr<Background>, ParamResult> Background::factory(Logger &logger, const std::string &name, const ParamMap &param_map, const Items<Texture> &textures)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::DarkSky: return DarkSkyBackground::factory(logger, name, param_map);
		case Type::Gradient: return GradientBackground::factory(logger, name, param_map);
		case Type::SunSky: return SunSkyBackground::factory(logger, name, param_map);
		case Type::Texture: return TextureBackground::factory(logger, name, param_map, textures);
		case Type::Constant: return ConstantBackground::factory(logger, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

Background::Background(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : params_{param_result, param_map}, logger_{logger}
{
	//Empty
}

std::string Background::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<background>" << std::endl;
	ss << param_map.exportMap(indent_level + 1, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level, '\t') << "</background>" << std::endl;
	return ss.str();
}

} //namespace yafaray
