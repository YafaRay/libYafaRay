/****************************************************************************
 *      background_constant.cc: a background using a constant color
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

#include "background/background_constant.h"
#include "param/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"

namespace yafaray {

ConstantBackground::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(color_);
}

ParamMap ConstantBackground::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(color_);
	PARAM_SAVE_END;
}

ParamMap ConstantBackground::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Background::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Background *, ParamError> ConstantBackground::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto background = new ConstantBackground(logger, param_error, param_map);
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<ConstantBackground>(name, {"type"}));
	if(background->Background::params_.ibl_)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = background->Background::params_.ibl_samples_;
		bgp["with_caustic"] = background->Background::params_.with_caustic_;
		bgp["with_diffuse"] = background->Background::params_.with_diffuse_;
		bgp["cast_shadows"] = background->Background::params_.cast_shadows_;

		std::unique_ptr<Light> bglight{Light::factory(logger, scene, "light", bgp).first};
		bglight->setBackground(background);
		background->addLight(std::move(bglight));
	}
	return {background, param_error};
}

ConstantBackground::ConstantBackground(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		Background{logger, param_error, param_map}, params_{param_error, param_map}, color_{params_.color_ * Background::params_.power_}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

Rgb ConstantBackground::eval(const Vec3f &dir, bool use_ibl_blur) const
{
	return color_;
}

} //namespace yafaray
