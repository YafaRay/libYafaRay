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

EmissionIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
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

std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> EmissionIntegrator::factory(Logger &logger, const ParamMap &param_map, const Items<VolumeRegion> &volume_regions)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto integrator {std::make_unique<ThisClassType_t>(logger, param_result, param_map, volume_regions)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {std::move(integrator), param_result};
}

EmissionIntegrator::EmissionIntegrator(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items<VolumeRegion> &volume_regions) : VolumeIntegrator(logger, param_result, param_map), params_{param_result, param_map}, volume_regions_{volume_regions}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	//render_info_ += getClassName() + ": '" + params_.debug_type_.print() + "' | ";
}

Rgb EmissionIntegrator::transmittance(RandomGenerator &random_generator, const Ray &ray) const
{
	Rgb result {1.f};
	for(const auto &vr : volume_regions_) result *= vr.item_->tau(ray, 0, 0);
	return Rgb{math::exp(-result.getR()), math::exp(-result.getG()), math::exp(-result.getB())};
}

Rgb EmissionIntegrator::integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const
{
	int n = 10; // samples + 1 on the ray inside the volume
	const bool hit = ray.tmax_ > 0.f;
	Rgb result {0.f};
	for(const auto &vr : volume_regions_)
	{
		Bound<float>::Cross cross{vr.item_->crossBound(ray)};
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
			const Rgb step_tau = vr.item_->tau(step_ray, 0, 0);
			tr *= Rgb(math::exp(-step_tau.getR()), math::exp(-step_tau.getG()), math::exp(-step_tau.getB()));
			result += tr * vr.item_->emission(step_ray.from_, step_ray.dir_);
			pos += step;
		}
		result *= step;
	}
	return result;
}

} //namespace yafaray
