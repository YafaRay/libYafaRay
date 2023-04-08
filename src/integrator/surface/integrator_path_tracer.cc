/****************************************************************************
 *      pathtracer.cc: A rather simple MC path integrator
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
 *      Copyright (C) 2009  Rodrigo Placencia (DarkTide)
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

#include "integrator/surface/integrator_path_tracer.h"
#include "color/color_layers.h"
#include "sampler/sample.h"
#include "sampler/halton.h"
#include "render/render_data.h"
#include "common/timer.h"
#include "material/sample.h"
#include "volume/handler/volume_handler.h"
#include "integrator/volume/integrator_volume.h"
#include "render/imagefilm.h"

namespace yafaray {

class Pdf1D;

std::map<std::string, const ParamMeta *> PathIntegrator::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(path_samples_);
	PARAM_META(bounces_);
	PARAM_META(russian_roulette_min_bounces_);
	PARAM_META(no_recursive_);
	PARAM_META(caustic_type_);
	return param_meta_map;
}

PathIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(path_samples_);
	PARAM_LOAD(bounces_);
	PARAM_LOAD(russian_roulette_min_bounces_);
	PARAM_LOAD(no_recursive_);
	PARAM_ENUM_LOAD(caustic_type_);
}

ParamMap PathIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(path_samples_);
	PARAM_SAVE(bounces_);
	PARAM_SAVE(russian_roulette_min_bounces_);
	PARAM_SAVE(no_recursive_);
	PARAM_ENUM_SAVE(caustic_type_);
	return param_map;
}

std::pair<SurfaceIntegrator *, ParamResult> PathIntegrator::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto integrator {new PathIntegrator(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {integrator, param_result};
}

PathIntegrator::PathIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : ParentClassType_t(logger, param_result, name, param_map), params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

bool PathIntegrator::preprocess(RenderControl &render_control, const Scene &scene)
{
	bool success = SurfaceIntegrator::preprocess(render_control, scene);
	std::stringstream set;

	render_control.addTimerEvent("prepass");
	render_control.startTimer("prepass");

	set << "Path Tracing  ";

	if(MonteCarloIntegrator::params_.transparent_shadows_)
	{
		set << "ShadowDepth=" << MonteCarloIntegrator::params_.shadow_depth_ << "  ";
	}
	set << "RayDepth=" << MonteCarloIntegrator::params_.r_depth_ << " npaths=" << params_.path_samples_ << " bounces=" << params_.bounces_ << " min_bounces=" << params_.russian_roulette_min_bounces_ << " ";


	if(params_.caustic_type_.has(CausticType::Photon))
	{
		success = success && createCausticMap(render_control);
	}

	if(params_.caustic_type_ == CausticType::Path)
	{
		set << "\nCaustics: Path" << " ";
	}
	else if(params_.caustic_type_ == CausticType::Photon)
	{
		set << "\nCaustics: Photons=";
	}
	else if(params_.caustic_type_ == CausticType::Both)
	{
		set << "\nCaustics: Path + Photons=";
	}

	if(params_.caustic_type_.has(CausticType::Photon))
	{
		set << n_caus_photons_ << " search=" << CausticPhotonIntegrator::params_.n_caus_search_ << " radius=" << CausticPhotonIntegrator::params_.caus_radius_ << " depth=" << CausticPhotonIntegrator::params_.caus_depth_ << "  ";
	}

	render_control.stopTimer("prepass");
	logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), render_control.getTimerTime("prepass"), "s", " (", num_threads_photons_, " thread(s))");

	set << "| photon maps: " << std::fixed << std::setprecision(1) << render_control.getTimerTime("prepass") << "s" << " [" << num_threads_photons_ << " thread(s)]";

	render_control.setRenderInfo(render_control.getRenderInfo() + set.str());

	if(logger_.isVerbose())
	{
		for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
	}

	return success;
}

std::pair<Rgb, float> PathIntegrator::integrate(ImageFilm *image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier)
{
	static int calls = 0;
	++calls;
	Rgb col {0.f};
	float alpha = 1.f;
	float w = 0.f;
	const auto base_sampling_offset{image_film->getBaseSamplingOffset()};
	const auto camera{image_film->getCamera()};
	const auto [sp, tmax] = accelerator_->intersect(ray, camera);
	ray.tmax_ = tmax;
	if(sp)
	{
		const BsdfFlags mat_bsdfs = sp->mat_data_->bsdf_flags_;
		const Vec3f wo{-ray.dir_};
		additional_depth = std::max(additional_depth, sp->getMaterial()->getAdditionalDepth());

		// contribution of light emitting surfaces
		if(mat_bsdfs.has(BsdfFlags::Emit))
		{
			const Rgb col_emit = sp->emit(wo);
			col += col_emit;
			if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_emit;
			}
		}

		if(mat_bsdfs.has(BsdfFlags::Diffuse))
		{
			col += estimateAllDirectLight(random_generator, color_layers, camera, chromatic_enabled, wavelength, aa_light_sample_multiplier, *sp, wo, ray_division, pixel_sampling_data);
			if(params_.caustic_type_.has(CausticType::Photon))
			{
				col += causticPhotons(color_layers, ray, *sp, wo, aa_noise_params_.clamp_indirect_, caustic_map_.get(), CausticPhotonIntegrator::params_.caus_radius_, CausticPhotonIntegrator::params_.n_caus_search_);
			}
		}
		// path tracing:
		// the first path segment is "unrolled" from the loop because for the spot the camera hit
		// we do things slightly differently (e.g. may not sample specular, need not to init BSDF anymore,
		// have more efficient ways to compute samples...)

		BsdfFlags path_flags = params_.no_recursive_ ? BsdfFlags::All : (BsdfFlags::Diffuse);

		if(mat_bsdfs.has(path_flags))
		{
			Rgb path_col(0.0);
			path_flags |= BsdfFlags{BsdfFlags::Diffuse | BsdfFlags::Reflect | BsdfFlags::Transmit};
			int n_samples = std::max(1, params_.path_samples_ / ray_division.division_);
			for(int i = 0; i < n_samples; ++i)
			{
				unsigned int offs = params_.path_samples_ * pixel_sampling_data.sample_ + pixel_sampling_data.offset_ + i; // some redunancy here...
				Rgb throughput(1.0);
				Rgb lcol, scol;
				auto hit = std::make_unique<const SurfacePoint>(*sp);
				Vec3f pwo{wo};
				Ray p_ray;

				const float wavelength_dispersive = chromatic_enabled ? sample::riS(offs) : 0.f;
				//this mat already is initialized, just sample (diffuse...non-specular?)
				float s_1 = sample::riVdC(offs);
				float s_2 = Halton::lowDiscrepancySampling(fast_random_, 2, offs);
				if(ray_division.division_ > 1)
				{
					s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
					s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
				}
				// do proper sampling now...
				Sample s(s_1, s_2, path_flags);
				scol = sp->sample(pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength_dispersive, camera);
				scol *= w;
				throughput = scol;
				p_ray.tmin_ = ray_min_dist_;
				p_ray.tmax_ = -1.f;
				p_ray.from_ = sp->p_;
				std::tie(hit, p_ray.tmax_) = accelerator_->intersect(p_ray, camera);
				if(!hit) continue; //hit background
				if(s.sampled_flags_ != BsdfFlags::None) pwo = -p_ray.dir_; //Fix for white dots in path tracing with shiny diffuse with transparent PNG texture and transparent shadows, especially in Win32, (precision?). Sometimes the first sampling does not take place and pRay.dir is not initialized, so before this change when that happened pwo = -pRay.dir was getting a random_generator non-initialized value! This fix makes that, if the first sample fails for some reason, pwo is not modified and the rest of the sampling continues with the same pwo value. FIXME: Question: if the first sample fails, should we continue as now or should we exit the loop with the "continue" command?
				lcol = estimateOneDirectLight(random_generator, correlative_sample_number, base_sampling_offset, thread_id, camera, chromatic_enabled, wavelength_dispersive, *hit, pwo, offs, aa_light_sample_multiplier, ray_division, pixel_sampling_data);
				const BsdfFlags mat_bsd_fs = hit->mat_data_->bsdf_flags_;
				if(mat_bsd_fs.has(BsdfFlags::Emit))
				{
					const Rgb col_emit = hit->emit(pwo);
					lcol += col_emit;
					if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_emit;
					}
				}

				path_col += lcol * throughput;

				bool caustic = false;

				for(int depth = 1; depth < params_.bounces_; ++depth)
				{
					int d_4 = 4 * depth;
					s.s_1_ = Halton::lowDiscrepancySampling(fast_random_, d_4 + 3, offs); //ourRandom();//
					s.s_2_ = Halton::lowDiscrepancySampling(fast_random_, d_4 + 4, offs); //ourRandom();//

					if(ray_division.division_ > 1)
					{
						s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
						s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
					}

					s.flags_ = BsdfFlags::All;

					scol = hit->sample(pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength_dispersive, camera);
					scol *= w;
					if(scol.isBlack()) break;
					throughput *= scol;
					caustic = params_.caustic_type_.has(CausticType::Path) && s.sampled_flags_.has(BsdfFlags::Specular | BsdfFlags::Glossy | BsdfFlags::Filter);
					p_ray.tmin_ = ray_min_dist_;
					p_ray.tmax_ = -1.f;
					p_ray.from_ = hit->p_;
					auto [intersect_sp, intersect_tmax] = accelerator_->intersect(p_ray, camera);
					if(!intersect_sp) break; //hit background
					std::swap(hit, intersect_sp);
					pwo = -p_ray.dir_;

					if(mat_bsd_fs.has(BsdfFlags::Diffuse)) lcol = estimateOneDirectLight(random_generator, correlative_sample_number, base_sampling_offset, thread_id, camera, chromatic_enabled, wavelength_dispersive, *hit, pwo, offs, aa_light_sample_multiplier, ray_division, pixel_sampling_data);
					else lcol = Rgb(0.f);

					if(mat_bsd_fs.has(BsdfFlags::Volumetric))
					{
						if(const VolumeHandler *vol = hit->getMaterial()->getVolumeHandler(hit->n_ * pwo < 0))
						{
							throughput *= vol->transmittance(p_ray);
						}
					}
					// Russian roulette for terminating paths with low probability
					if(depth > params_.russian_roulette_min_bounces_)
					{
						const float random_value = random_generator();
						const float probability = throughput.maximum();
						if(probability <= 0.f || probability < random_value) break;
						throughput *= 1.f / probability;
					}

					if(mat_bsd_fs.has(BsdfFlags::Emit) && caustic)
					{
						const Rgb col_tmp = hit->emit(pwo);
						lcol += col_tmp;
						if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
						{
							if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_tmp;
						}
					}
					path_col += lcol * throughput;
				}
			}
			col += path_col / n_samples;
		}
		const auto [raytrace_col, raytrace_alpha]{recursiveRaytrace(image_film, random_generator, correlative_sample_number, color_layers, thread_id, ray_level + 1, chromatic_enabled, aa_light_sample_multiplier, aa_indirect_sample_multiplier, wavelength, ray, mat_bsdfs, *sp, wo, additional_depth, ray_division, pixel_sampling_data)};
		col += raytrace_col;
		alpha = raytrace_alpha;
		if(color_layers)
		{
			generateCommonLayers(color_layers, *sp, mask_params_, object_index_highest, material_index_highest);
			generateOcclusionLayers(color_layers, *accelerator_, chromatic_enabled, wavelength, ray_division, camera, pixel_sampling_data, *sp, wo, MonteCarloIntegrator::params_.ao_samples_, SurfaceIntegrator::params_.shadow_bias_auto_, shadow_bias_, MonteCarloIntegrator::params_.ao_distance_, MonteCarloIntegrator::params_.ao_color_, MonteCarloIntegrator::params_.shadow_depth_);
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugObjectTime))
			{
				const float col_combined_gray = col.col2Bri();
				if(sp->hasMotionBlur()) *color_layer += {col_combined_gray * ray.time_, 0.f, col_combined_gray * (1.f - ray.time_), 1.f};
				else *color_layer += {0.f, col_combined_gray, 0.f, 1.f};
			}
		}
	}
	else //nothing hit, return background
	{
		std::tie(col, alpha) = background(ray, color_layers, MonteCarloIntegrator::params_.transparent_background_, MonteCarloIntegrator::params_.transparent_background_refraction_, background_, ray_level);
	}

	if(vol_integrator_)
	{
		applyVolumetricEffects(col, alpha, color_layers, ray, random_generator, vol_integrator_.get(), MonteCarloIntegrator::params_.transparent_background_);
	}
	return {std::move(col), alpha};
}

} //namespace yafaray
