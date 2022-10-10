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

#include "volume/region/volume_sky.h"
#include "geometry/surface.h"
#include "texture/texture.h"
#include "param/param.h"

namespace yafaray {

SkyVolumeRegion::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
}

ParamMap SkyVolumeRegion::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap SkyVolumeRegion::getAsParamMap(bool only_non_default) const
{
	ParamMap result{VolumeRegion::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<VolumeRegion *, ParamError> SkyVolumeRegion::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new SkyVolumeRegion(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<SkyVolumeRegion>(name, {"type"}));
	return {result, param_error};
}

SkyVolumeRegion::SkyVolumeRegion(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		VolumeRegion{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	if(logger_.isVerbose()) logger_.logVerbose(getClassName() + ": Vol. [", s_ray_, ", ", s_mie_, ", ", l_e_, "]");
}

Rgb SkyVolumeRegion::sigmaA(const Point3f &p, const Vec3f &v) const
{
	return Rgb{0.f};
}

Rgb SkyVolumeRegion::sigmaS(const Point3f &p, const Vec3f &v) const
{
	//if (bBox.includes(p)) {
	return s_ray_ + s_mie_;
	//}
	//else
	//	return Rgb{0.f};
}

Rgb SkyVolumeRegion::tau(const Ray &ray, float step, float offset) const
{
	Bound<float>::Cross cross{crossBound(ray)};
	// ray doesn't hit the BB
	if(!cross.crossed_) return Rgb{0.f};
	if(ray.tmax_ < cross.enter_ && ray.tmax_ >= 0) return Rgb{0.f};
	if(ray.tmax_ < cross.leave_ && ray.tmax_ >= 0) cross.leave_ = ray.tmax_;
	// t0 < 0 means, ray.from is in the volume
	if(cross.enter_ < 0.f) cross.enter_ = 0.f;
	// distance travelled in the volume
	const float dist = cross.leave_ - cross.enter_;
	return (s_ray_ + s_mie_) * dist;
}

Rgb SkyVolumeRegion::emission(const Point3f &p, const Vec3f &v) const
{
	if(b_box_.includes(p))
	{
		return l_e_;
	}
	else
		return Rgb{0.f};
}

float SkyVolumeRegion::p(const Vec3f &w_l, const Vec3f &w_s) const
{
	return phaseRayleigh(w_l, w_s) + phaseMie(w_l, w_s);
}

float SkyVolumeRegion::phaseRayleigh(const Vec3f &w_l, const Vec3f &w_s) const
{
	float costheta = (w_l * w_s);
	return 3.f / (16.f * math::num_pi<>) * (1.f + costheta * costheta) * s_ray_.energy();
}

float SkyVolumeRegion::phaseMie(const Vec3f &w_l, const Vec3f &w_s) const
{
	float k = 1.55f * VolumeRegion::params_.g_ - .55f * VolumeRegion::params_.g_ * VolumeRegion::params_.g_ * VolumeRegion::params_.g_;
	float kcostheta = k * (w_l * w_s);
	return 1.f / (4.f * math::num_pi<>) * (1.f - k * k) / ((1.f - kcostheta) * (1.f - kcostheta)) * s_mie_.energy();
}

} //namespace yafaray