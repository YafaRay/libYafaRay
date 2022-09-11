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

namespace yafaray {

VolumeHandler::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
}

ParamMap VolumeHandler::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap VolumeHandler::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<VolumeHandler *, ParamError> VolumeHandler::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Beer: return BeerVolumeHandler::factory(logger, scene, name, param_map);
		case Type::Sss: return SssVolumeHandler::factory(logger, scene, name, param_map);
		default: return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	}
}

VolumeHandler::VolumeHandler(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		params_{param_error, param_map}, logger_{logger}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

} //namespace yafaray

