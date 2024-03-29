/****************************************************************************
 *              sppm.cc: A stochastic progressive photon map integrator
 *              This is part of the libYafaRay package
 *              Copyright (C) 2011  Rodrigo Placencia (DarkTide)
 *
 *              This library is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Lesser General Public
 *              License as published by the Free Software Foundation; either
 *              version 2.1 of the License, or (at your option) any later version.
 *
 *              This library is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *              Lesser General Public License for more details.
 *
 *              You should have received a copy of the GNU Lesser General Public
 *              License along with this library; if not, write to the Free Software
 *              Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "integrator/surface/integrator_sppm.h"
#include "geometry/surface.h"
#include "param/param.h"
#include "render/imagefilm.h"
#include "sampler/sample_pdf1d.h"
#include "light/light.h"
#include "camera/camera.h"
#include "color/spectrum.h"
#include "color/color_layers.h"
#include "render/render_data.h"
#include "photon/photon.h"
#include "render/render_control.h"
#include "photon/photon_sample.h"
#include "volume/handler/volume_handler.h"
#include "integrator/volume/integrator_volume.h"
#include "common/sysinfo.h"
#include "render/render_monitor.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> SppmIntegrator::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(num_photons_);
	PARAM_META(num_passes_);
	PARAM_META(bounces_);
	PARAM_META(times_);
	PARAM_META(photon_radius_);
	PARAM_META(search_num_);
	PARAM_META(pm_ire_);
	PARAM_META(threads_photons_);
	return param_meta_map;
}

SppmIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(num_photons_);
	PARAM_LOAD(num_passes_);
	PARAM_LOAD(bounces_);
	PARAM_LOAD(times_);
	PARAM_LOAD(photon_radius_);
	PARAM_LOAD(search_num_);
	PARAM_LOAD(pm_ire_);
	PARAM_LOAD(threads_photons_);
}

ParamMap SppmIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(num_photons_);
	PARAM_SAVE(num_passes_);
	PARAM_SAVE(bounces_);
	PARAM_SAVE(times_);
	PARAM_SAVE(photon_radius_);
	PARAM_SAVE(search_num_);
	PARAM_SAVE(pm_ire_);
	PARAM_SAVE(threads_photons_);
	return param_map;
}

std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> SppmIntegrator::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto integrator {std::make_unique<SppmIntegrator>(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {std::move(integrator), param_result};
}

SppmIntegrator::SppmIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : ParentClassType_t(logger, param_result, name, param_map), params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	caustic_map_ = std::make_unique<PhotonMap>(logger);
	getCausticMap()->setName("Caustic Photon Map");
	diffuse_map_ = std::make_unique<PhotonMap>(logger);
	getDiffuseMap()->setName("Diffuse Photon Map");
}

bool SppmIntegrator::render(RenderControl &render_control, RenderMonitor &render_monitor)
{
	std::stringstream pass_string;
	std::stringstream aa_settings;
	aa_settings << " passes=" << params_.num_passes_ << " samples=" << aa_noise_params_->samples_ << " inc_samples=" << aa_noise_params_->inc_samples_;
	aa_settings << " clamp=" << aa_noise_params_->clamp_samples_ << " ind.clamp=" << aa_noise_params_->clamp_indirect_;
	render_monitor.setAaNoiseInfo(render_monitor.getAaNoiseInfo() + aa_settings.str());

	render_monitor.setTotalPasses(params_.num_passes_);	//passNum is total number of passes in SPPM

	float aa_light_sample_multiplier = 1.f;
	float aa_indirect_sample_multiplier = 1.f;

	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": AA_clamp_samples: ", aa_noise_params_->clamp_samples_);
	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": AA_clamp_indirect: ", aa_noise_params_->clamp_indirect_);

	std::stringstream set;

	set << "SPPM  ";

	if(MonteCarloIntegrator::params_.transparent_shadows_)
	{
		set << "ShadowDepth=" << MonteCarloIntegrator::params_.shadow_depth_ << "  ";
	}
	set << "RayDepth=" << MonteCarloIntegrator::params_.r_depth_ << "  ";

	render_monitor.setRenderInfo(render_monitor.getRenderInfo() + set.str());
	if(logger_.isVerbose()) logger_.logVerbose(set.str());


	pass_string << "Rendering pass 1 of " << std::max(1, params_.num_passes_) << "...";
	logger_.logInfo(getName(), ": ", pass_string.str());
	render_monitor.setProgressBarTag(pass_string.str());

	render_monitor.addTimerEvent("rendert");
	render_monitor.startTimer("rendert");

	image_film_->resetImagesAutoSaveTimer();
	render_monitor.addTimerEvent("imagesAutoSaveTimer");

	image_film_->resetFilmAutoSaveTimer();
	render_monitor.addTimerEvent("filmAutoSaveTimer");

	image_film_->init(render_control, render_monitor, *this);

	if(render_control.resumed())
	{
		pass_string.clear();
		pass_string << "Loading film file, skipping pass 1...";
		render_monitor.setProgressBarTag(pass_string.str());
	}

	logger_.logInfo(getName(), ": ", pass_string.str());

	if(image_film_->getLayers()->isDefinedAny({LayerDef::ZDepthNorm, LayerDef::Mist})) precalcDepths();

	int acum_aa_samples = 1;
	std::vector<int> correlative_sample_number(num_threads_, 0);  //!< Used to sample lights more uniformly when using estimateOneDirectLight
	initializePpm(); // seems could integrate into the preRender
	if(render_control.resumed())
	{
		acum_aa_samples = image_film_->getSamplingOffset();
		renderPass(render_control, render_monitor, correlative_sample_number, 0, acum_aa_samples, false, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);
	}
	else
		renderPass(render_control, render_monitor, correlative_sample_number, 1, 0, false, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);

	std::string initial_estimate = "no";
	if(pm_ire_) initial_estimate = "yes";

	pm_ire_ = false;

	int hp_num = image_film_->getCamera()->resX() * image_film_->getCamera()->resY();
	int pass_info = 1;
	for(int i = 1; i < params_.num_passes_; ++i) //progress pass, the offset start from 1 as it is 0 based.
	{
		if(render_control.canceled()) break;
		pass_info = i + 1;
		image_film_->nextPass(render_control, render_monitor, false, getName());
		n_refined_ = 0;
		renderPass(render_control, render_monitor, correlative_sample_number, 1, acum_aa_samples, false, i, aa_light_sample_multiplier, aa_indirect_sample_multiplier); // offset are only related to the passNum, since we alway have only one sample.
		acum_aa_samples += 1;
		logger_.logInfo(getName(), ": This pass refined ", n_refined_, " of ", hp_num, " pixels.");
	}
	render_monitor.stopTimer("rendert");
	render_monitor.stopTimer("imagesAutoSaveTimer");
	render_monitor.stopTimer("filmAutoSaveTimer");
	render_control.setFinished();
	logger_.logInfo(getName(), ": Overall rendertime: ", render_monitor.getTimerTime("rendert"), "s.");

	// Integrator Settings for "drawRenderSettings()" in imageFilm, SPPM has own render method, so "getSettings()"
	// in integrator.h has no effect and Integrator settings won't be printed to the parameter badge.

	set.clear();

	set << "Passes rendered: " << pass_info << "  ";

	set << "\nPhotons=" << n_photons_ << " search=" << params_.search_num_ << " radius=" << params_.photon_radius_ << "(init.estim=" << initial_estimate << ") total photons=" << totaln_photons_ << "  ";

	render_monitor.setRenderInfo(render_monitor.getRenderInfo() + set.str());
	if(logger_.isVerbose()) logger_.logVerbose(set.str());
	return true;
}


bool SppmIntegrator::renderTile(std::vector<int> &correlative_sample_number, const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const RenderMonitor &render_monitor, const RenderControl &render_control)
{
	const int camera_res_x = image_film_->getCamera()->resX();
	RandomGenerator random_generator(rand() + offset * (camera_res_x * a.y_ + a.x_) + 123);
	const bool sample_lns = image_film_->getCamera()->sampleLens();
	const int pass_offs = offset, end_x = a.x_ + a.w_, end_y = a.y_ + a.h_;
	int aa_max_possible_samples = aa_noise_params_->samples_;
	for(int i = 1; i < aa_noise_params_->passes_; ++i)
	{
		aa_max_possible_samples += ceilf(aa_noise_params_->inc_samples_ * math::pow(aa_noise_params_->sample_multiplier_factor_, i));
	}
	float inv_aa_max_possible_samples = 1.f / static_cast<float>(aa_max_possible_samples);
	ColorLayers color_layers(*image_film_->getLayers());
	const int x_start_film = image_film_->getCx0();
	const int y_start_film = image_film_->getCy0();
	float d_1 = 1.f / static_cast<float>(n_samples);
	for(int i = a.y_; i < end_y; ++i)
	{
		for(int j = a.x_; j < end_x; ++j)
		{
			if(render_control.canceled()) break;
			color_layers.setDefaultColors();
			PixelSamplingData pixel_sampling_data{
					thread_id,
					camera_res_x * i + j,
					sample::fnv32ABuf(i * sample::fnv32ABuf(j)), //fnv_32a_buf(rstate.pixelNumber);
					aa_light_sample_multiplier,
					aa_indirect_sample_multiplier
			};
			float toff = Halton::lowDiscrepancySampling(fast_random_, 5, pass_offs + pixel_sampling_data.offset_); // **shall be just the pass number...**
			for(int sample = 0; sample < n_samples; ++sample) //set n_samples = 1.
			{
				pixel_sampling_data.sample_ = pass_offs + sample;
				const float time = TiledIntegrator::params_.time_forced_ ? TiledIntegrator::params_.time_forced_value_ : math::addMod1(static_cast<float>(sample) * d_1, toff); //(0.5+(float)sample)*d1;
				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA //!< the current (normalized) frame time  //FIXME, time not currently used in libYafaRay
				pixel_sampling_data.time_ = time;
				float dx = 0.5f, dy = 0.5f;
				dx = sample::riVdC(pixel_sampling_data.sample_, pixel_sampling_data.offset_);
				dy = sample::riS(pixel_sampling_data.sample_, pixel_sampling_data.offset_);

				Uv<float> lens_uv{0.5f, 0.5f};
				if(sample_lns)
				{
					lens_uv = {
							static_cast<float>(Halton::lowDiscrepancySampling(fast_random_, 3, pixel_sampling_data.sample_ + pixel_sampling_data.offset_)),
							static_cast<float>(Halton::lowDiscrepancySampling(fast_random_, 4, pixel_sampling_data.sample_ + pixel_sampling_data.offset_))
					};
				}
				CameraRay camera_ray = image_film_->getCamera()->shootRay(static_cast<float>(j) + dx, static_cast<float>(i) + dy, lens_uv); // wt need to be considered
				if(!camera_ray.valid_)
				{
					image_film_->addSample({{j, i}}, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers); //maybe not need
					continue;
				}
				if(ray_differentials_enabled_)
				{
					//setup ray differentials
					camera_ray.ray_.differentials_ = std::make_unique<RayDifferentials>();
					const CameraRay camera_diff_ray_x = image_film_->getCamera()->shootRay(static_cast<float>(j) + 1 + dx, static_cast<float>(i) + dy, lens_uv);
					camera_ray.ray_.differentials_->xfrom_ = camera_diff_ray_x.ray_.from_;
					camera_ray.ray_.differentials_->xdir_ = camera_diff_ray_x.ray_.dir_;
					const CameraRay camera_diff_ray_y = image_film_->getCamera()->shootRay(static_cast<float>(j) + dx, static_cast<float>(i) + 1 + dy, lens_uv);
					camera_ray.ray_.differentials_->yfrom_ = camera_diff_ray_y.ray_.from_;
					camera_ray.ray_.differentials_->ydir_ = camera_diff_ray_y.ray_.dir_;
					// col = T * L_o + L_v
				}
				camera_ray.ray_.time_ = time;
				//for sppm progressive
				const int index = ((i - y_start_film) * image_film_->getCamera()->resX()) + (j - x_start_film);
				HitPoint &hp = hit_points_[index];
				RayDivision ray_division;
				const GatherInfo g_info = traceGatherRay(camera_ray.ray_, hp, random_generator, &color_layers, 0, true, 0.f, ray_division, pixel_sampling_data);
				hp.constant_randiance_ += g_info.constant_randiance_; // accumulate the constant radiance for later usage.
				// progressive refinement
				const float alpha = 0.7f; // another common choice is 0.8, seems not changed much.
				// The author's refine formular
				if(g_info.photon_count_ > 0)
				{
					float g = std::min((hp.acc_photon_count_ + alpha * g_info.photon_count_) / (hp.acc_photon_count_ + g_info.photon_count_), 1.0f);
					hp.radius_2_ *= g;
					hp.acc_photon_count_ += g_info.photon_count_ * alpha;
					hp.acc_photon_flux_ = (hp.acc_photon_flux_ + g_info.photon_flux_) * g;
					n_refined_++; // record the pixel that has refined.
				}

				//radiance estimate
				//colorPasses.probe_mult(PASS_INT_DIFFUSE_INDIRECT, 1.f / (hp.radius2 * num_pi * totalnPhotons));
				const Rgba col_indirect = hp.acc_photon_flux_ / (hp.radius_2_ * math::num_pi<> * totaln_photons_);
				Rgba color = col_indirect;
				color += g_info.constant_randiance_;
				color.a_ = g_info.constant_randiance_.a_; //the alpha value is hold in the constantRadiance variable
				color_layers(LayerDef::Combined) = color;

				for(auto &[layer_def, layer_col] : color_layers)
				{
					switch(layer_def)
					{
						case LayerDef::ObjIndexMask:
						case LayerDef::ObjIndexMaskShadow:
						case LayerDef::ObjIndexMaskAll:
						case LayerDef::MatIndexMask:
						case LayerDef::MatIndexMaskShadow:
						case LayerDef::MatIndexMaskAll:
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							layer_col.clampRgb01();
							if(image_film_->getMaskParams()->invert_)
							{
								layer_col = Rgba(1.f) - layer_col;
							}
							if(!image_film_->getMaskParams()->only_)
							{
								Rgba col_combined = color_layers(LayerDef::Combined);
								col_combined.a_ = 1.f;
								layer_col *= col_combined;
							}
							break;
						case LayerDef::ZDepthAbs:
							if(camera_ray.ray_.tmax_ < 0.f) layer_col = Rgba(0.f, 0.f); // Show background as fully transparent
							else layer_col = Rgba{camera_ray.ray_.tmax_};
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
						case LayerDef::ZDepthNorm:
							if(camera_ray.ray_.tmax_ < 0.f) layer_col = Rgba(0.f, 0.f); // Show background as fully transparent
							else layer_col = Rgba{1.f - (camera_ray.ray_.tmax_ - image_film_->getMinDepth()) * image_film_->getMaxDepthInverse()}; // Distance normalization
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
						case LayerDef::Mist:
							if(camera_ray.ray_.tmax_ < 0.f) layer_col = Rgba(0.f, 0.f); // Show background as fully transparent
							else layer_col = Rgba{(camera_ray.ray_.tmax_ - image_film_->getMinDepth()) * image_film_->getMaxDepthInverse()}; // Distance normalization
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
						case LayerDef::Indirect:
							layer_col = col_indirect;
							layer_col.a_ = g_info.constant_randiance_.a_;
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
						default:
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
					}
				}
				image_film_->addSample({{j, i}}, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
			}
		}
	}
	return true;
}

void SppmIntegrator::photonWorker(RenderControl &render_control, RenderMonitor &render_monitor, unsigned int &total_photons_shot, int thread_id, int num_d_lights, const Pdf1D *light_power_d, const std::vector<const Light *> &tmplights, int pb_step)
{
	//shoot photons
	bool done = false;
	unsigned int curr = 0;

	std::unique_ptr<const SurfacePoint> hit_prev, hit_curr;

	const auto f_num_lights = static_cast<float>(num_d_lights);

	unsigned int n_photons_thread = 1 + ((n_photons_ - 1) / num_threads_photons_);

	std::vector<Photon> local_caustic_photons;
	local_caustic_photons.clear();
	local_caustic_photons.reserve(n_photons_thread);

	std::vector<Photon> local_diffuse_photons;
	local_diffuse_photons.clear();
	local_diffuse_photons.reserve(n_photons_thread);

	//Pregather  photons
	const float inv_diff_photons = 1.f / static_cast<float>(n_photons_);

	unsigned int nd_photon_stored = 0;
	//	unsigned int ncPhotonStored = 0;

	while(!done)
	{
		unsigned int haltoncurr = curr + n_photons_thread * thread_id;
		const float wavelength = Halton::lowDiscrepancySampling(fast_random_, 5, haltoncurr);

		// Tried LD, get bad and strange results for some stategy.
		mutex_.lock();
		const float s_1 = hal_1_.getNext();
		const float s_2 = hal_2_.getNext();
		const float s_3 = hal_3_.getNext();
		const float s_4 = hal_4_.getNext();
		mutex_.unlock();
		const float s_l = static_cast<float>(haltoncurr) * inv_diff_photons; // Does sL also need more random_generator for each pass?
		const auto [light_num, light_num_pdf]{light_power_d->dSample(s_l)};
		if(light_num >= num_d_lights)
		{
			logger_.logError(getName(), ": lightPDF sample error! ", s_l, "/", light_num);
			return;
		}
		float time = TiledIntegrator::params_.time_forced_ ? TiledIntegrator::params_.time_forced_value_ : math::addMod1(static_cast<float>(curr) / static_cast<float>(n_photons_thread), s_2); //FIXME: maybe we should use an offset for time that is independent from the space-related samples as s_2 now
		auto[ray, light_pdf, pcol]{tmplights[light_num]->emitPhoton(s_1, s_2, s_3, s_4, time)};
		ray.tmin_ = ray_min_dist_;
		ray.tmax_ = -1.f;
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of th pdf, hence *=...

		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= n_photons_);
			continue;
		}
		else if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
		{
			logger_.logWarning(getName(), ": NaN  on photon color for light", light_num + 1, ".");
			continue;
		}

		int n_bounces = 0;
		bool caustic_photon = false;
		bool direct_photon = true;
		const Material *material_prev = nullptr;
		BsdfFlags mat_bsdfs_prev = BsdfFlags::None;
		bool chromatic_enabled = true;
		while(true)   //scatter photons.
		{
			std::tie(hit_curr, ray.tmax_) = accelerator_->intersect(ray);
			if(!hit_curr) break;
			Rgb transm(1.f);
			if(material_prev && hit_prev && mat_bsdfs_prev.has(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = material_prev->getVolumeHandler(hit_prev->ng_ * ray.dir_ < 0))
				{
					transm = vol->transmittance(ray);
				}
			}
			const Vec3f wi{-ray.dir_};
			const BsdfFlags mat_bsdfs = hit_curr->mat_data_->bsdf_flags_;

			//deposit photon on diffuse surface, now we only have one map for all, elimate directPhoton for we estimate it directly
			if(!direct_photon && !caustic_photon && mat_bsdfs.has(BsdfFlags::Diffuse))
			{
				Photon np{wi, hit_curr->p_, pcol, ray.time_};// pcol used here
				if(b_hashgrid_) photon_grid_.pushPhoton(std::move(np));
				else local_diffuse_photons.emplace_back(std::move(np));
				nd_photon_stored++;
			}
			// add caustic photon
			if(!direct_photon && caustic_photon && mat_bsdfs.has(BsdfFlags::Diffuse | BsdfFlags::Glossy))
			{
				Photon np{wi, hit_curr->p_, pcol, ray.time_};// pcol used here
				if(b_hashgrid_) photon_grid_.pushPhoton(std::move(np));
				else local_caustic_photons.emplace_back(std::move(np));
				nd_photon_stored++;
			}

			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == params_.bounces_) break;

			// scatter photon
			const float s_5 = fast_random_.getNextFloatNormalized(); // now should use this to see correctness
			const float s_6 = fast_random_.getNextFloatNormalized();
			const float s_7 = fast_random_.getNextFloatNormalized();

			PSample sample(s_5, s_6, s_7, BsdfFlags::All, pcol, transm);

			Vec3f wo;
			bool scattered = hit_curr->scatterPhoton(wi, wo, sample, chromatic_enabled, wavelength);
			if(!scattered) break; //photon was absorped.  actually based on russian roulette

			pcol = sample.color_;

			caustic_photon = (sample.sampled_flags_.has((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.has((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
			direct_photon = sample.sampled_flags_.has(BsdfFlags::Filter) && direct_photon;

			if(chromatic_enabled && sample.sampled_flags_.has(BsdfFlags::Dispersive))
			{
				chromatic_enabled = false;
				pcol *= spectrum::wl2Rgb(wavelength);
			}
			ray.from_ = hit_curr->p_;
			ray.dir_ = wo;
			ray.tmin_ = ray_min_dist_;
			ray.tmax_ = -1.f;
			material_prev = hit_curr->getMaterial();
			mat_bsdfs_prev = mat_bsdfs;
			std::swap(hit_prev, hit_curr);
			++n_bounces;
		}
		++curr;
		if(curr % pb_step == 0)
		{
			render_monitor.updateProgressBar();
			if(render_control.canceled()) { return; }
		}
		done = (curr >= n_photons_thread);
	}
	getDiffuseMap()->lock();
	getCausticMap()->lock();
	getDiffuseMap()->appendVector(local_diffuse_photons, curr);
	getCausticMap()->appendVector(local_caustic_photons, curr);
	total_photons_shot += curr;
	getCausticMap()->unlock();
	getDiffuseMap()->unlock();
}


//photon pass, scatter photon
void SppmIntegrator::prePass(RenderControl &render_control, RenderMonitor &render_monitor, int samples, int offset, bool adaptive)
{
	if(getLights().empty()) return;
	render_monitor.addTimerEvent("prepass");
	render_monitor.startTimer("prepass");

	logger_.logInfo(getName(), ": Starting Photon tracing pass...");

	if(b_hashgrid_) photon_grid_.clear();
	else
	{
		getDiffuseMap()->clear();
		getDiffuseMap()->setNumPaths(0);
		getDiffuseMap()->reserveMemory(n_photons_);
		getDiffuseMap()->setNumThreadsPkDtree(num_threads_photons_);

		getCausticMap()->clear();
		getCausticMap()->setNumPaths(0);
		getCausticMap()->reserveMemory(n_photons_);
		getCausticMap()->setNumThreadsPkDtree(num_threads_photons_);
	}

	//background do not emit photons, or it is merged into normal light?

	const int num_lights = numLights();
	const auto f_num_lights = static_cast<float>(num_lights);
	std::vector<float> energies(num_lights);
	for(int i = 0; i < num_lights; ++i) energies[i] = getLight(i)->totalEnergy().energy();
	auto light_power_d = std::make_unique<Pdf1D>(energies);
	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for photon map:");

	for(int i = 0; i < num_lights; ++i)
	{
		auto[ray, light_pdf, pcol]{getLight(i)->emitPhoton(.5, .5, .5, .5, 0.f)}; //FIXME: what time to use?
		const float light_num_pdf = light_power_d->function(i) * light_power_d->invIntegral();
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", getLight(i)->getName(), "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
	}

	//shoot photons
	unsigned int curr = 0;
	RandomGenerator random_generator(rand() + offset * (4517) + 123);
	std::string previous_progress_tag = render_monitor.getProgressBarTag();
	int previous_progress_total_steps = render_monitor.getProgressBarTotalSteps();

	if(b_hashgrid_) logger_.logInfo(getName(), ": Building photon hashgrid...");
	else logger_.logInfo(getName(), ": Building photon map...");
	render_monitor.initProgressBar(128, logger_.getConsoleLogColorsEnabled());
	const int pb_step = std::max(1, n_photons_ / 128);
	render_monitor.setProgressBarTag(previous_progress_tag + " - building photon map...");

	n_photons_ = std::max(num_threads_photons_, (n_photons_ / num_threads_photons_) * num_threads_photons_); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

	logger_.logParams(getName(), ": Shooting ", n_photons_, " photons across ", num_threads_photons_, " threads (", (n_photons_ / num_threads_photons_), " photons/thread)");

	std::vector<std::thread> threads;
	threads.reserve(num_threads_photons_);
	for(int i = 0; i < num_threads_photons_; ++i) threads.emplace_back(&SppmIntegrator::photonWorker, this, std::ref(render_control), std::ref(render_monitor), std::ref(curr), i, num_lights, light_power_d.get(), getLights(), pb_step);
	for(auto &t : threads) t.join();

	render_monitor.setProgressBarAsDone();
	render_monitor.setProgressBarTag(previous_progress_tag + " - photon map built.");
	if(logger_.isVerbose()) logger_.logVerbose(getName(), ":Photon map built.");
	logger_.logInfo(getName(), ": Shot ", curr, " photons from ", num_lights, " light(s)");

	totaln_photons_ +=  n_photons_;	// accumulate the total photon number, not using nPath for the case of hashgrid.

	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored photons: ", getDiffuseMap()->nPhotons() + getCausticMap()->nPhotons());

	if(b_hashgrid_)
	{
		logger_.logInfo(getName(), ": Building photons hashgrid:");
		photon_grid_.updateGrid();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
	}
	else
	{
		if(getDiffuseMap()->nPhotons() > 0)
		{
			logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
			getDiffuseMap()->updateTree(render_monitor, render_control);
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
		if(getCausticMap()->nPhotons() > 0)
		{
			logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
			getCausticMap()->updateTree(render_monitor, render_control);
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
		if(getDiffuseMap()->nPhotons() < 50)
		{
			logger_.logError(getName(), ": Too few photons, stopping now.");
			return;
		}
	}

	render_monitor.stopTimer("prepass");

	if(b_hashgrid_)
		logger_.logInfo(getName(), ": PhotonGrid building time: ", render_monitor.getTimerTime("prepass"));
	else
		logger_.logInfo(getName(), ": PhotonMap building time: ", render_monitor.getTimerTime("prepass"));

	render_monitor.setProgressBarTag(previous_progress_tag);
	render_monitor.initProgressBar(previous_progress_total_steps, logger_.getConsoleLogColorsEnabled());
}

//now it's a dummy function
std::pair<Rgb, float> SppmIntegrator::integrate(Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data)
{
	return {Rgb{0.f}, 0.f};
}

GatherInfo SppmIntegrator::traceGatherRay(Ray &ray, HitPoint &hp, RandomGenerator &random_generator, ColorLayers *color_layers, int ray_level, bool chromatic_enabled, float wavelength, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data)
{
	GatherInfo g_info;
	float alpha = MonteCarloIntegrator::params_.transparent_background_ ? 0.f : 1.f;
	const auto [sp, tmax] = accelerator_->intersect(ray, image_film_->getCamera());
	ray.tmax_ = tmax;
	if(sp)
	{
		int additional_depth = 0;

		const Vec3f wo{-ray.dir_};
		const BsdfFlags mat_bsdfs = sp->mat_data_->bsdf_flags_;
		additional_depth = std::max(additional_depth, sp->getMaterial()->getAdditionalDepth());

		const Rgb col_emit = sp->emit(wo);
		g_info.constant_randiance_ += col_emit; //add only once, but FG seems add twice?
		if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_emit;
		}
		if(mat_bsdfs.has(BsdfFlags::Diffuse))
		{
			g_info.constant_randiance_ += estimateAllDirectLight(random_generator, color_layers, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
		}

		// estimate radiance using photon map
		auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_max_gather_]);

		//if PM_IRE is on. we should estimate the initial radius using the photonMaps. (PM_IRE is only for the first pass, so not consume much time)
		if(pm_ire_ && !hp.radius_setted_) // "waste" two gather here as it has two maps now. This make the logic simple.
		{
			float radius_1 = params_.photon_radius_ * params_.photon_radius_;
			float radius_2 = radius_1;
			int n_gathered_1 = 0, n_gathered_2 = 0;

			if(getDiffuseMap()->nPhotons() > 0)
				n_gathered_1 = getDiffuseMap()->gather(sp->p_, gathered.get(), params_.search_num_, radius_1);
			if(getCausticMap()->nPhotons() > 0)
				n_gathered_2 = getCausticMap()->gather(sp->p_, gathered.get(), params_.search_num_, radius_2);
			if(n_gathered_1 > 0 || n_gathered_2 > 0) // it none photon gathered, we just skip.
			{
				if(radius_1 < radius_2) // we choose the smaller one to be the initial radius.
					hp.radius_2_ = radius_1;
				else
					hp.radius_2_ = radius_2;

				hp.radius_setted_ = true;
			}
		}

		int n_gathered = 0;
		float radius_2 = hp.radius_2_;

		if(b_hashgrid_)
			n_gathered = photon_grid_.gather(sp->p_, gathered.get(), n_max_gather_, radius_2); // disable now
		else
		{
			if(getDiffuseMap()->nPhotons() > 0) // this is needed to avoid a runtime error.
			{
				n_gathered = getDiffuseMap()->gather(sp->p_, gathered.get(), n_max_gather_, radius_2); //we always collected all the photon inside the radius
			}

			if(n_gathered > 0 && logger_.isDebug())
			{
				if(n_gathered > n_max_gathered_)
				{
					n_max_gathered_ = n_gathered;
					if(logger_.isDebug())
					{
						logger_.logDebug("maximum Photons: ", n_max_gathered_, ", radius2: ", radius_2);
						if(n_max_gathered_ == 10)
						{
							for(int j = 0; j < n_gathered; ++j)
							{
								logger_.logDebug("col:", gathered[j].photon_->col_);
							}
						}
					}
				}
				for(int i = 0; i < n_gathered; ++i)
				{
					////test if the photon is in the ellipsoid
					//vector3d_t scale  = sp->P - gathered[i].photon->pos;
					//vector3d_t temp;
					//temp.x = scale VDOT sp->NU;
					//temp.y = scale VDOT sp->NV;
					//temp.z = scale VDOT sp->N;

					//double inv_radi = 1 / sqrt(radius2);
					//temp.x  *= inv_radi; temp.y *= inv_radi; temp.z *=  1. / (2.f * scene.rayMinDist);
					//if(temp.lengthSqr() > 1.)continue;

					g_info.photon_count_++;
					Vec3f pdir{gathered[i].photon_->dir_};
					Rgb surf_col = sp->eval(wo, pdir, BsdfFlags::Diffuse); // seems could speed up using rho, (something pbrt made)
					g_info.photon_flux_ += surf_col * gathered[i].photon_->col_;// * std::abs(sp->N*pdir); //< wrong!?
					//Rgb  flux= surfCol * gathered[i].photon->col_;// * std::abs(sp->N*pdir); //< wrong!?

					////start refine here
					//double ALPHA = 0.7;
					//double g = (hp.accPhotonCount*ALPHA+ALPHA) / (hp.accPhotonCount*ALPHA+1.0);
					//hp.radius2 *= g;
					//hp.accPhotonCount++;
					//hp.accPhotonFlux=((Rgb)hp.accPhotonFlux+flux)*g;
				}
			}

			// gather caustics photons
			if(mat_bsdfs.has(BsdfFlags::Diffuse) && getCausticMap()->ready())
			{

				radius_2 = hp.radius_2_; //reset radius2 & nGathered
				n_gathered = getCausticMap()->gather(sp->p_, gathered.get(), n_max_gather_, radius_2);
				if(n_gathered > 0)
				{
					Rgb surf_col(0.f);
					for(int i = 0; i < n_gathered; ++i)
					{
						Vec3f pdir{gathered[i].photon_->dir_};
						g_info.photon_count_++;
						surf_col = sp->eval(wo, pdir, BsdfFlags::All); // seems could speed up using rho, (something pbrt made)
						g_info.photon_flux_ += surf_col * gathered[i].photon_->col_;// * std::abs(sp->N*pdir); //< wrong!?//gInfo.photonFlux += colorPasses.probe_add(PASS_INT_DIFFUSE_INDIRECT, surfCol * gathered[i].photon->col_, state.ray_level == 0);// * std::abs(sp->N*pdir); //< wrong!?
						//Rgb  flux= surfCol * gathered[i].photon->col_;// * std::abs(sp->N*pdir); //< wrong!?

						////start refine here
						//double ALPHA = 0.7;
						//double g = (hp.accPhotonCount*ALPHA+ALPHA) / (hp.accPhotonCount*ALPHA+1.0);
						//hp.radius2 *= g;
						//hp.accPhotonCount++;
						//hp.accPhotonFlux=((Rgb)hp.accPhotonFlux+flux)*g;
					}
				}
			}
		}

		++ray_level; //FIXME: how set the ray_level in a better way?
		if(ray_level <= (MonteCarloIntegrator::params_.r_depth_ + additional_depth))
		{
			Halton hal_2(2);
			Halton hal_3(3);
			// dispersive effects with recursive raytracing:
			if(mat_bsdfs.has(BsdfFlags::Dispersive) && chromatic_enabled)
			{
				const int ray_samples_dispersive = ray_division.division_ > 1 ?
												   std::max(1, initial_ray_samples_dispersive_ / ray_division.division_) :
												   initial_ray_samples_dispersive_;
				RayDivision ray_division_new {ray_division};
				ray_division_new.division_ *= ray_samples_dispersive;
				int branch = ray_division_new.division_ * ray_division.offset_;
				const float d_1 = 1.f / static_cast<float>(ray_samples_dispersive);
				const float ss_1 = sample::riS(pixel_sampling_data.sample_ + pixel_sampling_data.offset_);
				Ray ref_ray;
				float w = 0.f;
				GatherInfo cing, t_cing; //Dispersive is different handled, not same as GLOSSY, at the BSDF_VOLUMETRIC part

				Rgb dcol_trans_accum;
				for(int ns = 0; ns < ray_samples_dispersive; ++ns)
				{
					float wavelength_dispersive;
					if(chromatic_enabled)
					{
						wavelength_dispersive = (ns + ss_1) * d_1;
						if(ray_division.division_ > 1) wavelength_dispersive = math::addMod1(wavelength_dispersive, ray_division.decorrelation_1_);
					}
					else wavelength_dispersive = 0.f;
					ray_division_new.decorrelation_1_ = Halton::lowDiscrepancySampling(fast_random_, 2 * ray_level + 1, branch + pixel_sampling_data.offset_);
					ray_division_new.decorrelation_2_ = Halton::lowDiscrepancySampling(fast_random_, 2 * ray_level + 2, branch + pixel_sampling_data.offset_);
					ray_division_new.offset_ = branch;
					++branch;
					Sample s(0.5f, 0.5f, BsdfFlags::Reflect | BsdfFlags::Transmit | BsdfFlags::Dispersive);
					Vec3f wi;
					Rgb mcol = sp->sample(wo, wi, s, w, chromatic_enabled, wavelength_dispersive, image_film_->getCamera());

					if(s.pdf_ > 1.0e-6f && s.sampled_flags_.has(BsdfFlags::Dispersive))
					{
						const Rgb wl_col = spectrum::wl2Rgb(wavelength_dispersive);
						ref_ray = {sp->p_, wi, ray.time_, ray_min_dist_};
						t_cing = traceGatherRay(ref_ray, hp, random_generator, nullptr, ray_level, false, wavelength_dispersive, ray_division_new, pixel_sampling_data);
						t_cing.photon_flux_ *= mcol * wl_col * w;
						t_cing.constant_randiance_ *= mcol * wl_col * w;
						if(color_layers)
						{
							if(color_layers->find(LayerDef::Trans)) dcol_trans_accum += t_cing.constant_randiance_;
						}
					}
					cing += t_cing;
				}
				if(mat_bsdfs.has(BsdfFlags::Volumetric))
				{
					if(const VolumeHandler *vol = sp->getMaterial()->getVolumeHandler(sp->ng_ * ref_ray.dir_ < 0))
					{
						const Rgb vcol = vol->transmittance(ref_ray);
						cing.photon_flux_ *= vcol;
						cing.constant_randiance_ *= vcol;
					}
				}
				g_info.constant_randiance_ += cing.constant_randiance_ * d_1;
				g_info.photon_flux_ += cing.photon_flux_ * d_1;
				g_info.photon_count_ += cing.photon_count_ * d_1;

				if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
				{
					if(Rgba *color_layer = color_layers->find(LayerDef::Trans))
					{
						dcol_trans_accum *= d_1;
						*color_layer += dcol_trans_accum;
					}
				}
			}
			// glossy reflection with recursive raytracing:  Pure GLOSSY material doesn't hold photons?
			if(mat_bsdfs.has(BsdfFlags::Glossy))
			{
				const int ray_samples_glossy = ray_division.division_ > 1 ?
											   std::max(1, initial_ray_samples_glossy_ / ray_division.division_) :
											   initial_ray_samples_glossy_;
				RayDivision ray_division_new {ray_division};
				ray_division_new.division_ *= ray_samples_glossy;
				int branch = ray_division_new.division_ * ray_division_new.offset_;
				unsigned int offs = ray_samples_glossy * pixel_sampling_data.sample_ + pixel_sampling_data.offset_;
				const float d_1 = 1.f / static_cast<float>(ray_samples_glossy);
				GatherInfo gather_info;
				hal_2.setStart(offs);
				hal_3.setStart(offs);
				Rgb gcol_indirect_accum;
				Rgb gcol_reflect_accum;
				Rgb gcol_transmit_accum;

				for(int ns = 0; ns < ray_samples_glossy; ++ns)
				{
					ray_division_new.decorrelation_1_ = Halton::lowDiscrepancySampling(fast_random_, 2 * ray_level + 1, branch + pixel_sampling_data.offset_);
					ray_division_new.decorrelation_2_ = Halton::lowDiscrepancySampling(fast_random_, 2 * ray_level + 2, branch + pixel_sampling_data.offset_);
					ray_division_new.offset_ = branch;
					++offs;
					++branch;

					float s_1 = hal_2.getNext();
					float s_2 = hal_3.getNext();

					float W = 0.f;

					Sample s(s_1, s_2, BsdfFlags::AllGlossy);
					Vec3f wi;
					Rgb mcol = sp->sample(wo, wi, s, W, chromatic_enabled, wavelength, image_film_->getCamera());

					if(mat_bsdfs.has(BsdfFlags::Reflect) && !mat_bsdfs.has(BsdfFlags::Transmit))
					{
						float w = 0.f;

						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Reflect);
						const Rgb mcol = sp->sample(wo, wi, s, w, chromatic_enabled, wavelength, image_film_->getCamera());
						Ray ref_ray{sp->p_, wi, ray.time_, ray_min_dist_};
						if(s.sampled_flags_.has(BsdfFlags::Reflect)) ref_ray.differentials_ = sp->reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
						else if(s.sampled_flags_.has(BsdfFlags::Transmit)) ref_ray.differentials_ = sp->refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp->getMaterial()->getMatIor());
						//gcol += tmpColorPasses.probe_add(PASS_INT_GLOSSY_INDIRECT, (Rgb)integ * mcol * W, state.ray_level == 1);
						GatherInfo trace_gather_ray = traceGatherRay(ref_ray, hp, random_generator, nullptr, ray_level, chromatic_enabled, wavelength, ray_division_new, pixel_sampling_data);
						trace_gather_ray.photon_flux_ *= mcol * w;
						trace_gather_ray.constant_randiance_ *= mcol * w;
						gather_info += trace_gather_ray;
					}
					else if(mat_bsdfs.has(BsdfFlags::Reflect) && mat_bsdfs.has(BsdfFlags::Transmit))
					{
						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::AllGlossy);
						Rgb mcol[2];
						float w[2];
						Vec3f dir[2];
						mcol[0] = sp->sample(wo, dir, mcol[1], s, w, chromatic_enabled, wavelength);
						if(s.sampled_flags_.has(BsdfFlags::Reflect) && !s.sampled_flags_.has(BsdfFlags::Dispersive))
						{
							Ray ref_ray{sp->p_, dir[0], ray.time_, ray_min_dist_};
							ref_ray.differentials_ = sp->reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
							const Rgb col_reflect_factor = mcol[0] * w[0];
							GatherInfo trace_gather_ray = traceGatherRay(ref_ray, hp, random_generator, nullptr, ray_level, chromatic_enabled, wavelength, ray_division_new, pixel_sampling_data);
							trace_gather_ray.photon_flux_ *= col_reflect_factor;
							trace_gather_ray.constant_randiance_ *= col_reflect_factor;
							if(color_layers)
							{
								if(color_layers->find(LayerDef::GlossyIndirect)) gcol_indirect_accum += trace_gather_ray.constant_randiance_;
							}
							gather_info += trace_gather_ray;
						}

						if(s.sampled_flags_.has(BsdfFlags::Transmit))
						{
							Ray ref_ray{sp->p_, dir[1], ray.time_, ray_min_dist_};
							ref_ray.differentials_ = sp->refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp->getMaterial()->getMatIor());
							const Rgb col_transmit_factor = mcol[1] * w[1];
							GatherInfo trace_gather_ray = traceGatherRay(ref_ray, hp, random_generator, nullptr, ray_level, chromatic_enabled, wavelength, ray_division_new, pixel_sampling_data);
							trace_gather_ray.photon_flux_ *= col_transmit_factor;
							trace_gather_ray.constant_randiance_ *= col_transmit_factor;
							if(color_layers)
							{
								if(color_layers->find(LayerDef::GlossyIndirect)) gcol_transmit_accum += trace_gather_ray.constant_randiance_;
							}
							gather_info += trace_gather_ray;
						}
					}

					else if(s.sampled_flags_.has(BsdfFlags::Glossy))
					{
						Ray ref_ray{sp->p_, wi, ray.time_, ray_min_dist_};
						if(ray.differentials_)
						{
							if(s.sampled_flags_.has(BsdfFlags::Reflect)) ref_ray.differentials_ = sp->reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_);
							else if(s.sampled_flags_.has(BsdfFlags::Transmit)) ref_ray.differentials_ = sp->refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp->getMaterial()->getMatIor());
						}

						GatherInfo trace_gather_ray = traceGatherRay(ref_ray, hp, random_generator, nullptr, ray_level, chromatic_enabled, wavelength, ray_division_new, pixel_sampling_data);
						trace_gather_ray.photon_flux_ *= mcol * W;
						trace_gather_ray.constant_randiance_ *= mcol * W;
						if(color_layers)
						{
							if(color_layers->find(LayerDef::Trans)) gcol_reflect_accum += trace_gather_ray.constant_randiance_;
						}
						gather_info += trace_gather_ray;
					}
					if(mat_bsdfs.has(BsdfFlags::Volumetric))
					{
						const Ray ref_ray{sp->p_, wi, ray.time_, ray_min_dist_};
						if(const VolumeHandler *vol = sp->getMaterial()->getVolumeHandler(sp->ng_ * ref_ray.dir_ < 0))
						{
							const Rgb vcol = vol->transmittance(ref_ray);
							gather_info.photon_flux_ *= vcol;
							gather_info.constant_randiance_ *= vcol;
						}
					}
				}
				g_info.constant_randiance_ += gather_info.constant_randiance_ * d_1;
				g_info.photon_flux_ += gather_info.photon_flux_ * d_1;
				g_info.photon_count_ += gather_info.photon_count_ * d_1;
				if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
				{
					if(Rgba *color_layer = color_layers->find(LayerDef::GlossyIndirect))
					{
						gcol_indirect_accum *= d_1;
						*color_layer += gcol_indirect_accum;
					}
					if(Rgba *color_layer = color_layers->find(LayerDef::Trans))
					{
						gcol_reflect_accum *= d_1;
						*color_layer += gcol_reflect_accum;
					}
					if(Rgba *color_layer = color_layers->find(LayerDef::GlossyIndirect))
					{
						gcol_transmit_accum *= d_1;
						*color_layer += gcol_transmit_accum;
					}
				}
			}
			//...perfect specular reflection/refraction with recursive raytracing...
			if(mat_bsdfs.has(BsdfFlags::Specular | BsdfFlags::Filter))
			{
				const Specular specular = sp->getSpecular(ray_level, wo, chromatic_enabled, wavelength);
				if(specular.reflect_)
				{
					Ray ref_ray{sp->p_, specular.reflect_->dir_, ray.time_, ray_min_dist_};
					if(ray.differentials_) ref_ray.differentials_ = sp->reflectedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_); // compute the ray differentaitl
					GatherInfo refg = traceGatherRay(ref_ray, hp, random_generator, nullptr, ray_level, chromatic_enabled, wavelength, ray_division, pixel_sampling_data);
					if(mat_bsdfs.has(BsdfFlags::Volumetric))
					{
						if(const VolumeHandler *vol = sp->getMaterial()->getVolumeHandler(sp->ng_ * ref_ray.dir_ < 0))
						{
							const Rgb vcol = vol->transmittance(ref_ray);
							refg.constant_randiance_ *= vcol;
							refg.photon_flux_ *= vcol;
						}
					}
					const Rgba col_radiance_reflect = refg.constant_randiance_ * Rgba(specular.reflect_->col_);
					g_info.constant_randiance_ += col_radiance_reflect;
					if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::ReflectPerfect)) *color_layer += col_radiance_reflect;
					}
					g_info.photon_flux_ += refg.photon_flux_ * Rgba(specular.reflect_->col_);
					g_info.photon_count_ += refg.photon_count_;
				}
				if(specular.refract_)
				{
					Ray ref_ray{sp->p_, specular.refract_->dir_, ray.time_, ray_min_dist_};
					if(ray.differentials_) ref_ray.differentials_ = sp->refractedRay(ray.differentials_.get(), ray.dir_, ref_ray.dir_, sp->getMaterial()->getMatIor());
					GatherInfo refg = traceGatherRay(ref_ray, hp, random_generator, nullptr, ray_level, chromatic_enabled, wavelength, ray_division, pixel_sampling_data);
					if(mat_bsdfs.has(BsdfFlags::Volumetric))
					{
						if(const VolumeHandler *vol = sp->getMaterial()->getVolumeHandler(sp->ng_ * ref_ray.dir_ < 0))
						{
							const Rgb vcol = vol->transmittance(ref_ray);
							refg.constant_randiance_ *= vcol;
							refg.photon_flux_ *= vcol;
						}
					}
					const Rgba col_radiance_refract = refg.constant_randiance_ * Rgba(specular.refract_->col_);
					g_info.constant_randiance_ += col_radiance_refract;
					if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::RefractPerfect)) *color_layer += col_radiance_refract;
					}
					g_info.photon_flux_ += refg.photon_flux_ * Rgba(specular.refract_->col_);
					g_info.photon_count_ += refg.photon_count_;
					alpha = refg.constant_randiance_.a_;
				}
			}
		}
		if(color_layers)
		{
			generateCommonLayers(color_layers, *sp, *image_film_->getMaskParams(), object_index_highest_, material_index_highest_);
			generateOcclusionLayers(color_layers, *accelerator_, chromatic_enabled, wavelength, ray_division, image_film_->getCamera(), pixel_sampling_data, *sp, wo, MonteCarloIntegrator::params_.ao_samples_, SurfaceIntegrator::params_.shadow_bias_auto_, shadow_bias_, MonteCarloIntegrator::params_.ao_distance_, MonteCarloIntegrator::params_.ao_color_, MonteCarloIntegrator::params_.shadow_depth_);
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugObjectTime))
			{
				const float col_combined_gray = g_info.constant_randiance_.col2Bri();
				if(sp->hasMotionBlur()) *color_layer += {col_combined_gray * ray.time_, 0.f, col_combined_gray * (1.f - ray.time_), 1.f};
				else *color_layer += {0.f, col_combined_gray, 0.f, 1.f};
			}
		}
		if(MonteCarloIntegrator::params_.transparent_background_refraction_)
		{
			const float mat_alpha = sp->getAlpha(wo, image_film_->getCamera());
			alpha = mat_alpha + (1.f - mat_alpha) * alpha;
		}
		else alpha = 1.f;
	}
	else //nothing hit, return background
	{
		const auto [background_col, background_alpha] = background(ray, color_layers, MonteCarloIntegrator::params_.transparent_background_, MonteCarloIntegrator::params_.transparent_background_refraction_, background_, ray_level);
		g_info.constant_randiance_ = Rgba{background_col};
		alpha = background_alpha;
	}

	if(vol_integrator_)
	{
		applyVolumetricEffects(g_info.constant_randiance_, alpha, color_layers, ray, random_generator, *vol_integrator_, MonteCarloIntegrator::params_.transparent_background_);
	}
	g_info.constant_randiance_.a_ = alpha; // a small trick for just hold the alpha value.
	return g_info;
}

void SppmIntegrator::initializePpm()
{
	const unsigned int resolution = image_film_->getCamera()->resX() * image_film_->getCamera()->resY();
	hit_points_.reserve(resolution);
	// initialize SPPM statistics
	// Now using Scene Bound, this could get a bigger initial radius, and need more tests
	float initial_radius = ((scene_bound_.length(Axis::X) + scene_bound_.length(Axis::Y) + scene_bound_.length(Axis::Z)) / 3.f) / ((image_film_->getCamera()->resX() + image_film_->getCamera()->resY()) / 2.0f) * 2.f ;
	initial_radius = std::min(initial_radius, 1.f); //Fix the overflow bug
	for(unsigned int i = 0; i < resolution; i++)
	{
		HitPoint hp;
		hp.acc_photon_flux_  = Rgba(0.f);
		hp.acc_photon_count_ = 0;
		hp.radius_2_ = (initial_radius * initial_factor_) * (initial_radius * initial_factor_);
		hp.constant_randiance_ = Rgba(0.f);
		hp.radius_setted_ = false; // the flag used for IRE
		hit_points_.emplace_back(hp);
	}
	if(b_hashgrid_) photon_grid_.setParm(initial_radius * 2.f, n_photons_, scene_bound_);
}

int SppmIntegrator::setNumThreadsPhotons(int threads_photons)
{
	int result = threads_photons;
	if(threads_photons == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Active.");
		result = sysinfo::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported for Photon Mapping: [", result, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Inactive.");
	}
	logger_.logParams("Renderer '", getName(), "' using for Photon Mapping [", result, "] Threads.");
	return result;
}

} //namespace yafaray
