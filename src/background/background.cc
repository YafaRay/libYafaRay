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

Background::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(power_);
	PARAM_LOAD(ibl_);
	PARAM_LOAD(ibl_samples_);
	PARAM_LOAD(with_caustic_);
	PARAM_LOAD(with_diffuse_);
	PARAM_LOAD(cast_shadows_);
}

ParamMap Background::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(power_);
	PARAM_SAVE(ibl_);
	PARAM_SAVE(ibl_samples_);
	PARAM_SAVE(with_caustic_);
	PARAM_SAVE(with_diffuse_);
	PARAM_SAVE(cast_shadows_);
	PARAM_SAVE_END;
}

ParamMap Background::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<std::unique_ptr<Background>, ParamResult> Background::factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::DarkSky: return DarkSkyBackground::factory(logger, scene, name, param_map);
		case Type::Gradient: return GradientBackground::factory(logger, scene, name, param_map);
		case Type::SunSky: return SunSkyBackground::factory(logger, scene, name, param_map);
		case Type::Texture: return TextureBackground::factory(logger, scene, name, param_map);
		case Type::Constant: return ConstantBackground::factory(logger, scene, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

Background::Background(Logger &logger, ParamResult &param_result, Items<Light> &lights, const ParamMap &param_map) : params_{param_result, param_map}, lights_{lights}, logger_{logger}
{
	//Empty
}

Background::~Background() = default;

} //namespace yafaray
