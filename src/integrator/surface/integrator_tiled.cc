/****************************************************************************
 *      integrator.cc: Basic tile based surface integrator
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
 *      Copyright (C) 2009  Rodrigo Placencia (DarkTide)
 *      Previous code might belong to:
 *      Alejandro Conty (jandro)
 *      Alfredo Greef (eshlo)
 *      Others?
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

#include "integrator/surface/integrator_tiled.h"
#include "integrator/volume/integrator_volume.h"
#include "common/layers.h"
#include "background/background.h"
#include "geometry/surface.h"
#include "geometry/primitive/primitive.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "camera/camera.h"
#include "sampler/sample.h"
#include "color/color_layers.h"
#include "render/render_data.h"
#include "accelerator/accelerator.h"
#include "photon/photon.h"
#include "param/param.h"
#include "material/sample.h"
#include "render/render_monitor.h"

namespace yafaray {

void TiledIntegrator::renderWorker(ThreadControl *control, std::vector<int> &correlative_sample_number, int thread_id, int samples, int offset, bool adaptive, int aa_pass, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const RenderMonitor &render_monitor, const RenderControl &render_control)
{
	RenderArea a;

	while(image_film_->nextArea(a))
	{
		if(render_control.canceled()) break;
		renderTile(correlative_sample_number, a, samples, offset, adaptive, thread_id, aa_pass, aa_light_sample_multiplier, aa_indirect_sample_multiplier, render_monitor, render_control);

		std::unique_lock<std::mutex> lk(control->m_);
		control->areas_.emplace_back(a);
		control->c_.notify_one();

	}
	std::unique_lock<std::mutex> lk(control->m_);
	++(control->finished_threads_);
	control->c_.notify_one();
}

void TiledIntegrator::precalcDepths() const
{
	float min_depth = 0.f;
	float max_depth = std::numeric_limits<float>::max();
	const auto camera{image_film_->getCamera()};
	if(camera->getFarClip() > -1.f)
	{
		min_depth = camera->getNearClip();
		max_depth = camera->getFarClip();
	}
	else
	{
		// We sample the scene at render resolution to get the precision required for AA
		const int w = camera->resX();
		const int h = camera->resY();
		for(int i = 0; i < h; ++i)
		{
			for(int j = 0; j < w; ++j)
			{
				CameraRay camera_ray = camera->shootRay(i, j, {0.5f, 0.5f});
				const auto [sp, tmax] = accelerator_->intersect(camera_ray.ray_, camera);
				if(tmax > max_depth) max_depth = tmax;
				if(tmax < min_depth && tmax >= 0.f) min_depth = tmax;
				camera_ray.ray_.tmax_ = tmax;
			}
		}
	}
	// we use the inverse multiplicative of the value aquired
	if(max_depth > 0.f) max_depth = 1.f / (max_depth - min_depth);
	image_film_->setMinDepth(min_depth);
	image_film_->setMaxDepthInverse(max_depth);
}

bool TiledIntegrator::render(RenderControl &render_control, RenderMonitor &render_monitor)
{
	std::stringstream pass_string;
	std::stringstream aa_settings;
	aa_settings << " passes=" << aa_noise_params_->passes_;
	aa_settings << " samples=" << aa_noise_params_->samples_ << " inc_samples=" << aa_noise_params_->inc_samples_ << " resamp.floor=" << aa_noise_params_->resampled_floor_ << "\nsample.mul=" << aa_noise_params_->sample_multiplier_factor_ << " light.sam.mul=" << aa_noise_params_->light_sample_multiplier_factor_ << " ind.sam.mul=" << aa_noise_params_->indirect_sample_multiplier_factor_ << "\ncol.noise=" << aa_noise_params_->detect_color_noise_;

	if(aa_noise_params_->dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear) aa_settings << " AA thr(lin)=" << aa_noise_params_->threshold_ << ",dark_fac=" << aa_noise_params_->dark_threshold_factor_;
	else if(aa_noise_params_->dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve) aa_settings << " AA.thr(curve)";
	else aa_settings << " AA thr=" << aa_noise_params_->threshold_;

	aa_settings << " var.edge=" << aa_noise_params_->variance_edge_size_ << " var.pix=" << aa_noise_params_->variance_pixels_ << " clamp=" << aa_noise_params_->clamp_samples_ << " ind.clamp=" << aa_noise_params_->clamp_indirect_;
	render_monitor.setAaNoiseInfo(render_monitor.getAaNoiseInfo() + aa_settings.str());
	render_monitor.setTotalPasses(aa_noise_params_->passes_);

	int aa_resampled_floor_pixels = (int) floorf(aa_noise_params_->resampled_floor_ * (float) image_film_->getTotalPixels() / 100.f);

	logger_.logParams(getName(), ": Rendering ", aa_noise_params_->passes_, " passes");
	logger_.logParams("Min. ", aa_noise_params_->samples_, " samples");
	logger_.logParams(aa_noise_params_->inc_samples_, " per additional pass");
	logger_.logParams("Resampled pixels floor: ", aa_noise_params_->resampled_floor_, "% (", aa_resampled_floor_pixels, " pixels)");
	if(logger_.isVerbose())
	{
		logger_.logVerbose("AA_sample_multiplier_factor: ", aa_noise_params_->sample_multiplier_factor_);
		logger_.logVerbose("AA_light_sample_multiplier_factor: ", aa_noise_params_->light_sample_multiplier_factor_);
		logger_.logVerbose("AA_indirect_sample_multiplier_factor: ", aa_noise_params_->indirect_sample_multiplier_factor_);
		logger_.logVerbose("AA_detect_color_noise: ", aa_noise_params_->detect_color_noise_);

		if(aa_noise_params_->dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear) logger_.logVerbose("AA_threshold (linear): ", aa_noise_params_->threshold_, ", dark factor: ", aa_noise_params_->dark_threshold_factor_);
		else if(aa_noise_params_->dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve) logger_.logVerbose("AA_threshold (curve)");
		else logger_.logVerbose("AA threshold:", aa_noise_params_->threshold_);

		logger_.logVerbose("AA_variance_edge_size: ", aa_noise_params_->variance_edge_size_);
		logger_.logVerbose("AA_variance_pixels: ", aa_noise_params_->variance_pixels_);
		logger_.logVerbose("AA_clamp_samples: ", aa_noise_params_->clamp_samples_);
		logger_.logVerbose("AA_clamp_indirect: ", aa_noise_params_->clamp_indirect_);
	}
	logger_.logParams("Max. ", aa_noise_params_->samples_ + std::max(0, aa_noise_params_->passes_ - 1) * aa_noise_params_->inc_samples_, " total samples");

	pass_string << "Rendering pass 1 of " << std::max(1, aa_noise_params_->passes_) << "...";

	logger_.logInfo(pass_string.str());
	render_monitor.setProgressBarTag(pass_string.str());

	render_monitor.addTimerEvent("rendert");
	render_monitor.startTimer("rendert");

	image_film_->init(render_control, render_monitor, *this);

	if(render_control.resumed())
	{
		pass_string.clear();
		pass_string << "Combining ImageFilm files, skipping pass 1...";
		render_monitor.setProgressBarTag(pass_string.str());
	}

	logger_.logInfo(getName(), ": ", pass_string.str());

	if(image_film_->getLayers()->isDefinedAny({LayerDef::ZDepthNorm, LayerDef::Mist})) precalcDepths();

	std::vector<int> correlative_sample_number(num_threads_, 0);  //!< Used to sample lights more uniformly when using estimateOneDirectLight

	int resampled_pixels = 0;
	float aa_sample_multiplier = 1.f;
	float aa_light_sample_multiplier = 1.f;
	float aa_indirect_sample_multiplier = 1.f;

	if(render_control.resumed())
	{
		renderPass(render_control, render_monitor, correlative_sample_number, 0, image_film_->getSamplingOffset(), false, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);
	}
	else
		renderPass(render_control, render_monitor, correlative_sample_number, aa_noise_params_->samples_, 0, false, 0, aa_light_sample_multiplier, aa_indirect_sample_multiplier);

	bool aa_threshold_changed = true;
	int acum_aa_samples = aa_noise_params_->samples_;
	image_film_->setAaThresholdCalculated(aa_noise_params_->threshold_);

	for(int i = 1; i < aa_noise_params_->passes_; ++i)
	{
		if(render_control.canceled()) break;

		aa_sample_multiplier *= aa_noise_params_->sample_multiplier_factor_;
		aa_light_sample_multiplier *= aa_noise_params_->light_sample_multiplier_factor_;
		aa_indirect_sample_multiplier *= aa_noise_params_->indirect_sample_multiplier_factor_;

		logger_.logInfo(getName(), ": Sample multiplier = ", aa_sample_multiplier, ", Light Sample multiplier = ", aa_light_sample_multiplier, ", Indirect Sample multiplier = ", aa_indirect_sample_multiplier);

		if(resampled_pixels <= 0.f && !aa_threshold_changed)
		{
			logger_.logInfo(getName(), ": in previous pass there were 0 pixels to be resampled and the AA threshold did not change, so this pass resampling check and rendering will be skipped.");
			image_film_->nextPass(render_control, render_monitor, true, getName(), /*skipNextPass=*/true);
		}
		else
		{
			resampled_pixels = image_film_->nextPass(render_control, render_monitor, true, getName());
			aa_threshold_changed = false;
		}

		int aa_samples_mult = (int) ceilf(aa_noise_params_->inc_samples_ * aa_sample_multiplier);

		if(logger_.isDebug())logger_.logDebug("acumAASamples=", acum_aa_samples, " AA_samples=", aa_noise_params_->samples_, " AA_samples_mult=", aa_samples_mult);

		if(resampled_pixels > 0) renderPass(render_control, render_monitor, correlative_sample_number, aa_samples_mult, acum_aa_samples, true, i, aa_light_sample_multiplier, aa_indirect_sample_multiplier);

		acum_aa_samples += aa_samples_mult;

		if(resampled_pixels < aa_resampled_floor_pixels)
		{
			float aa_variation_ratio = std::min(8.f, ((float) aa_resampled_floor_pixels / resampled_pixels)); //This allows the variation for the new pass in the AA threshold and AA samples to depend, with a certain maximum per pass, on the ratio between how many pixeles were resampled and the target floor, to get a faster approach for noise removal.
			image_film_->setAaThresholdCalculated(image_film_->getAaThresholdCalculated() * (1.f - 0.1f * aa_variation_ratio));

			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Resampled pixels (", resampled_pixels, ") below the floor (", aa_resampled_floor_pixels, "): new AA Threshold (-", aa_variation_ratio * 0.1f * 100.f, "%) for next pass = ", aa_noise_params_->threshold_);

			if(aa_noise_params_->threshold_ > 0.f) aa_threshold_changed = true;
		}
	}
	render_monitor.stopTimer("rendert");
	render_control.setFinished();
	logger_.logInfo(getName(), ": Overall rendertime: ", render_monitor.getTimerTime("rendert"), "s");
	return true;
}


bool TiledIntegrator::renderPass(RenderControl &render_control, RenderMonitor &render_monitor, std::vector<int> &correlative_sample_number, int samples, int offset, bool adaptive, int aa_pass_number, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier)
{
	if(logger_.isDebug())logger_.logDebug("Sampling: samples=", samples, " Offset=", offset, " Base Offset=", + image_film_->getBaseSamplingOffset(), "  AA_pass_number=", aa_pass_number);

	prePass(render_control, render_monitor, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive);

	render_monitor.setCurrentPass(aa_pass_number + 1);

	image_film_->setSamplingOffset(offset + samples);

	ThreadControl tc;
	std::vector<std::thread> threads;
	threads.reserve(num_threads_);
	for(int i = 0; i < num_threads_; ++i) threads.emplace_back(&TiledIntegrator::renderWorker, this, &tc, std::ref(correlative_sample_number), i, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, aa_pass_number, aa_light_sample_multiplier, aa_indirect_sample_multiplier, std::ref(render_monitor), std::ref(render_control));

	std::unique_lock<std::mutex> lk(tc.m_);
	while(tc.finished_threads_ < num_threads_)
	{
		tc.c_.wait(lk);
		for(const auto &area : tc.areas_)
		{
			image_film_->finishArea(render_control, render_monitor, area);
		}
		tc.areas_.clear();
	}

	for(auto &t : threads) t.join();	//join all threads (although they probably have exited already, but not necessarily):

	return true; //hm...quite useless the return value :)
}

bool TiledIntegrator::renderTile(std::vector<int> &correlative_sample_number, const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const RenderMonitor &render_monitor, const RenderControl &render_control)
{
	const int camera_res_x = image_film_->getCamera()->resX();
	RandomGenerator random_generator(rand() + offset * (camera_res_x * a.y_ + a.x_) + 123);
	const bool sample_lns = image_film_->getCamera()->sampleLens();
	const int pass_offs = offset, end_x = a.x_ + a.w_, end_y = a.y_ + a.h_;
	int aa_max_possible_samples = aa_noise_params_->samples_;
	for(int i = 1; i < aa_noise_params_->passes_; ++i)
	{
		aa_max_possible_samples += ceilf(aa_noise_params_->inc_samples_ * pow(aa_noise_params_->sample_multiplier_factor_, i));	//DAVID FIXME: if the per-material sampling factor is used, values higher than 1.f will appear in the Sample Count render pass. Is that acceptable or not?
	}
	const float inv_aa_max_possible_samples = 1.f / static_cast<float>(aa_max_possible_samples);
	Uv<Halton> hal{Halton{3}, Halton{5}};
	ColorLayers color_layers(*image_film_->getLayers());
	const Image *sampling_factor_image_pass = (*image_film_->getImageLayers())(LayerDef::DebugSamplingFactor).image_.get();
	const int film_cx_0 = image_film_->getCx0();
	const int film_cy_0 = image_film_->getCy0();
	float d_1 = 1.f / static_cast<float>(n_samples);
	for(int i = a.y_; i < end_y; ++i)
	{
		for(int j = a.x_; j < end_x; ++j)
		{
			if(render_control.canceled()) break;
			float mat_sample_factor = 1.f;
			int n_samples_adjusted = n_samples;
			if(adaptive)
			{
				if(!image_film_->doMoreSamples({{j, i}})) continue;
				if(sampling_factor_image_pass)
				{
					const float weight = image_film_->getWeight({{j - film_cx_0, i - film_cy_0}});
					mat_sample_factor = weight > 0.f ? sampling_factor_image_pass->getColor({{j - film_cx_0, i - film_cy_0}}).normalized(weight).r_ : 1.f;
					if(image_film_->getBackgroundResampling()) mat_sample_factor = std::max(mat_sample_factor, 1.f); //If the background is set to be resampled, make sure the matSampleFactor is always >= 1.f
					if(mat_sample_factor > 0.f && mat_sample_factor < 1.f) mat_sample_factor = 1.f;	//This is to ensure in the edges between objects and background we always shoot samples, otherwise we might not shoot enough samples at the boundaries with the background where they are needed for antialiasing, however if the factor is equal to 0.f (as in the background) then no more samples will be shot
				}
				if(mat_sample_factor != 1.f)
				{
					n_samples_adjusted = static_cast<int>(std::round(static_cast<float>(n_samples) * mat_sample_factor));
					d_1 = 1.f / static_cast<float>(n_samples_adjusted);	//DAVID FIXME: is this correct???
				}
			}
			PixelSamplingData pixel_sampling_data{
					thread_id,
					camera_res_x * i + j,
					sample::fnv32ABuf(i * sample::fnv32ABuf(j)), //fnv_32a_buf(rstate.pixelNumber);
					aa_light_sample_multiplier,
					aa_indirect_sample_multiplier
			};
			const float toff = Halton::lowDiscrepancySampling(fast_random_, 5, pass_offs + pixel_sampling_data.offset_); // **shall be just the pass number...**
			hal.u_.setStart(pass_offs + pixel_sampling_data.offset_);
			hal.v_.setStart(pass_offs + pixel_sampling_data.offset_);
			for(int sample = 0; sample < n_samples_adjusted; ++sample)
			{
				color_layers.setDefaultColors();
				pixel_sampling_data.sample_ = pass_offs + sample;

				const float time = TiledIntegrator::params_.time_forced_ ? TiledIntegrator::params_.time_forced_value_ : math::addMod1(static_cast<float>(sample) * d_1, toff); //(0.5+(float)sample)*d1;
				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA  //!< the current (normalized) frame time  //FIXME, time not currently used in libYafaRay
				pixel_sampling_data.time_ = time;
				float dx = 0.5f, dy = 0.5f;
				if(aa_noise_params_->passes_ > 1)
				{
					dx = sample::riVdC(pixel_sampling_data.sample_, pixel_sampling_data.offset_);
					dy = sample::riS(pixel_sampling_data.sample_, pixel_sampling_data.offset_);
				}
				else if(n_samples_adjusted > 1)
				{
					dx = (0.5f + static_cast<float>(sample)) * d_1;
					dy = sample::riLp(sample + pixel_sampling_data.offset_);
				}
				Uv<float> lens_uv{0.5f, 0.5f};
				if(sample_lns)
				{
					lens_uv = {hal.u_.getNext(), hal.v_.getNext()};
				}
				CameraRay camera_ray = image_film_->getCamera()->shootRay(static_cast<float>(j) + dx, static_cast<float>(i) + dy, lens_uv);
				if(!camera_ray.valid_)
				{
					image_film_->addSample({{j, i}}, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
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
				}
				camera_ray.ray_.time_ = time;
				RayDivision ray_division;
				const auto [integ_col, integ_alpha] = integrate(camera_ray.ray_, random_generator, correlative_sample_number, &color_layers, 0, true, 0.f, 0, ray_division, pixel_sampling_data);
				color_layers(LayerDef::Combined) = {integ_col, integ_alpha};
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
							if(image_film_->getMaskParams()->invert_) layer_col = Rgba(1.f) - layer_col;
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

void TiledIntegrator::generateCommonLayers(ColorLayers *color_layers, const SurfacePoint &sp, const MaskParams &mask_params, unsigned int object_index_highest, unsigned int material_index_highest)
{
	if(color_layers)
	{
		if(color_layers->getFlags().has(LayerDef::Flags::DebugLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Uv))
			{
				*color_layer = Rgba(sp.uv_.u_, sp.uv_.v_, 0.f, 1.f);
			}
			/* FIXME: probably does not make sense when we have now also quads in addition to triangles...
			if(Rgba *color_layer = color_layers->find(LayerDef::BarycentricUvw))
			{
				*color_layer = Rgba(sp.intersect_data_.u_, sp.intersect_data_.v_, sp.intersect_data_.barycentric_w_, 1.f);
			}*/
			if(Rgba *color_layer = color_layers->find(LayerDef::NormalSmooth))
			{
				*color_layer = Rgba((sp.n_[Axis::X] + 1.f) * .5f, (sp.n_[Axis::Y] + 1.f) * .5f, (sp.n_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::NormalGeom))
			{
				*color_layer = Rgba((sp.ng_[Axis::X] + 1.f) * .5f, (sp.ng_[Axis::Y] + 1.f) * .5f, (sp.ng_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdu))
			{
				*color_layer = Rgba((sp.dp_.u_[Axis::X] + 1.f) * .5f, (sp.dp_.u_[Axis::Y] + 1.f) * .5f, (sp.dp_.u_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdv))
			{
				*color_layer = Rgba((sp.dp_.v_[Axis::X] + 1.f) * .5f, (sp.dp_.v_[Axis::Y] + 1.f) * .5f, (sp.dp_.v_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDsdu))
			{
				*color_layer = Rgba((sp.ds_.u_[Axis::X] + 1.f) * .5f, (sp.ds_.u_[Axis::Y] + 1.f) * .5f, (sp.ds_.u_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDsdv))
			{
				*color_layer = Rgba((sp.ds_.v_[Axis::X] + 1.f) * .5f, (sp.ds_.v_[Axis::Y] + 1.f) * .5f, (sp.ds_.v_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugNu))
			{
				*color_layer = Rgba((sp.uvn_.u_[Axis::X] + 1.f) * .5f, (sp.uvn_.u_[Axis::Y] + 1.f) * .5f, (sp.uvn_.u_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugNv))
			{
				*color_layer = Rgba((sp.uvn_.v_[Axis::X] + 1.f) * .5f, (sp.uvn_.v_[Axis::Y] + 1.f) * .5f, (sp.uvn_.v_[Axis::Z] + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugWireframe))
			{
				Rgba wireframe_color = Rgba(0.f, 0.f, 0.f, 0.f);
				sp.getMaterial()->applyWireFrame(wireframe_color, 1.f, sp);
				*color_layer = wireframe_color;
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugSamplingFactor))
			{
				*color_layer = Rgba(sp.getMaterial()->getSamplingFactor());
			}
			if(color_layers->isDefinedAny({LayerDef::DebugDpLengths, LayerDef::DebugDpdx, LayerDef::DebugDpdy, LayerDef::DebugDpdxy, LayerDef::DebugDudxDvdx, LayerDef::DebugDudyDvdy, LayerDef::DebugDudxyDvdxy}))
			{
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpLengths))
				{
					if(sp.differentials_) *color_layer = Rgba(sp.differentials_->dp_dx_.length(), sp.differentials_->dp_dy_.length(), 0.f, 1.f);
				}
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdx))
				{
					if(sp.differentials_) *color_layer = Rgba((sp.differentials_->dp_dx_[Axis::X] + 1.f) * .5f, (sp.differentials_->dp_dx_[Axis::Y] + 1.f) * .5f, (sp.differentials_->dp_dx_[Axis::Z] + 1.f) * .5f, 1.f);
				}
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdy))
				{
					if(sp.differentials_) *color_layer = Rgba((sp.differentials_->dp_dy_[Axis::X] + 1.f) * .5f, (sp.differentials_->dp_dy_[Axis::Y] + 1.f) * .5f, (sp.differentials_->dp_dy_[Axis::Z] + 1.f) * .5f, 1.f);
				}
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdxy))
				{
					if(sp.differentials_) *color_layer = Rgba((sp.differentials_->dp_dx_[Axis::X] + sp.differentials_->dp_dy_[Axis::X] + 1.f) * .5f, (sp.differentials_->dp_dx_[Axis::Y] + sp.differentials_->dp_dy_[Axis::Y] + 1.f) * .5f, (sp.differentials_->dp_dx_[Axis::Z] + sp.differentials_->dp_dy_[Axis::Z] + 1.f) * .5f, 1.f);
				}
				if(color_layers->isDefinedAny({LayerDef::DebugDudxDvdx, LayerDef::DebugDudyDvdy, LayerDef::DebugDudxyDvdxy}))
				{
					const auto [d_dx, d_dy]{sp.getUVdifferentialsXY()};
					if(Rgba *color_layer = color_layers->find(LayerDef::DebugDudxDvdx))
					{
						*color_layer = Rgba((d_dx.u_ + 1.f) * .5f, (d_dx.v_ + 1.f) * .5f, 0.f, 1.f);
					}
					if(Rgba *color_layer = color_layers->find(LayerDef::DebugDudyDvdy))
					{
						*color_layer = Rgba((d_dy.u_ + 1.f) * .5f, (d_dy.v_ + 1.f) * .5f, 0.f, 1.f);
					}
					if(Rgba *color_layer = color_layers->find(LayerDef::DebugDudxyDvdxy))
					{
						*color_layer = Rgba((d_dx.u_ + d_dy.u_ + 1.f) * .5f, (d_dx.v_ + d_dy.v_ + 1.f) * .5f, 0.f, 1.f);
					}
				}
			}
		}
		if(color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::ReflectAll))
			{
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::ReflectPerfect))
				{
					*color_layer += *color_layer_2;
				}
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::Glossy))
				{
					*color_layer += *color_layer_2;
				}
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::GlossyIndirect))
				{
					*color_layer += *color_layer_2;
				}
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::RefractAll))
			{
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::RefractPerfect))
				{
					*color_layer += *color_layer_2;
				}
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::Trans))
				{
					*color_layer += *color_layer_2;
				}
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::TransIndirect))
				{
					*color_layer += *color_layer_2;
				}
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::IndirectAll))
			{
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::Indirect))
				{
					*color_layer += *color_layer_2;
				}
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::DiffuseIndirect))
				{
					*color_layer += *color_layer_2;
				}
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseColor))
			{
				*color_layer = Rgba{sp.getMaterial()->getDiffuseColor(sp.mat_data_->node_tree_data_)};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::GlossyColor))
			{
				*color_layer = Rgba{sp.getMaterial()->getGlossyColor(sp.mat_data_->node_tree_data_)};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::TransColor))
			{
				*color_layer = Rgba{sp.getMaterial()->getTransColor(sp.mat_data_->node_tree_data_)};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::SubsurfaceColor))
			{
				*color_layer = Rgba{sp.getMaterial()->getSubSurfaceColor(sp.mat_data_->node_tree_data_)};
			}
		}
		if(color_layers->getFlags().has(LayerDef::Flags::IndexLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexAbs))
			{
				*color_layer = Rgba{static_cast<float>(sp.getObjectIndex())};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexNorm))
			{
				*color_layer = Rgba{static_cast<float>(sp.getObjectIndex()) / object_index_highest};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexAuto))
			{
				*color_layer = Rgba{sp.getObjectIndexAutoColor()};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexAutoAbs))
			{
				*color_layer = Rgba{static_cast<float>(sp.getObjectId() + 1)};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexAbs))
			{
				*color_layer = Rgba{static_cast<float>(sp.getMaterial()->getPassIndex())};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexNorm))
			{
				*color_layer = Rgba{static_cast<float>(sp.getMaterial()->getPassIndex()) / material_index_highest};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexAuto))
			{
				*color_layer = Rgba{sp.getMaterial()->getPassIndexAutoColor()};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexAutoAbs))
			{
				*color_layer = Rgba{static_cast<float>(sp.getMaterial()->getId() + 1)};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexMask))
			{
				if(sp.getObjectIndex() == mask_params.obj_index_) *color_layer = Rgba(1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::ObjIndexMaskAll))
			{
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::ObjIndexMask))
				{
					*color_layer += *color_layer_2;
				}
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::ObjIndexMaskShadow))
				{
					*color_layer += *color_layer_2;
				}
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexMask))
			{
				if(sp.getMaterial()->getPassIndex() == mask_params.mat_index_) *color_layer = Rgba(1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexMaskAll))
			{
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::MatIndexMask))
				{
					*color_layer += *color_layer_2;
				}
				if(Rgba *color_layer_2 = color_layers->find(LayerDef::MatIndexMaskShadow))
				{
					*color_layer += *color_layer_2;
				}
			}
		}
	}
}

void TiledIntegrator::generateOcclusionLayers(ColorLayers *color_layers, const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const SurfacePoint &sp, const Vec3f &wo, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth)
{
	if(Rgba *color_layer = color_layers->find(LayerDef::Ao))
	{
		*color_layer += sampleAmbientOcclusion(accelerator, chromatic_enabled, wavelength, sp, wo, ray_division, camera, pixel_sampling_data, false, false, ao_samples, shadow_bias_auto, shadow_bias, ao_dist, ao_col, transp_shadows_depth);
	}
	if(Rgba *color_layer = color_layers->find(LayerDef::AoClay))
	{
		*color_layer += sampleAmbientOcclusion(accelerator, chromatic_enabled, wavelength, sp, wo, ray_division, camera, pixel_sampling_data, false, true, ao_samples, shadow_bias_auto, shadow_bias, ao_dist, ao_col, transp_shadows_depth);
	}
}

Rgb TiledIntegrator::sampleAmbientOcclusion(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, bool transparent_shadows, bool clay, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth)
{
	Rgb col{0.f};
	const BsdfFlags mat_bsdfs = sp.mat_data_->bsdf_flags_;
	Ray light_ray{sp.p_, Vec3f{0.f}, sp.time_};
	int n = ao_samples;//(int) ceilf(aoSamples*getSampleMultiplier());
	if(ray_division.division_ > 1) n = std::max(1, n / ray_division.division_);
	const unsigned int offs = n * pixel_sampling_data.sample_ + pixel_sampling_data.offset_;
	Halton hal_2(2, offs - 1);
	Halton hal_3(3, offs - 1);
	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();
		if(ray_division.division_ > 1)
		{
			s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
			s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
		}
		if(shadow_bias_auto) light_ray.tmin_ = shadow_bias * std::max(1.f, sp.p_.length());
		else light_ray.tmin_ = shadow_bias;
		light_ray.tmax_ = ao_dist;
		float w = 0.f;
		const BsdfFlags sample_flags = clay ?
									   BsdfFlags::All :
									   BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Reflect;
		Sample s(s_1, s_2, sample_flags);
		const Rgb surf_col = clay ?
							 Material::sampleClay(sp, wo, light_ray.dir_, s, w) :
							 sp.sample(wo, light_ray.dir_, s, w, chromatic_enabled, wavelength, camera);
		if(clay) s.pdf_ = 1.f;
		if(mat_bsdfs.has(BsdfFlags::Emit))
		{
			col += sp.emit(wo) * s.pdf_;
		}
		bool shadowed = false;
		Rgb scol {0.f};
		const Primitive *shadow_casting_primitive = nullptr;
		if(transparent_shadows) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator.isShadowedTransparentShadow(light_ray, transp_shadows_depth, camera);
		else std::tie(shadowed, shadow_casting_primitive) = accelerator.isShadowed(light_ray);
		if(!shadowed)
		{
			const float cos = std::abs(sp.n_ * light_ray.dir_);
			if(transparent_shadows) col += ao_col * scol * surf_col * cos * w;
			else col += ao_col * surf_col * cos * w;
		}
	}
	return col / static_cast<float>(n);
}

void TiledIntegrator::applyVolumetricEffects(Rgb &col, float &alpha, ColorLayers *color_layers, const Ray &ray, RandomGenerator &random_generator, const VolumeIntegrator &volume_integrator, bool transparent_background)
{
	const Rgb col_vol_transmittance = volume_integrator.transmittance(random_generator, ray);
	const Rgb col_vol_integration = volume_integrator.integrate(random_generator, ray);
	if(transparent_background) alpha = std::max(alpha, 1.f - col_vol_transmittance.r_);
	if(color_layers)
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::VolumeTransmittance)) *color_layer = Rgba{col_vol_transmittance};
		if(Rgba *color_layer = color_layers->find(LayerDef::VolumeIntegration)) *color_layer = Rgba{col_vol_integration};
	}
	col = (col * col_vol_transmittance) + col_vol_integration;
}

std::pair<Rgb, float> TiledIntegrator::background(const Ray &ray, ColorLayers *color_layers, bool transparent_background, bool transparent_refracted_background, const Background *background, int ray_level)
{
	if(transparent_background && (ray_level == 0 || transparent_refracted_background)) return {Rgb{0.f}, 0.f};
	else if(background)
	{
		const Rgb col = (*background)(ray.dir_);
		if(color_layers)
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Env)) *color_layer = Rgba{col};
		}
		return {std::move(col), 1.f};
	}
	else return {Rgb{0.f}, 1.f};
}


} //namespace yafaray
