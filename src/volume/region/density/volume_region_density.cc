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

#include "volume/region/density/volume_region_density.h"
#include "common/logger.h"
#include "param/param.h"

namespace yafaray {

DensityVolumeRegion::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
}

ParamMap DensityVolumeRegion::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap DensityVolumeRegion::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

DensityVolumeRegion::DensityVolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

Rgb DensityVolumeRegion::tau(const Ray &ray, float step_size, float offset) const
{
	Bound<float>::Cross cross{crossBound(ray)};
	// ray doesn't hit the BB
	if(!cross.crossed_) return Rgb{0.f};
	if(ray.tmax_ < cross.enter_ && ray.tmax_ >= 0.f) return Rgb{0.f};
	if(ray.tmax_ < cross.leave_ && ray.tmax_ >= 0.f) cross.leave_ = ray.tmax_;
	if(cross.enter_ < 0.f) cross.enter_ = 0.f;

	// distance travelled in the volume
	float step = step_size; // length between two sample points along the ray
	float pos = cross.enter_ + offset * step;
	Rgb tau_val(0.f);
	Rgb tau_prev(0.f);
	bool adaptive = false; //FIXME: unused, always false??
	while(pos < cross.leave_)
	{
		Rgb tau_tmp = sigmaT(ray.from_ + (ray.dir_ * pos), ray.dir_);
		if(adaptive) //FIXME: unused, always false??
		{
			float epsilon = 0.01f;
			if(std::abs(tau_tmp.energy() - tau_prev.energy()) > epsilon && step > step_size / 50.f)
			{
				pos -= step;
				step /= 2.f;
			}
			else
			{
				tau_val += tau_tmp * step;
				tau_prev = tau_tmp;
			}
		}
		else
		{
			tau_val += tau_tmp * step;
		}
		pos += step;
	}
	return tau_val;
}

} //namespace yafaray

