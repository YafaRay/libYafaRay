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

#include "volume/region/density/volume_exp_density.h"
#include "common/logger.h"
#include "param/param.h"

namespace yafaray {

ExpDensityVolumeRegion::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(a_);
	PARAM_LOAD(b_);
}

ParamMap ExpDensityVolumeRegion::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(a_);
	PARAM_SAVE(b_);
	PARAM_SAVE_END;
}

ParamMap ExpDensityVolumeRegion::getAsParamMap(bool only_non_default) const
{
	ParamMap result{DensityVolumeRegion::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<VolumeRegion *, ParamError> ExpDensityVolumeRegion::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new ExpDensityVolumeRegion(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<ExpDensityVolumeRegion>(name, {"type"}));
	return {result, param_error};
}

ExpDensityVolumeRegion::ExpDensityVolumeRegion(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		DensityVolumeRegion{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	if(logger_.isVerbose()) logger_.logVerbose(getClassName() + " vol: ", s_a_, " ", s_s_, " ", l_e_, " ", params_.a_, " ", params_.b_);
}

float ExpDensityVolumeRegion::density(const Point3f &p) const
{
	float height = p[Axis::Z] - b_box_.a_[Axis::Z];
	return params_.a_ * math::exp(-params_.b_ * height);
}

} //namespace yafaray
