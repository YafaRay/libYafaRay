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
#include "common/layers.h"
#include "volume/volume.h"
#include "common/param.h"
#include "scene/scene.h"
#include "render/progress_bar.h"
#include "common/timer.h"
#include "render/imagefilm.h"
#include "camera/camera.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "light/light.h"
#include "material/material.h"
#include "color/spectrum.h"
#include "color/color_layers.h"
#include "background/background.h"
#include "render/render_data.h"
#include "accelerator/accelerator.h"

BEGIN_YAFARAY

static constexpr int n_max_gather_global = 1000; //used to gather all the photon in the radius. seems could get a better way to do that

SppmIntegrator::SppmIntegrator(Logger &logger, unsigned int d_photons, int passnum, bool transp_shad, int shadow_depth) : MonteCarloIntegrator(logger)
{
	n_photons_ = d_photons;
	pass_num_ = passnum;
	totaln_photons_ = 0;
	initial_factor_ = 1.f;

	s_depth_ = shadow_depth;
	tr_shad_ = transp_shad;
	b_hashgrid_ = false;

	caustic_map_ = std::unique_ptr<PhotonMap>(new PhotonMap(logger));
	caustic_map_->setName("Caustic Photon Map");
	diffuse_map_ = std::unique_ptr<PhotonMap>(new PhotonMap(logger));
	diffuse_map_->setName("Diffuse Photon Map");
}

bool SppmIntegrator::preprocess(const RenderControl &render_control, const RenderView *render_view, ImageFilm *image_film)
{
	image_film_ = image_film;
	return true;
}

bool SppmIntegrator::render(RenderControl &render_control, const RenderView *render_view)
{
	std::stringstream pass_string;
	aa_noise_params_ = scene_->getAaParameters();

	std::stringstream aa_settings;
	aa_settings << " passes=" << pass_num_ << " samples=" << aa_noise_params_.samples_ << " inc_samples=" << aa_noise_params_.inc_samples_;
	aa_settings << " clamp=" << aa_noise_params_.clamp_samples_ << " ind.clamp=" << aa_noise_params_.clamp_indirect_;

	aa_noise_info_ += aa_settings.str();

	render_control.setTotalPasses(pass_num_);	//passNum is total number of passes in SPPM

	aa_sample_multiplier_ = 1.f;
	aa_light_sample_multiplier_ = 1.f;
	aa_indirect_sample_multiplier_ = 1.f;

	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": AA_clamp_samples: ", aa_noise_params_.clamp_samples_);
	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": AA_clamp_indirect: ", aa_noise_params_.clamp_indirect_);

	std::stringstream set;

	set << "SPPM  ";

	if(tr_shad_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}
	set << "RayDepth=" << r_depth_ << "  ";

	render_info_ += set.str();
	if(logger_.isVerbose()) logger_.logVerbose(set.str());


	pass_string << "Rendering pass 1 of " << std::max(1, pass_num_) << "...";
	logger_.logInfo(getName(), ": ", pass_string.str());
	if(intpb_) intpb_->setTag(pass_string.str().c_str());

	g_timer_global.addEvent("rendert");
	g_timer_global.start("rendert");

	image_film_->resetImagesAutoSaveTimer();
	g_timer_global.addEvent("imagesAutoSaveTimer");

	image_film_->resetFilmAutoSaveTimer();
	g_timer_global.addEvent("filmAutoSaveTimer");

	image_film_->init(render_control, pass_num_);
	image_film_->setAaNoiseParams(aa_noise_params_);

	if(render_control.resumed())
	{
		pass_string.clear();
		pass_string << "Loading film file, skipping pass 1...";
		intpb_->setTag(pass_string.str().c_str());
	}

	logger_.logInfo(getName(), ": ", pass_string.str());

	const Camera *camera = render_view->getCamera();

	max_depth_ = 0.f;
	min_depth_ = 1e38f;

	diff_rays_enabled_ = render_control.getDifferentialRaysEnabled();	//enable ray differentials for mipmap calculation if there is at least one image texture using Mipmap interpolation

	if(scene_->getLayers().isDefinedAny({Layer::ZDepthNorm, Layer::Mist})) precalcDepths(render_view);

	int acum_aa_samples = 1;

	initializePpm(render_view); // seems could integrate into the preRender
	if(render_control.resumed())
	{
		acum_aa_samples = image_film_->getSamplingOffset();
		renderPass(render_view, 0, acum_aa_samples, false, 0, render_control);
	}
	else renderPass(render_view, 1, 0, false, 0, render_control);

	std::string initial_estimate = "no";
	if(pm_ire_) initial_estimate = "yes";

	pm_ire_ = false;

	int hp_num = camera->resX() * camera->resY();
	int pass_info = 1;
	for(int i = 1; i < pass_num_; ++i) //progress pass, the offset start from 1 as it is 0 based.
	{
		if(render_control.canceled()) break;
		pass_info = i + 1;
		image_film_->nextPass(render_view, render_control, false, getName(), scene_->getEdgeToonParams());
		n_refined_ = 0;
		renderPass(render_view, 1, acum_aa_samples, false, i, render_control); // offset are only related to the passNum, since we alway have only one sample.
		acum_aa_samples += 1;
		logger_.logInfo(getName(), ": This pass refined ", n_refined_, " of ", hp_num, " pixels.");
	}
	max_depth_ = 0.f;
	g_timer_global.stop("rendert");
	g_timer_global.stop("imagesAutoSaveTimer");
	g_timer_global.stop("filmAutoSaveTimer");
	render_control.setFinished();
	logger_.logInfo(getName(), ": Overall rendertime: ", g_timer_global.getTime("rendert"), "s.");

	// Integrator Settings for "drawRenderSettings()" in imageFilm, SPPM has own render method, so "getSettings()"
	// in integrator.h has no effect and Integrator settings won't be printed to the parameter badge.

	set.clear();

	set << "Passes rendered: " << pass_info << "  ";

	set << "\nPhotons=" << n_photons_ << " search=" << n_search_ << " radius=" << ds_radius_ << "(init.estim=" << initial_estimate << ") total photons=" << totaln_photons_ << "  ";

	render_info_ += set.str();
	if(logger_.isVerbose()) logger_.logVerbose(set.str());

	return true;
}


bool SppmIntegrator::renderTile(RenderArea &a, const RenderView *render_view, const RenderControl &render_control, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number)
{
	int x;
	const Camera *camera = render_view->getCamera();
	const float x_start_film = image_film_->getCx0();
	const float y_start_film = image_film_->getCy0();
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
		aa_max_possible_samples += ceilf(aa_noise_params_.inc_samples_ * math::pow(aa_noise_params_.sample_multiplier_factor_, i));
	}

	float inv_aa_max_possible_samples = 1.f / ((float) aa_max_possible_samples);

	const Layers &layers = scene_->getLayers();
	const MaskParams &mask_params = scene_->getMaskParams();
	ColorLayers color_layers(layers);

	for(int i = a.y_; i < end_y; ++i)
	{
		for(int j = a.x_; j < end_x; ++j)
		{
			if(render_control.canceled()) break;

			rstate.pixel_number_ = x * i + j;
			rstate.sampling_offs_ = sample::fnv32ABuf(i * sample::fnv32ABuf(j)); //fnv_32a_buf(rstate.pixelNumber);
			float toff = Halton::lowDiscrepancySampling(5, pass_offs + rstate.sampling_offs_); // **shall be just the pass number...**

			for(int sample = 0; sample < n_samples; ++sample) //set n_samples = 1.
			{
				rstate.setDefaults();
				rstate.pixel_sample_ = pass_offs + sample;
				rstate.time_ = math::addMod1((float) sample * d_1, toff); //(0.5+(float)sample)*d1;
				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA

				dx = sample::riVdC(rstate.pixel_sample_, rstate.sampling_offs_);
				dy = sample::riS(rstate.pixel_sample_, rstate.sampling_offs_);

				if(sample_lns)
				{
					lens_u = Halton::lowDiscrepancySampling(3, rstate.pixel_sample_ + rstate.sampling_offs_);
					lens_v = Halton::lowDiscrepancySampling(4, rstate.pixel_sample_ + rstate.sampling_offs_);
				}
				c_ray = camera->shootRay(j + dx, i + dy, lens_u, lens_v, wt); // wt need to be considered
				if(wt == 0.0)
				{
					image_film_->addSample(j, i, dx, dy, &a, sample, aa_pass_number, inv_aa_max_possible_samples, &color_layers); //maybe not need
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
					// col = T * L_o + L_v
				}

				c_ray.time_ = rstate.time_;

				//for sppm progressive
				int index = ((i - y_start_film) * camera->resX()) + (j - x_start_film);
				HitPoint &hp = hit_points_[index];

				GatherInfo g_info = traceGatherRay(rstate, c_ray, hp);
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
				const Rgba col_indirect = hp.acc_photon_flux_ / (hp.radius_2_ * math::num_pi * totaln_photons_);
				Rgba color = col_indirect;
				color += g_info.constant_randiance_;
				color.a_ = g_info.constant_randiance_.a_; //the alpha value is hold in the constantRadiance variable
				color_layers(Layer::Combined).color_ = color;

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
						case Layer::Indirect:
							it.second.color_ = col_indirect;
							it.second.color_.a_ = g_info.constant_randiance_.a_;
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

void SppmIntegrator::photonWorker(PhotonMap *diffuse_map, PhotonMap *caustic_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, unsigned int n_photons, const Pdf1D *light_power_d, int num_d_lights, const std::vector<const Light *> &tmplights, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot, int max_bounces, Random &prng)
{
	const Accelerator *accelerator = scene_->getAccelerator();
	if(!accelerator) return;

	Ray ray;
	float light_num_pdf, light_pdf, s_1, s_2, s_3, s_4, s_5, s_6, s_7, s_l;
	Rgb pcol;

	//shoot photons
	bool done = false;
	unsigned int curr = 0;

	SurfacePoint sp;
	RenderData render_data(&prng);
	alignas (16) unsigned char userdata[user_data_size_];
	render_data.arena_ = static_cast<void *>(userdata);
	render_data.cam_ = render_view->getCamera();

	float f_num_lights = (float)num_d_lights;

	unsigned int n_photons_thread = 1 + ((n_photons - 1) / scene->getNumThreadsPhotons());

	std::vector<Photon> local_caustic_photons;
	local_caustic_photons.clear();
	local_caustic_photons.reserve(n_photons_thread);

	std::vector<Photon> local_diffuse_photons;
	local_diffuse_photons.clear();
	local_diffuse_photons.reserve(n_photons_thread);

	//Pregather  photons
	float inv_diff_photons = 1.f / (float)n_photons;

	unsigned int nd_photon_stored = 0;
	//	unsigned int ncPhotonStored = 0;

	while(!done)
	{
		unsigned int haltoncurr = curr + n_photons_thread * thread_id;

		render_data.chromatic_ = true;
		render_data.wavelength_ = Halton::lowDiscrepancySampling(5, haltoncurr);

		// Tried LD, get bad and strange results for some stategy.
		{
			std::lock_guard<std::mutex> lock_halton(mutex_);
			s_1 = hal_1_.getNext();
			s_2 = hal_2_.getNext();
			s_3 = hal_3_.getNext();
			s_4 = hal_4_.getNext();
		}

		s_l = float(haltoncurr) * inv_diff_photons; // Does sL also need more random for each pass?
		int light_num = light_power_d->dSample(logger_, s_l, &light_num_pdf);
		if(light_num >= num_d_lights)
		{
			diffuse_map->mutx_.lock();
			logger_.logError(getName(), ": lightPDF sample error! ", s_l, "/", light_num);
			diffuse_map->mutx_.unlock();
			return;
		}

		pcol = tmplights[light_num]->emitPhoton(s_1, s_2, s_3, s_4, ray, light_pdf);
		ray.tmin_ = scene->ray_min_dist_;
		ray.tmax_ = -1.0;
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of th pdf, hence *=...

		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= n_photons);
			continue;
		}

		int n_bounces = 0;
		bool caustic_photon = false;
		bool direct_photon = true;
		const Material *material = nullptr;
		BsdfFlags bsdfs;

		while(accelerator->intersect(ray, sp))   //scatter photons.
		{
			if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
			{
				diffuse_map->mutx_.lock();
				logger_.logWarning(getName(), ": NaN  on photon color for light", light_num + 1, ".");
				diffuse_map->mutx_.unlock();
				continue;
			}

			Rgb transm(1.f);
			Rgb vcol(0.f);
			const VolumeHandler *vol;

			if(material)
			{
				if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * -ray.dir_ < 0)))
				{
					if(vol->transmittance(render_data, ray, vcol)) transm = vcol;
				}
			}

			Vec3 wi = -ray.dir_, wo;
			material = sp.material_;
			material->initBsdf(render_data, sp, bsdfs);

			//deposit photon on diffuse surface, now we only have one map for all, elimate directPhoton for we estimate it directly
			if(!direct_photon && !caustic_photon && bsdfs.hasAny(BsdfFlags::Diffuse))
			{
				Photon np(wi, sp.p_, pcol);// pcol used here

				if(b_hashgrid_) photon_grid_.pushPhoton(np);
				else
				{
					local_diffuse_photons.push_back(np);
				}
				nd_photon_stored++;
			}
			// add caustic photon
			if(!direct_photon && caustic_photon && bsdfs.hasAny(BsdfFlags::Diffuse | BsdfFlags::Glossy))
			{
				Photon np(wi, sp.p_, pcol);// pcol used here

				if(b_hashgrid_) photon_grid_.pushPhoton(np);
				else
				{
					local_caustic_photons.push_back(np);
				}
				nd_photon_stored++;
			}

			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == max_bounces) break;

			// scatter photon
			s_5 = FastRandom::getNextFloatNormalized(); // now should use this to see correctness
			s_6 = FastRandom::getNextFloatNormalized();
			s_7 = FastRandom::getNextFloatNormalized();

			PSample sample(s_5, s_6, s_7, BsdfFlags::All, pcol, transm);

			bool scattered = material->scatterPhoton(render_data, sp, wi, wo, sample);
			if(!scattered) break; //photon was absorped.  actually based on russian roulette

			pcol = sample.color_;

			caustic_photon = (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
			direct_photon = sample.sampled_flags_.hasAny(BsdfFlags::Filter) && direct_photon;

			if(render_data.chromatic_ && sample.sampled_flags_.hasAny(BsdfFlags::Dispersive))
			{
				render_data.chromatic_ = false;
				Rgb wl_col;
				wl2Rgb_global(render_data.wavelength_, wl_col);
				pcol *= wl_col;
			}

			ray.from_ = sp.p_;
			ray.dir_ = wo;
			ray.tmin_ = scene->ray_min_dist_;
			ray.tmax_ = -1.0;
			++n_bounces;
		}
		++curr;
		if(curr % pb_step == 0)
		{
			pb->update();
			if(render_control.canceled()) { return; }
		}
		done = (curr >= n_photons_thread);
	}
	diffuse_map->mutx_.lock();
	caustic_map->mutx_.lock();
	diffuse_map->appendVector(local_diffuse_photons, curr);
	caustic_map->appendVector(local_caustic_photons, curr);
	total_photons_shot += curr;
	caustic_map->mutx_.unlock();
	diffuse_map->mutx_.unlock();
}


//photon pass, scatter photon
void SppmIntegrator::prePass(int samples, int offset, bool adaptive, const RenderControl &render_control, const RenderView *render_view)
{
	g_timer_global.addEvent("prepass");
	g_timer_global.start("prepass");

	logger_.logInfo(getName(), ": Starting Photon tracing pass...");

	if(b_hashgrid_) photon_grid_.clear();
	else
	{
		diffuse_map_->clear();
		diffuse_map_->setNumPaths(0);
		diffuse_map_->reserveMemory(n_photons_);
		diffuse_map_->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

		caustic_map_->clear();
		caustic_map_->setNumPaths(0);
		caustic_map_->reserveMemory(n_photons_);
		caustic_map_->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());
	}

	lights_ = render_view->getLightsVisible();
	std::vector<const Light *> tmplights;

	//background do not emit photons, or it is merged into normal light?

	Ray ray;
	float light_num_pdf, light_pdf;
	int num_d_lights = 0;
	float f_num_lights = 0.f;
	std::unique_ptr<float[]> energies;
	Rgb pcol;

	tmplights.clear();

	for(int i = 0; i < (int)lights_.size(); ++i)
	{
		num_d_lights++;
		tmplights.push_back(lights_[i]);
	}

	f_num_lights = (float)num_d_lights;
	energies = std::unique_ptr<float[]>(new float[num_d_lights]);

	for(int i = 0; i < num_d_lights; ++i) energies[i] = tmplights[i]->totalEnergy().energy();

	light_power_d_ = std::unique_ptr<Pdf1D>(new Pdf1D(energies.get(), num_d_lights));

	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for photon map:");

	for(int i = 0; i < num_d_lights; ++i)
	{
		pcol = tmplights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
		light_num_pdf = light_power_d_->func_[i] * light_power_d_->inv_integral_;
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", i + 1, "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
	}

	//shoot photons
	unsigned int curr = 0;
	Random prng(rand() + offset * (4517) + 123);
	RenderData render_data(&prng);
	alignas (16) unsigned char userdata[user_data_size_];
	render_data.arena_ = static_cast<void *>(userdata);
	render_data.cam_ = render_view->getCamera();

	std::shared_ptr<ProgressBar> pb;
	std::string previous_progress_tag;
	int previous_progress_total_steps = 0;
	int pb_step;
	if(intpb_)
	{
		pb = intpb_;
		previous_progress_tag = pb->getTag();
		previous_progress_total_steps = pb->getTotalSteps();
	}
	else pb = std::make_shared<ConsoleProgressBar>(80);

	if(b_hashgrid_) logger_.logInfo(getName(), ": Building photon hashgrid...");
	else logger_.logInfo(getName(), ": Building photon map...");

	pb->init(128, logger_.getConsoleLogColorsEnabled());
	pb_step = std::max(1U, n_photons_ / 128);
	pb->setTag(previous_progress_tag + " - building photon map...");

	int n_threads = scene_->getNumThreadsPhotons();

	n_photons_ = std::max((unsigned int) n_threads, (n_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

	logger_.logParams(getName(), ": Shooting ", n_photons_, " photons across ", n_threads, " threads (", (n_photons_ / n_threads), " photons/thread)");

	std::vector<std::thread> threads;
	for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&SppmIntegrator::photonWorker, this, diffuse_map_.get(), caustic_map_.get(), i, scene_, render_view, std::ref(render_control), n_photons_, light_power_d_.get(), num_d_lights, tmplights, pb.get(), pb_step, std::ref(curr), max_bounces_, std::ref(prng)));
	for(auto &t : threads) t.join();

	pb->done();
	pb->setTag(previous_progress_tag + " - photon map built.");
	if(logger_.isVerbose()) logger_.logVerbose(getName(), ":Photon map built.");
	logger_.logInfo(getName(), ": Shot ", curr, " photons from ", num_d_lights, " light(s)");

	totaln_photons_ +=  n_photons_;	// accumulate the total photon number, not using nPath for the case of hashgrid.

	if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored photons: ", diffuse_map_->nPhotons() + caustic_map_->nPhotons());

	if(b_hashgrid_)
	{
		logger_.logInfo(getName(), ": Building photons hashgrid:");
		photon_grid_.updateGrid();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
	}
	else
	{
		if(diffuse_map_->nPhotons() > 0)
		{
			logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
			diffuse_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
		if(caustic_map_->nPhotons() > 0)
		{
			logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
			caustic_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
		if(diffuse_map_->nPhotons() < 50)
		{
			logger_.logError(getName(), ": Too few photons, stopping now.");
			return;
		}
	}

	tmplights.clear();

	g_timer_global.stop("prepass");

	if(b_hashgrid_)
		logger_.logInfo(getName(), ": PhotonGrid building time: ", g_timer_global.getTime("prepass"));
	else
		logger_.logInfo(getName(), ": PhotonMap building time: ", g_timer_global.getTime("prepass"));

	if(intpb_)
	{
		intpb_->setTag(previous_progress_tag);
		intpb_->init(previous_progress_total_steps, logger_.getConsoleLogColorsEnabled());
	}
	return;
}

//now it's a dummy function
Rgba SppmIntegrator::integrate(RenderData &render_data, const DiffRay &ray, int additional_depth, ColorLayers *color_layers, const RenderView *render_view) const
{
	return Rgba(0.f);
}


GatherInfo SppmIntegrator::traceGatherRay(RenderData &render_data, DiffRay &ray, HitPoint &hp, ColorLayers *color_layers)
{
	const Accelerator *accelerator = scene_->getAccelerator();
	if(!accelerator) return {};

	const bool layers_used = render_data.raylevel_ == 1 && color_layers && color_layers->getFlags() != Layer::Flags::None;

	static int n_max_global = 0;
	static int calls_global = 0;
	++calls_global;
	GatherInfo g_info;

	float alpha;
	SurfacePoint sp;

	void *o_udat = render_data.arena_;
	const bool old_lights_geometry_material_emit = render_data.lights_geometry_material_emit_;

	if(transp_background_) alpha = 0.0;
	else alpha = 1.0;

	if(accelerator->intersect(ray, sp))
	{
		alignas (16) unsigned char userdata[user_data_size_];
		render_data.arena_ = static_cast<void *>(userdata);
		if(render_data.raylevel_ == 0)
		{
			render_data.chromatic_ = true;
			render_data.lights_geometry_material_emit_ = true;
		}

		BsdfFlags bsdfs;
		int additional_depth = 0;

		Vec3 wo = -ray.dir_;
		const Material *material = sp.material_;
		material->initBsdf(render_data, sp, bsdfs);

		if(additional_depth < material->getAdditionalDepth()) additional_depth = material->getAdditionalDepth();

		const Rgb col_emit = material->emit(render_data, sp, wo);
		g_info.constant_randiance_ += col_emit; //add only once, but FG seems add twice?
		if(layers_used)
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_emit;
		}
		render_data.lights_geometry_material_emit_ = false;
		SpDifferentials sp_diff(sp, ray);

		if(bsdfs.hasAny(BsdfFlags::Diffuse))
		{
			g_info.constant_randiance_ += estimateAllDirectLight(render_data, sp, wo, color_layers);
		}

		// estimate radiance using photon map
		auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_max_gather_global]);

		//if PM_IRE is on. we should estimate the initial radius using the photonMaps. (PM_IRE is only for the first pass, so not consume much time)
		if(pm_ire_ && !hp.radius_setted_) // "waste" two gather here as it has two maps now. This make the logic simple.
		{
			float radius_1 = ds_radius_ * ds_radius_;
			float radius_2 = radius_1;
			int n_gathered_1 = 0, n_gathered_2 = 0;

			if(diffuse_map_->nPhotons() > 0)
				n_gathered_1 = diffuse_map_->gather(sp.p_, gathered.get(), n_search_, radius_1);
			if(caustic_map_->nPhotons() > 0)
				n_gathered_2 = caustic_map_->gather(sp.p_, gathered.get(), n_search_, radius_2);
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
			n_gathered = photon_grid_.gather(sp.p_, gathered.get(), n_max_gather_global, radius_2); // disable now
		else
		{
			if(diffuse_map_->nPhotons() > 0) // this is needed to avoid a runtime error.
			{
				n_gathered = diffuse_map_->gather(sp.p_, gathered.get(), n_max_gather_global, radius_2); //we always collected all the photon inside the radius
			}

			if(n_gathered > 0)
			{
				if(n_gathered > n_max_global)
				{
					n_max_global = n_gathered;
					if(logger_.isDebug())
					{
						logger_.logDebug("maximum Photons: ", n_max_global, ", radius2: ", radius_2);
						if(n_max_global == 10)
						{
							for(int j = 0; j < n_gathered; ++j)
							{
								logger_.logDebug("col:", gathered[j].photon_->color());
							}
						}
					}
				}
				for(int i = 0; i < n_gathered; ++i)
				{
					////test if the photon is in the ellipsoid
					//vector3d_t scale  = sp.P - gathered[i].photon->pos;
					//vector3d_t temp;
					//temp.x = scale VDOT sp.NU;
					//temp.y = scale VDOT sp.NV;
					//temp.z = scale VDOT sp.N;

					//double inv_radi = 1 / sqrt(radius2);
					//temp.x  *= inv_radi; temp.y *= inv_radi; temp.z *=  1. / (2.f * scene->rayMinDist);
					//if(temp.lengthSqr() > 1.)continue;

					g_info.photon_count_++;
					Vec3 pdir = gathered[i].photon_->direction();
					Rgb surf_col = material->eval(render_data, sp, wo, pdir, BsdfFlags::Diffuse); // seems could speed up using rho, (something pbrt made)
					g_info.photon_flux_ += surf_col * gathered[i].photon_->color();// * std::abs(sp.N*pdir); //< wrong!?
					//Rgb  flux= surfCol * gathered[i].photon->color();// * std::abs(sp.N*pdir); //< wrong!?

					////start refine here
					//double ALPHA = 0.7;
					//double g = (hp.accPhotonCount*ALPHA+ALPHA) / (hp.accPhotonCount*ALPHA+1.0);
					//hp.radius2 *= g;
					//hp.accPhotonCount++;
					//hp.accPhotonFlux=((Rgb)hp.accPhotonFlux+flux)*g;
				}
			}

			// gather caustics photons
			if(bsdfs.hasAny(BsdfFlags::Diffuse) && caustic_map_->ready())
			{

				radius_2 = hp.radius_2_; //reset radius2 & nGathered
				n_gathered = caustic_map_->gather(sp.p_, gathered.get(), n_max_gather_global, radius_2);
				if(n_gathered > 0)
				{
					Rgb surf_col(0.f);
					for(int i = 0; i < n_gathered; ++i)
					{
						Vec3 pdir = gathered[i].photon_->direction();
						g_info.photon_count_++;
						surf_col = material->eval(render_data, sp, wo, pdir, BsdfFlags::All); // seems could speed up using rho, (something pbrt made)
						g_info.photon_flux_ += surf_col * gathered[i].photon_->color();// * std::abs(sp.N*pdir); //< wrong!?//gInfo.photonFlux += colorPasses.probe_add(PASS_INT_DIFFUSE_INDIRECT, surfCol * gathered[i].photon->color(), state.raylevel == 0);// * std::abs(sp.N*pdir); //< wrong!?
						//Rgb  flux= surfCol * gathered[i].photon->color();// * std::abs(sp.N*pdir); //< wrong!?

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

		render_data.raylevel_++;
		if(render_data.raylevel_ <= (r_depth_ + additional_depth))
		{
			Halton hal_2(2);
			Halton hal_3(3);
			// dispersive effects with recursive raytracing:
			if(bsdfs.hasAny(BsdfFlags::Dispersive) && render_data.chromatic_)
			{
				render_data.lights_geometry_material_emit_ = false; //debatable...
				int dsam = 8;
				int old_division = render_data.ray_division_;
				int old_offset = render_data.ray_offset_;
				float old_dc_1 = render_data.dc_1_, old_dc_2 = render_data.dc_2_;
				if(render_data.ray_division_ > 1) dsam = std::max(1, dsam / old_division);
				render_data.ray_division_ *= dsam;
				int branch = render_data.ray_division_ * old_offset;
				float d_1 = 1.f / (float)dsam;
				float ss_1 = sample::riS(render_data.pixel_sample_ + render_data.sampling_offs_);
				Rgb vcol(1.f);
				Vec3 wi;
				const VolumeHandler *vol;
				DiffRay ref_ray;
				float w = 0.f;
				GatherInfo cing, t_cing; //Dispersive is different handled, not same as GLOSSY, at the BSDF_VOLUMETRIC part

				Rgb dcol_trans_accum;
				for(int ns = 0; ns < dsam; ++ns)
				{
					render_data.wavelength_ = (ns + ss_1) * d_1;
					render_data.dc_1_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 1, branch + render_data.sampling_offs_);
					render_data.dc_2_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 2, branch + render_data.sampling_offs_);
					if(old_division > 1) render_data.wavelength_ = math::addMod1(render_data.wavelength_, old_dc_1);
					render_data.ray_offset_ = branch;
					++branch;
					Sample s(0.5f, 0.5f, BsdfFlags::Reflect | BsdfFlags::Transmit | BsdfFlags::Dispersive);
					Rgb mcol = material->sample(render_data, sp, wo, wi, s, w);

					if(s.pdf_ > 1.0e-6f && s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
					{
						render_data.chromatic_ = false;
						Rgb wl_col;
						wl2Rgb_global(render_data.wavelength_, wl_col);
						ref_ray = DiffRay(sp.p_, wi, scene_->ray_min_dist_);
						t_cing = traceGatherRay(render_data, ref_ray, hp);
						t_cing.photon_flux_ *= mcol * wl_col * w;
						t_cing.constant_randiance_ *= mcol * wl_col * w;

						if(layers_used)
						{
							if(color_layers->find(Layer::Trans)) dcol_trans_accum += t_cing.constant_randiance_;
						}

						render_data.chromatic_ = true;
					}
					cing += t_cing;
				}
				if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
				{
					vol->transmittance(render_data, ref_ray, vcol);
					cing.photon_flux_ *= vcol;
					cing.constant_randiance_ *= vcol;
				}

				g_info.constant_randiance_ += cing.constant_randiance_ * d_1;
				g_info.photon_flux_ += cing.photon_flux_ * d_1;
				g_info.photon_count_ += cing.photon_count_ * d_1;

				if(layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::Trans))
					{
						dcol_trans_accum *= d_1;
						color_layer->color_ += dcol_trans_accum;
					}
				}

				render_data.ray_division_ = old_division;
				render_data.ray_offset_ = old_offset;
				render_data.dc_1_ = old_dc_1; render_data.dc_2_ = old_dc_2;
			}

			// glossy reflection with recursive raytracing:  Pure GLOSSY material doesn't hold photons?

			if(bsdfs.hasAny(BsdfFlags::Glossy))
			{
				render_data.lights_geometry_material_emit_ = false;
				int gsam = 8;
				int old_division = render_data.ray_division_;
				int old_offset = render_data.ray_offset_;
				float old_dc_1 = render_data.dc_1_, old_dc_2 = render_data.dc_2_;
				if(render_data.ray_division_ > 1) gsam = std::max(1, gsam / old_division);
				render_data.ray_division_ *= gsam;
				int branch = render_data.ray_division_ * old_offset;
				unsigned int offs = gsam * render_data.pixel_sample_ + render_data.sampling_offs_;
				float d_1 = 1.f / (float)gsam;
				Rgb vcol(1.f);
				Vec3 wi;
				const VolumeHandler *vol;
				DiffRay ref_ray;

				GatherInfo ging, t_ging;

				hal_2.setStart(offs);
				hal_3.setStart(offs);

				Rgb gcol_indirect_accum;
				Rgb gcol_reflect_accum;
				Rgb gcol_transmit_accum;

				for(int ns = 0; ns < gsam; ++ns)
				{
					render_data.dc_1_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 1, branch + render_data.sampling_offs_);
					render_data.dc_2_ = Halton::lowDiscrepancySampling(2 * render_data.raylevel_ + 2, branch + render_data.sampling_offs_);
					render_data.ray_offset_ = branch;
					++offs;
					++branch;

					float s_1 = hal_2.getNext();
					float s_2 = hal_3.getNext();

					float W = 0.f;

					Sample s(s_1, s_2, BsdfFlags::AllGlossy);
					Rgb mcol = material->sample(render_data, sp, wo, wi, s, W);

					if(material->getFlags().hasAny(BsdfFlags::Reflect) && !material->getFlags().hasAny(BsdfFlags::Transmit))
					{
						float w = 0.f;

						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Reflect);
						Rgb mcol = material->sample(render_data, sp, wo, wi, s, w);
						Rgba integ = 0.f;
						ref_ray = DiffRay(sp.p_, wi, scene_->ray_min_dist_);
						if(s.sampled_flags_.hasAny(BsdfFlags::Reflect)) sp_diff.reflectedRay(ray, ref_ray);
						else if(s.sampled_flags_.hasAny(BsdfFlags::Transmit)) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
						integ = (Rgb) integrate(render_data, ref_ray, additional_depth, nullptr, nullptr);

						if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
						{
							if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
						}

						//gcol += tmpColorPasses.probe_add(PASS_INT_GLOSSY_INDIRECT, (Rgb)integ * mcol * W, state.raylevel == 1);
						t_ging = traceGatherRay(render_data, ref_ray, hp);
						t_ging.photon_flux_ *= mcol * w;
						t_ging.constant_randiance_ *= mcol * w;
						ging += t_ging;
					}
					else if(material->getFlags().hasAny(BsdfFlags::Reflect) && material->getFlags().hasAny(BsdfFlags::Transmit))
					{
						Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::AllGlossy);
						Rgb mcol[2];
						float w[2];
						Vec3 dir[2];

						mcol[0] = material->sample(render_data, sp, wo, dir, mcol[1], s, w);
						Rgba integ = 0.f;

						if(s.sampled_flags_.hasAny(BsdfFlags::Reflect) && !s.sampled_flags_.hasAny(BsdfFlags::Dispersive))
						{
							ref_ray = DiffRay(sp.p_, dir[0], scene_->ray_min_dist_);
							sp_diff.reflectedRay(ray, ref_ray);
							integ = integrate(render_data, ref_ray, additional_depth, nullptr, nullptr);
							if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
							{
								if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
							}
							Rgb col_reflect_factor = mcol[0] * w[0];

							t_ging = traceGatherRay(render_data, ref_ray, hp);
							t_ging.photon_flux_ *= col_reflect_factor;
							t_ging.constant_randiance_ *= col_reflect_factor;

							if(layers_used)
							{
								if(color_layers->find(Layer::GlossyIndirect)) gcol_indirect_accum += (Rgb) t_ging.constant_randiance_;
							}
							ging += t_ging;
						}

						if(s.sampled_flags_.hasAny(BsdfFlags::Transmit))
						{
							ref_ray = DiffRay(sp.p_, dir[1], scene_->ray_min_dist_);
							sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
							integ = integrate(render_data, ref_ray, additional_depth, nullptr, nullptr);
							if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
							{
								if(vol->transmittance(render_data, ref_ray, vcol)) integ *= vcol;
							}

							Rgb col_transmit_factor = mcol[1] * w[1];
							alpha = integ.a_;
							t_ging = traceGatherRay(render_data, ref_ray, hp);
							t_ging.photon_flux_ *= col_transmit_factor;
							t_ging.constant_randiance_ *= col_transmit_factor;
							if(layers_used)
							{
								if(color_layers->find(Layer::GlossyIndirect)) gcol_transmit_accum += (Rgb) t_ging.constant_randiance_;
							}
							ging += t_ging;
						}
					}

					else if(s.sampled_flags_.hasAny(BsdfFlags::Glossy))
					{
						ref_ray = DiffRay(sp.p_, wi, scene_->ray_min_dist_);
						if(diff_rays_enabled_)
						{
							if(s.sampled_flags_.hasAny(BsdfFlags::Reflect)) sp_diff.reflectedRay(ray, ref_ray);
							else if(s.sampled_flags_.hasAny(BsdfFlags::Transmit)) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
						}

						t_ging = traceGatherRay(render_data, ref_ray, hp);
						t_ging.photon_flux_ *= mcol * W;
						t_ging.constant_randiance_ *= mcol * W;
						if(layers_used)
						{
							if(color_layers->find(Layer::Trans)) gcol_reflect_accum += t_ging.constant_randiance_;
						}
						ging += t_ging;
					}

					if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
					{
						if(vol->transmittance(render_data, ref_ray, vcol))
						{
							ging.photon_flux_ *= vcol;
							ging.constant_randiance_ *= vcol;
						}
					}

				}

				g_info.constant_randiance_ += ging.constant_randiance_ * d_1;
				g_info.photon_flux_ += ging.photon_flux_ * d_1;
				g_info.photon_count_ += ging.photon_count_ * d_1;

				if(layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::GlossyIndirect))
					{
						gcol_indirect_accum *= d_1;
						color_layer->color_ += gcol_indirect_accum;
					}
					if(ColorLayer *color_layer = color_layers->find(Layer::Trans))
					{
						gcol_reflect_accum *= d_1;
						color_layer->color_ += gcol_reflect_accum;
					}
					if(ColorLayer *color_layer = color_layers->find(Layer::GlossyIndirect))
					{
						gcol_transmit_accum *= d_1;
						color_layer->color_ += gcol_transmit_accum;
					}
				}

				render_data.ray_division_ = old_division;
				render_data.ray_offset_ = old_offset;
				render_data.dc_1_ = old_dc_1;
				render_data.dc_2_ = old_dc_2;
			}

			//...perfect specular reflection/refraction with recursive raytracing...
			if(bsdfs.hasAny(BsdfFlags::Specular | BsdfFlags::Filter))
			{
				render_data.lights_geometry_material_emit_ = true;
				const Material::Specular specular = material->getSpecular(render_data, sp, wo);
				if(specular.reflect_.enabled_)
				{
					DiffRay ref_ray(sp.p_, specular.reflect_.dir_, scene_->ray_min_dist_);
					if(diff_rays_enabled_) sp_diff.reflectedRay(ray, ref_ray); // compute the ray differentaitl
					GatherInfo refg = traceGatherRay(render_data, ref_ray, hp);
					const VolumeHandler *vol;
					if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
					{
						Rgb vcol;
						if(vol->transmittance(render_data, ref_ray, vcol))
						{
							refg.constant_randiance_ *= vcol;
							refg.photon_flux_ *= vcol;
						}
					}
					const Rgba col_radiance_reflect = refg.constant_randiance_ * Rgba(specular.reflect_.col_);
					g_info.constant_randiance_ += col_radiance_reflect;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::ReflectPerfect)) color_layer->color_ += col_radiance_reflect;
					}
					g_info.photon_flux_ += refg.photon_flux_ * Rgba(specular.reflect_.col_);
					g_info.photon_count_ += refg.photon_count_;
				}
				if(specular.refract_.enabled_)
				{
					DiffRay ref_ray(sp.p_, specular.refract_.dir_, scene_->ray_min_dist_);
					if(diff_rays_enabled_) sp_diff.refractedRay(ray, ref_ray, material->getMatIor());
					GatherInfo refg = traceGatherRay(render_data, ref_ray, hp);
					const VolumeHandler *vol;
					if(bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * ref_ray.dir_ < 0)))
					{
						Rgb vcol;
						if(vol->transmittance(render_data, ref_ray, vcol))
						{
							refg.constant_randiance_ *= vcol;
							refg.photon_flux_ *= vcol;
						}
					}
					const Rgba col_radiance_refract = refg.constant_randiance_ * Rgba(specular.refract_.col_);
					g_info.constant_randiance_ += col_radiance_refract;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::RefractPerfect)) color_layer->color_ += col_radiance_refract;
					}
					g_info.photon_flux_ += refg.photon_flux_ * Rgba(specular.refract_.col_);
					g_info.photon_count_ += refg.photon_count_;
					alpha = refg.constant_randiance_.a_;
				}
			}
		}
		--render_data.raylevel_;

		if(layers_used)
		{
			generateCommonLayers(render_data, sp, ray, scene_->getMaskParams(), color_layers);

			if(ColorLayer *color_layer = color_layers->find(Layer::Ao))
			{
				color_layer->color_ = sampleAmbientOcclusionLayer(render_data, sp, wo);
			}

			if(ColorLayer *color_layer = color_layers->find(Layer::AoClay))
			{
				color_layer->color_ = sampleAmbientOcclusionClayLayer(render_data, sp, wo);
			}
		}

		if(transp_refracted_background_)
		{
			float m_alpha = material->getAlpha(render_data, sp, wo);
			alpha = m_alpha + (1.f - m_alpha) * alpha;
		}
		else alpha = 1.0;
	}

	else //nothing hit, return background
	{
		if(scene_->getBackground() && !transp_refracted_background_)
		{
			const Rgb col_tmp = (*scene_->getBackground())(ray, render_data);
			g_info.constant_randiance_ += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Env)) color_layer->color_ = col_tmp;
			}
		}
	}

	render_data.arena_ = o_udat;
	render_data.lights_geometry_material_emit_ = old_lights_geometry_material_emit;

	Rgba col_vol_transmittance = scene_->vol_integrator_->transmittance(render_data, ray);
	Rgba col_vol_integration = scene_->vol_integrator_->integrate(render_data, ray);

	if(transp_background_) alpha = std::max(alpha, 1.f - col_vol_transmittance.r_);

	if(layers_used)
	{
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeTransmittance)) color_layer->color_ = col_vol_transmittance;
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeIntegration)) color_layer->color_ = col_vol_integration;
	}

	g_info.constant_randiance_ = (g_info.constant_randiance_ * col_vol_transmittance) + col_vol_integration;
	g_info.constant_randiance_.a_ = alpha; // a small trick for just hold the alpha value.
	return g_info;
}

void SppmIntegrator::initializePpm(const RenderView *render_view)
{
	const Camera *camera = render_view->getCamera();
	unsigned int resolution = camera->resX() * camera->resY();

	hit_points_.reserve(resolution);
	Bound b_box = scene_->getSceneBound(); // Now using Scene Bound, this could get a bigger initial radius, and need more tests

	// initialize SPPM statistics
	float initial_radius = ((b_box.longX() + b_box.longY() + b_box.longZ()) / 3.f) / ((camera->resX() + camera->resY()) / 2.0f) * 2.f ;
	initial_radius = std::min(initial_radius, 1.f); //Fix the overflow bug
	for(unsigned int i = 0; i < resolution; i++)
	{
		HitPoint hp;
		hp.acc_photon_flux_  = Rgba(0.f);
		hp.acc_photon_count_ = 0;
		hp.radius_2_ = (initial_radius * initial_factor_) * (initial_radius * initial_factor_);
		hp.constant_randiance_ = Rgba(0.f);
		hp.radius_setted_ = false;	   // the flag used for IRE

		hit_points_.push_back(hp);
	}

	if(b_hashgrid_) photon_grid_.setParm(initial_radius * 2.f, n_photons_, b_box);

}

std::unique_ptr<Integrator> SppmIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	bool transp_shad = false;
	bool pm_ire = false;
	int shadow_depth = 5; //may used when integrate Direct Light
	int raydepth = 5;
	int pass_num = 1000;
	int num_photons = 500000;
	int bounces = 5;
	float times = 1.f;
	int search_num = 100;
	float ds_rad = 1.0f;
	bool do_ao = false;
	int ao_samples = 32;
	double ao_dist = 1.0;
	Rgb ao_col(1.f);
	bool bg_transp = false;
	bool bg_transp_refract = false;

	params.getParam("transpShad", transp_shad);
	params.getParam("shadowDepth", shadow_depth);
	params.getParam("raydepth", raydepth);
	params.getParam("photons", num_photons);
	params.getParam("passNums", pass_num);
	params.getParam("bounces", bounces);
	params.getParam("times", times); // initial radius times

	params.getParam("photonRadius", ds_rad);
	params.getParam("searchNum", search_num);
	params.getParam("pmIRE", pm_ire);

	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);
	params.getParam("do_AO", do_ao);
	params.getParam("AO_samples", ao_samples);
	params.getParam("AO_distance", ao_dist);
	params.getParam("AO_color", ao_col);

	auto inte = std::unique_ptr<SppmIntegrator>(new SppmIntegrator(logger, num_photons, pass_num, transp_shad, shadow_depth));
	inte->r_depth_ = raydepth;
	inte->max_bounces_ = bounces;
	inte->initial_factor_ = times;

	inte->ds_radius_ = ds_rad; // under tests enable now
	inte->n_search_ = search_num;
	inte->pm_ire_ = pm_ire;
	// Background settings
	inte->transp_background_ = bg_transp;
	inte->transp_refracted_background_ = bg_transp_refract;
	// AO settings
	inte->use_ambient_occlusion_ = do_ao;
	inte->ao_samples_ = ao_samples;
	inte->ao_dist_ = ao_dist;
	inte->ao_col_ = ao_col;

	return inte;
}

END_YAFARAY
