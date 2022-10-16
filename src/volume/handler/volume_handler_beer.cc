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
#include "scene/scene.h"
#include "material/material.h"
#include "param/param.h"
#include "geometry/ray.h"

namespace yafaray {

BeerVolumeHandler::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(absorption_col_);
	PARAM_LOAD(absorption_dist_);
}

ParamMap BeerVolumeHandler::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(absorption_col_);
	PARAM_SAVE(absorption_dist_);
	PARAM_SAVE_END;
}

ParamMap BeerVolumeHandler::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<VolumeHandler>, ParamError> BeerVolumeHandler::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto volume_handler {std::make_unique<ThisClassType_t>(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	return {std::move(volume_handler), param_error};
}

BeerVolumeHandler::BeerVolumeHandler(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	const float maxlog = math::log(1e38f);
	sigma_a_.r_ = (params_.absorption_col_.r_ > 1e-38f) ? -math::log(params_.absorption_col_.r_) : maxlog;
	sigma_a_.g_ = (params_.absorption_col_.g_ > 1e-38f) ? -math::log(params_.absorption_col_.g_) : maxlog;
	sigma_a_.b_ = (params_.absorption_col_.b_ > 1e-38f) ? -math::log(params_.absorption_col_.b_) : maxlog;
	if(params_.absorption_dist_ != 0.f) sigma_a_ *= 1.f / params_.absorption_dist_;
}

Rgb BeerVolumeHandler::transmittance(const Ray &ray) const
{
	if(ray.tmax_ < 0.f || ray.tmax_ > 1e30f) return Rgb{0.f}; //infinity check...
	const float dist = ray.tmax_; // maybe substract ray.tmin...
	const Rgb be(-dist * sigma_a_);
	const Rgb col = Rgb(math::exp(be.getR()), math::exp(be.getG()), math::exp(be.getB()));
	return col;
}

bool BeerVolumeHandler::scatter(const Ray &ray, Ray &s_ray, PSample &s) const
{
	return false;
}

} //namespace yafaray
