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
#include <cmath>
#include <memory>
#include "common/layers.h"
#include "background/background.h"
#include "geometry/surface.h"
#include "geometry/object/object.h"
#include "geometry/primitive/primitive.h"
#include "common/timer.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "camera/camera.h"
#include "render/progress_bar.h"
#include "sampler/sample.h"
#include "color/color_layers.h"
#include "render/render_data.h"
#include "image/image_output.h"
#include "accelerator/accelerator.h"
#include "photon/photon.h"

BEGIN_YAFARAY

void TiledIntegrator::renderWorker(ThreadControl *control, FastRandom &fast_random, std::vector<int> &correlative_sample_number, int thread_id, int samples, int offset, bool adaptive, int aa_pass, unsigned int object_index_highest, unsigned int material_index_highest)
{
	RenderArea a;

	while(image_film_->nextArea(render_view_, render_control_, a))
	{
		if(render_control_.canceled()) break;
		renderTile(fast_random, correlative_sample_number, a, samples, offset, adaptive, thread_id, aa_pass, object_index_highest, material_index_highest);

		std::unique_lock<std::mutex> lk(control->m_);
		control->areas_.emplace_back(a);
		control->c_.notify_one();

	}
	std::unique_lock<std::mutex> lk(control->m_);
	++(control->finished_threads_);
	control->c_.notify_one();
}

void TiledIntegrator::precalcDepths()
{
	if(camera_->getFarClip() > -1)
	{
		min_depth_ = camera_->getNearClip();
		max_depth_ = camera_->getFarClip();
	}
	else
	{
		// We sample the scene at render resolution to get the precision required for AA
		const int w = camera_->resX();
		const int h = camera_->resY();
		for(int i = 0; i < h; ++i)
		{
			for(int j = 0; j < w; ++j)
			{
				CameraRay camera_ray = camera_->shootRay(i, j, {0.5f, 0.5f});
				const auto [sp, tmax] = accelerator_->intersect(camera_ray.ray_, camera_);
				if(tmax > max_depth_) max_depth_ = tmax;
				if(tmax < min_depth_ && tmax >= 0.f) min_depth_ = tmax;
				camera_ray.ray_.tmax_ = tmax;
			}
		}
	}
	// we use the inverse multiplicative of the value aquired
	if(max_depth_ > 0.f) max_depth_ = 1.f / (max_depth_ - min_depth_);
}

bool TiledIntegrator::render(FastRandom &fast_random, unsigned int object_index_highest, unsigned int material_index_highest)
{
	std::stringstream pass_string;
	std::stringstream aa_settings;
	aa_settings << " passes=" << aa_noise_params_.passes_;
	aa_settings << " samples=" << aa_noise_params_.samples_ << " inc_samples=" << aa_noise_params_.inc_samples_ << " resamp.floor=" << aa_noise_params_.resampled_floor_ << "\nsample.mul=" << aa_noise_params_.sample_multiplier_factor_ << " light.sam.mul=" << aa_noise_params_.light_sample_multiplier_factor_ << " ind.sam.mul=" << aa_noise_params_.indirect_sample_multiplier_factor_ << "\ncol.noise=" << aa_noise_params_.detect_color_noise_;

	if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear) aa_settings << " AA thr(lin)=" << aa_noise_params_.threshold_ << ",dark_fac=" << aa_noise_params_.dark_threshold_factor_;
	else if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve) aa_settings << " AA.thr(curve)";
	else aa_settings << " AA thr=" << aa_noise_params_.threshold_;

	aa_settings << " var.edge=" << aa_noise_params_.variance_edge_size_ << " var.pix=" << aa_noise_params_.variance_pixels_ << " clamp=" << aa_noise_params_.clamp_samples_ << " ind.clamp=" << aa_noise_params_.clamp_indirect_;

	aa_noise_info_ += aa_settings.str();

	i_aa_passes_ = 1.f / (float) aa_noise_params_.passes_;

	render_control_.setTotalPasses(aa_noise_params_.passes_);

	aa_sample_multiplier_ = 1.f;
	aa_light_sample_multiplier_ = 1.f;
	aa_indirect_sample_multiplier_ = 1.f;

	int aa_resampled_floor_pixels = (int) floorf(aa_noise_params_.resampled_floor_ * (float) image_film_->getTotalPixels() / 100.f);

	logger_.logParams(getName(), ": Rendering ", aa_noise_params_.passes_, " passes");
	logger_.logParams("Min. ", aa_noise_params_.samples_, " samples");
	logger_.logParams(aa_noise_params_.inc_samples_, " per additional pass");
	logger_.logParams("Resampled pixels floor: ", aa_noise_params_.resampled_floor_, "% (", aa_resampled_floor_pixels, " pixels)");
	if(logger_.isVerbose())
	{
		logger_.logVerbose("AA_sample_multiplier_factor: ", aa_noise_params_.sample_multiplier_factor_);
		logger_.logVerbose("AA_light_sample_multiplier_factor: ", aa_noise_params_.light_sample_multiplier_factor_);
		logger_.logVerbose("AA_indirect_sample_multiplier_factor: ", aa_noise_params_.indirect_sample_multiplier_factor_);
		logger_.logVerbose("AA_detect_color_noise: ", aa_noise_params_.detect_color_noise_);

		if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear) logger_.logVerbose("AA_threshold (linear): ", aa_noise_params_.threshold_, ", dark factor: ", aa_noise_params_.dark_threshold_factor_);
		else if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve) logger_.logVerbose("AA_threshold (curve)");
		else logger_.logVerbose("AA threshold:", aa_noise_params_.threshold_);

		logger_.logVerbose("AA_variance_edge_size: ", aa_noise_params_.variance_edge_size_);
		logger_.logVerbose("AA_variance_pixels: ", aa_noise_params_.variance_pixels_);
		logger_.logVerbose("AA_clamp_samples: ", aa_noise_params_.clamp_samples_);
		logger_.logVerbose("AA_clamp_indirect: ", aa_noise_params_.clamp_indirect_);
	}
	logger_.logParams("Max. ", aa_noise_params_.samples_ + std::max(0, aa_noise_params_.passes_ - 1) * aa_noise_params_.inc_samples_, " total samples");

	pass_string << "Rendering pass 1 of " << std::max(1, aa_noise_params_.passes_) << "...";

	logger_.logInfo(pass_string.str());
	if(intpb_) intpb_->setTag(pass_string.str());

	timer_->addEvent("rendert");
	timer_->start("rendert");

	image_film_->init(render_control_, aa_noise_params_.passes_);
	image_film_->setAaNoiseParams(aa_noise_params_);

	if(render_control_.resumed())
	{
		pass_string.clear();
		pass_string << "Combining ImageFilm files, skipping pass 1...";
		if(intpb_) intpb_->setTag(pass_string.str());
	}

	logger_.logInfo(getName(), ": ", pass_string.str());

	max_depth_ = 0.f;
	min_depth_ = 1e38f;

	if(layers_->isDefinedAny({LayerDef::ZDepthNorm, LayerDef::Mist})) precalcDepths();

	std::vector<int> correlative_sample_number(num_threads_, 0);  //!< Used to sample lights more uniformly when using estimateOneDirectLight

	int resampled_pixels = 0;
	if(render_control_.resumed())
	{
		renderPass(fast_random, correlative_sample_number, 0, image_film_->getSamplingOffset(), false, 0, object_index_highest, material_index_highest);
	}
	else
		renderPass(fast_random, correlative_sample_number, aa_noise_params_.samples_, 0, false, 0, object_index_highest, material_index_highest);

	bool aa_threshold_changed = true;
	int acum_aa_samples = aa_noise_params_.samples_;

	for(int i = 1; i < aa_noise_params_.passes_; ++i)
	{
		if(render_control_.canceled()) break;

		aa_sample_multiplier_ *= aa_noise_params_.sample_multiplier_factor_;
		aa_light_sample_multiplier_ *= aa_noise_params_.light_sample_multiplier_factor_;
		aa_indirect_sample_multiplier_ *= aa_noise_params_.indirect_sample_multiplier_factor_;

		logger_.logInfo(getName(), ": Sample multiplier = ", aa_sample_multiplier_, ", Light Sample multiplier = ", aa_light_sample_multiplier_, ", Indirect Sample multiplier = ", aa_indirect_sample_multiplier_);

		image_film_->setAaNoiseParams(aa_noise_params_);

		if(resampled_pixels <= 0.f && !aa_threshold_changed)
		{
			logger_.logInfo(getName(), ": in previous pass there were 0 pixels to be resampled and the AA threshold did not change, so this pass resampling check and rendering will be skipped.");
			image_film_->nextPass(render_view_, render_control_, true, getName(), edge_toon_params_, /*skipNextPass=*/true);
		}
		else
		{
			image_film_->setAaThreshold(aa_noise_params_.threshold_);
			resampled_pixels = image_film_->nextPass(render_view_, render_control_, true, getName(), edge_toon_params_);
			aa_threshold_changed = false;
		}

		int aa_samples_mult = (int) ceilf(aa_noise_params_.inc_samples_ * aa_sample_multiplier_);

		if(logger_.isDebug())logger_.logDebug("acumAASamples=", acum_aa_samples, " AA_samples=", aa_noise_params_.samples_, " AA_samples_mult=", aa_samples_mult);

		if(resampled_pixels > 0) renderPass(fast_random, correlative_sample_number, aa_samples_mult, acum_aa_samples, true, i, object_index_highest, material_index_highest);

		acum_aa_samples += aa_samples_mult;

		if(resampled_pixels < aa_resampled_floor_pixels)
		{
			float aa_variation_ratio = std::min(8.f, ((float) aa_resampled_floor_pixels / resampled_pixels)); //This allows the variation for the new pass in the AA threshold and AA samples to depend, with a certain maximum per pass, on the ratio between how many pixeles were resampled and the target floor, to get a faster approach for noise removal.
			aa_noise_params_.threshold_ *= (1.f - 0.1f * aa_variation_ratio);

			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Resampled pixels (", resampled_pixels, ") below the floor (", aa_resampled_floor_pixels, "): new AA Threshold (-", aa_variation_ratio * 0.1f * 100.f, "%) for next pass = ", aa_noise_params_.threshold_);

			if(aa_noise_params_.threshold_ > 0.f) aa_threshold_changed = true;
		}
	}
	max_depth_ = 0.f;
	timer_->stop("rendert");
	render_control_.setFinished();
	logger_.logInfo(getName(), ": Overall rendertime: ", timer_->getTime("rendert"), "s");

	return true;
}


bool TiledIntegrator::renderPass(FastRandom &fast_random, std::vector<int> &correlative_sample_number, int samples, int offset, bool adaptive, int aa_pass_number, unsigned int object_index_highest, unsigned int material_index_highest)
{
	if(logger_.isDebug())logger_.logDebug("Sampling: samples=", samples, " Offset=", offset, " Base Offset=", + image_film_->getBaseSamplingOffset(), "  AA_pass_number=", aa_pass_number);

	prePass(fast_random, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive);

	render_control_.setCurrentPass(aa_pass_number + 1);

	image_film_->setSamplingOffset(offset + samples);

	ThreadControl tc;
	std::vector<std::thread> threads;
	threads.reserve(num_threads_);
	for(int i = 0; i < num_threads_; ++i) threads.emplace_back(&TiledIntegrator::renderWorker, this, &tc, std::ref(fast_random), std::ref(correlative_sample_number), i, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, aa_pass_number, object_index_highest, material_index_highest);

	std::unique_lock<std::mutex> lk(tc.m_);
	while(tc.finished_threads_ < num_threads_)
	{
		tc.c_.wait(lk);
		for(const auto &area : tc.areas_)
		{
			image_film_->finishArea(render_view_, render_control_, area, edge_toon_params_);
		}
		tc.areas_.clear();
	}

	for(auto &t : threads) t.join();	//join all threads (although they probably have exited already, but not necessarily):

	return true; //hm...quite useless the return value :)
}

bool TiledIntegrator::renderTile(FastRandom &fast_random, std::vector<int> &correlative_sample_number, const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number, unsigned int object_index_highest, unsigned int material_index_highest)
{
	const int camera_res_x = camera_->resX();
	RandomGenerator random_generator(rand() + offset * (camera_res_x * a.y_ + a.x_) + 123);
	const bool sample_lns = camera_->sampleLense();
	const int pass_offs = offset, end_x = a.x_ + a.w_, end_y = a.y_ + a.h_;
	int aa_max_possible_samples = aa_noise_params_.samples_;
	for(int i = 1; i < aa_noise_params_.passes_; ++i)
	{
		aa_max_possible_samples += ceilf(aa_noise_params_.inc_samples_ * pow(aa_noise_params_.sample_multiplier_factor_, i));	//DAVID FIXME: if the per-material sampling factor is used, values higher than 1.f will appear in the Sample Count render pass. Is that acceptable or not?
	}
	const float inv_aa_max_possible_samples = 1.f / static_cast<float>(aa_max_possible_samples);
	Uv<Halton> hal{Halton{3}, Halton{5}};
	ColorLayers color_layers(*layers_);
	const Image *sampling_factor_image_pass = (*image_film_->getImageLayers())(LayerDef::DebugSamplingFactor).image_.get();
	const int film_cx_0 = image_film_->getCx0();
	const int film_cy_0 = image_film_->getCy0();
	float d_1 = 1.f / static_cast<float>(n_samples);
	for(int i = a.y_; i < end_y; ++i)
	{
		for(int j = a.x_; j < end_x; ++j)
		{
			if(render_control_.canceled()) break;
			float mat_sample_factor = 1.f;
			int n_samples_adjusted = n_samples;
			if(adaptive)
			{
				if(!image_film_->doMoreSamples(j, i)) continue;
				if(sampling_factor_image_pass)
				{
					const float weight = image_film_->getWeight(j - film_cx_0, i - film_cy_0);
					mat_sample_factor = weight > 0.f ? sampling_factor_image_pass->getColor(j - film_cx_0, i - film_cy_0).normalized(weight).r_ : 1.f;
					if(image_film_->getBackgroundResampling()) mat_sample_factor = std::max(mat_sample_factor, 1.f); //If the background is set to be resampled, make sure the matSampleFactor is always >= 1.f
					if(mat_sample_factor > 0.f && mat_sample_factor < 1.f) mat_sample_factor = 1.f;	//This is to ensure in the edges between objects and background we always shoot samples, otherwise we might not shoot enough samples at the boundaries with the background where they are needed for antialiasing, however if the factor is equal to 0.f (as in the background) then no more samples will be shot
				}
				if(mat_sample_factor != 1.f)
				{
					n_samples_adjusted = static_cast<int>(std::round(static_cast<float>(n_samples) * mat_sample_factor));
					d_1 = 1.f / static_cast<float>(n_samples_adjusted);	//DAVID FIXME: is this correct???
				}
			}
			PixelSamplingData pixel_sampling_data;
			pixel_sampling_data.number_ = camera_res_x * i + j;
			pixel_sampling_data.offset_ = sample::fnv32ABuf(i * sample::fnv32ABuf(j)); //fnv_32a_buf(rstate.pixelNumber);
			const float toff = Halton::lowDiscrepancySampling(fast_random, 5, pass_offs + pixel_sampling_data.offset_); // **shall be just the pass number...**
			hal.u_.setStart(pass_offs + pixel_sampling_data.offset_);
			hal.v_.setStart(pass_offs + pixel_sampling_data.offset_);
			for(int sample = 0; sample < n_samples_adjusted; ++sample)
			{
				color_layers.setDefaultColors();
				pixel_sampling_data.sample_ = pass_offs + sample;

				const float time = time_forced_ ? time_forced_value_ : math::addMod1(static_cast<float>(sample) * d_1, toff); //(0.5+(float)sample)*d1;
				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA  //!< the current (normalized) frame time  //FIXME, time not currently used in libYafaRay
				pixel_sampling_data.time_ = time;
				float dx = 0.5f, dy = 0.5f;
				if(aa_noise_params_.passes_ > 1)
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
				CameraRay camera_ray = camera_->shootRay(static_cast<float>(j) + dx, static_cast<float>(i) + dy, lens_uv);
				if(!camera_ray.valid_)
				{
					image_film_->addSample(static_cast<float>(j), static_cast<float>(i), dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
					continue;
				}
				if(render_control_.getDifferentialRaysEnabled())
				{
					//setup ray differentials
					camera_ray.ray_.differentials_ = std::make_unique<RayDifferentials>();
					const CameraRay camera_diff_ray_x = camera_->shootRay(static_cast<float>(j) + 1 + dx, static_cast<float>(i) + dy, lens_uv);
					camera_ray.ray_.differentials_->xfrom_ = camera_diff_ray_x.ray_.from_;
					camera_ray.ray_.differentials_->xdir_ = camera_diff_ray_x.ray_.dir_;
					const CameraRay camera_diff_ray_y = camera_->shootRay(static_cast<float>(j) + dx, static_cast<float>(i) + 1 + dy, lens_uv);
					camera_ray.ray_.differentials_->yfrom_ = camera_diff_ray_y.ray_.from_;
					camera_ray.ray_.differentials_->ydir_ = camera_diff_ray_y.ray_.dir_;
				}
				camera_ray.ray_.time_ = time;
				RayDivision ray_division;
				const auto [integ_col, integ_alpha] = integrate(camera_ray.ray_, fast_random, random_generator, correlative_sample_number, &color_layers, thread_id, 0, true, 0.f, 0, ray_division, pixel_sampling_data, object_index_highest, material_index_highest);
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
							if(mask_params_.invert_) layer_col = Rgba(1.f) - layer_col;
							if(!mask_params_.only_)
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
							else layer_col = Rgba{1.f - (camera_ray.ray_.tmax_ - min_depth_) * max_depth_}; // Distance normalization
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
						case LayerDef::Mist:
							if(camera_ray.ray_.tmax_ < 0.f) layer_col = Rgba(0.f, 0.f); // Show background as fully transparent
							else layer_col = Rgba{(camera_ray.ray_.tmax_ - min_depth_) * max_depth_}; // Distance normalization
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
						default:
							if(layer_col.a_ > 1.f) layer_col.a_ = 1.f;
							break;
					}
				}
				image_film_->addSample(j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
			}
		}
	}
	return true;
}

void TiledIntegrator::generateCommonLayers(ColorLayers *color_layers, const SurfacePoint &sp, const MaskParams &mask_params, unsigned int object_index_highest, unsigned int material_index_highest)
{
	if(color_layers)
	{
		if(color_layers->getFlags().hasAny(LayerDef::Flags::DebugLayers))
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
				*color_layer = Rgba((sp.n_.x() + 1.f) * .5f, (sp.n_.y() + 1.f) * .5f, (sp.n_.z() + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::NormalGeom))
			{
				*color_layer = Rgba((sp.ng_.x() + 1.f) * .5f, (sp.ng_.y() + 1.f) * .5f, (sp.ng_.z() + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdu))
			{
				*color_layer = Rgba((sp.dp_.u_.x() + 1.f) * .5f, (sp.dp_.u_.y() + 1.f) * .5f, (sp.dp_.u_.z() + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdv))
			{
				*color_layer = Rgba((sp.dp_.v_.x() + 1.f) * .5f, (sp.dp_.v_.y() + 1.f) * .5f, (sp.dp_.v_.z() + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDsdu))
			{
				*color_layer = Rgba((sp.ds_.u_.x() + 1.f) * .5f, (sp.ds_.u_.y() + 1.f) * .5f, (sp.ds_.u_.z() + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugDsdv))
			{
				*color_layer = Rgba((sp.ds_.v_.x() + 1.f) * .5f, (sp.ds_.v_.y() + 1.f) * .5f, (sp.ds_.v_.z() + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugNu))
			{
				*color_layer = Rgba((sp.uvn_.u_.x() + 1.f) * .5f, (sp.uvn_.u_.y() + 1.f) * .5f, (sp.uvn_.u_.z() + 1.f) * .5f, 1.f);
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugNv))
			{
				*color_layer = Rgba((sp.uvn_.v_.x() + 1.f) * .5f, (sp.uvn_.v_.y() + 1.f) * .5f, (sp.uvn_.v_.z() + 1.f) * .5f, 1.f);
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
					if(sp.differentials_) *color_layer = Rgba((sp.differentials_->dp_dx_.x() + 1.f) * .5f, (sp.differentials_->dp_dx_.y() + 1.f) * .5f, (sp.differentials_->dp_dx_.z() + 1.f) * .5f, 1.f);
				}
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdy))
				{
					if(sp.differentials_) *color_layer = Rgba((sp.differentials_->dp_dy_.x() + 1.f) * .5f, (sp.differentials_->dp_dy_.y() + 1.f) * .5f, (sp.differentials_->dp_dy_.z() + 1.f) * .5f, 1.f);
				}
				if(Rgba *color_layer = color_layers->find(LayerDef::DebugDpdxy))
				{
					if(sp.differentials_) *color_layer = Rgba((sp.differentials_->dp_dx_.x() + sp.differentials_->dp_dy_.x() + 1.f) * .5f, (sp.differentials_->dp_dx_.y() + sp.differentials_->dp_dy_.y() + 1.f) * .5f, (sp.differentials_->dp_dx_.z() + sp.differentials_->dp_dy_.z() + 1.f) * .5f, 1.f);
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
		if(color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
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
		if(color_layers->getFlags().hasAny(LayerDef::Flags::IndexLayers))
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
				*color_layer = Rgba{static_cast<float>(sp.getObjectIndexAuto())};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexAbs))
			{
				*color_layer = Rgba{static_cast<float>(sp.getMaterial()->getIndex())};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexNorm))
			{
				*color_layer = Rgba{static_cast<float>(sp.getMaterial()->getIndex()) / material_index_highest};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexAuto))
			{
				*color_layer = Rgba{sp.getMaterial()->getIndexAutoColor()};
			}
			if(Rgba *color_layer = color_layers->find(LayerDef::MatIndexAutoAbs))
			{
				*color_layer = Rgba{static_cast<float>(sp.getMaterial()->getIndexAuto())};
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
				if(sp.getMaterial()->getIndex() == mask_params.mat_index_) *color_layer = Rgba(1.f);
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

void TiledIntegrator::generateOcclusionLayers(ColorLayers *color_layers, const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, const SurfacePoint &sp, const Vec3 &wo, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth)
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

Rgb TiledIntegrator::sampleAmbientOcclusion(const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, bool transparent_shadows, bool clay, int ao_samples, bool shadow_bias_auto, float shadow_bias, float ao_dist, const Rgb &ao_col, int transp_shadows_depth)
{
	Rgb col{0.f};
	const BsdfFlags &mat_bsdfs = sp.mat_data_->bsdf_flags_;
	Ray light_ray{sp.p_, Vec3{0.f}, sp.intersect_data_.time()};
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
		if(mat_bsdfs.hasAny(BsdfFlags::Emit))
		{
			col += sp.emit(wo) * s.pdf_;
		}
		bool shadowed = false;
		Rgb scol {0.f};
		const Primitive *shadow_casting_primitive = nullptr;
		if(transparent_shadows) std::tie(shadowed, scol, shadow_casting_primitive) = accelerator.isShadowed(light_ray, transp_shadows_depth, shadow_bias, camera);
		else std::tie(shadowed, shadow_casting_primitive) = accelerator.isShadowed(light_ray, shadow_bias);
		if(!shadowed)
		{
			const float cos = std::abs(sp.n_ * light_ray.dir_);
			if(transparent_shadows) col += ao_col * scol * surf_col * cos * w;
			else col += ao_col * surf_col * cos * w;
		}
	}
	return col / static_cast<float>(n);
}

void TiledIntegrator::applyVolumetricEffects(Rgb &col, float &alpha, ColorLayers *color_layers, const Ray &ray, RandomGenerator &random_generator, const VolumeIntegrator *volume_integrator, bool transparent_background)
{
	const Rgb col_vol_transmittance = volume_integrator->transmittance(random_generator, ray);
	const Rgb col_vol_integration = volume_integrator->integrate(random_generator, ray);
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
		return {col, 1.f};
	}
	else return {Rgb{0.f}, 1.f};
}


END_YAFARAY
