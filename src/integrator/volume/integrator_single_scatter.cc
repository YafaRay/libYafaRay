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

#include "integrator/volume/integrator_single_scatter.h"
#include "geometry/surface.h"
#include "common/logger.h"
#include "volume/volume.h"
#include "scene/scene.h"
#include "background/background.h"
#include "light/light.h"
#include "common/param.h"
#include "render/render_data.h"
#include "accelerator/accelerator.h"

BEGIN_YAFARAY

SingleScatterIntegrator::SingleScatterIntegrator(Logger &logger, float s_size, bool adapt, bool opt) : VolumeIntegrator(logger)
{
	adaptive_ = adapt;
	step_size_ = s_size;
	optimize_ = opt;
	adaptive_step_size_ = s_size * 100.0f;
	logger_.logParams("SingleScatter: stepSize: ", step_size_, " adaptive: ", adaptive_, " optimize: ", optimize_);
}

bool SingleScatterIntegrator::preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film)
{
	logger_.logInfo("SingleScatter: Preprocessing...");
	lights_ = render_view->getLightsVisible();
	const auto &volumes = scene_->getVolumeRegions();
	vr_size_ = volumes.size();
	i_vr_size_ = 1.f / (float)vr_size_;
	if(optimize_)
	{
		for(const auto &v : volumes)
		{
			const auto &vr = v.second;
			Bound bb = vr->getBb();

			int x_size = vr->att_grid_x_;
			int y_size = vr->att_grid_y_;
			int z_size = vr->att_grid_z_;

			float x_size_inv = 1.f / (float)x_size;
			float y_size_inv = 1.f / (float)y_size;
			float z_size_inv = 1.f / (float)z_size;

			logger_.logParams("SingleScatter: volume, attGridMaps with size: ", x_size, " ", y_size, " ", x_size);

			for(auto l = lights_.begin(); l != lights_.end(); ++l)
			{
				Rgb lcol(0.0);

				float *attenuation_grid = (float *)malloc(x_size * y_size * z_size * sizeof(float));
				vr->attenuation_grid_map_[(*l)] = attenuation_grid;

				for(int z = 0; z < z_size; ++z)
				{
					for(int y = 0; y < y_size; ++y)
					{
						for(int x = 0; x < x_size; ++x)
						{
							// generate the world position inside the grid
							Point3 p(bb.longX() * x_size_inv * x + bb.a_.x_,
									 bb.longY() * y_size_inv * y + bb.a_.y_,
									 bb.longZ() * z_size_inv * z + bb.a_.z_);

							SurfacePoint sp;
							sp.p_ = p;

							Ray light_ray;

							light_ray.from_ = sp.p_;

							// handle lights with delta distribution, e.g. point and directional lights
							if((*l)->diracLight())
							{
								bool ill = (*l)->illuminate(sp, lcol, light_ray);
								light_ray.tmin_ = scene_->shadow_bias_;
								if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10;  // infinitely distant light

								// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
								Rgb lightstep_tau(0.f);
								if(ill)
								{
									for(const auto &v_2 : volumes)
									{
										lightstep_tau += v_2.second->tau(light_ray, step_size_, 0.0f);
									}
								}

								float light_tr = math::exp(-lightstep_tau.energy());
								attenuation_grid[x + y * x_size + y_size * x_size * z] = light_tr;
							}
							else // area light and suchlike
							{
								float light_tr = 0;
								int n = (*l)->nSamples() >> 1; // samples / 2
								if(n < 1) n = 1;
								LSample ls;
								for(int i = 0; i < n; ++i)
								{
									ls.s_1_ = 0.5f; //(*state.random_generator)();
									ls.s_2_ = 0.5f; //(*state.random_generator)();

									(*l)->illumSample(sp, ls, light_ray);
									light_ray.tmin_ = scene_->shadow_bias_;
									if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10;  // infinitely distant light

									// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
									Rgb lightstep_tau(0.f);
									for(const auto &v_2 : volumes)
									{
										lightstep_tau += v_2.second->tau(light_ray, step_size_, 0.0f);
									}
									light_tr += math::exp(-lightstep_tau.energy());
								}

								attenuation_grid[x + y * x_size + y_size * x_size * z] = light_tr / (float)n;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

Rgb SingleScatterIntegrator::getInScatter(RandomGenerator &random_generator, const Ray &step_ray, float current_step) const
{
	const Accelerator *accelerator = scene_->getAccelerator();
	if(!accelerator) return {0.f};

	Rgb in_scatter(0.f);
	SurfacePoint sp;
	sp.p_ = step_ray.from_;
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	Ray light_ray;
	light_ray.from_ = sp.p_;
	const auto &volumes = scene_->getVolumeRegions();

	for(auto l = lights_.begin(); l != lights_.end(); ++l)
	{
		Rgb lcol(0.0);

		// handle lights with delta distribution, e.g. point and directional lights
		if((*l)->diracLight())
		{
			if((*l)->illuminate(sp, lcol, light_ray))
			{
				// ...shadowed...
				if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10;  // infinitely distant light
				bool shadowed = accelerator->isShadowed(light_ray, mask_obj_index, mask_mat_index, scene_->getShadowBias());
				if(!shadowed)
				{
					float light_tr = 0.0f;
					// replace lightTr with precalculated attenuation
					if(optimize_)
					{
						// replaced by
						for(const auto &v : volumes)
						{
							const Bound::Cross cross = v.second->crossBound(light_ray);
							if(cross.crossed_) light_tr += v.second->attenuation(sp.p_, (*l));
						}
					}
					else
					{
						// replaced by
						Rgb lightstep_tau(0.f);
						for(const auto &v : volumes)
						{
							const Bound::Cross cross = v.second->crossBound(light_ray);
							if(cross.crossed_) lightstep_tau += v.second->tau(light_ray, current_step, 0.f);
						}
						// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
						light_tr = math::exp(-lightstep_tau.energy());
					}
					in_scatter += light_tr * lcol;
				}
			}
		}
		else // area light and suchlike
		{
			int n = (*l)->nSamples() >> 2; // samples / 4
			if(n < 1) n = 1;
			float i_n = 1.f / (float)n; // inverse of n
			Rgb ccol(0.0);
			float light_tr = 0.0f;
			LSample ls;

			for(int i = 0; i < n; ++i)
			{
				// ...get sample val...
				ls.s_1_ = random_generator();
				ls.s_2_ = random_generator();

				if((*l)->illumSample(sp, ls, light_ray))
				{
					// ...shadowed...
					if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10;  // infinitely distant light
					bool shadowed = accelerator->isShadowed(light_ray, mask_obj_index, mask_mat_index, scene_->getShadowBias());
					if(!shadowed)
					{
						ccol += ls.col_ / ls.pdf_;

						// replace lightTr with precalculated attenuation
						if(optimize_)
						{
							// replaced by
							for(const auto &v : volumes)
							{
								const Bound::Cross cross = v.second->crossBound(light_ray);
								if(cross.crossed_)
								{
									light_tr += v.second->attenuation(sp.p_, (*l));
									break;
								}
							}
						}
						else
						{
							// replaced by
							Rgb lightstep_tau(0.f);
							for(const auto &v : volumes)
							{
								const Bound::Cross cross = v.second->crossBound(light_ray);
								if(cross.crossed_)
								{
									lightstep_tau += v.second->tau(light_ray, current_step * 4.f, 0.0f);
								}
							}
							// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
							light_tr += math::exp(-lightstep_tau.energy());
						}

					}
				}
			} // end of area light sample loop

			light_tr *= i_n;

			ccol = ccol * i_n;
			in_scatter += light_tr * ccol;
		} // end of area lights loop
	}

	return in_scatter;
}

Rgba SingleScatterIntegrator::transmittance(RandomGenerator &random_generator, const Ray &ray) const {
	if(vr_size_ == 0) return {1.f};
	Rgba tr(1.f);
	const auto &volumes = scene_->getVolumeRegions();
	for(const auto &v : volumes)
	{
		const Bound::Cross cross = v.second->crossBound(ray);
		if(cross.crossed_)
		{
			const float random_value = random_generator();
			const Rgb optical_thickness = v.second->tau(ray, step_size_, random_value);
			tr *= Rgba(math::exp(-optical_thickness.energy()));
		}
	}

	return tr;
}

Rgba SingleScatterIntegrator::integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const
{
	float t_0 = 1e10f, t_1 = -1e10f;

	Rgba result(0.f);
	//return result;
	if(vr_size_ == 0) return result;

	bool hit = (ray.tmax_ > 0.f);
	const auto &volumes = scene_->getVolumeRegions();

	// find min t0 and max t1
	for(const auto &v : volumes)
	{
		Bound::Cross cross = v.second->crossBound(ray);
		if(!cross.crossed_) continue;
		if(hit && ray.tmax_ < cross.enter_) continue;

		if(cross.enter_ < 0.f) cross.enter_ = 0.f;
		if(hit && ray.tmax_ < cross.leave_) cross.leave_ = ray.tmax_;
		if(cross.leave_ > t_1) t_1 = cross.leave_;
		if(cross.enter_ < t_0) t_0 = cross.enter_;
	}

	float dist = t_1 - t_0;
	if(dist < 1e-3f) return result;

	float pos;
	int samples;
	pos = t_0 - random_generator() * step_size_; // start position of ray marching
	dist = t_1 - pos;
	samples = dist / step_size_ + 1;

	std::vector<float> density_samples;
	std::vector<float> accum_density;
	int adaptive_resolution = 1;

	if(adaptive_)
	{
		adaptive_resolution = adaptive_step_size_ / step_size_;

		density_samples.resize(samples);
		accum_density.resize(samples);

		accum_density.at(0) = 0.f;
		for(int i = 0; i < samples; ++i)
		{
			Point3 p = ray.from_ + (step_size_ * i + pos) * ray.dir_;

			float density = 0;
			for(const auto &v : volumes)
			{
				density += v.second->sigmaT(p, Vec3()).energy();
			}

			density_samples.at(i) = density;
			if(i > 0)
				accum_density.at(i) = accum_density.at(i - 1) + density * step_size_;
		}
	}

	float adapt_thresh = .01f;
	bool adapt_now = false;
	float current_step = step_size_;
	int step_length = 1;
	int step_to_stop_adapt = -1;
	Rgb tr_tmp(1.f); // transmissivity during ray marching

	if(adaptive_)
	{
		current_step = adaptive_step_size_;
		step_length = adaptive_resolution;
	}

	Rgb step_tau(0.f);
	int lookahead_samples = adaptive_resolution / 10;

	for(int step_sample = 0; step_sample < samples; step_sample += step_length)
	{
		if(adaptive_)
		{
			if(!adapt_now)
			{
				int next_sample = (step_sample + adaptive_resolution > samples - 1) ? samples - 1 : step_sample + adaptive_resolution;
				if(std::abs(accum_density.at(step_sample) - accum_density.at(next_sample)) > adapt_thresh)
				{
					adapt_now = true;
					step_length = 1;
					step_to_stop_adapt = step_sample + lookahead_samples;
					current_step = step_size_;
				}
			}
		}

		Ray step_ray(ray.from_ + (ray.dir_ * pos), ray.dir_, 0, current_step, 0);

		if(adaptive_)
		{
			step_tau = accum_density.at(step_sample);
		}
		else
		{
			for(const auto &v : volumes)
			{
				const Bound::Cross cross = v.second->crossBound(step_ray);
				if(cross.crossed_)
				{
					step_tau += v.second->sigmaT(step_ray.from_, step_ray.dir_) * current_step;
				}
			}
		}

		tr_tmp = math::exp(-step_tau.energy());

		if(optimize_ && tr_tmp.energy() < 1e-3f)
		{
			float random_val = random_generator();
			if(random_val < 0.5f)
			{
				break;
			}
			tr_tmp = tr_tmp / random_val;
		}

		float sigma_s = 0.0f;
		for(const auto &v : volumes)
		{
			const Bound::Cross cross = v.second->crossBound(step_ray);
			if(cross.crossed_)
			{
				sigma_s += v.second->sigmaS(step_ray.from_, step_ray.dir_).energy();
			}
		}

		// with a sigma_s close to 0, no light can be scattered -> computation can be skipped

		if(optimize_ && sigma_s < 1e-3f)
		{
			float random_val = random_generator();
			if(random_val < 0.5f)
			{
				pos += current_step;
				continue;
			}
			sigma_s = sigma_s / random_val;
		}

		result += tr_tmp * getInScatter(random_generator, step_ray, current_step) * sigma_s * current_step;

		if(adaptive_)
		{
			if(adapt_now && step_sample >= step_to_stop_adapt)
			{
				int next_sample = (step_sample + adaptive_resolution > samples - 1) ? samples - 1 : step_sample + adaptive_resolution;
				if(std::abs(accum_density.at(step_sample) - accum_density.at(next_sample)) > adapt_thresh)
				{
					// continue moving slowly ahead until the discontinuity is found
					step_to_stop_adapt = step_sample + lookahead_samples;
				}
				else
				{
					adapt_now = false;
					step_length = adaptive_resolution;
					current_step = adaptive_step_size_;
				}
			}
		}

		pos += current_step;
	}
	result.a_ = 1.0f; // FIXME: get correct alpha value, does it even matter?
	return result;
}

std::unique_ptr<Integrator> SingleScatterIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	bool adapt = false;
	bool opt = false;
	float s_size = 1.f;
	params.getParam("stepSize", s_size);
	params.getParam("adaptive", adapt);
	params.getParam("optimize", opt);
	return std::unique_ptr<Integrator>(new SingleScatterIntegrator(logger, s_size, adapt, opt));
}

END_YAFARAY
