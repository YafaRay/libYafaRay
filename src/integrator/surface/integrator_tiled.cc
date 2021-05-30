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
#include "common/logger.h"
#include "common/session.h"
#include "common/layers.h"
#include "material/material.h"
#include "geometry/surface.h"
#include "geometry/object.h"
#include "common/timer.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "camera/camera.h"
#include "scene/scene.h"
#include "render/progress_bar.h"
#include "sampler/sample.h"
#include "color/color_layers.h"
#include "render/render_data.h"
#include "output/output.h"
#include "accelerator/accelerator.h"

BEGIN_YAFARAY


std::vector<int> TiledIntegrator::correlative_sample_number_(0);

void TiledIntegrator::renderWorker(TiledIntegrator *integrator, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, ThreadControl *control, int thread_id, int samples, int offset, bool adaptive, int aa_pass)
{
	RenderArea a;

	while(image_film_->nextArea(a))
	{
		if(render_control.canceled()) break;
		integrator->renderTile(a, render_view, render_control, samples, offset, adaptive, thread_id, aa_pass);

		std::unique_lock<std::mutex> lk(control->m_);
		control->areas_.push_back(a);
		control->c_.notify_one();

	}
	std::unique_lock<std::mutex> lk(control->m_);
	++(control->finished_threads_);
	control->c_.notify_one();
}

void TiledIntegrator::precalcDepths(const RenderView *render_view)
{
	const Camera *camera = render_view->getCamera();

	if(camera->getFarClip() > -1)
	{
		min_depth_ = camera->getNearClip();
		max_depth_ = camera->getFarClip();
	}
	else
	{
		DiffRay ray;
		// We sample the scene at render resolution to get the precision required for AA
		int w = camera->resX();
		int h = camera->resY();
		float wt = 0.f; // Dummy variable
		SurfacePoint sp;
		for(int i = 0; i < h; ++i)
		{
			for(int j = 0; j < w; ++j)
			{
				ray.tmax_ = -1.f;
				ray = camera->shootRay(i, j, 0.5f, 0.5f, wt);
				scene_->getAccelerator()->intersect(ray, sp);
				if(ray.tmax_ > max_depth_) max_depth_ = ray.tmax_;
				if(ray.tmax_ < min_depth_ && ray.tmax_ >= 0.f) min_depth_ = ray.tmax_;
			}
		}
	}
	// we use the inverse multiplicative of the value aquired
	if(max_depth_ > 0.f) max_depth_ = 1.f / (max_depth_ - min_depth_);
}

bool TiledIntegrator::render(RenderControl &render_control, const RenderView *render_view)
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

	g_timer_global.addEvent("rendert");
	g_timer_global.start("rendert");

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

	diff_rays_enabled_ = session_global.getDifferentialRaysEnabled();	//enable ray differentials for mipmap calculation if there is at least one image texture using Mipmap interpolation

	if(scene_->getLayers().isDefinedAny({Layer::ZDepthNorm, Layer::Mist})) precalcDepths(render_view);

	correlative_sample_number_.clear();
	correlative_sample_number_.resize(scene_->getNumThreads());
	std::fill(correlative_sample_number_.begin(), correlative_sample_number_.end(), 0);

	if(render_control.resumed())
	{
		renderPass(render_view, 0, image_film_->getSamplingOffset(), false, 0, render_control);
	}
	else renderPass(render_view, aa_noise_params_.samples_, 0, false, 0, render_control);

	bool aa_threshold_changed = true;
	int resampled_pixels = 0;
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
			image_film_->nextPass(render_view, render_control, true, getName(), /*skipNextPass=*/true);
		}
		else
		{
			image_film_->setAaThreshold(aa_noise_params_.threshold_);
			resampled_pixels = image_film_->nextPass(render_view, render_control, true, getName());
			aa_threshold_changed = false;
		}

		int aa_samples_mult = (int) ceilf(aa_noise_params_.inc_samples_ * aa_sample_multiplier_);

		if(logger_.isDebug())logger_.logDebug("acumAASamples=", acum_aa_samples, " AA_samples=", aa_noise_params_.samples_, " AA_samples_mult=", aa_samples_mult);

		if(resampled_pixels > 0) renderPass(render_view, aa_samples_mult, acum_aa_samples, true, i, render_control);

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
	g_timer_global.stop("rendert");
	render_control.setFinished();
	logger_.logInfo(getName(), ": Overall rendertime: ", g_timer_global.getTime("rendert"), "s");

	return true;
}


bool TiledIntegrator::renderPass(const RenderView *render_view, int samples, int offset, bool adaptive, int aa_pass_number, RenderControl &render_control)
{
	if(logger_.isDebug())logger_.logDebug("Sampling: samples=", samples, " Offset=", offset, " Base Offset=", + image_film_->getBaseSamplingOffset(), "  AA_pass_number=", aa_pass_number);

	prePass(samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, render_control, render_view);

	int nthreads = scene_->getNumThreads();

	render_control.setCurrentPass(aa_pass_number + 1);

	image_film_->setSamplingOffset(offset + samples);

	ThreadControl tc;
	std::vector<std::thread> threads;
	for(int i = 0; i < nthreads; ++i)
	{
		threads.push_back(std::thread(&TiledIntegrator::renderWorker, this, this, scene_, render_view, std::ref(render_control), &tc, i, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, aa_pass_number));
	}

	std::unique_lock<std::mutex> lk(tc.m_);
	while(tc.finished_threads_ < nthreads)
	{
		tc.c_.wait(lk);
		for(size_t i = 0; i < tc.areas_.size(); ++i)
		{
			image_film_->finishArea(render_view, render_control, tc.areas_[i]);
		}
		tc.areas_.clear();
	}

	for(auto &t : threads) t.join();	//join all threads (although they probably have exited already, but not necessarily):

	return true; //hm...quite useless the return value :)
}

bool TiledIntegrator::renderTile(RenderArea &a, const RenderView *render_view, const RenderControl &render_control, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number)
{
	int x;
	const Camera *camera = render_view->getCamera();
	x = camera->resX();
	DiffRay c_ray;
	Ray d_ray;
	float dx = 0.5, dy = 0.5, d_1 = 1.0 / (float)n_samples;
	float lens_u = 0.5f, lens_v = 0.5f;
	float wt, wt_dummy;
	Random prng(rand() + offset * (x * a.y_ + a.x_) + 123);
	RenderData rstate(&prng);
	rstate.thread_id_ = thread_id;
	rstate.cam_ = camera;
	bool sample_lns = camera->sampleLense();
	int pass_offs = offset, end_x = a.x_ + a.w_, end_y = a.y_ + a.h_;

	int aa_max_possible_samples = aa_noise_params_.samples_;

	for(int i = 1; i < aa_noise_params_.passes_; ++i)
	{
		aa_max_possible_samples += ceilf(aa_noise_params_.inc_samples_ * pow(aa_noise_params_.sample_multiplier_factor_, i));	//DAVID FIXME: if the per-material sampling factor is used, values higher than 1.f will appear in the Sample Count render pass. Is that acceptable or not?
	}

	float inv_aa_max_possible_samples = 1.f / ((float) aa_max_possible_samples);

	Halton hal_u(3);
	Halton hal_v(5);

	const Layers &layers = scene_->getLayers();
	const MaskParams &mask_params = layers.getMaskParams();
	ColorLayers color_layers(layers);

	const Image *sampling_factor_image_pass = (*image_film_->getImageLayers())(Layer::DebugSamplingFactor).image_.get();

	int film_cx_0 = image_film_->getCx0();
	int film_cy_0 = image_film_->getCy0();

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
					mat_sample_factor = sampling_factor_image_pass->getColor(j - film_cx_0, i - film_cy_0).normalized(weight).r_;

					if(image_film_->getBackgroundResampling()) mat_sample_factor = std::max(mat_sample_factor, 1.f); //If the background is set to be resampled, make sure the matSampleFactor is always >= 1.f

					if(mat_sample_factor > 0.f && mat_sample_factor < 1.f) mat_sample_factor = 1.f;	//This is to ensure in the edges between objects and background we always shoot samples. Otherwise we might not shoot enough samples at the boundaries with the background where they are needed for antialiasing. However if the factor is equal to 0.f (as in the background) then no more samples will be shot
				}

				if(mat_sample_factor != 1.f)
				{
					n_samples_adjusted = (int) round((float) n_samples * mat_sample_factor);
					d_1 = 1.0 / (float)n_samples_adjusted;	//DAVID FIXME: is this correct???
				}
			}

			//if(logger_.isDebug())logger_.logDebug("idxSamplingFactorExtPass="<<idxSamplingFactorExtPass<<" idxSamplingFactorAuxPass="<<idxSamplingFactorAuxPass<<" matSampleFactor="<<matSampleFactor<<" n_samples_adjusted="<<n_samples_adjusted<<" n_samples="<<n_samples<<YENDL;

			rstate.pixel_number_ = x * i + j;
			rstate.sampling_offs_ = sample::fnv32ABuf(i * sample::fnv32ABuf(j)); //fnv_32a_buf(rstate.pixelNumber);
			float toff = Halton::lowDiscrepancySampling(5, pass_offs + rstate.sampling_offs_); // **shall be just the pass number...**

			hal_u.setStart(pass_offs + rstate.sampling_offs_);
			hal_v.setStart(pass_offs + rstate.sampling_offs_);

			for(int sample = 0; sample < n_samples_adjusted; ++sample)
			{
				color_layers.setDefaultColors();
				rstate.setDefaults();
				rstate.pixel_sample_ = pass_offs + sample;
				rstate.time_ = math::addMod1((float) sample * d_1, toff); //(0.5+(float)sample)*d1;

				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA
				if(aa_noise_params_.passes_ > 1)
				{
					dx = sample::riVdC(rstate.pixel_sample_, rstate.sampling_offs_);
					dy = sample::riS(rstate.pixel_sample_, rstate.sampling_offs_);
				}
				else if(n_samples_adjusted > 1)
				{
					dx = (0.5 + (float)sample) * d_1;
					dy = sample::riLp(sample + rstate.sampling_offs_);
				}

				if(sample_lns)
				{
					lens_u = hal_u.getNext();
					lens_v = hal_v.getNext();
				}
				c_ray = camera->shootRay(j + dx, i + dy, lens_u, lens_v, wt);

				if(wt == 0.0)
				{
					image_film_->addSample(j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
					continue;
				}
				if(diff_rays_enabled_)
				{
					//setup ray differentials
					d_ray = camera->shootRay(j + 1 + dx, i + dy, lens_u, lens_v, wt_dummy);
					c_ray.xfrom_ = d_ray.from_;
					c_ray.xdir_ = d_ray.dir_;
					d_ray = camera->shootRay(j + dx, i + 1 + dy, lens_u, lens_v, wt_dummy);
					c_ray.yfrom_ = d_ray.from_;
					c_ray.ydir_ = d_ray.dir_;
					c_ray.has_differentials_ = true;
				}

				c_ray.time_ = rstate.time_;

				color_layers(Layer::Combined).color_ = integrate(rstate, c_ray, 0, &color_layers, render_view);

				for(auto &it : color_layers)
				{
					switch(it.first)
					{
						case Layer::ObjIndexMask:
						case Layer::ObjIndexMaskShadow:
						case Layer::ObjIndexMaskAll:
						case Layer::MatIndexMask:
						case Layer::MatIndexMaskShadow:
						case Layer::MatIndexMaskAll:
							it.second.color_ *= wt;
							if(it.second.color_.a_ > 1.f) it.second.color_.a_ = 1.f;
							it.second.color_.clampRgb01();
							if(mask_params.invert_)
							{
								it.second.color_ = Rgba(1.f) - it.second.color_;
							}
							if(!mask_params.only_)
							{
								Rgba col_combined = color_layers(Layer::Combined).color_;
								col_combined.a_ = 1.f;
								it.second.color_ *= col_combined;
							}
							break;
						case Layer::ZDepthAbs:
							if(c_ray.tmax_ < 0.f) it.second.color_ = Rgba(0.f, 0.f); // Show background as fully transparent
							else it.second.color_ = Rgb(c_ray.tmax_);
							it.second.color_ *= wt;
							if(it.second.color_.a_ > 1.f) it.second.color_.a_ = 1.f;
							break;
						case Layer::ZDepthNorm:
							if(c_ray.tmax_ < 0.f) it.second.color_ = Rgba(0.f, 0.f); // Show background as fully transparent
							else it.second.color_ = Rgb(1.f - (c_ray.tmax_ - min_depth_) * max_depth_); // Distance normalization
							it.second.color_ *= wt;
							if(it.second.color_.a_ > 1.f) it.second.color_.a_ = 1.f;
							break;
						case Layer::Mist:
							if(c_ray.tmax_ < 0.f) it.second.color_ = Rgba(0.f, 0.f); // Show background as fully transparent
							else it.second.color_ = Rgb((c_ray.tmax_ - min_depth_) * max_depth_); // Distance normalization
							it.second.color_ *= wt;
							if(it.second.color_.a_ > 1.f) it.second.color_.a_ = 1.f;
							break;
						default:
							it.second.color_ *= wt;
							if(it.second.color_.a_ > 1.f) it.second.color_.a_ = 1.f;
							break;
					}
				}

				image_film_->addSample(j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers);
			}
		}
	}
	return true;
}

void TiledIntegrator::generateCommonLayers(RenderData &render_data, const SurfacePoint &sp, const DiffRay &ray, ColorLayers *color_layers)
{
	const bool layers_used = render_data.raylevel_ == 0 && color_layers && color_layers->getFlags() != Layer::Flags::None;

	if(layers_used)
	{
		ColorLayer *color_layer;

		if(color_layers->getFlags().hasAny(Layer::Flags::DebugLayers))
		{
			if((color_layer = color_layers->find(Layer::Uv)))
			{
				color_layer->color_ = Rgba(sp.u_, sp.v_, 0.f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::BarycentricUvw)))
			{
				color_layer->color_ = Rgba(sp.intersect_data_.barycentric_u_, sp.intersect_data_.barycentric_v_, sp.intersect_data_.barycentric_w_, 1.f);
			}
			if((color_layer = color_layers->find(Layer::NormalSmooth)))
			{
				color_layer->color_ = Rgba((sp.n_.x_ + 1.f) * .5f, (sp.n_.y_ + 1.f) * .5f, (sp.n_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::NormalGeom)))
			{
				color_layer->color_ = Rgba((sp.ng_.x_ + 1.f) * .5f, (sp.ng_.y_ + 1.f) * .5f, (sp.ng_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::DebugDpdu)))
			{
				color_layer->color_ = Rgba((sp.dp_du_.x_ + 1.f) * .5f, (sp.dp_du_.y_ + 1.f) * .5f, (sp.dp_du_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::DebugDpdv)))
			{
				color_layer->color_ = Rgba((sp.dp_dv_.x_ + 1.f) * .5f, (sp.dp_dv_.y_ + 1.f) * .5f, (sp.dp_dv_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::DebugDsdu)))
			{
				color_layer->color_ = Rgba((sp.ds_du_.x_ + 1.f) * .5f, (sp.ds_du_.y_ + 1.f) * .5f, (sp.ds_du_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::DebugDsdv)))
			{
				color_layer->color_ = Rgba((sp.ds_dv_.x_ + 1.f) * .5f, (sp.ds_dv_.y_ + 1.f) * .5f, (sp.ds_dv_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::DebugNu)))
			{
				color_layer->color_ = Rgba((sp.nu_.x_ + 1.f) * .5f, (sp.nu_.y_ + 1.f) * .5f, (sp.nu_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::DebugNv)))
			{
				color_layer->color_ = Rgba((sp.nv_.x_ + 1.f) * .5f, (sp.nv_.y_ + 1.f) * .5f, (sp.nv_.z_ + 1.f) * .5f, 1.f);
			}
			if((color_layer = color_layers->find(Layer::DebugWireframe)))
			{
				Rgba wireframe_color = Rgba(0.f, 0.f, 0.f, 0.f);
				sp.material_->applyWireFrame(wireframe_color, 1.f, sp);
				color_layer->color_ = wireframe_color;
			}
			if((color_layer = color_layers->find(Layer::DebugSamplingFactor)))
			{
				color_layer->color_ = Rgba(sp.material_->getSamplingFactor());
			}
			if(color_layers->isDefinedAny({Layer::DebugDpLengths, Layer::DebugDpdx, Layer::DebugDpdy, Layer::DebugDpdxy, Layer::DebugDudxDvdx, Layer::DebugDudyDvdy, Layer::DebugDudxyDvdxy}))
			{
				SpDifferentials sp_diff(sp, ray);

				if((color_layer = color_layers->find(Layer::DebugDpLengths)))
				{
					color_layer->color_ = Rgba(sp_diff.dp_dx_.length(), sp_diff.dp_dy_.length(), 0.f, 1.f);
				}
				if((color_layer = color_layers->find(Layer::DebugDpdx)))
				{
					color_layer->color_ = Rgba((sp_diff.dp_dx_.x_ + 1.f) * .5f, (sp_diff.dp_dx_.y_ + 1.f) * .5f, (sp_diff.dp_dx_.z_ + 1.f) * .5f, 1.f);
				}
				if((color_layer = color_layers->find(Layer::DebugDpdy)))
				{
					color_layer->color_ = Rgba((sp_diff.dp_dy_.x_ + 1.f) * .5f, (sp_diff.dp_dy_.y_ + 1.f) * .5f, (sp_diff.dp_dy_.z_ + 1.f) * .5f, 1.f);
				}
				if((color_layer = color_layers->find(Layer::DebugDpdxy)))
				{
					color_layer->color_ = Rgba((sp_diff.dp_dx_.x_ + sp_diff.dp_dy_.x_ + 1.f) * .5f, (sp_diff.dp_dx_.y_ + sp_diff.dp_dy_.y_ + 1.f) * .5f, (sp_diff.dp_dx_.z_ + sp_diff.dp_dy_.z_ + 1.f) * .5f, 1.f);
				}
				if(color_layers->isDefinedAny({Layer::DebugDudxDvdx, Layer::DebugDudyDvdy, Layer::DebugDudxyDvdxy}))
				{
					float du_dx = 0.f, dv_dx = 0.f;
					float du_dy = 0.f, dv_dy = 0.f;
					sp_diff.getUVdifferentials(du_dx, dv_dx, du_dy, dv_dy);

					if((color_layer = color_layers->find(Layer::DebugDudxDvdx)))
					{
						color_layer->color_ = Rgba((du_dx + 1.f) * .5f, (dv_dx + 1.f) * .5f, 0.f, 1.f);
					}
					if((color_layer = color_layers->find(Layer::DebugDudyDvdy)))
					{
						color_layer->color_ = Rgba((du_dy + 1.f) * .5f, (dv_dy + 1.f) * .5f, 0.f, 1.f);
					}
					if((color_layer = color_layers->find(Layer::DebugDudxyDvdxy)))
					{
						color_layer->color_ = Rgba((du_dx + du_dy + 1.f) * .5f, (dv_dx + dv_dy + 1.f) * .5f, 0.f, 1.f);
					}
				}
			}
		}

		if(color_layers->getFlags().hasAny(Layer::Flags::BasicLayers))
		{
			if((color_layer = color_layers->find(Layer::ReflectAll)))
			{
				ColorLayer *color_layer_2;
				if((color_layer_2 = color_layers->find(Layer::ReflectPerfect)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if((color_layer_2 = color_layers->find(Layer::Glossy)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if((color_layer_2 = color_layers->find(Layer::GlossyIndirect)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}
			if((color_layer = color_layers->find(Layer::RefractAll)))
			{
				ColorLayer *color_layer_2;
				if((color_layer_2 = color_layers->find(Layer::RefractPerfect)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if((color_layer_2 = color_layers->find(Layer::Trans)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if((color_layer_2 = color_layers->find(Layer::TransIndirect)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}

			if((color_layer = color_layers->find(Layer::IndirectAll)))
			{
				ColorLayer *color_layer_2;
				if((color_layer_2 = color_layers->find(Layer::Indirect)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if((color_layer_2 = color_layers->find(Layer::DiffuseIndirect)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}

			if((color_layer = color_layers->find(Layer::DiffuseColor)))
			{
				color_layer->color_ = sp.material_->getDiffuseColor(render_data);
			}
			if((color_layer = color_layers->find(Layer::GlossyColor)))
			{
				color_layer->color_ = sp.material_->getGlossyColor(render_data);
			}
			if((color_layer = color_layers->find(Layer::TransColor)))
			{
				color_layer->color_ = sp.material_->getTransColor(render_data);
			}
			if((color_layer = color_layers->find(Layer::SubsurfaceColor)))
			{
				color_layer->color_ = sp.material_->getSubSurfaceColor(render_data);
			}
		}

		if(color_layers->getFlags().hasAny(Layer::Flags::IndexLayers))
		{
			if((color_layer = color_layers->find(Layer::ObjIndexAbs)))
			{
				color_layer->color_ = sp.object_->getAbsObjectIndexColor();
			}
			if((color_layer = color_layers->find(Layer::ObjIndexNorm)))
			{
				color_layer->color_ = sp.object_->getNormObjectIndexColor();
			}
			if((color_layer = color_layers->find(Layer::ObjIndexAuto)))
			{
				color_layer->color_ = sp.object_->getAutoObjectIndexColor();
			}
			if((color_layer = color_layers->find(Layer::ObjIndexAutoAbs)))
			{
				color_layer->color_ = sp.object_->getAutoObjectIndexNumber();
			}

			if((color_layer = color_layers->find(Layer::MatIndexAbs)))
			{
				color_layer->color_ = sp.material_->getAbsMaterialIndexColor();
			}

			if((color_layer = color_layers->find(Layer::MatIndexNorm)))
			{
				color_layer->color_ = sp.material_->getNormMaterialIndexColor();
			}

			if((color_layer = color_layers->find(Layer::MatIndexAuto)))
			{
				color_layer->color_ = sp.material_->getAutoMaterialIndexColor();
			}

			if((color_layer = color_layers->find(Layer::MatIndexAutoAbs)))
			{
				color_layer->color_ = sp.material_->getAutoMaterialIndexNumber();
			}

			if((color_layer = color_layers->find(Layer::ObjIndexMask)))
			{
				if(sp.object_->getAbsObjectIndex() == color_layers->getMaskParams().obj_index_) color_layer->color_ = Rgba(1.f);
			}

			if((color_layer = color_layers->find(Layer::ObjIndexMaskAll)))
			{
				ColorLayer *color_layer_2;
				if((color_layer_2 = color_layers->find(Layer::ObjIndexMask)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if((color_layer_2 = color_layers->find(Layer::ObjIndexMaskShadow)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}

			if((color_layer = color_layers->find(Layer::MatIndexMask)))
			{
				if(sp.material_->getAbsMaterialIndex() == color_layers->getMaskParams().mat_index_) color_layer->color_ = Rgba(1.f);
			}

			if((color_layer = color_layers->find(Layer::MatIndexMaskAll)))
			{
				ColorLayer *color_layer_2;
				if((color_layer_2 = color_layers->find(Layer::MatIndexMask)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
				if((color_layer_2 = color_layers->find(Layer::MatIndexMaskShadow)))
				{
					color_layer->color_ += color_layer_2->color_;
				}
			}
		}
	}
}

END_YAFARAY
