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

Rgba EmissionIntegrator::transmittance(RenderData &render_data, Ray &ray) const {
	Rgba result(1.f);
	auto volumes = scene_->getVolumeRegions();
	for(const auto &v : volumes) result *= v.second->tau(ray, 0, 0);
	result = Rgba(math::exp(-result.getR()), math::exp(-result.getG()), math::exp(-result.getB()));
	return result;
}

Rgba EmissionIntegrator::integrate(RenderData &render_data, Ray &ray, int additional_depth) const {
	float t_0 = 0.f, t_1 = 0.f;
	int n = 10; // samples + 1 on the ray inside the volume

	Rgba result(0.f);
	bool hit = ray.tmax_ > 0.f;
	auto volumes = scene_->getVolumeRegions();
	for(const auto &v : volumes)
	{
		if(!v.second->intersect(ray, t_0, t_1)) continue;
		if(hit && ray.tmax_ < t_0) continue;
		if(hit && ray.tmax_ < t_1) t_1 = ray.tmax_;
		float step = (t_1 - t_0) / (float)n; // length between two sample points
		--n;
		float pos = t_0 + 0.5 * step;
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

Integrator *EmissionIntegrator::factory(ParamMap &params, const Scene &scene) {
	EmissionIntegrator *inte = new EmissionIntegrator();
	return inte;
}

END_YAFARAY
