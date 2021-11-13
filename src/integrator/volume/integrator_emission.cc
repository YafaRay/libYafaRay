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
#include "volume/volume.h"
#include "scene/scene.h"
#include "material/material.h"
#include "background/background.h"
#include "light/light.h"

BEGIN_YAFARAY

Rgba EmissionIntegrator::transmittance(RandomGenerator &random_generator, const Ray &ray) const
{
	Rgba result(1.f);
	auto &volumes = scene_->getVolumeRegions();
	for(const auto &v : volumes) result *= v.second.get()->tau(ray, 0, 0);
	result = Rgba(math::exp(-result.getR()), math::exp(-result.getG()), math::exp(-result.getB()));
	return result;
}

Rgba EmissionIntegrator::integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const {
	int n = 10; // samples + 1 on the ray inside the volume
	Rgba result(0.f);
	bool hit = ray.tmax_ > 0.f;
	auto &volumes = scene_->getVolumeRegions();
	for(const auto &v : volumes)
	{
		Bound::Cross cross = v.second.get()->crossBound(ray);
		if(!cross.crossed_) continue;
		if(hit && ray.tmax_ < cross.enter_) continue;
		if(hit && ray.tmax_ < cross.leave_) cross.leave_ = ray.tmax_;
		float step = (cross.leave_ - cross.enter_) / static_cast<float>(n); // length between two sample points
		--n;
		float pos = cross.enter_ + 0.5 * step;
		Rgb tr(1.f);
		for(int i = 0; i < n; ++i)
		{
			Ray step_ray(ray.from_ + (ray.dir_ * pos), ray.dir_, 0, step, 0);
			Rgb step_tau = v.second->tau(step_ray, 0, 0);
			tr *= Rgba(math::exp(-step_tau.getR()), math::exp(-step_tau.getG()), math::exp(-step_tau.getB()));
			result += tr * v.second->emission(step_ray.from_, step_ray.dir_);
			pos += step;
		}
		result *= step;
	}
	return result;
}

std::unique_ptr<Integrator> EmissionIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	return std::unique_ptr<Integrator>(new EmissionIntegrator(logger));
}

END_YAFARAY
