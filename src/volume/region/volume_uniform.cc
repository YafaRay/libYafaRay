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

#include "volume/region/volume_uniform.h"
#include "common/logger.h"
#include "geometry/surface.h"
#include "param/param.h"

namespace yafaray {

UniformVolumeRegion::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
}

ParamMap UniformVolumeRegion::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap UniformVolumeRegion::getAsParamMap(bool only_non_default) const
{
	ParamMap result{VolumeRegion::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<VolumeRegion *, ParamError> UniformVolumeRegion::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new UniformVolumeRegion(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<UniformVolumeRegion>(name, {"type"}));
	return {result, param_error};
}

UniformVolumeRegion::UniformVolumeRegion(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		VolumeRegion{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	if(logger_.isVerbose()) logger_.logVerbose(getClassName() + ": Vol.[", s_a_, ", ", s_s_, ", ", l_e_, ", ", b_box_.a_, ", ", b_box_.g_, ", ", VolumeRegion::params_.att_grid_scale_, "]");
}

Rgb UniformVolumeRegion::sigmaA(const Point3f &p, const Vec3f &v) const
{
	if(!have_s_a_) return Rgb{0.f};
	if(b_box_.includes(p))
	{
		return s_a_;
	}
	else
		return Rgb{0.f};

}

Rgb UniformVolumeRegion::sigmaS(const Point3f &p, const Vec3f &v) const
{
	if(!have_s_s_) return Rgb{0.f};
	if(b_box_.includes(p))
	{
		return s_s_;
	}
	else
		return Rgb{0.f};
}

Rgb UniformVolumeRegion::tau(const Ray &ray, float step, float offset) const
{
	Bound<float>::Cross cross{crossBound(ray)};
	if(!cross.crossed_) return Rgb{0.f};
	if(ray.tmax_ < cross.enter_ && ray.tmax_ >= 0) return Rgb{0.f};
	if(ray.tmax_ < cross.leave_ && ray.tmax_ >= 0) cross.leave_ = ray.tmax_;
	// t0 < 0 means, ray.from is in the volume
	if(cross.enter_ < 0.f) cross.enter_ = 0.f;
	// distance travelled in the volume
	const float dist = cross.leave_ - cross.enter_;
	return dist * (s_s_ + s_a_);
}

Rgb UniformVolumeRegion::emission(const Point3f &p, const Vec3f &v) const
{
	if(!have_l_e_) return Rgb{0.f};
	if(b_box_.includes(p))
	{
		return l_e_;
	}
	else
		return Rgb{0.f};
}

} //namespace yafaray
