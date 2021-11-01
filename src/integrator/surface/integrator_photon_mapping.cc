/****************************************************************************
 *      photonintegr.cc: integrator for photon mapping and final gather
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

#include "integrator/surface/integrator_photon_mapping.h"
#include "geometry/surface.h"
#include "volume/volume.h"
#include "color/color_layers.h"
#include "common/param.h"
#include "scene/scene.h"
#include "render/progress_bar.h"
#include "sampler/halton.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "light/light.h"
#include "material/material.h"
#include "common/timer.h"
#include "background/background.h"
#include "render/imagefilm.h"
#include "render/render_data.h"
#include "accelerator/accelerator.h"
#include "photon/photon.h"

BEGIN_YAFARAY

void PhotonIntegrator::preGatherWorker(PreGatherData *gdata, float ds_rad, int n_search)
{
	unsigned int start, end, total;
	float ds_radius_2 = ds_rad * ds_rad;

	gdata->mutx_.lock();
	start = gdata->fetched_;
	total = gdata->rad_points_.size();
	end = gdata->fetched_ = std::min(total, start + 32);
	gdata->mutx_.unlock();

	auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_search]);

	float radius = 0.f;
	float i_scale = 1.f / ((float)gdata->diffuse_map_->nPaths() * math::num_pi);
	float scale = 0.f;

	while(start < total)
	{
		for(unsigned int n = start; n < end; ++n)
		{
			radius = ds_radius_2;//actually the square radius...
			int n_gathered = gdata->diffuse_map_->gather(gdata->rad_points_[n].pos_, gathered.get(), n_search, radius);

			Vec3 rnorm = gdata->rad_points_[n].normal_;

			Rgb sum(0.0);

			if(n_gathered > 0)
			{
				scale = i_scale / radius;

				for(int i = 0; i < n_gathered; ++i)
				{
					Vec3 pdir = gathered[i].photon_->direction();

					if(rnorm * pdir > 0.f) sum += gdata->rad_points_[n].refl_ * scale * gathered[i].photon_->color();
					else sum += gdata->rad_points_[n].transm_ * scale * gathered[i].photon_->color();
				}
			}

			gdata->radiance_vec_[n] = Photon(rnorm, gdata->rad_points_[n].pos_, sum);
		}
		gdata->mutx_.lock();
		start = gdata->fetched_;
		end = gdata->fetched_ = std::min(total, start + 32);
		gdata->pbar_->update(32);
		gdata->mutx_.unlock();
	}
}

PhotonIntegrator::PhotonIntegrator(Logger &logger, unsigned int d_photons, unsigned int c_photons, bool transp_shad, int shadow_depth, float ds_rad, float c_rad) : MonteCarloIntegrator(logger)
{
	use_photon_caustics_ = true;
	use_photon_diffuse_ = true;
	tr_shad_ = transp_shad;
	final_gather_ = true;
	n_diffuse_photons_ = d_photons;
	n_caus_photons_ = c_photons;
	s_depth_ = shadow_depth;
	ds_radius_ = ds_rad;
	caus_radius_ = c_rad;
	r_depth_ = 6;
	max_bounces_ = 5;

	diffuse_map_ = std::unique_ptr<PhotonMap>(new PhotonMap(logger));
	diffuse_map_->setName("Diffuse Photon Map");
	radiance_map_ = std::unique_ptr<PhotonMap>(new PhotonMap(logger));
	radiance_map_->setName("FG Radiance Photon Map");
}

void PhotonIntegrator::diffuseWorker(PhotonMap *diffuse_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, const Timer &timer, unsigned int n_diffuse_photons, const Pdf1D *light_power_d, int num_d_lights, const std::vector<const Light *> &tmplights, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot, int max_bounces, bool final_gather, PreGatherData &pgdat)
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
	RenderData render_data;
	render_data.cam_ = render_view->getCamera();

	float f_num_lights = (float)num_d_lights;

	unsigned int n_diffuse_photons_thread = 1 + ((n_diffuse_photons - 1) / scene->getNumThreadsPhotons());

	std::vector<Photon> local_diffuse_photons;
	std::vector<RadData> local_rad_points;

	local_diffuse_photons.clear();
	local_diffuse_photons.reserve(n_diffuse_photons_thread);
	local_rad_points.clear();

	float inv_diff_photons = 1.f / (float)n_diffuse_photons;

	while(!done)
	{
		unsigned int haltoncurr = curr + n_diffuse_photons_thread * thread_id;

		s_1 = sample::riVdC(haltoncurr);
		s_2 = Halton::lowDiscrepancySampling(2, haltoncurr);
		s_3 = Halton::lowDiscrepancySampling(3, haltoncurr);
		s_4 = Halton::lowDiscrepancySampling(4, haltoncurr);

		s_l = float(haltoncurr) * inv_diff_photons;
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
			done = (curr >= n_diffuse_photons_thread);
			continue;
		}

		int n_bounces = 0;
		bool caustic_photon = false;
		bool direct_photon = true;
		const Material *material = nullptr;

		while(accelerator->intersect(ray, sp, render_data.cam_))
		{
			if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
			{
				diffuse_map->mutx_.lock();
				logger_.logWarning(getName(), ": NaN  on photon color for light", light_num + 1, ".");
				diffuse_map->mutx_.unlock();
				continue;
			}

			Rgb transm(1.f);

			const BsdfFlags &mat_bsdfs = sp.mat_data_->bsdf_flags_;
			if(material)
			{
				//FIXME??? HOW DOES THIS WORK, WITH PREVIOUS MATERIAL AND FLAGS?? Then this needs to be properly written to account for both previous material and previous mat_data!
				const VolumeHandler *vol;
				if(mat_bsdfs.hasAny(BsdfFlags::Volumetric) && (vol = material->getVolumeHandler(sp.ng_ * -ray.dir_ < 0)))
				{
					transm = vol->transmittance(ray);
				}
			}

			Vec3 wi = -ray.dir_, wo;
			material = sp.material_;

			if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
			{
				//deposit photon on surface
				if(!caustic_photon)
				{
					Photon np(wi, sp.p_, pcol);
					local_diffuse_photons.push_back(np);
				}
				// create entry for radiance photon:
				// don't forget to choose subset only, face normal forward; geometric vs. smooth normal?
				if(final_gather && FastRandom::getNextFloatNormalized() < 0.125 && !caustic_photon)
				{
					Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wi);
					RadData rd(sp.p_, n);
					rd.refl_ = material->getReflectivity(sp.mat_data_.get(), sp, BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Reflect, render_data.chromatic_, render_data.wavelength_, render_data.cam_);
					rd.transm_ = material->getReflectivity(sp.mat_data_.get(), sp, BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Transmit, render_data.chromatic_, render_data.wavelength_, render_data.cam_);
					local_rad_points.push_back(rd);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == max_bounces) break;
			// scatter photon
			int d_5 = 3 * n_bounces + 5;

			s_5 = Halton::lowDiscrepancySampling(d_5, haltoncurr);
			s_6 = Halton::lowDiscrepancySampling(d_5 + 1, haltoncurr);
			s_7 = Halton::lowDiscrepancySampling(d_5 + 2, haltoncurr);

			PSample sample(s_5, s_6, s_7, BsdfFlags::All, pcol, transm);

			bool scattered = material->scatterPhoton(sp.mat_data_.get(), sp, wi, wo, sample, render_data.chromatic_, render_data.wavelength_, render_data.cam_);
			if(!scattered) break; //photon was absorped.

			pcol = sample.color_;

			caustic_photon = (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
			direct_photon = sample.sampled_flags_.hasAny(BsdfFlags::Filter) && direct_photon;

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
		done = (curr >= n_diffuse_photons_thread);
	}
	diffuse_map->mutx_.lock();
	diffuse_map->appendVector(local_diffuse_photons, curr);
	total_photons_shot += curr;
	diffuse_map->mutx_.unlock();

	pgdat.mutx_.lock();
	pgdat.rad_points_.insert(std::end(pgdat.rad_points_), std::begin(local_rad_points), std::end(local_rad_points));
	pgdat.mutx_.unlock();
}

void PhotonIntegrator::photonMapKdTreeWorker(PhotonMap *photon_map)
{
	photon_map->updateTree();
}

bool PhotonIntegrator::preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film)
{
	image_film_ = image_film;
	std::shared_ptr<ProgressBar> pb;
	if(intpb_) pb = intpb_;
	else pb = std::make_shared<ConsoleProgressBar>(80);

	lookup_rad_ = 4 * ds_radius_ * ds_radius_;

	std::stringstream set;

	timer.addEvent("prepass");
	timer.start("prepass");

	logger_.logInfo(getName(), ": Starting preprocess...");

	set << "Photon Mapping  ";

	if(tr_shad_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}
	set << "RayDepth=" << r_depth_ << "  ";

	lights_ = render_view->getLightsVisible();
	std::vector<const Light *> tmplights;

	if(use_photon_caustics_)
	{
		set << "\nCaustic photons=" << n_caus_photons_ << " search=" << n_caus_search_ << " radius=" << caus_radius_ << " depth=" << caus_depth_ << "  ";
	}

	if(use_photon_diffuse_)
	{
		set << "\nDiffuse photons=" << n_diffuse_photons_ << " search=" << n_diffuse_search_ << " radius=" << ds_radius_ << "  ";
	}

	if(final_gather_)
	{
		set << " FG paths=" << n_paths_ << " bounces=" << gather_bounces_ << "  ";
	}

	if(photon_map_processing_ == PhotonsLoad)
	{
		bool caustic_map_failed_load = false;
		bool diffuse_map_failed_load = false;
		bool fg_radiance_map_failed_load = false;

		if(use_photon_caustics_)
		{
			pb->setTag("Loading caustic photon map from file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_caustic.photonmap";
			logger_.logInfo(getName(), ": Loading caustic photon map from: ", filename, ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(caustic_map_->load(filename))
			{
				if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic map loaded.");
			}
			else caustic_map_failed_load = true;
		}

		if(use_photon_diffuse_)
		{
			pb->setTag("Loading diffuse photon map from file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_diffuse.photonmap";
			logger_.logInfo(getName(), ": Loading diffuse photon map from: ", filename, ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(diffuse_map_->load(filename))
			{
				if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse map loaded.");
			}
			else diffuse_map_failed_load = true;
		}

		if(use_photon_diffuse_ && final_gather_)
		{
			pb->setTag("Loading FG radiance photon map from file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_fg_radiance.photonmap";
			logger_.logInfo(getName(), ": Loading FG radiance photon map from: ", filename, ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(radiance_map_->load(filename))
			{
				if(logger_.isVerbose()) logger_.logVerbose(getName(), ": FG radiance map loaded.");
			}
			else fg_radiance_map_failed_load = true;
		}

		if(caustic_map_failed_load || diffuse_map_failed_load || fg_radiance_map_failed_load)
		{
			photon_map_processing_ = PhotonsGenerateAndSave;
			logger_.logWarning(getName(), ": photon maps loading failed, changing to Generate and Save mode.");
		}
	}

	if(photon_map_processing_ == PhotonsReuse)
	{
		if(use_photon_caustics_)
		{
			logger_.logInfo(getName(), ": Reusing caustics photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(caustic_map_->nPhotons() == 0)
			{
				logger_.logWarning(getName(), ": Caustic photon map enabled but empty, cannot be reused: changing to Generate mode.");
				photon_map_processing_ = PhotonsGenerateOnly;
			}
		}

		if(use_photon_diffuse_)
		{
			logger_.logInfo(getName(), ": Reusing diffuse photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(diffuse_map_->nPhotons() == 0)
			{
				logger_.logWarning(getName(), ": Diffuse photon map enabled but empty, cannot be reused: changing to Generate mode.");
				photon_map_processing_ = PhotonsGenerateOnly;
			}
		}

		if(final_gather_)
		{
			logger_.logInfo(getName(), ": Reusing FG radiance photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(radiance_map_->nPhotons() == 0)
			{
				logger_.logWarning(getName(), ": FG radiance photon map enabled but empty, cannot be reused: changing to Generate mode.");
				photon_map_processing_ = PhotonsGenerateOnly;
			}
		}
	}

	if(photon_map_processing_ == PhotonsLoad)
	{
		set << " (loading photon maps from file)";
	}
	else if(photon_map_processing_ == PhotonsReuse)
	{
		set << " (reusing photon maps from memory)";
	}
	else if(photon_map_processing_ == PhotonsGenerateAndSave) set << " (saving photon maps to file)";

	if(photon_map_processing_ == PhotonsLoad || photon_map_processing_ == PhotonsReuse)
	{
		timer.stop("prepass");
		logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), timer.getTime("prepass"), "s");

		set << " [" << std::fixed << std::setprecision(1) << timer.getTime("prepass") << "s" << "]";

		render_info_ += set.str();

		if(logger_.isVerbose())
		{
			for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
		}
		return true;
	}

	diffuse_map_->clear();
	diffuse_map_->setNumPaths(0);
	diffuse_map_->reserveMemory(n_diffuse_photons_);
	diffuse_map_->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

	caustic_map_->clear();
	caustic_map_->setNumPaths(0);
	caustic_map_->reserveMemory(n_caus_photons_);
	caustic_map_->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

	radiance_map_->clear();
	radiance_map_->setNumPaths(0);
	radiance_map_->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

	Ray ray;
	float light_num_pdf, light_pdf;
	int num_c_lights = 0;
	int num_d_lights = 0;
	float f_num_lights = 0.f;
	std::unique_ptr<float[]> energies;
	Rgb pcol;

	//shoot photons
	unsigned int curr = 0;
	// for radiance map:
	PreGatherData pgdat(diffuse_map_.get());
	RenderData render_data;
	render_data.cam_ = render_view->getCamera();
	int pb_step;

	tmplights.clear();

	for(int i = 0; i < (int)lights_.size(); ++i)
	{
		if(lights_[i]->shootsDiffuseP())
		{
			num_d_lights++;
			tmplights.push_back(lights_[i]);
		}
	}

	if(num_d_lights == 0)
	{
		logger_.logWarning(getName(), ": No lights found that can shoot diffuse photons, disabling Diffuse photon processing");
		enableDiffuse(false);
	}

	if(use_photon_diffuse_)
	{
		f_num_lights = (float)num_d_lights;
		energies = std::unique_ptr<float[]>(new float[num_d_lights]);

		for(int i = 0; i < num_d_lights; ++i) energies[i] = tmplights[i]->totalEnergy().energy();

		light_power_d_ = std::unique_ptr<Pdf1D>(new Pdf1D(energies.get(), num_d_lights));

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for diffuse map:");
		for(int i = 0; i < num_d_lights; ++i)
		{
			pcol = tmplights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			light_num_pdf = light_power_d_->func_[i] * light_power_d_->inv_integral_;
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", i + 1, "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}

		//shoot photons
		curr = 0;

		logger_.logInfo(getName(), ": Building diffuse photon map...");

		pb->init(128, logger_.getConsoleLogColorsEnabled());
		pb_step = std::max(1U, n_diffuse_photons_ / 128);
		pb->setTag("Building diffuse photon map...");
		//Pregather diffuse photons

		int n_threads = scene_->getNumThreadsPhotons();

		n_diffuse_photons_ = std::max((unsigned int) n_threads, (n_diffuse_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		logger_.logParams(getName(), ": Shooting ", n_diffuse_photons_, " photons across ", n_threads, " threads (", (n_diffuse_photons_ / n_threads), " photons/thread)");

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&PhotonIntegrator::diffuseWorker, this, diffuse_map_.get(), i, scene_, render_view, std::ref(render_control), std::ref(timer), n_diffuse_photons_, light_power_d_.get(), num_d_lights, tmplights, pb.get(), pb_step, std::ref(curr), max_bounces_, final_gather_, std::ref(pgdat)));
		for(auto &t : threads) t.join();

		pb->done();
		pb->setTag("Diffuse photon map built.");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse photon map built.");
		logger_.logInfo(getName(), ": Shot ", curr, " photons from ", num_d_lights, " light(s)");

		tmplights.clear();

		if(diffuse_map_->nPhotons() < 50)
		{
			logger_.logError(getName(), ": Too few diffuse photons, stopping now.");
			return false;
		}

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored diffuse photons: ", diffuse_map_->nPhotons());
	}
	else
	{
		logger_.logInfo(getName(), ": Diffuse photon mapping disabled, skipping...");
	}

	std::thread diffuse_map_build_kd_tree_thread;

	if(use_photon_diffuse_ && diffuse_map_->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
		pb->setTag("Building diffuse photons kd-tree...");

		diffuse_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, this, diffuse_map_.get());
	}
	else

		if(use_photon_diffuse_ && diffuse_map_->nPhotons() > 0)
		{
			logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
			pb->setTag("Building diffuse photons kd-tree...");
			diffuse_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}

	for(int i = 0; i < (int)lights_.size(); ++i)
	{
		if(lights_[i]->shootsCausticP())
		{
			num_c_lights++;
			tmplights.push_back(lights_[i]);
		}
	}

	if(num_c_lights == 0)
	{
		logger_.logWarning(getName(), ": No lights found that can shoot caustic photons, disabling Caustic photon processing");
		enableCaustics(false);
	}

	if(use_photon_caustics_)
	{
		curr = 0;

		f_num_lights = (float)num_c_lights;
		energies = std::unique_ptr<float[]>(new float[num_c_lights]);

		for(int i = 0; i < num_c_lights; ++i) energies[i] = tmplights[i]->totalEnergy().energy();

		light_power_d_ = std::unique_ptr<Pdf1D>(new Pdf1D(energies.get(), num_c_lights));

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for caustics map:");
		for(int i = 0; i < num_c_lights; ++i)
		{
			pcol = tmplights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			light_num_pdf = light_power_d_->func_[i] * light_power_d_->inv_integral_;
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", i + 1, "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}

		logger_.logInfo(getName(), ": Building caustics photon map...");
		pb->init(128, logger_.getConsoleLogColorsEnabled());
		pb_step = std::max(1U, n_caus_photons_ / 128);
		pb->setTag("Building caustics photon map...");
		//Pregather caustic photons

		int n_threads = scene_->getNumThreadsPhotons();

		n_caus_photons_ = std::max((unsigned int) n_threads, (n_caus_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		logger_.logParams(getName(), ": Shooting ", n_caus_photons_, " photons across ", n_threads, " threads (", (n_caus_photons_ / n_threads), " photons/thread)");

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&PhotonIntegrator::causticWorker, this, caustic_map_.get(), i, scene_, render_view, std::ref(render_control), std::ref(timer), n_caus_photons_, light_power_d_.get(), num_c_lights, tmplights, caus_depth_, pb.get(), pb_step, std::ref(curr)));
		for(auto &t : threads) t.join();

		pb->done();
		pb->setTag("Caustics photon map built.");
		logger_.logInfo(getName(), ": Shot ", curr, " caustic photons from ", num_c_lights, " light(s).");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored caustic photons: ", caustic_map_->nPhotons());
	}
	else
	{
		logger_.logInfo(getName(), ": Caustics photon mapping disabled, skipping...");
	}

	tmplights.clear();

	std::thread caustic_map_build_kd_tree_thread;

	if(use_photon_caustics_ && caustic_map_->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
		pb->setTag("Building caustic photons kd-tree...");

		caustic_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, this, caustic_map_.get());
	}
	else
	{
		if(use_photon_caustics_ && caustic_map_->nPhotons() > 0)
		{
			logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
			pb->setTag("Building caustic photons kd-tree...");
			caustic_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
	}

	if(use_photon_diffuse_ && diffuse_map_->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		diffuse_map_build_kd_tree_thread.join();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse photon map: done.");
	}

	if(use_photon_diffuse_ && final_gather_) //create radiance map:
	{
		// == remove too close radiance points ==//
		auto r_tree = std::unique_ptr<kdtree::PointKdTree<RadData>>(new kdtree::PointKdTree<RadData>(logger_, pgdat.rad_points_, "FG Radiance Photon Map", scene_->getNumThreadsPhotons()));
		std::vector< RadData > cleaned;
		for(unsigned int i = 0; i < pgdat.rad_points_.size(); ++i)
		{
			if(pgdat.rad_points_[i].use_)
			{
				cleaned.push_back(pgdat.rad_points_[i]);
				EliminatePhoton elim_proc(pgdat.rad_points_[i].normal_);
				float maxrad = 0.01f * ds_radius_; // 10% of diffuse search radius
				r_tree->lookup(pgdat.rad_points_[i].pos_, elim_proc, maxrad);
			}
		}
		pgdat.rad_points_.swap(cleaned);
		// ================ //
		int n_threads = scene_->getNumThreads();
		pgdat.radiance_vec_.resize(pgdat.rad_points_.size());
		if(intpb_) pgdat.pbar_ = intpb_;
		else pgdat.pbar_ = std::make_shared<ConsoleProgressBar>(80);
		pgdat.pbar_->init(pgdat.rad_points_.size(), logger_.getConsoleLogColorsEnabled());
		pgdat.pbar_->setTag("Pregathering radiance data for final gathering...");

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&PhotonIntegrator::preGatherWorker, this, &pgdat, ds_radius_, n_diffuse_search_));
		for(auto &t : threads) t.join();

		radiance_map_->swapVector(pgdat.radiance_vec_);
		pgdat.pbar_->done();
		pgdat.pbar_->setTag("Pregathering radiance data done...");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Radiance tree built... Updating the tree...");
		radiance_map_->updateTree();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
	}

	if(use_photon_caustics_ && caustic_map_->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		caustic_map_build_kd_tree_thread.join();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic photon map: done.");
	}

	if(photon_map_processing_ == PhotonsGenerateAndSave)
	{
		if(use_photon_diffuse_)
		{
			pb->setTag("Saving diffuse photon map to file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_diffuse.photonmap";
			logger_.logInfo(getName(), ": Saving diffuse photon map to: ", filename);
			if(diffuse_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse map saved.");
		}

		if(use_photon_caustics_)
		{
			pb->setTag("Saving caustic photon map to file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_caustic.photonmap";
			logger_.logInfo(getName(), ": Saving caustic photon map to: ", filename);
			if(caustic_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic map saved.");
		}

		if(use_photon_diffuse_ && final_gather_)
		{
			pb->setTag("Saving FG radiance photon map to file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_fg_radiance.photonmap";
			logger_.logInfo(getName(), ": Saving FG radiance photon map to: ", filename);
			if(radiance_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": FG radiance map saved.");
		}
	}

	timer.stop("prepass");
	logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), timer.getTime("prepass"), "s", " (", scene_->getNumThreadsPhotons(), " thread(s))");

	set << "| photon maps: " << std::fixed << std::setprecision(1) << timer.getTime("prepass") << "s" << " [" << scene_->getNumThreadsPhotons() << " thread(s)]";

	render_info_ += set.str();

	if(logger_.isVerbose())
	{
		for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
	}
	return true;
}

// final gathering: this is basically a full path tracer only that it uses the radiance map only
// at the path end. I.e. paths longer than 1 are only generated to overcome lack of local radiance detail.
// precondition: initBSDF of current spot has been called!
Rgb PhotonIntegrator::finalGathering(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	const Accelerator *accelerator = scene_->getAccelerator();
	if(!accelerator) return {0.f};

	Rgb path_col(0.0);
	const VolumeHandler *vol;
	Rgb vcol(0.f);
	float w = 0.f;

	int n_sampl = (int) ceilf(std::max(1, n_paths_ / render_data.ray_division_) * aa_indirect_sample_multiplier_);
	for(int i = 0; i < n_sampl; ++i)
	{
		Rgb throughput(1.0);
		float length = 0;
		SurfacePoint hit = sp;
		Vec3 pwo = wo;
		Ray p_ray;
		bool did_hit;
		const Material *p_mat = sp.material_;
		unsigned int offs = n_paths_ * render_data.pixel_sample_ + render_data.sampling_offs_ + i; // some redundancy here...
		Rgb lcol, scol;
		// "zero'th" FG bounce:
		float s_1 = sample::riVdC(offs);
		float s_2 = Halton::lowDiscrepancySampling(2, offs);
		if(render_data.ray_division_ > 1)
		{
			s_1 = math::addMod1(s_1, render_data.dc_1_);
			s_2 = math::addMod1(s_2, render_data.dc_2_);
		}

		Sample s(s_1, s_2, BsdfFlags::Diffuse | BsdfFlags::Reflect | BsdfFlags::Transmit); // glossy/dispersion/specular done via recursive raytracing
		scol = p_mat->sample(hit.mat_data_.get(), hit, pwo, p_ray.dir_, s, w, render_data.chromatic_, render_data.wavelength_, render_data.cam_);

		scol *= w;
		if(scol.isBlack()) continue;

		p_ray.tmin_ = scene_->ray_min_dist_;
		p_ray.tmax_ = -1.0;
		p_ray.from_ = hit.p_;
		throughput = scol;

		if(!(did_hit = accelerator->intersect(p_ray, hit, render_data.cam_))) continue;   //hit background

		p_mat = hit.material_;
		length = p_ray.tmax_;
		const BsdfFlags &mat_bsd_fs = hit.mat_data_->bsdf_flags_;
		bool has_spec = mat_bsd_fs.hasAny(BsdfFlags::Specular);
		bool caustic = false;
		bool close = length < gather_dist_;
		bool do_bounce = close || has_spec;
		// further bounces construct a path just as with path tracing:
		for(int depth = 0; depth < gather_bounces_ && do_bounce; ++depth)
		{
			int d_4 = 4 * depth;
			pwo = -p_ray.dir_;

			if(mat_bsd_fs.hasAny(BsdfFlags::Volumetric) && (vol = p_mat->getVolumeHandler(hit.n_ * pwo < 0)))
			{
				throughput *= vol->transmittance(p_ray);
			}

			if(mat_bsd_fs.hasAny(BsdfFlags::Diffuse))
			{
				if(close)
				{
					lcol = estimateOneDirectLight(render_data, hit, pwo, offs);
				}
				else if(caustic)
				{
					Vec3 sf = SurfacePoint::normalFaceForward(hit.ng_, hit.n_, pwo);
					const Photon *nearest = radiance_map_->findNearest(hit.p_, sf, lookup_rad_);
					if(nearest) lcol = nearest->color();
				}

				if(close || caustic)
				{
					if(mat_bsd_fs.hasAny(BsdfFlags::Emit)) lcol += p_mat->emit(hit.mat_data_.get(), hit, pwo, render_data.lights_geometry_material_emit_);
					path_col += lcol * throughput;
				}
			}

			s_1 = Halton::lowDiscrepancySampling(d_4 + 3, offs);
			s_2 = Halton::lowDiscrepancySampling(d_4 + 4, offs);

			if(render_data.ray_division_ > 1)
			{
				s_1 = math::addMod1(s_1, render_data.dc_1_);
				s_2 = math::addMod1(s_2, render_data.dc_2_);
			}

			Sample sb(s_1, s_2, (close) ? BsdfFlags::All : BsdfFlags::AllSpecular | BsdfFlags::Filter);
			scol = p_mat->sample(hit.mat_data_.get(), hit, pwo, p_ray.dir_, sb, w, render_data.chromatic_, render_data.wavelength_, render_data.cam_);

			if(sb.pdf_ <= 1.0e-6f)
			{
				did_hit = false;
				break;
			}

			scol *= w;

			p_ray.tmin_ = scene_->ray_min_dist_;
			p_ray.tmax_ = -1.0;
			p_ray.from_ = hit.p_;
			throughput *= scol;
			did_hit = accelerator->intersect(p_ray, hit, render_data.cam_);

			if(!did_hit) //hit background
			{
				const auto &background = scene_->getBackground();
				if(caustic && background && background->hasIbl() && background->shootsCaustic())
				{
					path_col += throughput * (*background)(p_ray, true);
				}
				break;
			}

			p_mat = hit.material_;
			length += p_ray.tmax_;
			caustic = (caustic || !depth) && sb.sampled_flags_.hasAny(BsdfFlags::Specular | BsdfFlags::Filter);
			close = length < gather_dist_;
			do_bounce = caustic || close;
		}

		if(did_hit)
		{
			if(mat_bsd_fs.hasAny(BsdfFlags::Diffuse | BsdfFlags::Glossy))
			{
				Vec3 sf = SurfacePoint::normalFaceForward(hit.ng_, hit.n_, -p_ray.dir_);
				const Photon *nearest = radiance_map_->findNearest(hit.p_, sf, lookup_rad_);
				if(nearest) lcol = nearest->color();
				if(mat_bsd_fs.hasAny(BsdfFlags::Emit)) lcol += p_mat->emit(hit.mat_data_.get(), hit, -p_ray.dir_, render_data.lights_geometry_material_emit_);
				path_col += lcol * throughput;
			}
		}
	}
	return path_col / (float)n_sampl;
}

Rgba PhotonIntegrator::integrate(RenderData &render_data, const DiffRay &ray, int additional_depth, ColorLayers *color_layers, const RenderView *render_view) const
{
	const bool layers_used = render_data.raylevel_ == 0 && color_layers && color_layers->getFlags() != Layer::Flags::None;

	static int n_max = 0;
	static int calls = 0;
	++calls;
	Rgb col(0.0);
	float alpha;
	SurfacePoint sp;

	const bool old_lights_geometry_material_emit = render_data.lights_geometry_material_emit_;

	if(transp_background_) alpha = 0.0;
	else alpha = 1.0;

	const Accelerator *accelerator = scene_->getAccelerator();
	if(accelerator && accelerator->intersect(ray, sp, render_data.cam_))
	{
		if(render_data.raylevel_ == 0)
		{
			render_data.chromatic_ = true;
			render_data.lights_geometry_material_emit_ = true;
		}

		Vec3 wo = -ray.dir_;
		const Material *material = sp.material_;
		const BsdfFlags &mat_bsdfs = sp.mat_data_->bsdf_flags_;

		if(additional_depth < material->getAdditionalDepth()) additional_depth = material->getAdditionalDepth();

		const Rgb col_emit = material->emit(sp.mat_data_.get(), sp, wo, render_data.lights_geometry_material_emit_);
		col += col_emit;
		if(layers_used)
		{
			if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_emit;
		}

		render_data.lights_geometry_material_emit_ = false;

		if(use_photon_diffuse_ && final_gather_)
		{
			if(show_map_)
			{
				Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
				const Photon *nearest = radiance_map_->findNearest(sp.p_, n, lookup_rad_);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::Radiance))
					{
						Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
						const Photon *nearest = radiance_map_->findNearest(sp.p_, n, lookup_rad_);
						if(nearest) color_layer->color_ = nearest->color();
					}
				}

				// contribution of light emitting surfaces
				if(mat_bsdfs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = material->emit(sp.mat_data_.get(), sp, wo, render_data.lights_geometry_material_emit_);
					col += col_tmp;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_tmp;
					}
				}

				if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
				{
					col += estimateAllDirectLight(render_data, sp, wo, color_layers);
					Rgb col_tmp = finalGathering(render_data, sp, wo);
					if(aa_noise_params_.clamp_indirect_ > 0.f) col_tmp.clampProportionalRgb(aa_noise_params_.clamp_indirect_);
					col += col_tmp;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::DiffuseIndirect)) color_layer->color_ = col_tmp;
					}
				}
			}
		}
		else
		{
			if(use_photon_diffuse_ && show_map_)
			{
				Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
				const Photon *nearest = diffuse_map_->findNearest(sp.p_, n, ds_radius_);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(use_photon_diffuse_ && layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::Radiance))
					{
						Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
						const Photon *nearest = radiance_map_->findNearest(sp.p_, n, lookup_rad_);
						if(nearest) color_layer->color_ = nearest->color();
					}
				}

				if(mat_bsdfs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = material->emit(sp.mat_data_.get(), sp, wo, render_data.lights_geometry_material_emit_);
					col += col_tmp;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_tmp;
					}
				}

				if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
				{
					col += estimateAllDirectLight(render_data, sp, wo, color_layers);
				}

				FoundPhoton *gathered = (FoundPhoton *)alloca(n_diffuse_search_ * sizeof(FoundPhoton));
				float radius = ds_radius_; //actually the square radius...

				int n_gathered = 0;

				if(use_photon_diffuse_ && diffuse_map_->nPhotons() > 0) n_gathered = diffuse_map_->gather(sp.p_, gathered, n_diffuse_search_, radius);
				if(use_photon_diffuse_ && n_gathered > 0)
				{
					if(n_gathered > n_max) n_max = n_gathered;

					float scale = 1.f / ((float)diffuse_map_->nPaths() * radius * math::num_pi);
					for(int i = 0; i < n_gathered; ++i)
					{
						const Vec3 pdir = gathered[i].photon_->direction();
						const Rgb surf_col = material->eval(sp.mat_data_.get(), sp, wo, pdir, BsdfFlags::Diffuse);

						const Rgb col_tmp = surf_col * scale * gathered[i].photon_->color();
						col += col_tmp;
						if(layers_used)
						{
							if(ColorLayer *color_layer = color_layers->find(Layer::DiffuseIndirect)) color_layer->color_ += col_tmp;
						}
					}
				}
			}
		}

		// add caustics
		if(use_photon_caustics_ && mat_bsdfs.hasAny(BsdfFlags::Diffuse))
		{
			Rgb col_tmp = estimateCausticPhotons(sp, wo);
			if(aa_noise_params_.clamp_indirect_ > 0.f) col_tmp.clampProportionalRgb(aa_noise_params_.clamp_indirect_);
			col += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Indirect)) color_layer->color_ = col_tmp;
			}
		}

		recursiveRaytrace(render_data, ray, mat_bsdfs, sp, wo, col, alpha, additional_depth, color_layers);

		if(layers_used)
		{
			generateCommonLayers(render_data.raylevel_, sp, ray, scene_->getMaskParams(), color_layers);

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
			float m_alpha = material->getAlpha(sp.mat_data_.get(), sp, wo, render_data.cam_);
			alpha = m_alpha + (1.f - m_alpha) * alpha;
		}
		else alpha = 1.0;
	}
	else //nothing hit, return background
	{
		if(scene_->getBackground() && !transp_refracted_background_)
		{
			const Rgb col_tmp = (*scene_->getBackground())(ray);
			col += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Env)) color_layer->color_ = col_tmp;
			}
		}
	}

	render_data.lights_geometry_material_emit_ = old_lights_geometry_material_emit;

	Rgb col_vol_transmittance = scene_->vol_integrator_->transmittance(render_data.prng_, ray);
	Rgb col_vol_integration = scene_->vol_integrator_->integrate(render_data, ray);

	if(transp_background_) alpha = std::max(alpha, 1.f - col_vol_transmittance.r_);

	if(layers_used)
	{
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeTransmittance)) color_layer->color_ += col_vol_transmittance;
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeIntegration)) color_layer->color_ += col_vol_integration;
	}

	col = (col * col_vol_transmittance) + col_vol_integration;

	return Rgba(col, alpha);
}

std::unique_ptr<Integrator> PhotonIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	bool transp_shad = false;
	bool final_gather = true;
	bool show_map = false;
	int shadow_depth = 5;
	int raydepth = 5;
	int num_photons = 100000;
	int num_c_photons = 500000;
	int search = 50;
	int caustic_mix = 50;
	int bounces = 5;
	int fg_paths = 32;
	int fg_bounces = 2;
	float ds_rad = 0.1;
	float c_rad = 0.01;
	float gather_dist = 0.2;
	bool do_ao = false;
	int ao_samples = 32;
	double ao_dist = 1.0;
	Rgb ao_col(1.f);
	bool bg_transp = false;
	bool bg_transp_refract = false;
	bool caustics = true;
	bool diffuse = true;
	std::string photon_maps_processing_str = "generate";

	params.getParam("caustics", caustics);
	params.getParam("diffuse", diffuse);

	params.getParam("transpShad", transp_shad);
	params.getParam("shadowDepth", shadow_depth);
	params.getParam("raydepth", raydepth);
	params.getParam("photons", num_photons);
	params.getParam("cPhotons", num_c_photons);
	params.getParam("diffuseRadius", ds_rad);
	params.getParam("causticRadius", c_rad);
	params.getParam("search", search);
	caustic_mix = search;
	params.getParam("caustic_mix", caustic_mix);
	params.getParam("bounces", bounces);
	params.getParam("finalGather", final_gather);
	params.getParam("fg_samples", fg_paths);
	params.getParam("fg_bounces", fg_bounces);
	gather_dist = ds_rad;
	params.getParam("fg_min_pathlen", gather_dist);
	params.getParam("show_map", show_map);
	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);
	params.getParam("do_AO", do_ao);
	params.getParam("AO_samples", ao_samples);
	params.getParam("AO_distance", ao_dist);
	params.getParam("AO_color", ao_col);
	params.getParam("photon_maps_processing", photon_maps_processing_str);

	auto inte = std::unique_ptr<PhotonIntegrator>(new PhotonIntegrator(logger, num_photons, num_c_photons, transp_shad, shadow_depth, ds_rad, c_rad));

	inte->use_photon_caustics_ = caustics;
	inte->use_photon_diffuse_ = diffuse;

	inte->r_depth_ = raydepth;
	inte->n_diffuse_search_ = search;
	inte->n_caus_search_ = caustic_mix;
	inte->final_gather_ = final_gather;
	inte->max_bounces_ = bounces;
	inte->caus_depth_ = bounces;
	inte->n_paths_ = fg_paths;
	inte->gather_bounces_ = fg_bounces;
	inte->show_map_ = show_map;
	inte->gather_dist_ = gather_dist;
	// Background settings
	inte->transp_background_ = bg_transp;
	inte->transp_refracted_background_ = bg_transp_refract;
	// AO settings
	inte->use_ambient_occlusion_ = do_ao;
	inte->ao_samples_ = ao_samples;
	inte->ao_dist_ = ao_dist;
	inte->ao_col_ = ao_col;

	if(photon_maps_processing_str == "generate-save") inte->photon_map_processing_ = PhotonsGenerateAndSave;
	else if(photon_maps_processing_str == "load") inte->photon_map_processing_ = PhotonsLoad;
	else if(photon_maps_processing_str == "reuse-previous") inte->photon_map_processing_ = PhotonsReuse;
	else inte->photon_map_processing_ = PhotonsGenerateOnly;

	return inte;
}

END_YAFARAY
