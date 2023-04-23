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
#include "geometry/vector.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> GradientBackground::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(horizon_color_);
	PARAM_META(zenith_color_);
	PARAM_META(horizon_ground_color_);
	PARAM_META(zenith_ground_color_);
	return param_meta_map;
}

GradientBackground::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(horizon_color_);
	PARAM_LOAD(zenith_color_);
	zenith_ground_color_ = zenith_color_;
	horizon_ground_color_ = horizon_color_;
	PARAM_LOAD(horizon_ground_color_);
	PARAM_LOAD(zenith_ground_color_);
}

ParamMap GradientBackground::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(horizon_color_);
	PARAM_SAVE(zenith_color_);
	PARAM_SAVE(horizon_ground_color_);
	PARAM_SAVE(zenith_ground_color_);
	return param_map;
}

std::pair<std::unique_ptr<Background>, ParamResult> GradientBackground::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto background{std::make_unique<GradientBackground>(logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(background), param_result};
}

GradientBackground::GradientBackground(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
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

std::vector<std::pair<std::string, ParamMap>> GradientBackground::getRequestedIblLights() const
{
	if(ParentClassType_t::params_.ibl_)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = ParentClassType_t::params_.ibl_samples_;
		bgp["with_caustic"] = ParentClassType_t::params_.with_caustic_;
		bgp["with_diffuse"] = ParentClassType_t::params_.with_diffuse_;
		bgp["cast_shadows"] = ParentClassType_t::params_.cast_shadows_;
		return {{ThisClassType_t::lightName(), std::move(bgp)}};
	}
	else return {};
}

} //namespace yafaray
