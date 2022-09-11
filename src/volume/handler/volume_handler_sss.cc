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

#include "volume/handler/volume_handler_sss.h"
#include "scene/scene.h"
#include "material/material.h"
#include "param/param.h"
#include "sampler/sample.h"
#include "photon/photon_sample.h"
#include "geometry/ray.h"

namespace yafaray {

SssVolumeHandler::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(scatter_col_);
}

ParamMap SssVolumeHandler::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(scatter_col_);
	PARAM_SAVE_END;
}

ParamMap SssVolumeHandler::getAsParamMap(bool only_non_default) const
{
	ParamMap result{BeerVolumeHandler::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<VolumeHandler *, ParamError> SssVolumeHandler::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new SssVolumeHandler(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<SssVolumeHandler>(name, {"type"}));
	return {result, param_error};
}

SssVolumeHandler::SssVolumeHandler(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		BeerVolumeHandler{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

bool SssVolumeHandler::scatter(const Ray &ray, Ray &s_ray, PSample &s) const
{
	float dist = -BeerVolumeHandler::params_.absorption_dist_ * math::log(s.s_1_);
	if(dist >= ray.tmax_) return false;
	s_ray.from_ = ray.from_ + dist * ray.dir_;
	s_ray.dir_ = sample::sphere(s.s_2_, s.s_3_);
	s.color_ = params_.scatter_col_;
	return true;
}

} //namespace yafaray
