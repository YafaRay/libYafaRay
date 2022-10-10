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

#include "integrator/volume/integrator_emission.h"
#include "scene/scene.h"
#include "material/material.h"
#include "background/background.h"
#include "light/light.h"
#include "volume/handler/volume_handler.h"
#include "volume/region/volume_region.h"

namespace yafaray {

EmissionIntegrator::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
}

ParamMap EmissionIntegrator::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap EmissionIntegrator::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<VolumeIntegrator>, ParamError> EmissionIntegrator::factory(Logger &logger, const ParamMap &param_map, const Scene &scene)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto integrator {std::make_unique<ThisClassType_t>(logger, param_error, param_map, scene.getVolumeRegions())};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<ThisClassType_t>(getClassName(), {"type"}));
	return {std::move(integrator), param_error};
}

EmissionIntegrator::EmissionIntegrator(Logger &logger, ParamError &param_error, const ParamMap &param_map, const std::map<std::string, std::unique_ptr<VolumeRegion>> &volume_regions) : VolumeIntegrator(logger, param_error, param_map), volume_regions_{volume_regions}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	//render_info_ += getClassName() + ": '" + params_.debug_type_.print() + "' | ";
}

Rgb EmissionIntegrator::transmittance(RandomGenerator &random_generator, const Ray &ray) const
{
	Rgb result {1.f};
	for(const auto &[vr_name, vr] : volume_regions_) result *= vr->tau(ray, 0, 0);
	return Rgb{math::exp(-result.getR()), math::exp(-result.getG()), math::exp(-result.getB())};
}

Rgb EmissionIntegrator::integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const
{
	int n = 10; // samples + 1 on the ray inside the volume
	const bool hit = ray.tmax_ > 0.f;
	Rgb result {0.f};
	for(const auto &[vr_name, vr] : volume_regions_)
	{
		Bound<float>::Cross cross{vr->crossBound(ray)};
		if(!cross.crossed_) continue;
		if(hit && ray.tmax_ < cross.enter_) continue;
		if(hit && ray.tmax_ < cross.leave_) cross.leave_ = ray.tmax_;
		const float step = (cross.leave_ - cross.enter_) / static_cast<float>(n); // length between two sample points
		--n;
		float pos = cross.enter_ + 0.5f * step;
		Rgb tr(1.f);
		for(int i = 0; i < n; ++i)
		{
			const Ray step_ray{ray.from_ + (ray.dir_ * pos), ray.dir_, ray.time_, 0, step};
			const Rgb step_tau = vr->tau(step_ray, 0, 0);
			tr *= Rgb(math::exp(-step_tau.getR()), math::exp(-step_tau.getG()), math::exp(-step_tau.getB()));
			result += tr * vr->emission(step_ray.from_, step_ray.dir_);
			pos += step;
		}
		result *= step;
	}
	return result;
}

} //namespace yafaray
