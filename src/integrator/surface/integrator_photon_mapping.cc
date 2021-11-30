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
#include "render/progress_bar.h"
#include "sampler/halton.h"
#include "sampler/sample_pdf1d.h"
#include "light/light.h"
#include "background/background.h"
#include "render/imagefilm.h"
#include "render/render_data.h"
#include "accelerator/accelerator.h"
#include "math/interpolation.h"

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

PhotonIntegrator::PhotonIntegrator(RenderControl &render_control, Logger &logger, unsigned int d_photons, unsigned int c_photons, bool transp_shad, int shadow_depth, float ds_rad, float c_rad) : MonteCarloIntegrator(render_control, logger)
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

void PhotonIntegrator::diffuseWorker(PreGatherData &pgdat, unsigned int &total_photons_shot, int thread_id, const Pdf1D *light_power_d, const std::vector<const Light *> &lights_diffuse, int pb_step)
{
	//shoot photons
	bool done = false;
	unsigned int curr = 0;
	std::unique_ptr<const SurfacePoint> hit_prev, hit_curr;
	const int num_lights_diffuse = lights_diffuse.size();
	const float f_num_lights = static_cast<float>(num_lights_diffuse);
	unsigned int n_diffuse_photons_thread = 1 + ((n_diffuse_photons_ - 1) / num_threads_photons_);
	std::vector<Photon> local_diffuse_photons;
	std::vector<RadData> local_rad_points;
	local_diffuse_photons.clear();
	local_diffuse_photons.reserve(n_diffuse_photons_thread);
	local_rad_points.clear();
	const float inv_diff_photons = 1.f / static_cast<float>(n_diffuse_photons_);
	while(!done)
	{
		unsigned int haltoncurr = curr + n_diffuse_photons_thread * thread_id;
		const float s_1 = sample::riVdC(haltoncurr);
		const float s_2 = Halton::lowDiscrepancySampling(2, haltoncurr);
		const float s_3 = Halton::lowDiscrepancySampling(3, haltoncurr);
		const float s_4 = Halton::lowDiscrepancySampling(4, haltoncurr);
		const float s_l = float(haltoncurr) * inv_diff_photons;
		float light_num_pdf;
		const int light_num = light_power_d->dSample(s_l, light_num_pdf);
		if(light_num >= num_lights_diffuse)
		{
			logger_.logError(getName(), ": lightPDF sample error! ", s_l, "/", light_num);
			return;
		}
		Ray ray;
		float light_pdf;
		Rgb pcol = lights_diffuse[light_num]->emitPhoton(s_1, s_2, s_3, s_4, ray, light_pdf);
		ray.tmin_ = ray_min_dist_;
		ray.tmax_ = -1.f;
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= n_diffuse_photons_thread);
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
		while(true)
		{
			std::tie(hit_curr, ray.tmax_) = accelerator_->intersect(ray, camera_);
			if(!hit_curr) break;
			Rgb transm(1.f);
			if(material_prev && hit_prev && mat_bsdfs_prev.hasAny(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = material_prev->getVolumeHandler(hit_prev->ng_ * -ray.dir_ < 0))
				{
					transm = vol->transmittance(ray);
				}
			}
			const Vec3 wi = -ray.dir_;
			const BsdfFlags &mat_bsdfs = hit_curr->mat_data_->bsdf_flags_;
			if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
			{
				//deposit photon on surface
				if(!caustic_photon)
				{
					Photon np(wi, hit_curr->p_, pcol);
					local_diffuse_photons.push_back(np);
				}
				// create entry for radiance photon:
				// don't forget to choose subset only, face normal forward; geometric vs. smooth normal?
				if(final_gather_ && FastRandom::getNextFloatNormalized() < 0.125 && !caustic_photon)
				{
					const Vec3 n = SurfacePoint::normalFaceForward(hit_curr->ng_, hit_curr->n_, wi);
					RadData rd(hit_curr->p_, n);
					rd.refl_ = hit_curr->getReflectivity(BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Reflect, true, 0.f, camera_);
					rd.transm_ = hit_curr->getReflectivity(BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Transmit, true, 0.f, camera_);
					local_rad_points.push_back(rd);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == max_bounces_) break;
			// scatter photon
			const int d_5 = 3 * n_bounces + 5;
			const float s_5 = Halton::lowDiscrepancySampling(d_5, haltoncurr);
			const float s_6 = Halton::lowDiscrepancySampling(d_5 + 1, haltoncurr);
			const float s_7 = Halton::lowDiscrepancySampling(d_5 + 2, haltoncurr);
			PSample sample(s_5, s_6, s_7, BsdfFlags::All, pcol, transm);
			Vec3 wo;
			bool scattered = hit_curr->scatterPhoton(wi, wo, sample, true, 0.f, camera_);
			if(!scattered) break; //photon was absorped.
			pcol = sample.color_;
			caustic_photon = (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.hasAny((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) &&
							 caustic_photon);
			direct_photon = sample.sampled_flags_.hasAny(BsdfFlags::Filter) && direct_photon;
			ray.from_ = hit_curr->p_;
			ray.dir_ = wo;
			ray.tmin_ = ray_min_dist_;
			ray.tmax_ = -1.f;
			material_prev = hit_curr->material_;
			mat_bsdfs_prev = mat_bsdfs;
			std::swap(hit_prev, hit_curr);
			++n_bounces;
		}
		++curr;
		if(curr % pb_step == 0)
		{
			intpb_->update();
			if(render_control_.canceled()) { return; }
		}
		done = (curr >= n_diffuse_photons_thread);
	}
	diffuse_map_->mutx_.lock();
	diffuse_map_->appendVector(local_diffuse_photons, curr);
	total_photons_shot += curr;
	diffuse_map_->mutx_.unlock();
	pgdat.mutx_.lock();
	pgdat.rad_points_.insert(std::end(pgdat.rad_points_), std::begin(local_rad_points), std::end(local_rad_points));
	pgdat.mutx_.unlock();
}

void PhotonIntegrator::photonMapKdTreeWorker(PhotonMap *photon_map)
{
	photon_map->updateTree();
}

bool PhotonIntegrator::preprocess(ImageFilm *image_film, const RenderView *render_view, const Scene &scene)
{
	bool success = SurfaceIntegrator::preprocess(image_film, render_view, scene);
	lookup_rad_ = 4 * ds_radius_ * ds_radius_;

	std::stringstream set;

	timer_->addEvent("prepass");
	timer_->start("prepass");

	logger_.logInfo(getName(), ": Starting preprocess...");

	set << "Photon Mapping  ";

	if(tr_shad_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}
	set << "RayDepth=" << r_depth_ << "  ";

	lights_ = render_view_->getLightsVisible();

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
			intpb_->setTag("Loading caustic photon map from file...");
			const std::string filename = image_film_->getFilmSavePath() + "_caustic.photonmap";
			logger_.logInfo(getName(), ": Loading caustic photon map from: ", filename, ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(caustic_map_->load(filename))
			{
				if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic map loaded.");
			}
			else caustic_map_failed_load = true;
		}

		if(use_photon_diffuse_)
		{
			intpb_->setTag("Loading diffuse photon map from file...");
			const std::string filename = image_film_->getFilmSavePath() + "_diffuse.photonmap";
			logger_.logInfo(getName(), ": Loading diffuse photon map from: ", filename, ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!");
			if(diffuse_map_->load(filename))
			{
				if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse map loaded.");
			}
			else diffuse_map_failed_load = true;
		}

		if(use_photon_diffuse_ && final_gather_)
		{
			intpb_->setTag("Loading FG radiance photon map from file...");
			const std::string filename = image_film_->getFilmSavePath() + "_fg_radiance.photonmap";
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
		timer_->stop("prepass");
		logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), timer_->getTime("prepass"), "s");

		set << " [" << std::fixed << std::setprecision(1) << timer_->getTime("prepass") << "s" << "]";

		render_info_ += set.str();

		if(logger_.isVerbose())
		{
			for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
		}
		return success;
	}

	diffuse_map_->clear();
	diffuse_map_->setNumPaths(0);
	diffuse_map_->reserveMemory(n_diffuse_photons_);
	diffuse_map_->setNumThreadsPkDtree(num_threads_photons_);

	caustic_map_->clear();
	caustic_map_->setNumPaths(0);
	caustic_map_->reserveMemory(n_caus_photons_);
	caustic_map_->setNumThreadsPkDtree(num_threads_photons_);

	radiance_map_->clear();
	radiance_map_->setNumPaths(0);
	radiance_map_->setNumThreadsPkDtree(num_threads_photons_);

	//shoot photons
	unsigned int curr = 0;
	// for radiance map:
	PreGatherData pgdat(diffuse_map_.get());

	const std::vector<const Light *> lights_diffuse = render_view_->getLightsEmittingDiffusePhotons();
	if(lights_diffuse.empty())
	{
		logger_.logWarning(getName(), ": No lights found that can shoot diffuse photons, disabling Diffuse photon processing");
		enableDiffuse(false);
	}

	if(use_photon_diffuse_ && !lights_diffuse.empty())
	{
		const int num_lights_diffuse = lights_diffuse.size();
		const float f_num_lights = static_cast<float>(num_lights_diffuse);
		std::vector<float> energies_diffuse(num_lights_diffuse);
		for(int i = 0; i < num_lights_diffuse; ++i) energies_diffuse[i] = lights_diffuse[i]->totalEnergy().energy();
		auto light_power_d_diffuse = std::unique_ptr<Pdf1D>(new Pdf1D(energies_diffuse));
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for diffuse map:");
		for(int i = 0; i < num_lights_diffuse; ++i)
		{
			Ray ray;
			float light_pdf;
			Rgb pcol = lights_diffuse[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			const float light_num_pdf = light_power_d_diffuse->function(i) * light_power_d_diffuse->invIntegral();
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", lights_diffuse[i]->getName(), "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}
		//shoot photons
		curr = 0;
		logger_.logInfo(getName(), ": Building diffuse photon map...");
		intpb_->init(128, logger_.getConsoleLogColorsEnabled());
		const int pb_step = std::max(1U, n_diffuse_photons_ / 128);
		intpb_->setTag("Building diffuse photon map...");
		//Pregather diffuse photons
		n_diffuse_photons_ = std::max((unsigned int) num_threads_photons_, (n_diffuse_photons_ / num_threads_photons_) * num_threads_photons_); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread
		logger_.logParams(getName(), ": Shooting ", n_diffuse_photons_, " photons across ", num_threads_photons_, " threads (", (n_diffuse_photons_ / num_threads_photons_), " photons/thread)");
		std::vector<std::thread> threads;
		for(int i = 0; i < num_threads_photons_; ++i) threads.push_back(std::thread(&PhotonIntegrator::diffuseWorker, this, std::ref(pgdat), std::ref(curr), i, light_power_d_diffuse.get(), lights_diffuse, pb_step));
		for(auto &t : threads) t.join();

		intpb_->done();
		intpb_->setTag("Diffuse photon map built.");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse photon map built.");
		logger_.logInfo(getName(), ": Shot ", curr, " photons from ", num_lights_diffuse, " light(s)");

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

	if(use_photon_diffuse_ && diffuse_map_->nPhotons() > 0)
	{
		if(num_threads_photons_ >= 2)
		{
			logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
			intpb_->setTag("Building diffuse photons kd-tree...");
			diffuse_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, this, diffuse_map_.get());
		}
		else
		{
			logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
			intpb_->setTag("Building diffuse photons kd-tree...");
			diffuse_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
	}

	const std::vector<const Light *> lights_caustic = render_view_->getLightsEmittingCausticPhotons();
	if(lights_caustic.empty())
	{
		logger_.logWarning(getName(), ": No lights found that can shoot caustic photons, disabling Caustic photon processing");
		enableCaustics(false);
	}

	if(use_photon_caustics_ && !lights_caustic.empty())
	{
		curr = 0;
		const int num_lights_caustic = lights_caustic.size();
		const float f_num_lights = static_cast<float>(num_lights_caustic);
		std::vector<float> energies_caustic(num_lights_caustic);

		for(int i = 0; i < num_lights_caustic; ++i) energies_caustic[i] = lights_caustic[i]->totalEnergy().energy();

		auto light_power_d_caustic = std::unique_ptr<Pdf1D>(new Pdf1D(energies_caustic));

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for caustics map:");
		for(int i = 0; i < num_lights_caustic; ++i)
		{
			Ray ray;
			float light_pdf;
			Rgb pcol = lights_caustic[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			const float light_num_pdf = light_power_d_caustic->function(i) * light_power_d_caustic->invIntegral();
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", lights_caustic[i]->getName(), "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}

		logger_.logInfo(getName(), ": Building caustics photon map...");
		intpb_->init(128, logger_.getConsoleLogColorsEnabled());
		const int pb_step = std::max(1U, n_caus_photons_ / 128);
		intpb_->setTag("Building caustics photon map...");
		//Pregather caustic photons

		n_caus_photons_ = std::max((unsigned int) num_threads_photons_, (n_caus_photons_ / num_threads_photons_) * num_threads_photons_); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		logger_.logParams(getName(), ": Shooting ", n_caus_photons_, " photons across ", num_threads_photons_, " threads (", (n_caus_photons_ / num_threads_photons_), " photons/thread)");

		std::vector<std::thread> threads;
		for(int i = 0; i < num_threads_photons_; ++i) threads.push_back(std::thread(&PhotonIntegrator::causticWorker, this, std::ref(curr), i, light_power_d_caustic.get(), lights_caustic, pb_step));
		for(auto &t : threads) t.join();

		intpb_->done();
		intpb_->setTag("Caustics photon map built.");
		logger_.logInfo(getName(), ": Shot ", curr, " caustic photons from ", num_lights_caustic, " light(s).");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored caustic photons: ", caustic_map_->nPhotons());
	}
	else
	{
		logger_.logInfo(getName(), ": Caustics photon mapping disabled, skipping...");
	}

	std::thread caustic_map_build_kd_tree_thread;
	if(use_photon_caustics_ && caustic_map_->nPhotons() > 0)
	{
		if(num_threads_photons_ >= 2)
		{
			logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
			intpb_->setTag("Building caustic photons kd-tree...");
			caustic_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, this, caustic_map_.get());
		}
		else
		{
			logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
			intpb_->setTag("Building caustic photons kd-tree...");
			caustic_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
	}

	if(use_photon_diffuse_ && diffuse_map_->nPhotons() > 0 && num_threads_photons_ >= 2)
	{
		diffuse_map_build_kd_tree_thread.join();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse photon map: done.");
	}

	if(use_photon_diffuse_ && final_gather_) //create radiance map:
	{
		// == remove too close radiance points ==//
		auto r_tree = std::unique_ptr<kdtree::PointKdTree<RadData>>(new kdtree::PointKdTree<RadData>(logger_, pgdat.rad_points_, "FG Radiance Photon Map", num_threads_photons_));
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
		int n_threads = num_threads_;
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

	if(use_photon_caustics_ && caustic_map_->nPhotons() > 0 && num_threads_photons_ >= 2)
	{
		caustic_map_build_kd_tree_thread.join();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic photon map: done.");
	}

	if(photon_map_processing_ == PhotonsGenerateAndSave)
	{
		if(use_photon_diffuse_)
		{
			intpb_->setTag("Saving diffuse photon map to file...");
			const std::string filename = image_film_->getFilmSavePath() + "_diffuse.photonmap";
			logger_.logInfo(getName(), ": Saving diffuse photon map to: ", filename);
			if(diffuse_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse map saved.");
		}

		if(use_photon_caustics_)
		{
			intpb_->setTag("Saving caustic photon map to file...");
			const std::string filename = image_film_->getFilmSavePath() + "_caustic.photonmap";
			logger_.logInfo(getName(), ": Saving caustic photon map to: ", filename);
			if(caustic_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic map saved.");
		}

		if(use_photon_diffuse_ && final_gather_)
		{
			intpb_->setTag("Saving FG radiance photon map to file...");
			const std::string filename = image_film_->getFilmSavePath() + "_fg_radiance.photonmap";
			logger_.logInfo(getName(), ": Saving FG radiance photon map to: ", filename);
			if(radiance_map_->save(filename) && logger_.isVerbose()) logger_.logVerbose(getName(), ": FG radiance map saved.");
		}
	}

	timer_->stop("prepass");
	logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), timer_->getTime("prepass"), "s", " (", num_threads_photons_, " thread(s))");

	set << "| photon maps: " << std::fixed << std::setprecision(1) << timer_->getTime("prepass") << "s" << " [" << num_threads_photons_ << " thread(s)]";

	render_info_ += set.str();

	if(logger_.isVerbose())
	{
		for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
	}
	return success;
}

// final gathering: this is basically a full path tracer only that it uses the radiance map only
// at the path end. I.e. paths longer than 1 are only generated to overcome lack of local radiance detail.
// precondition: initBSDF of current spot has been called!
Rgb PhotonIntegrator::finalGathering(RandomGenerator &random_generator, int thread_id, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3 &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	Rgb path_col(0.0);
	float w = 0.f;

	int n_sampl = (int) ceilf(std::max(1, n_paths_ / ray_division.division_) * aa_indirect_sample_multiplier_);
	for(int i = 0; i < n_sampl; ++i)
	{
		Rgb throughput(1.0);
		float length = 0;
		auto hit = std::unique_ptr<const SurfacePoint>(new SurfacePoint(sp));
		Vec3 pwo = wo;
		Ray p_ray;
		bool did_hit;
		unsigned int offs = n_paths_ * pixel_sampling_data.sample_ + pixel_sampling_data.offset_ + i; // some redundancy here...
		Rgb lcol, scol;
		// "zero'th" FG bounce:
		float s_1 = sample::riVdC(offs);
		float s_2 = Halton::lowDiscrepancySampling(2, offs);
		if(ray_division.division_ > 1)
		{
			s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
			s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
		}

		Sample s(s_1, s_2, BsdfFlags::Diffuse | BsdfFlags::Reflect | BsdfFlags::Transmit); // glossy/dispersion/specular done via recursive raytracing
		scol = hit->sample(pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength, camera_);

		scol *= w;
		if(scol.isBlack()) continue;

		p_ray.tmin_ = ray_min_dist_;
		p_ray.tmax_ = -1.f;
		p_ray.from_ = hit->p_;
		throughput = scol;
		std::tie(hit, p_ray.tmax_) = accelerator_->intersect(p_ray, camera_);
		did_hit = static_cast<bool>(hit);
		if(!did_hit) continue;   //hit background
		length = p_ray.tmax_;
		const BsdfFlags &mat_bsd_fs = hit->mat_data_->bsdf_flags_;
		bool has_spec = mat_bsd_fs.hasAny(BsdfFlags::Specular);
		bool caustic = false;
		bool close = length < gather_dist_;
		bool do_bounce = close || has_spec;
		// further bounces construct a path just as with path tracing:
		for(int depth = 0; depth < gather_bounces_ && do_bounce; ++depth)
		{
			int d_4 = 4 * depth;
			pwo = -p_ray.dir_;
			if(mat_bsd_fs.hasAny(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = hit->material_->getVolumeHandler(hit->n_ * pwo < 0))
				{
					throughput *= vol->transmittance(p_ray);
				}
			}
			if(mat_bsd_fs.hasAny(BsdfFlags::Diffuse))
			{
				if(close)
				{
					lcol = estimateOneDirectLight(random_generator, thread_id, chromatic_enabled, wavelength, *hit, pwo, offs, ray_division, pixel_sampling_data);
				}
				else if(caustic)
				{
					Vec3 sf = SurfacePoint::normalFaceForward(hit->ng_, hit->n_, pwo);
					const Photon *nearest = radiance_map_->findNearest(hit->p_, sf, lookup_rad_);
					if(nearest) lcol = nearest->color();
				}

				if(close || caustic)
				{
					if(mat_bsd_fs.hasAny(BsdfFlags::Emit)) lcol += hit->emit(pwo);
					path_col += lcol * throughput;
				}
			}

			s_1 = Halton::lowDiscrepancySampling(d_4 + 3, offs);
			s_2 = Halton::lowDiscrepancySampling(d_4 + 4, offs);

			if(ray_division.division_ > 1)
			{
				s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
				s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
			}

			Sample sb(s_1, s_2, (close) ? BsdfFlags::All : BsdfFlags::AllSpecular | BsdfFlags::Filter);
			scol = hit->sample(pwo, p_ray.dir_, sb, w, chromatic_enabled, wavelength, camera_);

			if(sb.pdf_ <= 1.0e-6f)
			{
				did_hit = false;
				break;
			}
			scol *= w;
			p_ray.tmin_ = ray_min_dist_;
			p_ray.tmax_ = -1.f;
			p_ray.from_ = hit->p_;
			throughput *= scol;
			std::tie(hit, p_ray.tmax_) = accelerator_->intersect(p_ray, camera_);
			did_hit = static_cast<bool>(hit);
			if(!did_hit) break; //hit background
			length += p_ray.tmax_;
			caustic = (caustic || !depth) && sb.sampled_flags_.hasAny(BsdfFlags::Specular | BsdfFlags::Filter);
			close = length < gather_dist_;
			do_bounce = caustic || close;
		}

		if(did_hit)
		{
			if(mat_bsd_fs.hasAny(BsdfFlags::Diffuse | BsdfFlags::Glossy))
			{
				Vec3 sf = SurfacePoint::normalFaceForward(hit->ng_, hit->n_, -p_ray.dir_);
				const Photon *nearest = radiance_map_->findNearest(hit->p_, sf, lookup_rad_);
				if(nearest) lcol = nearest->color(); //FIXME should lcol be a local variable? Is it getting its value from previous functions or not??
				if(mat_bsd_fs.hasAny(BsdfFlags::Emit)) lcol += hit->emit(-p_ray.dir_);
				path_col += lcol * throughput;
			}
		}
	}
	return path_col / (float)n_sampl;
}

std::unique_ptr<Integrator> PhotonIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene, RenderControl &render_control)
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

	auto inte = std::unique_ptr<PhotonIntegrator>(new PhotonIntegrator(render_control, logger, num_photons, num_c_photons, transp_shad, shadow_depth, ds_rad, c_rad));

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

std::pair<Rgb, float> PhotonIntegrator::integrate(Ray &ray, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const
{
	static int n_max = 0;
	static int calls = 0;
	++calls;
	Rgb col {0.f};
	float alpha = 1.f;
	std::unique_ptr<const SurfacePoint> sp;
	std::tie(sp, ray.tmax_) = accelerator_->intersect(ray, camera_);
	if(sp)
	{
		const Vec3 wo = -ray.dir_;
		const BsdfFlags &mat_bsdfs = sp->mat_data_->bsdf_flags_;

		additional_depth = std::max(additional_depth, sp->material_->getAdditionalDepth());

		const Rgb col_emit = sp->emit(wo);
		col += col_emit;
		if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_emit;
		}
		if(use_photon_diffuse_ && final_gather_)
		{
			if(show_map_)
			{
				const Vec3 n = SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo);
				const Photon *nearest = radiance_map_->findNearest(sp->p_, n, lookup_rad_);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
				{
					if(Rgba *color_layer = color_layers->find(LayerDef::Radiance))
					{
						const Vec3 n = SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo);
						const Photon *nearest = radiance_map_->findNearest(sp->p_, n, lookup_rad_);
						if(nearest) *color_layer = nearest->color();
					}
				}

				// contribution of light emitting surfaces
				if(mat_bsdfs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = sp->emit(wo);
					col += col_tmp;
					if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_tmp;
					}
				}

				if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
				{
					col += estimateAllDirectLight(random_generator, color_layers, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
					Rgb col_tmp = finalGathering(random_generator, thread_id, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
					if(aa_noise_params_.clamp_indirect_ > 0.f) col_tmp.clampProportionalRgb(aa_noise_params_.clamp_indirect_);
					col += col_tmp;
					if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::DiffuseLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseIndirect)) *color_layer = col_tmp;
					}
				}
			}
		}
		else
		{
			if(use_photon_diffuse_ && show_map_)
			{
				const Vec3 n = SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo);
				const Photon *nearest = diffuse_map_->findNearest(sp->p_, n, ds_radius_);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(use_photon_diffuse_ && color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
				{
					if(Rgba *color_layer = color_layers->find(LayerDef::Radiance))
					{
						const Vec3 n = SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo);
						const Photon *nearest = radiance_map_->findNearest(sp->p_, n, lookup_rad_);
						if(nearest) *color_layer = nearest->color();
					}
				}

				if(mat_bsdfs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = sp->emit(wo);
					col += col_tmp;
					if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_tmp;
					}
				}

				if(mat_bsdfs.hasAny(BsdfFlags::Diffuse))
				{
					col += estimateAllDirectLight(random_generator, color_layers, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
				}

				FoundPhoton *gathered = (FoundPhoton *)alloca(n_diffuse_search_ * sizeof(FoundPhoton));
				float radius = ds_radius_; //actually the square radius...

				int n_gathered = 0;

				if(use_photon_diffuse_ && diffuse_map_->nPhotons() > 0) n_gathered = diffuse_map_->gather(sp->p_, gathered, n_diffuse_search_, radius);
				if(use_photon_diffuse_ && n_gathered > 0)
				{
					if(n_gathered > n_max) n_max = n_gathered;

					float scale = 1.f / ((float)diffuse_map_->nPaths() * radius * math::num_pi);
					for(int i = 0; i < n_gathered; ++i)
					{
						const Vec3 pdir = gathered[i].photon_->direction();
						const Rgb surf_col = sp->eval(wo, pdir, BsdfFlags::Diffuse);

						const Rgb col_tmp = surf_col * scale * gathered[i].photon_->color();
						col += col_tmp;
						if(color_layers && color_layers->getFlags().hasAny(LayerDef::Flags::BasicLayers))
						{
							if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseIndirect)) *color_layer += col_tmp;
						}
					}
				}
			}
		}

		// add caustics
		if(use_photon_caustics_ && mat_bsdfs.hasAny(BsdfFlags::Diffuse))
		{
			col += causticPhotons(color_layers, ray, *sp, wo, aa_noise_params_.clamp_indirect_, caustic_map_.get(), caus_radius_, n_caus_search_);
		}

		const auto recursive_result = recursiveRaytrace(random_generator, color_layers, thread_id, ray_level + 1, chromatic_enabled, wavelength, ray, mat_bsdfs, *sp, wo, additional_depth, ray_division, pixel_sampling_data);
		col += recursive_result.first;
		alpha = recursive_result.second;
		if(color_layers)
		{
			generateCommonLayers(color_layers, *sp, mask_params_);
			generateOcclusionLayers(color_layers, *accelerator_, chromatic_enabled, wavelength, ray_division, camera_, pixel_sampling_data, *sp, wo, ao_samples_, shadow_bias_auto_, shadow_bias_, ao_dist_, ao_col_, s_depth_);
		}
	}
	else //nothing hit, return background
	{
		std::tie(col, alpha) = background(ray, color_layers, transp_background_, transp_refracted_background_, background_, ray_level);
	}
	if(vol_integrator_)
	{
		applyVolumetricEffects(col, alpha, color_layers, ray, random_generator, vol_integrator_, transp_background_);
	}
	return {col, alpha};
}

END_YAFARAY
