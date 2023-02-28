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

std::map<std::string, const ParamMeta *> SssVolumeHandler::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(scatter_col_);
	return param_meta_map;
}

SssVolumeHandler::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(scatter_col_);
}

ParamMap SssVolumeHandler::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map{BeerVolumeHandler::getAsParamMap(only_non_default)};
	PARAM_SAVE(scatter_col_);
	return param_map;
}

std::pair<std::unique_ptr<VolumeHandler>, ParamResult> SssVolumeHandler::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto volume_handler {std::make_unique<SssVolumeHandler>(logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(volume_handler), param_result};
}

SssVolumeHandler::SssVolumeHandler(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

bool SssVolumeHandler::scatter(const Ray &ray, Ray &s_ray, PSample &s) const
{
	float dist = -ParentClassType_t::params_.absorption_dist_ * math::log(s.s_1_);
	if(dist >= ray.tmax_) return false;
	s_ray.from_ = ray.from_ + dist * ray.dir_;
	s_ray.dir_ = sample::sphere(s.s_2_, s.s_3_);
	s.color_ = params_.scatter_col_;
	return true;
}

} //namespace yafaray
