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
#include "common/layers.h"
#include "background/background.h"
#include "geometry/surface.h"
#include "geometry/object/object.h"
#include "common/timer.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "camera/camera.h"
#include "scene/scene.h"
#include "render/progress_bar.h"
#include "sampler/sample.h"
#include "color/color_layers.h"
#include "render/render_data.h"
#include "image/image_output.h"
#include "accelerator/accelerator.h"
#include "photon/photon.h"

BEGIN_YAFARAY


std::vector<int> TiledIntegrator::correlative_sample_number_(0);

void TiledIntegrator::renderWorker(TiledIntegrator *integrator, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, const Timer &timer, ThreadControl *control, int thread_id, int samples, int offset, bool adaptive, int aa_pass)
{
	RenderArea a;

	while(image_film_->nextArea(render_view, render_control, a))
	{
		if(render_control.canceled()) break;
		integrator->renderTile(a, render_view->getCamera(), render_control, timer, samples, offset, adaptive, thread_id, aa_pass);

		std::unique_lock<std::mutex> lk(control->m_);
		control->areas_.push_back(a);
		control->c_.notify_one();

	}
	std::unique_lock<std::mutex> lk(control->m_);
	++(control->finished_threads_);
	control->c_.notify_one();
}

void TiledIntegrator::precalcDepths(const Camera *camera)
{
	const Accelerator *accelerator = scene_->getAccelerator();
	if(!accelerator) return;

	if(camera->getFarClip() > -1)
	{
		min_depth_ = camera->getNearClip();
		max_depth_ = camera->getFarClip();
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
				CameraRay camera_ray = camera->shootRay(i, j, 0.5f, 0.5f);
				SurfacePoint sp;
				accelerator->intersect(camera_ray.ray_, sp, camera);
				if(camera_ray.ray_.tmax_ > max_depth_) max_depth_ = camera_ray.ray_.tmax_;
				if(camera_ray.ray_.tmax_ < min_depth_ && camera_ray.ray_.tmax_ >= 0.f) min_depth_ = camera_ray.ray_.tmax_;
			}
		}
	}
	// we use the inverse multiplicative of the value aquired
	if(max_depth_ > 0.f) max_depth_ = 1.f / (max_depth_ - min_depth_);
}

bool TiledIntegrator::render(RenderControl &render_control, Timer &timer, const RenderView *render_view)
{
	std::stringstream pass_string;
	aa_noise_params_ = scene_->getAaParameters();

	std::stringstream aa_settings;
	aa_settings << " passes=" << aa_noise_params_.passes_;
	aa_settings << " samples=" << aa_noise_params_.samples_ << " inc_samples=" << aa_noise_params_.inc_samples_ << " resamp.floor=" << aa_noise_params_.resampled_floor_ << "\nsample.mul=" << aa_noise_params_.sample_multiplier_factor_ << " light.sam.mul=" << aa_noise_params_.light_sample_multiplier_factor_ << " ind.sam.mul=" << aa_noise_params_.indirect_sample_multiplier_factor_ << "\ncol.noise=" << aa_noise_params_.detect_color_noise_;

	if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear) aa_settings << " AA thr(lin)=" << aa_noise_params_.threshold_ << ",dark_fac=" << aa_noise_params_.dark_threshold_factor_;
	else if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve) aa_settings << " AA.thr(curve)";
	else aa_settings << " AA thr=" << aa_noise_params_.threshold_;

	aa_settings << " var.edge=" << aa_noise_params_.variance_edge_size_ << " var.pix=" << aa_noise_params_.variance_pixels_ << " clamp=" << aa_noise_params_.clamp_samples_ << " ind.clamp=" << aa_noise_params_.clamp_indirect_;

	aa_noise_info_ += aa_settings.str();

	i_aa_passes_ = 1.f / (float) aa_noise_params_.passes_;

	render_control.setTotalPasses(aa_noise_params_.passes_);

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
	if(intpb_) intpb_->setTag(pass_string.str().c_str());

	timer.addEvent("rendert");
	timer.start("rendert");

	image_film_->init(render_control, aa_noise_params_.passes_);
	image_film_->setAaNoiseParams(aa_noise_params_);

	if(render_control.resumed())
	{
		pass_string.clear();
		pass_string << "Combining ImageFilm files, skipping pass 1...";
		if(intpb_) intpb_->setTag(pass_string.str().c_str());
	}

	logger_.logInfo(getName(), ": ", pass_string.str());

	max_depth_ = 0.f;
	min_depth_ = 1e38f;

	if(scene_->getLayers().isDefinedAny({Layer::ZDepthNorm, Layer::Mist})) precalcDepths(render_view->getCamera());

	correlative_sample_number_.clear();
	correlative_sample_number_.resize(scene_->getNumThreads());
	std::fill(correlative_sample_number_.begin(), correlative_sample_number_.end(), 0);

	int resampled_pixels = 0;
	if(render_control.resumed())
	{
		renderPass(render_view, 0, image_film_->getSamplingOffset(), false, 0, render_control, timer);
	}
	else renderPass(render_view, aa_noise_params_.samples_, 0, false, 0, render_control, timer);

	bool aa_threshold_changed = true;
	int acum_aa_samples = aa_noise_params_.samples_;

	for(int i = 1; i < aa_noise_params_.passes_; ++i)
	{
		if(render_control.canceled()) break;

		//scene->getSurfIntegrator()->setSampleMultiplier(scene->getSurfIntegrator()->getSampleMultiplier() * AA_sample_multiplier_factor);

		aa_sample_multiplier_ *= aa_noise_params_.sample_multiplier_factor_;
		aa_light_sample_multiplier_ *= aa_noise_params_.light_sample_multiplier_factor_;
		aa_indirect_sample_multiplier_ *= aa_noise_params_.indirect_sample_multiplier_factor_;

		logger_.logInfo(getName(), ": Sample multiplier = ", aa_sample_multiplier_, ", Light Sample multiplier = ", aa_light_sample_multiplier_, ", Indirect Sample multiplier = ", aa_indirect_sample_multiplier_);

		image_film_->setAaNoiseParams(aa_noise_params_);

		if(resampled_pixels <= 0.f && !aa_threshold_changed)
		{
			logger_.logInfo(getName(), ": in previous pass there were 0 pixels to be resampled and the AA threshold did not change, so this pass resampling check and rendering will be skipped.");
			image_film_->nextPass(render_view, render_control, true, getName(), scene_->getEdgeToonParams(), /*skipNextPass=*/true);
		}
		else
		{
			image_film_->setAaThreshold(aa_noise_params_.threshold_);
			resampled_pixels = image_film_->nextPass(render_view, render_control, true, getName(), scene_->getEdgeToonParams());
			aa_threshold_changed = false;
		}

		int aa_samples_mult = (int) ceilf(aa_noise_params_.inc_samples_ * aa_sample_multiplier_);

		if(logger_.isDebug())logger_.logDebug("acumAASamples=", acum_aa_samples, " AA_samples=", aa_noise_params_.samples_, " AA_samples_mult=", aa_samples_mult);

		if(resampled_pixels > 0) renderPass(render_view, aa_samples_mult, acum_aa_samples, true, i, render_control, timer);

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
	timer.stop("rendert");
	render_control.setFinished();
	logger_.logInfo(getName(), ": Overall rendertime: ", timer.getTime("rendert"), "s");

	return true;
}


bool TiledIntegrator::renderPass(const RenderView *render_view, int samples, int offset, bool adaptive, int aa_pass_number, RenderControl &render_control, Timer &timer)
{
	if(logger_.isDebug())logger_.logDebug("Sampling: samples=", samples, " Offset=", offset, " Base Offset=", + image_film_->getBaseSamplingOffset(), "  AA_pass_number=", aa_pass_number);

	prePass(samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, render_control, timer, render_view);

	int nthreads = scene_->getNumThreads();

	render_control.setCurrentPass(aa_pass_number + 1);

	image_film_->setSamplingOffset(offset + samples);

	ThreadControl tc;
	std::vector<std::thread> threads;
	for(int i = 0; i < nthreads; ++i)
	{
		threads.push_back(std::thread(&TiledIntegrator::renderWorker, this, this, scene_, render_view, std::ref(render_control), std::ref(timer), &tc, i, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, aa_pass_number));
	}

	std::unique_lock<std::mutex> lk(tc.m_);
	while(tc.finished_threads_ < nthreads)
	{
		tc.c_.wait(lk);
		for(size_t i = 0; i < tc.areas_.size(); ++i)
		{
			image_film_->finishArea(render_view, render_control, tc.areas_[i], scene_->getEdgeToonParams());
		}
		tc.areas_.clear();
	}

	for(auto &t : threads) t.join();	//join all threads (although they probably have exited already, but not necessarily):

	return true; //hm...quite useless the return value :)
}

bool TiledIntegrator::renderTile(const RenderArea &a, const Camera *camera, const RenderControl &render_control, const Timer &timer, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number)
{
	const int camera_res_x = camera->resX();
	RandomGenerator random_generator(rand() + offset * (camera_res_x * a.y_ + a.x_) + 123);
	const bool sample_lns = camera->sampleLense();
	const int pass_offs = offset, end_x = a.x_ + a.w_, end_y = a.y_ + a.h_;
	int aa_max_possible_samples = aa_noise_params_.samples_;
	for(int i = 1; i < aa_noise_params_.passes_; ++i)
	{
		aa_max_possible_samples += ceilf(aa_noise_params_.inc_samples_ * pow(aa_noise_params_.sample_multiplier_factor_, i));	//DAVID FIXME: if the per-material sampling factor is used, values higher than 1.f will appear in the Sample Count render pass. Is that acceptable or not?
	}
	const float inv_aa_max_possible_samples = 1.f / static_cast<float>(aa_max_possible_samples);
	Halton hal_u(3);
	Halton hal_v(5);
	const Layers &layers = scene_->getLayers();
	const MaskParams &mask_params = scene_->getMaskParams();
	ColorLayers color_layers(layers);
	const Image *sampling_factor_image_pass = (*image_film_->getImageLayers())(Layer::DebugSamplingFactor).image_.get();
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
					n_samples_adjusted = static_cast<int>(round(static_cast<float>(n_samples) * mat_sample_factor));
					d_1 = 1.f / static_cast<float>(n_samples_adjusted);	//DAVID FIXME: is this correct???
				}
			}
			PixelSamplingData pixel_sampling_data;
			pixel_sampling_data.number_ = camera_res_x * i + j;
			pixel_sampling_data.offset_ = sample::fnv32ABuf(i * sample::fnv32ABuf(j)); //fnv_32a_buf(rstate.pixelNumber);
			const float toff = Halton::lowDiscrepancySampling(5, pass_offs + pixel_sampling_data.offset_); // **shall be just the pass number...**
			hal_u.setStart(pass_offs + pixel_sampling_data.offset_);
			hal_v.setStart(pass_offs + pixel_sampling_data.offset_);
			for(int sample = 0; sample < n_samples_adjusted; ++sample)
			{
				color_layers.setDefaultColors();
				pixel_sampling_data.sample_ = pass_offs + sample;

				const float time = math::addMod1(static_cast<float>(sample) * d_1, toff); //(0.5+(float)sample)*d1;
				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA  //!< the current (normalized) frame time  //FIXME, time not currently used in libYafaRay
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
				float lens_u = 0.5f, lens_v = 0.5f;
				if(sample_lns)
				{
					lens_u = hal_u.getNext();
					lens_v = hal_v.getNext();
				}
				CameraRay camera_ray = camera->shootRay(j + dx, i + dy, lens_u, lens_v);
				if(!camera_ray.valid_)
				{
					image_film_->addSample(j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
					continue;
				}
				if(render_control.getDifferentialRaysEnabled())
				{
					//setup ray differentials
					camera_ray.ray_.differentials_ = std::unique_ptr<RayDifferentials>(new RayDifferentials());
					const CameraRay camera_diff_ray_x = camera->shootRay(j + 1 + dx, i + dy, lens_u, lens_v);
					camera_ray.ray_.differentials_->xfrom_ = camera_diff_ray_x.ray_.from_;
					camera_ray.ray_.differentials_->xdir_ = camera_diff_ray_x.ray_.dir_;
					const CameraRay camera_diff_ray_y = camera->shootRay(j + dx, i + 1 + dy, lens_u, lens_v);
					camera_ray.ray_.differentials_->yfrom_ = camera_diff_ray_y.ray_.from_;
					camera_ray.ray_.differentials_->ydir_ = camera_diff_ray_y.ray_.dir_;
				}
				camera_ray.ray_.time_ = time;
				RayDivision ray_division;
				color_layers(Layer::Combined).color_ = integrate(thread_id, 0, true, 0.f, camera_ray.ray_, 0, ray_division, &color_layers, camera, random_generator, pixel_sampling_data, false);
				for(auto &color_layer : color_layers)
				{
					switch(color_layer.first)
					{
						case Layer::ObjIndexMask:
						case Layer::ObjIndexMaskShadow:
						case Layer::ObjIndexMaskAll:
						case Layer::MatIndexMask:
						case Layer::MatIndexMaskShadow:
						case Layer::MatIndexMaskAll:
							if(color_layer.second.color_.a_ > 1.f) color_layer.second.color_.a_ = 1.f;
							color_layer.second.color_.clampRgb01();
							if(mask_params.invert_) color_layer.second.color_ = Rgba(1.f) - color_layer.second.color_;
							if(!mask_params.only_)
							{
								Rgba col_combined = color_layers(Layer::Combined).color_;
								col_combined.a_ = 1.f;
								color_layer.second.color_ *= col_combined;
							}
							break;
						case Layer::ZDepthAbs:
							if(camera_ray.ray_.tmax_ < 0.f) color_layer.second.color_ = Rgba(0.f, 0.f); // Show background as fully transparent
							else color_layer.second.color_ = Rgb(camera_ray.ray_.tmax_);
							if(color_layer.second.color_.a_ > 1.f) color_layer.second.color_.a_ = 1.f;
							break;
						case Layer::ZDepthNorm:
							if(camera_ray.ray_.tmax_ < 0.f) color_layer.second.color_ = Rgba(0.f, 0.f); // Show background as fully transparent
							else color_layer.second.color_ = Rgb(1.f - (camera_ray.ray_.tmax_ - min_depth_) * max_depth_); // Distance normalization
							if(color_layer.second.color_.a_ > 1.f) color_layer.second.color_.a_ = 1.f;
							break;
						case Layer::Mist:
							if(camera_ray.ray_.tmax_ < 0.f) color_layer.second.color_ = Rgba(0.f, 0.f); // Show background as fully transparent
							else color_layer.second.color_ = Rgb((camera_ray.ray_.tmax_ - min_depth_) * max_depth_); // Distance normalization
							if(color_layer.second.color_.a_ > 1.f) color_layer.second.color_.a_ = 1.f;
							break;
						default:
							if(color_layer.second.color_.a_ > 1.f) color_layer.second.color_.a_ = 1.f;
							break;
					}
				}
				image_film_->addSample(j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
			}
		}
	}
	return true;
}

void TiledIntegrator::generateCommonLayers(const SurfacePoint &sp, const MaskParams &mask_params, ColorLayers *color_layers)
{
	if(color_layers)
	{
		if(color_layers->getFlags().hasAny(Layer::Flags::DebugLayers))
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::Uv))
			{
				color_layer->color_ = Rgba(sp.u_, sp.v_, 0.f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::BarycentricUvw))
			{
				color_layer->color_ = Rgba(sp.intersect_data_.barycentric_u_, sp.intersect_data_.barycentric_v_, sp.intersect_data_.barycentric_w_, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::NormalSmooth))
			{
				color_layer->color_ = Rgba((sp.n_.x_ + 1.f) * .5f, (sp.n_.y_ + 1.f) * .5f, (sp.n_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::NormalGeom))
			{
				color_layer->color_ = Rgba((sp.ng_.x_ + 1.f) * .5f, (sp.ng_.y_ + 1.f) * .5f, (sp.ng_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugDpdu))
			{
				color_layer->color_ = Rgba((sp.dp_du_.x_ + 1.f) * .5f, (sp.dp_du_.y_ + 1.f) * .5f, (sp.dp_du_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugDpdv))
			{
				color_layer->color_ = Rgba((sp.dp_dv_.x_ + 1.f) * .5f, (sp.dp_dv_.y_ + 1.f) * .5f, (sp.dp_dv_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugDsdu))
			{
				color_layer->color_ = Rgba((sp.ds_du_.x_ + 1.f) * .5f, (sp.ds_du_.y_ + 1.f) * .5f, (sp.ds_du_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugDsdv))
			{
				color_layer->color_ = Rgba((sp.ds_dv_.x_ + 1.f) * .5f, (sp.ds_dv_.y_ + 1.f) * .5f, (sp.ds_dv_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugNu))
			{
				color_layer->color_ = Rgba((sp.nu_.x_ + 1.f) * .5f, (sp.nu_.y_ + 1.f) * .5f, (sp.nu_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugNv))
			{
				color_layer->color_ = Rgba((sp.nv_.x_ + 1.f) * .5f, (sp.nv_.y_ + 1.f) * .5f, (sp.nv_.z_ + 1.f) * .5f, 1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugWireframe))
			{
				Rgba wireframe_color = Rgba(0.f, 0.f, 0.f, 0.f);
				sp.material_->applyWireFrame(wireframe_color, 1.f, sp);
				color_layer->color_ = wireframe_color;
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DebugSamplingFactor))
			{
				color_layer->color_ = Rgba(sp.material_->getSamplingFactor());
			}
			if(color_layers->isDefinedAny({Layer::DebugDpLengths, Layer::DebugDpdx, Layer::DebugDpdy, Layer::DebugDpdxy, Layer::DebugDudxDvdx, Layer::DebugDudyDvdy, Layer::DebugDudxyDvdxy}))
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::DebugDpLengths))
				{
					if(sp.differentials_) color_layer->color_ = Rgba(sp.differentials_->dp_dx_.length(), sp.differentials_->dp_dy_.length(), 0.f, 1.f);
				}
				if(ColorLayer *color_layer = color_layers->find(Layer::DebugDpdx))
				{
					if(sp.differentials_) color_layer->color_ = Rgba((sp.differentials_->dp_dx_.x_ + 1.f) * .5f, (sp.differentials_->dp_dx_.y_ + 1.f) * .5f, (sp.differentials_->dp_dx_.z_ + 1.f) * .5f, 1.f);
				}
				if(ColorLayer *color_layer = color_layers->find(Layer::DebugDpdy))
				{
					if(sp.differentials_) color_layer->color_ = Rgba((sp.differentials_->dp_dy_.x_ + 1.f) * .5f, (sp.differentials_->dp_dy_.y_ + 1.f) * .5f, (sp.differentials_->dp_dy_.z_ + 1.f) * .5f, 1.f);
				}
				if(ColorLayer *color_layer = color_layers->find(Layer::DebugDpdxy))
				{
					if(sp.differentials_) color_layer->color_ = Rgba((sp.differentials_->dp_dx_.x_ + sp.differentials_->dp_dy_.x_ + 1.f) * .5f, (sp.differentials_->dp_dx_.y_ + sp.differentials_->dp_dy_.y_ + 1.f) * .5f, (sp.differentials_->dp_dx_.z_ + sp.differentials_->dp_dy_.z_ + 1.f) * .5f, 1.f);
				}
				if(color_layers->isDefinedAny({Layer::DebugDudxDvdx, Layer::DebugDudyDvdy, Layer::DebugDudxyDvdxy}))
				{
					float du_dx = 0.f, dv_dx = 0.f;
					float du_dy = 0.f, dv_dy = 0.f;
					sp.getUVdifferentials(du_dx, dv_dx, du_dy, dv_dy);
					if(ColorLayer *color_layer = color_layers->find(Layer::DebugDudxDvdx))
					{
						color_layer->color_ = Rgba((du_dx + 1.f) * .5f, (dv_dx + 1.f) * .5f, 0.f, 1.f);
					}
					if(ColorLayer *color_layer = color_layers->find(Layer::DebugDudyDvdy))
					{
						color_layer->color_ = Rgba((du_dy + 1.f) * .5f, (dv_dy + 1.f) * .5f, 0.f, 1.f);
					}
					if(ColorLayer *color_layer = color_layers->find(Layer::DebugDudxyDvdxy))
					{
						color_layer->color_ = Rgba((du_dx + du_dy + 1.f) * .5f, (dv_dx + dv_dy + 1.f) * .5f, 0.f, 1.f);
					}
				}
			}
		}
		if(color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::ReflectAll))
			{
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::ReflectPerfect))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::Glossy))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::GlossyIndirect))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::RefractAll))
			{
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::RefractPerfect))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::Trans))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::TransIndirect))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::IndirectAll))
			{
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::Indirect))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::DiffuseIndirect))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::DiffuseColor))
			{
				color_layer->color_ = sp.material_->getDiffuseColor(sp.mat_data_->node_tree_data_);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::GlossyColor))
			{
				color_layer->color_ = sp.material_->getGlossyColor(sp.mat_data_->node_tree_data_);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::TransColor))
			{
				color_layer->color_ = sp.material_->getTransColor(sp.mat_data_->node_tree_data_);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::SubsurfaceColor))
			{
				color_layer->color_ = sp.material_->getSubSurfaceColor(sp.mat_data_->node_tree_data_);
			}
		}
		if(color_layers->getFlags().hasAny(Layer::Flags::IndexLayers))
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexAbs))
			{
				color_layer->color_ = sp.object_->getAbsObjectIndexColor();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexNorm))
			{
				color_layer->color_ = sp.object_->getNormObjectIndexColor();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexAuto))
			{
				color_layer->color_ = sp.object_->getAutoObjectIndexColor();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexAutoAbs))
			{
				color_layer->color_ = sp.object_->getAutoObjectIndexNumber();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexAbs))
			{
				color_layer->color_ = sp.material_->getAbsMaterialIndexColor();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexNorm))
			{
				color_layer->color_ = sp.material_->getNormMaterialIndexColor();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexAuto))
			{
				color_layer->color_ = sp.material_->getAutoMaterialIndexColor();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexAutoAbs))
			{
				color_layer->color_ = sp.material_->getAutoMaterialIndexNumber();
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexMask))
			{
				if(sp.object_->getAbsObjectIndex() == mask_params.obj_index_) color_layer->color_ = Rgba(1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::ObjIndexMaskAll))
			{
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::ObjIndexMask))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::ObjIndexMaskShadow))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexMask))
			{
				if(sp.material_->getAbsMaterialIndex() == mask_params.mat_index_) color_layer->color_ = Rgba(1.f);
			}
			if(ColorLayer *color_layer = color_layers->find(Layer::MatIndexMaskAll))
			{
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::MatIndexMask))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if(ColorLayer *color_layer_2 = color_layers->find(Layer::MatIndexMaskShadow))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}
		}
	}
}

Rgb TiledIntegrator::sampleAmbientOcclusion(bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const Camera *camera, const PixelSamplingData &pixel_sampling_data, bool lights_geometry_material_emit, bool transparent_shadows, bool clay) const
{
	const Accelerator *accelerator = scene_->getAccelerator();
	if(!accelerator) return {0.f};
	Rgb col{0.f};
	const Material *material = sp.material_;
	const BsdfFlags &mat_bsdfs = sp.mat_data_->bsdf_flags_;
	Ray light_ray {sp.p_, {0.f}};
	float mask_obj_index = 0.f, mask_mat_index = 0.f;
	int n = ao_samples_;//(int) ceilf(aoSamples*getSampleMultiplier());
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
		if(scene_->shadow_bias_auto_) light_ray.tmin_ = scene_->shadow_bias_ * std::max(1.f, Vec3(sp.p_).length());
		else light_ray.tmin_ = scene_->shadow_bias_;
		light_ray.tmax_ = ao_dist_;
		float w = 0.f;
		const BsdfFlags sample_flags = clay ?
									   BsdfFlags::All :
									   BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Reflect;
		Sample s(s_1, s_2, sample_flags);
		const Rgb surf_col = clay ?
							 material->sampleClay(sp, wo, light_ray.dir_, s, w) :
							 material->sample(sp.mat_data_.get(), sp, wo, light_ray.dir_, s, w, chromatic_enabled, wavelength, camera);
		if(clay) s.pdf_ = 1.f;
		if(mat_bsdfs.hasAny(BsdfFlags::Emit))
		{
			col += material->emit(sp.mat_data_.get(), sp, wo, lights_geometry_material_emit) * s.pdf_;
		}
		Rgb scol;
		const bool shadowed = transparent_shadows ?
							  accelerator->isShadowed(light_ray, s_depth_, scol, mask_obj_index, mask_mat_index, scene_->getShadowBias(), camera) :
							  accelerator->isShadowed(light_ray, mask_obj_index, mask_mat_index, scene_->getShadowBias());
		if(!shadowed)
		{
			const float cos = std::abs(sp.n_ * light_ray.dir_);
			if(transparent_shadows) col += ao_col_ * scol * surf_col * cos * w;
			else col += ao_col_ * surf_col * cos * w;
		}
	}
	return col / static_cast<float>(n);
}

std::pair<Rgb, float> TiledIntegrator::volumetricEffects(const Ray &ray, ColorLayers *color_layers, RandomGenerator &random_generator, Rgb col, float alpha, const VolumeIntegrator *volume_integrator, bool transparent_background)
{
	const Rgb col_vol_transmittance = volume_integrator->transmittance(random_generator, ray);
	const Rgb col_vol_integration = volume_integrator->integrate(random_generator, ray);
	if(transparent_background) alpha = std::max(alpha, 1.f - col_vol_transmittance.r_);
	if(color_layers)
	{
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeTransmittance)) color_layer->color_ = col_vol_transmittance;
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeIntegration)) color_layer->color_ = col_vol_integration;
	}
	col = (col * col_vol_transmittance) + col_vol_integration;
	return {col, alpha};
}

std::pair<Rgb, float> TiledIntegrator::background(const Ray &ray, ColorLayers *color_layers, Rgb col, float alpha, bool transparent_background, bool transparent_refracted_background, const Background *background)
{
	if(transparent_background) alpha = 0.f;
	if(background && !transparent_refracted_background)
	{
		const Rgb col_tmp = (*background)(ray.dir_);
		col += col_tmp;
		if(color_layers)
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::Env)) color_layer->color_ += col_tmp;
		}
	}
	return {col, alpha};
}

END_YAFARAY
