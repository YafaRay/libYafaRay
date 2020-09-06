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

#include "integrator/integrator_tiled.h"
#include "common/logging.h"
#include "common/session.h"
#include "common/renderpasses.h"
#include "material/material.h"
#include "common/surface.h"
#include "object_geom/object_geom.h"
#include "common/timer.h"
#include "common/scr_halton.h"
#include "common/imagefilm.h"
#include "camera/camera.h"
#include "common/scene.h"
#include "common/monitor.h"
#include "utility/util_mcqmc.h"
#include "utility/util_sample.h"
#include <sstream>

BEGIN_YAFARAY


std::vector<int> TiledIntegrator::correlative_sample_number_(0);

void TiledIntegrator::renderWorker(int m_num_view, TiledIntegrator *integrator, Scene *scene, ImageFilm *image_film, ThreadControl *control, int thread_id, int samples, int offset, bool adaptive, int aa_pass)
{
	RenderArea a;

	while(image_film->nextArea(m_num_view, a))
	{
		if(scene->getSignals() & Y_SIG_ABORT) break;
		integrator->renderTile(m_num_view, a, samples, offset, adaptive, thread_id, aa_pass);

		std::unique_lock<std::mutex> lk(control->m_);
		control->areas_.push_back(a);
		control->c_.notify_one();

	}
	std::unique_lock<std::mutex> lk(control->m_);
	++(control->finished_threads_);
	control->c_.notify_one();
}

void TiledIntegrator::precalcDepths()
{
	const Camera *camera = scene_->getCamera();

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
				scene_->intersect(ray, sp);
				if(ray.tmax_ > max_depth_) max_depth_ = ray.tmax_;
				if(ray.tmax_ < min_depth_ && ray.tmax_ >= 0.f) min_depth_ = ray.tmax_;
			}
		}
	}
	// we use the inverse multiplicative of the value aquired
	if(max_depth_ > 0.f) max_depth_ = 1.f / (max_depth_ - min_depth_);
}

bool TiledIntegrator::render(int num_view, ImageFilm *image_film)
{
	std::stringstream pass_string;
	image_film_ = image_film;
	aa_noise_params_ = scene_->getAaParameters();

	std::stringstream aa_settings;
	aa_settings << " passes=" << aa_noise_params_.passes_;
	aa_settings << " samples=" << aa_noise_params_.samples_ << " inc_samples=" << aa_noise_params_.inc_samples_ << " resamp.floor=" << aa_noise_params_.resampled_floor_ << "\nsample.mul=" << aa_noise_params_.sample_multiplier_factor_ << " light.sam.mul=" << aa_noise_params_.light_sample_multiplier_factor_ << " ind.sam.mul=" << aa_noise_params_.indirect_sample_multiplier_factor_ << "\ncol.noise=" << aa_noise_params_.detect_color_noise_;

	if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear) aa_settings << " AA thr(lin)=" << aa_noise_params_.threshold_ << ",dark_fac=" << aa_noise_params_.dark_threshold_factor_;
	else if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve) aa_settings << " AA.thr(curve)";
	else aa_settings << " AA thr=" << aa_noise_params_.threshold_;

	aa_settings << " var.edge=" << aa_noise_params_.variance_edge_size_ << " var.pix=" << aa_noise_params_.variance_pixels_ << " clamp=" << aa_noise_params_.clamp_samples_ << " ind.clamp=" << aa_noise_params_.clamp_indirect_;

	logger__.appendAaNoiseSettings(aa_settings.str());

	i_aa_passes_ = 1.f / (float) aa_noise_params_.passes_;

	session__.setStatusTotalPasses(aa_noise_params_.passes_);

	aa_sample_multiplier_ = 1.f;
	aa_light_sample_multiplier_ = 1.f;
	aa_indirect_sample_multiplier_ = 1.f;

	int aa_resampled_floor_pixels = (int) floorf(aa_noise_params_.resampled_floor_ * (float) image_film_->getTotalPixels() / 100.f);

	Y_PARAMS << integrator_name_ << ": Rendering " << aa_noise_params_.passes_ << " passes" << YENDL;
	Y_PARAMS << "Min. " << aa_noise_params_.samples_ << " samples" << YENDL;
	Y_PARAMS << aa_noise_params_.inc_samples_ << " per additional pass" << YENDL;
	Y_PARAMS << "Resampled pixels floor: " << aa_noise_params_.resampled_floor_ << "% (" << aa_resampled_floor_pixels << " pixels)" << YENDL;
	Y_VERBOSE << "AA_sample_multiplier_factor: " << aa_noise_params_.sample_multiplier_factor_ << YENDL;
	Y_VERBOSE << "AA_light_sample_multiplier_factor: " << aa_noise_params_.light_sample_multiplier_factor_ << YENDL;
	Y_VERBOSE << "AA_indirect_sample_multiplier_factor: " << aa_noise_params_.indirect_sample_multiplier_factor_ << YENDL;
	Y_VERBOSE << "AA_detect_color_noise: " << aa_noise_params_.detect_color_noise_ << YENDL;

	if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear)	Y_VERBOSE << "AA_threshold (linear): " << aa_noise_params_.threshold_ << ", dark factor: " << aa_noise_params_.dark_threshold_factor_ << YENDL;
	else if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve)	Y_VERBOSE << "AA_threshold (curve)" << YENDL;
	else Y_VERBOSE << "AA threshold:" << aa_noise_params_.threshold_ << YENDL;

	Y_VERBOSE << "AA_variance_edge_size: " << aa_noise_params_.variance_edge_size_ << YENDL;
	Y_VERBOSE << "AA_variance_pixels: " << aa_noise_params_.variance_pixels_ << YENDL;
	Y_VERBOSE << "AA_clamp_samples: " << aa_noise_params_.clamp_samples_ << YENDL;
	Y_VERBOSE << "AA_clamp_indirect: " << aa_noise_params_.clamp_indirect_ << YENDL;
	Y_PARAMS << "Max. " << aa_noise_params_.samples_ + std::max(0, aa_noise_params_.passes_ - 1) * aa_noise_params_.inc_samples_ << " total samples" << YENDL;

	pass_string << "Rendering pass 1 of " << std::max(1, aa_noise_params_.passes_) << "...";

	Y_INFO << pass_string.str() << YENDL;
	if(intpb_) intpb_->setTag(pass_string.str().c_str());

	g_timer__.addEvent("rendert");
	g_timer__.start("rendert");

	image_film_->init(aa_noise_params_.passes_);
	image_film_->setAaNoiseParams(aa_noise_params_);

	if(session__.renderResumed())
	{
		pass_string.clear();
		pass_string << "Combining ImageFilm files, skipping pass 1...";
		if(intpb_) intpb_->setTag(pass_string.str().c_str());
	}

	Y_INFO << integrator_name_ << ": " << pass_string.str() << YENDL;

	max_depth_ = 0.f;
	min_depth_ = 1e38f;

	diff_rays_enabled_ = session__.getDifferentialRaysEnabled();	//enable ray differentials for mipmap calculation if there is at least one image texture using Mipmap interpolation

	if(scene_->passEnabled(PassIntZDepthNorm) || scene_->passEnabled(PassIntMist)) precalcDepths();

	correlative_sample_number_.clear();
	correlative_sample_number_.resize(scene_->getNumThreads());
	std::fill(correlative_sample_number_.begin(), correlative_sample_number_.end(), 0);

	int acum_aa_samples = aa_noise_params_.samples_;

	if(session__.renderResumed())
	{
		acum_aa_samples = image_film_->getSamplingOffset();
		renderPass(num_view, 0, acum_aa_samples, false, 0);
	}
	else renderPass(num_view, aa_noise_params_.samples_, 0, false, 0);

	bool aa_threshold_changed = true;
	int resampled_pixels = 0;

	for(int i = 1; i < aa_noise_params_.passes_; ++i)
	{
		if(scene_->getSignals() & Y_SIG_ABORT) break;

		//scene->getSurfIntegrator()->setSampleMultiplier(scene->getSurfIntegrator()->getSampleMultiplier() * AA_sample_multiplier_factor);

		aa_sample_multiplier_ *= aa_noise_params_.sample_multiplier_factor_;
		aa_light_sample_multiplier_ *= aa_noise_params_.light_sample_multiplier_factor_;
		aa_indirect_sample_multiplier_ *= aa_noise_params_.indirect_sample_multiplier_factor_;

		Y_INFO << integrator_name_ << ": Sample multiplier = " << aa_sample_multiplier_ << ", Light Sample multiplier = " << aa_light_sample_multiplier_ << ", Indirect Sample multiplier = " << aa_indirect_sample_multiplier_ << YENDL;

		image_film_->setAaNoiseParams(aa_noise_params_);

		if(resampled_pixels <= 0.f && !aa_threshold_changed)
		{
			Y_INFO << integrator_name_ << ": in previous pass there were 0 pixels to be resampled and the AA threshold did not change, so this pass resampling check and rendering will be skipped." << YENDL;
			image_film_->nextPass(num_view, true, integrator_name_, /*skipNextPass=*/true);
		}
		else
		{
			image_film_->setAaThreshold(aa_noise_params_.threshold_);
			resampled_pixels = image_film_->nextPass(num_view, true, integrator_name_);
			aa_threshold_changed = false;
		}

		int aa_samples_mult = (int) ceilf(aa_noise_params_.inc_samples_ * aa_sample_multiplier_);

		Y_DEBUG << "acumAASamples=" << acum_aa_samples << " AA_samples=" << aa_noise_params_.samples_ << " AA_samples_mult=" << aa_samples_mult << YENDL;

		if(resampled_pixels > 0) renderPass(num_view, aa_samples_mult, acum_aa_samples, true, i);

		acum_aa_samples += aa_samples_mult;

		if(resampled_pixels < aa_resampled_floor_pixels)
		{
			float aa_variation_ratio = std::min(8.f, ((float) aa_resampled_floor_pixels / resampled_pixels)); //This allows the variation for the new pass in the AA threshold and AA samples to depend, with a certain maximum per pass, on the ratio between how many pixeles were resampled and the target floor, to get a faster approach for noise removal.
			aa_noise_params_.threshold_ *= (1.f - 0.1f * aa_variation_ratio);

			Y_VERBOSE << integrator_name_ << ": Resampled pixels (" << resampled_pixels << ") below the floor (" << aa_resampled_floor_pixels << "): new AA Threshold (-" << aa_variation_ratio * 0.1f * 100.f << "%) for next pass = " << aa_noise_params_.threshold_ << YENDL;

			if(aa_noise_params_.threshold_ > 0.f) aa_threshold_changed = true;
		}
	}
	max_depth_ = 0.f;
	g_timer__.stop("rendert");
	session__.setStatusRenderFinished();
	Y_INFO << integrator_name_ << ": Overall rendertime: " << g_timer__.getTime("rendert") << "s" << YENDL;

	return true;
}


bool TiledIntegrator::renderPass(int num_view, int samples, int offset, bool adaptive, int aa_pass_number)
{
	Y_DEBUG << "Sampling: samples=" << samples << " Offset=" << offset << " Base Offset=" << + image_film_->getBaseSamplingOffset() << "  AA_pass_number=" << aa_pass_number << YENDL;

	prePass(samples, (offset + image_film_->getBaseSamplingOffset()), adaptive);

	int nthreads = scene_->getNumThreads();

	session__.setStatusCurrentPass(aa_pass_number + 1);

	image_film_->setSamplingOffset(offset + samples);

	if(nthreads > 1)
	{
		ThreadControl tc;
		std::vector<std::thread> threads;
		for(int i = 0; i < nthreads; ++i)
		{
			threads.push_back(std::thread(&TiledIntegrator::renderWorker, this, num_view, this, scene_, image_film_, &tc, i, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, aa_pass_number));
		}

		std::unique_lock<std::mutex> lk(tc.m_);
		while(tc.finished_threads_ < nthreads)
		{
			tc.c_.wait(lk);
			for(size_t i = 0; i < tc.areas_.size(); ++i)
			{
				image_film_->finishArea(num_view, tc.areas_[i]);
			}
			tc.areas_.clear();
		}

		for(auto &t : threads) t.join();	//join all threads (although they probably have exited already, but not necessarily):
	}
	else
	{
		RenderArea a;
		while(image_film_->nextArea(num_view, a))
		{
			if(scene_->getSignals() & Y_SIG_ABORT) break;
			renderTile(num_view, a, samples, (offset + image_film_->getBaseSamplingOffset()), adaptive, 0);
			image_film_->finishArea(num_view, a);
		}
	}

	return true; //hm...quite useless the return value :)
}

bool TiledIntegrator::renderTile(int num_view, RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number)
{
	int x;
	const Camera *camera = scene_->getCamera();
	x = camera->resX();
	DiffRay c_ray;
	Ray d_ray;
	float dx = 0.5, dy = 0.5, d_1 = 1.0 / (float)n_samples;
	float lens_u = 0.5f, lens_v = 0.5f;
	float wt, wt_dummy;
	Random prng(rand() + offset * (x * a.y_ + a.x_) + 123);
	RenderState rstate(&prng);
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

	const RenderPasses *render_passes = scene_->getRenderPasses();
	ColorPasses color_passes(render_passes);
	ColorPasses tmp_passes_zero(render_passes);

	Rgba2DImageWeighed_t *sampling_factor_image_pass = image_film_->getImagePassFromIntPassType(PassIntDebugSamplingFactor);

	int film_cx_0 = image_film_->getCx0();
	int film_cy_0 = image_film_->getCy0();

	for(int i = a.y_; i < end_y; ++i)
	{
		for(int j = a.x_; j < end_x; ++j)
		{
			if(scene_->getSignals() & Y_SIG_ABORT) break;

			float mat_sample_factor = 1.f;
			int n_samples_adjusted = n_samples;

			if(adaptive)
			{
				if(!image_film_->doMoreSamples(j, i)) continue;

				if(sampling_factor_image_pass)
				{
					mat_sample_factor = (*sampling_factor_image_pass)(j - film_cx_0, i - film_cy_0).normalized().r_;

					if(image_film_->getBackgroundResampling()) mat_sample_factor = std::max(mat_sample_factor, 1.f); //If the background is set to be resampled, make sure the matSampleFactor is always >= 1.f

					if(mat_sample_factor > 0.f && mat_sample_factor < 1.f) mat_sample_factor = 1.f;	//This is to ensure in the edges between objects and background we always shoot samples. Otherwise we might not shoot enough samples at the boundaries with the background where they are needed for antialiasing. However if the factor is equal to 0.f (as in the background) then no more samples will be shot
				}

				if(mat_sample_factor != 1.f)
				{
					n_samples_adjusted = (int) round((float) n_samples * mat_sample_factor);
					d_1 = 1.0 / (float)n_samples_adjusted;	//DAVID FIXME: is this correct???
				}
			}

			//Y_DEBUG << "idxSamplingFactorExtPass="<<idxSamplingFactorExtPass<<" idxSamplingFactorAuxPass="<<idxSamplingFactorAuxPass<<" matSampleFactor="<<matSampleFactor<<" n_samples_adjusted="<<n_samples_adjusted<<" n_samples="<<n_samples<<YENDL;

			rstate.pixel_number_ = x * i + j;
			rstate.sampling_offs_ = fnv32ABuf__(i * fnv32ABuf__(j)); //fnv_32a_buf(rstate.pixelNumber);
			float toff = scrHalton__(5, pass_offs + rstate.sampling_offs_); // **shall be just the pass number...**

			hal_u.setStart(pass_offs + rstate.sampling_offs_);
			hal_v.setStart(pass_offs + rstate.sampling_offs_);

			for(int sample = 0; sample < n_samples_adjusted; ++sample)
			{
				color_passes.resetColors();
				rstate.setDefaults();
				rstate.pixel_sample_ = pass_offs + sample;
				rstate.time_ = addMod1__((float) sample * d_1, toff); //(0.5+(float)sample)*d1;

				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA
				if(aa_noise_params_.passes_ > 1)
				{
					dx = riVdC__(rstate.pixel_sample_, rstate.sampling_offs_);
					dy = riS__(rstate.pixel_sample_, rstate.sampling_offs_);
				}
				else if(n_samples_adjusted > 1)
				{
					dx = (0.5 + (float)sample) * d_1;
					dy = riLp__(sample + rstate.sampling_offs_);
				}

				if(sample_lns)
				{
					lens_u = hal_u.getNext();
					lens_v = hal_v.getNext();
				}
				c_ray = camera->shootRay(j + dx, i + dy, lens_u, lens_v, wt);

				if(wt == 0.0)
				{
					image_film_->addSample(tmp_passes_zero, j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples);
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

				color_passes(PassIntCombined) = integrate(rstate, c_ray, color_passes);

				if(color_passes.enabled(PassIntZDepthNorm) || color_passes.enabled(PassIntZDepthAbs) || color_passes.enabled(PassIntMist))
				{
					float depth_abs = 0.f, depth_norm = 0.f;

					if(color_passes.enabled(PassIntZDepthNorm) || color_passes.enabled(PassIntMist))
					{
						if(c_ray.tmax_ > 0.f)
						{
							depth_norm = 1.f - (c_ray.tmax_ - min_depth_) * max_depth_; // Distance normalization
						}
						color_passes.probeSet(PassIntZDepthNorm, Rgba(depth_norm));
						color_passes.probeSet(PassIntMist, Rgba(1.f - depth_norm));
					}
					if(color_passes.enabled(PassIntZDepthAbs))
					{
						depth_abs = c_ray.tmax_;
						if(depth_abs <= 0.f)
						{
							depth_abs = 99999997952.f;
						}
						color_passes.probeSet(PassIntZDepthAbs, Rgba(depth_abs));
					}
				}

				for(int idx = 0; idx < color_passes.size(); ++idx)
				{
					if(color_passes(idx).a_ > 1.f) color_passes(idx).a_ = 1.f;

					IntPassTypes int_pass_type = color_passes.intPassTypeFromIndex(idx);

					switch(int_pass_type)
					{
						case PassIntZDepthNorm: break;
						case PassIntZDepthAbs: break;
						case PassIntMist: break;
						case PassIntNormalSmooth: break;
						case PassIntNormalGeom: break;
						case PassIntAo: break;
						case PassIntAoClay: break;
						case PassIntUv: break;
						case PassIntDebugNu: break;
						case PassIntDebugNv: break;
						case PassIntDebugDpdu: break;
						case PassIntDebugDpdv: break;
						case PassIntDebugDsdu: break;
						case PassIntDebugDsdv: break;
						case PassIntObjIndexAbs: break;
						case PassIntObjIndexNorm: break;
						case PassIntObjIndexAuto: break;
						case PassIntObjIndexAutoAbs: break;
						case PassIntMatIndexAbs: break;
						case PassIntMatIndexNorm: break;
						case PassIntMatIndexAuto: break;
						case PassIntMatIndexAutoAbs: break;
						case PassIntAaSamples: break;

						//Processing of mask render passes:
						case PassIntObjIndexMask:
						case PassIntObjIndexMaskShadow:
						case PassIntObjIndexMaskAll:
						case PassIntMatIndexMask:
						case PassIntMatIndexMaskShadow:
						case PassIntMatIndexMaskAll:

							color_passes(idx).clampRgb01();

							if(color_passes.getPassMaskInvert())
							{
								color_passes(idx) = Rgba(1.f) - color_passes(idx);
							}

							if(!color_passes.getPassMaskOnly())
							{
								Rgba col_combined = color_passes(PassIntCombined);
								col_combined.a_ = 1.f;
								color_passes(idx) *= col_combined;
							}
							break;

						default: color_passes(idx) *= wt; break;
					}
				}

				image_film_->addSample(color_passes, j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples);
			}
		}
	}
	return true;
}

#ifndef __clang__
inline
#endif
void TiledIntegrator::generateCommonRenderPasses(ColorPasses &color_passes, RenderState &state, const SurfacePoint &sp, const DiffRay &ray) const
{
	color_passes.probeSet(PassIntUv, Rgba(sp.u_, sp.v_, 0.f, 1.f));
	color_passes.probeSet(PassIntNormalSmooth, Rgba((sp.n_.x_ + 1.f) * .5f, (sp.n_.y_ + 1.f) * .5f, (sp.n_.z_ + 1.f) * .5f, 1.f));
	color_passes.probeSet(PassIntNormalGeom, Rgba((sp.ng_.x_ + 1.f) * .5f, (sp.ng_.y_ + 1.f) * .5f, (sp.ng_.z_ + 1.f) * .5f, 1.f));
	color_passes.probeSet(PassIntDebugDpdu, Rgba((sp.dp_du_.x_ + 1.f) * .5f, (sp.dp_du_.y_ + 1.f) * .5f, (sp.dp_du_.z_ + 1.f) * .5f, 1.f));
	color_passes.probeSet(PassIntDebugDpdv, Rgba((sp.dp_dv_.x_ + 1.f) * .5f, (sp.dp_dv_.y_ + 1.f) * .5f, (sp.dp_dv_.z_ + 1.f) * .5f, 1.f));
	color_passes.probeSet(PassIntDebugDsdu, Rgba((sp.ds_du_.x_ + 1.f) * .5f, (sp.ds_du_.y_ + 1.f) * .5f, (sp.ds_du_.z_ + 1.f) * .5f, 1.f));
	color_passes.probeSet(PassIntDebugDsdv, Rgba((sp.ds_dv_.x_ + 1.f) * .5f, (sp.ds_dv_.y_ + 1.f) * .5f, (sp.ds_dv_.z_ + 1.f) * .5f, 1.f));
	color_passes.probeSet(PassIntDebugNu, Rgba((sp.nu_.x_ + 1.f) * .5f, (sp.nu_.y_ + 1.f) * .5f, (sp.nu_.z_ + 1.f) * .5f, 1.f));
	color_passes.probeSet(PassIntDebugNv, Rgba((sp.nv_.x_ + 1.f) * .5f, (sp.nv_.y_ + 1.f) * .5f, (sp.nv_.z_ + 1.f) * .5f, 1.f));

	if(color_passes.enabled(PassIntReflectAll))
	{
		color_passes(PassIntReflectAll) = color_passes(PassIntReflectPerfect) + color_passes(PassIntGlossy) + color_passes(PassIntGlossyIndirect);
	}

	if(color_passes.enabled(PassIntRefractAll))
	{
		color_passes(PassIntRefractAll) = color_passes(PassIntRefractPerfect) + color_passes(PassIntTrans) + color_passes(PassIntTransIndirect);
	}

	if(color_passes.enabled(PassIntIndirectAll))
	{
		color_passes(PassIntIndirectAll) = color_passes(PassIntIndirect) + color_passes(PassIntDiffuseIndirect);
	}

	color_passes.probeSet(PassIntDiffuseColor, sp.material_->getDiffuseColor(state));
	color_passes.probeSet(PassIntGlossyColor, sp.material_->getGlossyColor(state));
	color_passes.probeSet(PassIntTransColor, sp.material_->getTransColor(state));
	color_passes.probeSet(PassIntSubsurfaceColor, sp.material_->getSubSurfaceColor(state));

	color_passes.probeSet(PassIntObjIndexAbs, sp.object_->getAbsObjectIndexColor());
	color_passes.probeSet(PassIntObjIndexNorm, sp.object_->getNormObjectIndexColor());
	color_passes.probeSet(PassIntObjIndexAuto, sp.object_->getAutoObjectIndexColor());
	color_passes.probeSet(PassIntObjIndexAutoAbs, sp.object_->getAutoObjectIndexNumber());

	color_passes.probeSet(PassIntMatIndexAbs, sp.material_->getAbsMaterialIndexColor());
	color_passes.probeSet(PassIntMatIndexNorm, sp.material_->getNormMaterialIndexColor());
	color_passes.probeSet(PassIntMatIndexAuto, sp.material_->getAutoMaterialIndexColor());
	color_passes.probeSet(PassIntMatIndexAutoAbs, sp.material_->getAutoMaterialIndexNumber());

	if(color_passes.enabled(PassIntObjIndexMask))
	{
		if(sp.object_->getAbsObjectIndex() == color_passes.getPassMaskObjIndex()) color_passes(PassIntObjIndexMask) = Rgba(1.f);
	}

	if(color_passes.enabled(PassIntObjIndexMaskAll))
	{
		color_passes(PassIntObjIndexMaskAll) = color_passes(PassIntObjIndexMask) + color_passes(PassIntObjIndexMaskShadow);
	}

	if(color_passes.enabled(PassIntMatIndexMask))
	{
		if(sp.material_->getAbsMaterialIndex() == color_passes.getPassMaskMatIndex()) color_passes(PassIntMatIndexMask) = Rgba(1.f);
	}

	if(color_passes.enabled(PassIntMatIndexMaskAll))
	{
		color_passes(PassIntMatIndexMaskAll) = color_passes(PassIntMatIndexMask) + color_passes(PassIntMatIndexMaskShadow);
	}

	if(color_passes.enabled(PassIntDebugWireframe))
	{
		Rgba wireframe_color = Rgba(0.f, 0.f, 0.f, 0.f);
		sp.material_->applyWireFrame(wireframe_color, 1.f, sp);
		color_passes(PassIntDebugWireframe) = wireframe_color;
	}

	if(color_passes.enabled(PassIntDebugSamplingFactor))
	{
		color_passes(PassIntDebugSamplingFactor) = Rgba(sp.material_->getSamplingFactor());
	}

	if(color_passes.enabled(PassIntDebugDpLengths) || color_passes.enabled(PassIntDebugDpdx) || color_passes.enabled(PassIntDebugDpdy) || color_passes.enabled(PassIntDebugDpdxy) || color_passes.enabled(PassIntDebugDudxDvdx) || color_passes.enabled(PassIntDebugDudyDvdy) || color_passes.enabled(PassIntDebugDudxyDvdxy))
	{
		SpDifferentials sp_diff(sp, ray);

		if(color_passes.enabled(PassIntDebugDpLengths))
		{
			color_passes(PassIntDebugDpLengths) = Rgba(sp_diff.dp_dx_.length(), sp_diff.dp_dy_.length(), 0.f, 1.f);
		}

		if(color_passes.enabled(PassIntDebugDpdx))
		{
			color_passes.probeSet(PassIntDebugDpdx, Rgba((sp_diff.dp_dx_.x_ + 1.f) * .5f, (sp_diff.dp_dx_.y_ + 1.f) * .5f, (sp_diff.dp_dx_.z_ + 1.f) * .5f, 1.f));
		}

		if(color_passes.enabled(PassIntDebugDpdy))
		{
			color_passes.probeSet(PassIntDebugDpdy, Rgba((sp_diff.dp_dy_.x_ + 1.f) * .5f, (sp_diff.dp_dy_.y_ + 1.f) * .5f, (sp_diff.dp_dy_.z_ + 1.f) * .5f, 1.f));
		}

		if(color_passes.enabled(PassIntDebugDpdxy))
		{
			color_passes.probeSet(PassIntDebugDpdxy, Rgba((sp_diff.dp_dx_.x_ + sp_diff.dp_dy_.x_ + 1.f) * .5f, (sp_diff.dp_dx_.y_ + sp_diff.dp_dy_.y_ + 1.f) * .5f, (sp_diff.dp_dx_.z_ + sp_diff.dp_dy_.z_ + 1.f) * .5f, 1.f));
		}

		if(color_passes.enabled(PassIntDebugDudxDvdx) || color_passes.enabled(PassIntDebugDudyDvdy) || color_passes.enabled(PassIntDebugDudxyDvdxy))
		{

			float du_dx = 0.f, dv_dx = 0.f;
			float du_dy = 0.f, dv_dy = 0.f;
			sp_diff.getUVdifferentials(du_dx, dv_dx, du_dy, dv_dy);

			if(color_passes.enabled(PassIntDebugDudxDvdx))
			{
				color_passes.probeSet(PassIntDebugDudxDvdx, Rgba((du_dx + 1.f) * .5f, (dv_dx + 1.f) * .5f, 0.f, 1.f));
			}

			if(color_passes.enabled(PassIntDebugDudyDvdy))
			{
				color_passes.probeSet(PassIntDebugDudyDvdy, Rgba((du_dy + 1.f) * .5f, (dv_dy + 1.f) * .5f, 0.f, 1.f));
			}

			if(color_passes.enabled(PassIntDebugDudxyDvdxy))
			{
				color_passes.probeSet(PassIntDebugDudxyDvdxy, Rgba((du_dx + du_dy + 1.f) * .5f, (dv_dx + dv_dy + 1.f) * .5f, 0.f, 1.f));
			}

		}
	}
}


END_YAFARAY
