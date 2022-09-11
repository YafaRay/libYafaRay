/****************************************************************************
 *      background_gradient.cc: a background using a simple color gradient
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

#include "background/background_gradient.h"
#include "param/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"
#include "geometry/vector.h"

namespace yafaray {

GradientBackground::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(horizon_color_);
	PARAM_LOAD(zenith_color_);
	zenith_ground_color_ = zenith_color_;
	horizon_ground_color_ = horizon_color_;
	PARAM_LOAD(horizon_ground_color_);
	PARAM_LOAD(zenith_ground_color_);
}

ParamMap GradientBackground::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(horizon_color_);
	PARAM_SAVE(zenith_color_);
	PARAM_SAVE(horizon_ground_color_);
	PARAM_SAVE(zenith_ground_color_);
	PARAM_SAVE_END;
}

ParamMap GradientBackground::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Background::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Background *, ParamError> GradientBackground::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto background = new GradientBackground(logger, param_error, param_map);
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<GradientBackground>(name, {"type"}));
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

GradientBackground::GradientBackground(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		Background{logger, param_error, param_map}, params_{param_error, param_map}, gzenith_{params_.zenith_ground_color_ * Background::params_.power_}, ghoriz_{params_.horizon_ground_color_ * Background::params_.power_}, szenith_{params_.zenith_color_ * Background::params_.power_}, shoriz_{params_.horizon_color_ * Background::params_.power_}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

Rgb GradientBackground::eval(const Vec3f &dir, bool use_ibl_blur) const
{
	float blend = dir[Axis::Z];
	Rgb color;
	if(blend >= 0.f) color = blend * szenith_ + (1.f - blend) * shoriz_;
	else
	{
		blend = -blend;
		color = blend * gzenith_ + (1.f - blend) * ghoriz_;
	}
	if(color.minimum() < 1e-6f) color = Rgb(1e-5f);
	return color;
}

} //namespace yafaray
