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
#include "scene/scene.h"
#include "background/background.h"
#include "light/light.h"
#include "param/param.h"
#include "accelerator/accelerator.h"
#include "volume/region/volume_region.h"
#include "integrator/surface/integrator_surface.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> SingleScatterIntegrator::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(step_size_);
	PARAM_META(adaptive_);
	PARAM_META(optimize_);
	return param_meta_map;
}

SingleScatterIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(step_size_);
	PARAM_LOAD(adaptive_);
	PARAM_LOAD(optimize_);
}

ParamMap SingleScatterIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(step_size_);
	PARAM_SAVE(adaptive_);
	PARAM_SAVE(optimize_);
	return param_map;
}

std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> SingleScatterIntegrator::factory(Logger &logger, const ParamMap &param_map, const Items<VolumeRegion> &volume_regions)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto integrator {std::make_unique<SingleScatterIntegrator>(logger, param_result, param_map, volume_regions)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {std::move(integrator), param_result};
}

SingleScatterIntegrator::SingleScatterIntegrator(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items<VolumeRegion> &volume_regions) : VolumeIntegrator(logger, param_result, param_map), params_{param_result, param_map}, volume_regions_{volume_regions}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	logger_.logParams("SingleScatter: stepSize: ", params_.step_size_, " adaptive: ", params_.adaptive_, " optimize: ", params_.optimize_);
}

bool SingleScatterIntegrator::preprocess(const Scene &scene, const SurfaceIntegrator &surf_integrator)
{
	accelerator_ = scene.getAccelerator();
	if(!accelerator_) return false;
	logger_.logInfo("SingleScatter: Preprocessing...");
	lights_ = surf_integrator.getLights();
	if(params_.optimize_)
	{
		for(const auto &vr : volume_regions_)
		{
			const Bound bb{vr.item_->getBb()};
			const int x_size = vr.item_->att_grid_x_;
			const int y_size = vr.item_->att_grid_y_;
			const int z_size = vr.item_->att_grid_z_;
			const float x_size_inv = 1.f / (float)x_size;
			const float y_size_inv = 1.f / (float)y_size;
			const float z_size_inv = 1.f / (float)z_size;

			logger_.logParams("SingleScatter: volume, attGridMaps with size: ", x_size, " ", y_size, " ", x_size);

			for(const auto &light : lights_)
			{
				auto *attenuation_grid = static_cast<float *>(malloc(x_size * y_size * z_size * sizeof(float)));
				vr.item_->attenuation_grid_map_[light] = attenuation_grid;
				for(int z = 0; z < z_size; ++z)
				{
					for(int y = 0; y < y_size; ++y)
					{
						for(int x = 0; x < x_size; ++x)
						{
							// generate the world position inside the grid
							const Point3f p{{bb.length(Axis::X) * x_size_inv * x + bb.a_[Axis::X],
									 bb.length(Axis::Y) * y_size_inv * y + bb.a_[Axis::Y],
									 bb.length(Axis::Z) * z_size_inv * z + bb.a_[Axis::Z]}};

							// handle lights with delta distribution, e.g. point and directional lights
							if(light->diracLight())
							{
								auto[hit, light_ray, lcol]{light->illuminate(p, 0.f)}; //FIXME: what time to use?
								light_ray.tmin_ = surf_integrator.getShadowBias();
								if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10f;  // infinitely distant light

								// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
								Rgb lightstep_tau{0.f};
								if(hit)
								{
									for(const auto &v_2 : volume_regions_)
									{
										lightstep_tau += v_2.item_->tau(light_ray, params_.step_size_, 0.0f);
									}
								}

								const float light_tr = math::exp(-lightstep_tau.energy());
								attenuation_grid[x + y * x_size + y_size * x_size * z] = light_tr;
							}
							else // area light and suchlike
							{
								float light_tr = 0;
								int n = light->nSamples() >> 1; // samples / 2
								if(n < 1) n = 1;
								LSample ls;
								for(int i = 0; i < n; ++i)
								{
									ls.s_1_ = 0.5f; //(*state.random_generator)();
									ls.s_2_ = 0.5f; //(*state.random_generator)();

									auto[hit, light_ray]{light->illumSample(p, ls, 0.f)}; //FIXME: what time to use?
									light_ray.tmin_ = surf_integrator.getShadowBias();
									if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10f;  // infinitely distant light

									// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
									Rgb lightstep_tau{0.f};
									for(const auto &v_2 : volume_regions_)
									{
										lightstep_tau += v_2.item_->tau(light_ray, params_.step_size_, 0.0f);
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
	Rgb in_scatter(0.f);
	for(const auto &light : lights_)
	{
		// handle lights with delta distribution, e.g. point and directional lights
		if(light->diracLight())
		{
			if(auto[hit, light_ray, lcol]{light->illuminate(step_ray.from_, step_ray.time_)}; hit)
			{
				// ...shadowed...
				if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10;  // infinitely distant light
				const auto [is_shadowed, primitive] = accelerator_->isShadowed(light_ray);
				if(!is_shadowed)
				{
					float light_tr = 0.0f;
					// replace lightTr with precalculated attenuation
					if(params_.optimize_)
					{
						// replaced by
						for(const auto &vr : volume_regions_)
						{
							const Bound<float>::Cross cross{vr.item_->crossBound(light_ray)};
							if(cross.crossed_) light_tr += vr.item_->attenuation(step_ray.from_, light);
						}
					}
					else
					{
						// replaced by
						Rgb lightstep_tau(0.f);
						for(const auto &vr : volume_regions_)
						{
							const Bound<float>::Cross cross{vr.item_->crossBound(light_ray)};
							if(cross.crossed_) lightstep_tau += vr.item_->tau(light_ray, current_step, 0.f);
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
			int n = light->nSamples() >> 2; // samples / 4
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

				if(auto[hit, light_ray]{light->illumSample(step_ray.from_, ls, step_ray.time_)}; hit)
				{
					// ...shadowed...
					if(light_ray.tmax_ < 0.f) light_ray.tmax_ = 1e10;  // infinitely distant light
					const auto [is_shadowed, primitive] = accelerator_->isShadowed(light_ray);
					if(!is_shadowed)
					{
						ccol += ls.col_ / ls.pdf_;

						// replace lightTr with precalculated attenuation
						if(params_.optimize_)
						{
							// replaced by
							for(const auto &vr : volume_regions_)
							{
								const Bound<float>::Cross cross{vr.item_->crossBound(light_ray)};
								if(cross.crossed_)
								{
									light_tr += vr.item_->attenuation(step_ray.from_, light);
									break;
								}
							}
						}
						else
						{
							// replaced by
							Rgb lightstep_tau(0.f);
							for(const auto &vr : volume_regions_)
							{
								const Bound<float>::Cross cross{vr.item_->crossBound(light_ray)};
								if(cross.crossed_)
								{
									lightstep_tau += vr.item_->tau(light_ray, current_step * 4.f, 0.0f);
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

Rgb SingleScatterIntegrator::transmittance(RandomGenerator &random_generator, const Ray &ray) const
{
	if(volume_regions_.empty()) return Rgb{1.f};
	Rgb tr{1.f};
	for(const auto &vr : volume_regions_)
	{
		const Bound<float>::Cross cross{vr.item_->crossBound(ray)};
		if(cross.crossed_)
		{
			const float random_value = random_generator();
			const Rgb optical_thickness = vr.item_->tau(ray, params_.step_size_, random_value);
			tr *= Rgb{math::exp(-optical_thickness.energy())};
		}
	}
	return tr;
}

Rgb SingleScatterIntegrator::integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const
{
	if(volume_regions_.empty()) return Rgb{0.f};
	const bool hit = (ray.tmax_ > 0.f);
	float t_0 = 1e10f, t_1 = -1e10f;
	// find min t0 and max t1
	for(const auto &vr : volume_regions_)
	{
		Bound<float>::Cross cross{vr.item_->crossBound(ray)};
		if(!cross.crossed_) continue;
		if(hit && ray.tmax_ < cross.enter_) continue;
		if(cross.enter_ < 0.f) cross.enter_ = 0.f;
		if(hit && ray.tmax_ < cross.leave_) cross.leave_ = ray.tmax_;
		if(cross.leave_ > t_1) t_1 = cross.leave_;
		if(cross.enter_ < t_0) t_0 = cross.enter_;
	}
	float dist = t_1 - t_0;
	if(dist < 1e-3f) return Rgb{0.f};

	float pos = t_0 - random_generator() * params_.step_size_; // start position of ray marching
	dist = t_1 - pos;
	const int samples = dist / params_.step_size_ + 1;
	std::vector<float> density_samples;
	std::vector<float> accum_density;
	int adaptive_resolution = 1;
	if(params_.adaptive_)
	{
		adaptive_resolution = adaptive_step_size_ / params_.step_size_;
		density_samples.resize(samples);
		accum_density.resize(samples);
		accum_density.at(0) = 0.f;
		for(int i = 0; i < samples; ++i)
		{
			const Point3f p{ray.from_ + (params_.step_size_ * i + pos) * ray.dir_};
			float density = 0;
			for(const auto &vr : volume_regions_)
			{
				density += vr.item_->sigmaT(p, {}).energy();
			}
			density_samples.at(i) = density;
			if(i > 0) accum_density.at(i) = accum_density.at(i - 1) + density * params_.step_size_;
		}
	}
	constexpr float adapt_thresh = .01f;
	bool adapt_now = false;
	float current_step = params_.step_size_;
	int step_length = 1;
	int step_to_stop_adapt = -1;
	Rgb tr_tmp(1.f); // transmissivity during ray marching
	if(params_.adaptive_)
	{
		current_step = adaptive_step_size_;
		step_length = adaptive_resolution;
	}
	Rgb step_tau(0.f);
	const int lookahead_samples = adaptive_resolution / 10;
	Rgb col {0.f};
	for(int step_sample = 0; step_sample < samples; step_sample += step_length)
	{
		if(params_.adaptive_)
		{
			if(!adapt_now)
			{
				const int next_sample = (step_sample + adaptive_resolution > samples - 1) ? samples - 1 : step_sample + adaptive_resolution;
				if(std::abs(accum_density.at(step_sample) - accum_density.at(next_sample)) > adapt_thresh)
				{
					adapt_now = true;
					step_length = 1;
					step_to_stop_adapt = step_sample + lookahead_samples;
					current_step = params_.step_size_;
				}
			}
		}
		const Ray step_ray{ray.from_ + (ray.dir_ * pos), ray.dir_, ray.time_, 0, current_step};
		if(params_.adaptive_)
		{
			step_tau = Rgb{accum_density.at(step_sample)};
		}
		else
		{
			for(const auto &vr : volume_regions_)
			{
				const Bound<float>::Cross cross{vr.item_->crossBound(step_ray)};
				if(cross.crossed_)
				{
					step_tau += vr.item_->sigmaT(step_ray.from_, step_ray.dir_) * current_step;
				}
			}
		}
		tr_tmp = Rgb{math::exp(-step_tau.energy())};
		if(params_.optimize_ && tr_tmp.energy() < 1e-3f)
		{
			const float random_val = random_generator();
			if(random_val < 0.5f) break;
			tr_tmp = tr_tmp / random_val;
		}
		float sigma_s = 0.0f;
		for(const auto &vr : volume_regions_)
		{
			const Bound<float>::Cross cross{vr.item_->crossBound(step_ray)};
			if(cross.crossed_)
			{
				sigma_s += vr.item_->sigmaS(step_ray.from_, step_ray.dir_).energy();
			}
		}
		// with a sigma_s close to 0, no light can be scattered -> computation can be skipped
		if(params_.optimize_ && sigma_s < 1e-3f)
		{
			const float random_val = random_generator();
			if(random_val < 0.5f)
			{
				pos += current_step;
				continue;
			}
			sigma_s = sigma_s / random_val;
		}
		col += tr_tmp * getInScatter(random_generator, step_ray, current_step) * sigma_s * current_step;
		if(params_.adaptive_)
		{
			if(adapt_now && step_sample >= step_to_stop_adapt)
			{
				const int next_sample = (step_sample + adaptive_resolution > samples - 1) ? samples - 1 : step_sample + adaptive_resolution;
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
	return col;
}

} //namespace yafaray
