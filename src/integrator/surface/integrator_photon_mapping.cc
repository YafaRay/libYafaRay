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
#include "common/session.h"
#include "volume/volume.h"
#include "color/color_layers.h"
#include "common/param.h"
#include "scene/scene.h"
#include "render/monitor.h"
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
	float i_scale = 1.f / ((float)gdata->diffuse_map_->nPaths() * M_PI);
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

PhotonIntegrator::PhotonIntegrator(unsigned int d_photons, unsigned int c_photons, bool transp_shad, int shadow_depth, float ds_rad, float c_rad)
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
}

void PhotonIntegrator::diffuseWorker(PhotonMap *diffuse_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, unsigned int n_diffuse_photons, const Pdf1D *light_power_d, int num_d_lights, const std::vector<const Light *> &tmplights, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot, int max_bounces, bool final_gather, PreGatherData &pgdat)
{
	Ray ray;
	float light_num_pdf, light_pdf, s_1, s_2, s_3, s_4, s_5, s_6, s_7, s_l;
	Rgb pcol;

	//shoot photons
	bool done = false;
	unsigned int curr = 0;

	SurfacePoint sp;
	RenderData render_data;
	alignas (16) unsigned char userdata[user_data_size_];
	render_data.arena_ = static_cast<void *>(userdata);
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
		int light_num = light_power_d->dSample(s_l, &light_num_pdf);
		if(light_num >= num_d_lights)
		{
			diffuse_map->mutx_.lock();
			Y_ERROR << getName() << ": lightPDF sample error! " << s_l << "/" << light_num << YENDL;
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
		BsdfFlags bsdfs;

		while(scene_->getAccelerator()->intersect(ray, sp))
		{
			if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
			{
				diffuse_map->mutx_.lock();
				Y_WARNING << getName() << ": NaN  on photon color for light" << light_num + 1 << "." << YENDL;
				diffuse_map->mutx_.unlock();
				continue;
			}

			Rgb transm(1.f);
			Rgb vcol(0.f);
			const VolumeHandler *vol = nullptr;

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

			if(bsdfs.hasAny(BsdfFlags::Diffuse))
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
					rd.refl_ = material->getReflectivity(render_data, sp, BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Reflect);
					rd.transm_ = material->getReflectivity(render_data, sp, BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Transmit);
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

			bool scattered = material->scatterPhoton(render_data, sp, wi, wo, sample);
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

bool PhotonIntegrator::preprocess(const RenderControl &render_control, const RenderView *render_view, ImageFilm *image_film)
{
	image_film_ = image_film;
	std::shared_ptr<ProgressBar> pb;
	if(intpb_) pb = intpb_;
	else pb = std::make_shared<ConsoleProgressBar>(80);

	lookup_rad_ = 4 * ds_radius_ * ds_radius_;

	std::stringstream set;
	g_timer_global.addEvent("prepass");
	g_timer_global.start("prepass");

	Y_INFO << getName() << ": Starting preprocess..." << YENDL;

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
			Y_INFO << getName() << ": Loading caustic photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
			if(session_global.caustic_map_.get()->load(filename))
			{
				if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Caustic map loaded." << YENDL;
			}
			else caustic_map_failed_load = true;
		}

		if(use_photon_diffuse_)
		{
			pb->setTag("Loading diffuse photon map from file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_diffuse.photonmap";
			Y_INFO << getName() << ": Loading diffuse photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
			if(session_global.diffuse_map_.get()->load(filename))
			{
				if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Diffuse map loaded." << YENDL;
			}
			else diffuse_map_failed_load = true;
		}

		if(use_photon_diffuse_ && final_gather_)
		{
			pb->setTag("Loading FG radiance photon map from file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_fg_radiance.photonmap";
			Y_INFO << getName() << ": Loading FG radiance photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
			if(session_global.radiance_map_.get()->load(filename))
			{
				if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": FG radiance map loaded." << YENDL;
			}
			else fg_radiance_map_failed_load = true;
		}

		if(caustic_map_failed_load || diffuse_map_failed_load || fg_radiance_map_failed_load)
		{
			photon_map_processing_ = PhotonsGenerateAndSave;
			Y_WARNING << getName() << ": photon maps loading failed, changing to Generate and Save mode." << YENDL;
		}
	}

	if(photon_map_processing_ == PhotonsReuse)
	{
		if(use_photon_caustics_)
		{
			Y_INFO << getName() << ": Reusing caustics photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
			if(session_global.caustic_map_.get()->nPhotons() == 0)
			{
				Y_WARNING << getName() << ": Caustic photon map enabled but empty, cannot be reused: changing to Generate mode." << YENDL;
				photon_map_processing_ = PhotonsGenerateOnly;
			}
		}

		if(use_photon_diffuse_)
		{
			Y_INFO << getName() << ": Reusing diffuse photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
			if(session_global.diffuse_map_.get()->nPhotons() == 0)
			{
				Y_WARNING << getName() << ": Diffuse photon map enabled but empty, cannot be reused: changing to Generate mode." << YENDL;
				photon_map_processing_ = PhotonsGenerateOnly;
			}
		}

		if(final_gather_)
		{
			Y_INFO << getName() << ": Reusing FG radiance photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << YENDL;
			if(session_global.radiance_map_.get()->nPhotons() == 0)
			{
				Y_WARNING << getName() << ": FG radiance photon map enabled but empty, cannot be reused: changing to Generate mode." << YENDL;
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
		g_timer_global.stop("prepass");
		Y_INFO << getName() << ": Photonmap building time: " << std::fixed << std::setprecision(1) << g_timer_global.getTime("prepass") << "s" << YENDL;

		set << " [" << std::fixed << std::setprecision(1) << g_timer_global.getTime("prepass") << "s" << "]";

		render_info_ += set.str();

		if(Y_LOG_HAS_VERBOSE)
		{
			for(std::string line; std::getline(set, line, '\n');) Y_VERBOSE << line << YENDL;
		}
		return true;
	}

	session_global.diffuse_map_.get()->clear();
	session_global.diffuse_map_.get()->setNumPaths(0);
	session_global.diffuse_map_.get()->reserveMemory(n_diffuse_photons_);
	session_global.diffuse_map_.get()->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

	session_global.caustic_map_.get()->clear();
	session_global.caustic_map_.get()->setNumPaths(0);
	session_global.caustic_map_.get()->reserveMemory(n_caus_photons_);
	session_global.caustic_map_.get()->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

	session_global.radiance_map_.get()->clear();
	session_global.radiance_map_.get()->setNumPaths(0);
	session_global.radiance_map_.get()->setNumThreadsPkDtree(scene_->getNumThreadsPhotons());

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
	PreGatherData pgdat(session_global.diffuse_map_.get());
	RenderData render_data;
	alignas (16) unsigned char userdata[user_data_size_];
	render_data.arena_ = static_cast<void *>(userdata);
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
		Y_WARNING << getName() << ": No lights found that can shoot diffuse photons, disabling Diffuse photon processing" << YENDL;
		enableDiffuse(false);
	}

	if(use_photon_diffuse_)
	{
		f_num_lights = (float)num_d_lights;
		energies = std::unique_ptr<float[]>(new float[num_d_lights]);

		for(int i = 0; i < num_d_lights; ++i) energies[i] = tmplights[i]->totalEnergy().energy();

		light_power_d_ = std::unique_ptr<Pdf1D>(new Pdf1D(energies.get(), num_d_lights));

		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Light(s) photon color testing for diffuse map:" << YENDL;
		for(int i = 0; i < num_d_lights; ++i)
		{
			pcol = tmplights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			light_num_pdf = light_power_d_->func_[i] * light_power_d_->inv_integral_;
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Light [" << i + 1 << "] Photon col:" << pcol << " | lnpdf: " << light_num_pdf << YENDL;
		}

		//shoot photons
		curr = 0;

		Y_INFO << getName() << ": Building diffuse photon map..." << YENDL;

		pb->init(128);
		pb_step = std::max(1U, n_diffuse_photons_ / 128);
		pb->setTag("Building diffuse photon map...");
		//Pregather diffuse photons

		int n_threads = scene_->getNumThreadsPhotons();

		n_diffuse_photons_ = std::max((unsigned int) n_threads, (n_diffuse_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		Y_PARAMS << getName() << ": Shooting " << n_diffuse_photons_ << " photons across " << n_threads << " threads (" << (n_diffuse_photons_ / n_threads) << " photons/thread)" << YENDL;

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&PhotonIntegrator::diffuseWorker, this, session_global.diffuse_map_.get(), i, scene_, render_view, std::ref(render_control), n_diffuse_photons_, light_power_d_.get(), num_d_lights, tmplights, pb.get(), pb_step, std::ref(curr), max_bounces_, final_gather_, std::ref(pgdat)));
		for(auto &t : threads) t.join();

		pb->done();
		pb->setTag("Diffuse photon map built.");
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Diffuse photon map built." << YENDL;
		Y_INFO << getName() << ": Shot " << curr << " photons from " << num_d_lights << " light(s)" << YENDL;

		tmplights.clear();

		if(session_global.diffuse_map_.get()->nPhotons() < 50)
		{
			Y_ERROR << getName() << ": Too few diffuse photons, stopping now." << YENDL;
			return false;
		}

		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Stored diffuse photons: " << session_global.diffuse_map_.get()->nPhotons() << YENDL;
	}
	else
	{
		Y_INFO << getName() << ": Diffuse photon mapping disabled, skipping..." << YENDL;
	}

	std::thread diffuse_map_build_kd_tree_thread;

	if(use_photon_diffuse_ && session_global.diffuse_map_.get()->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		Y_INFO << getName() << ": Building diffuse photons kd-tree:" << YENDL;
		pb->setTag("Building diffuse photons kd-tree...");

		diffuse_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, this, session_global.diffuse_map_.get());
	}
	else

		if(use_photon_diffuse_ && session_global.diffuse_map_.get()->nPhotons() > 0)
		{
			Y_INFO << getName() << ": Building diffuse photons kd-tree:" << YENDL;
			pb->setTag("Building diffuse photons kd-tree...");
			session_global.diffuse_map_.get()->updateTree();
			if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Done." << YENDL;
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
		Y_WARNING << getName() << ": No lights found that can shoot caustic photons, disabling Caustic photon processing" << YENDL;
		enableCaustics(false);
	}

	if(use_photon_caustics_)
	{
		curr = 0;

		f_num_lights = (float)num_c_lights;
		energies = std::unique_ptr<float[]>(new float[num_c_lights]);

		for(int i = 0; i < num_c_lights; ++i) energies[i] = tmplights[i]->totalEnergy().energy();

		light_power_d_ = std::unique_ptr<Pdf1D>(new Pdf1D(energies.get(), num_c_lights));

		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Light(s) photon color testing for caustics map:" << YENDL;
		for(int i = 0; i < num_c_lights; ++i)
		{
			pcol = tmplights[i]->emitPhoton(.5, .5, .5, .5, ray, light_pdf);
			light_num_pdf = light_power_d_->func_[i] * light_power_d_->inv_integral_;
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Light [" << i + 1 << "] Photon col:" << pcol << " | lnpdf: " << light_num_pdf << YENDL;
		}

		Y_INFO << getName() << ": Building caustics photon map..." << YENDL;
		pb->init(128);
		pb_step = std::max(1U, n_caus_photons_ / 128);
		pb->setTag("Building caustics photon map...");
		//Pregather caustic photons

		int n_threads = scene_->getNumThreadsPhotons();

		n_caus_photons_ = std::max((unsigned int) n_threads, (n_caus_photons_ / n_threads) * n_threads); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		Y_PARAMS << getName() << ": Shooting " << n_caus_photons_ << " photons across " << n_threads << " threads (" << (n_caus_photons_ / n_threads) << " photons/thread)" << YENDL;

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&PhotonIntegrator::causticWorker, this, session_global.caustic_map_.get(), i, scene_, render_view, std::ref(render_control), n_caus_photons_, light_power_d_.get(), num_c_lights, tmplights, caus_depth_, pb.get(), pb_step, std::ref(curr)));
		for(auto &t : threads) t.join();

		pb->done();
		pb->setTag("Caustics photon map built.");
		Y_INFO << getName() << ": Shot " << curr << " caustic photons from " << num_c_lights << " light(s)." << YENDL;
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Stored caustic photons: " << session_global.caustic_map_.get()->nPhotons() << YENDL;
	}
	else
	{
		Y_INFO << getName() << ": Caustics photon mapping disabled, skipping..." << YENDL;
	}

	tmplights.clear();

	std::thread caustic_map_build_kd_tree_thread;

	if(use_photon_caustics_ && session_global.caustic_map_.get()->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		Y_INFO << getName() << ": Building caustic photons kd-tree:" << YENDL;
		pb->setTag("Building caustic photons kd-tree...");

		caustic_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, this, session_global.caustic_map_.get());
	}
	else
	{
		if(use_photon_caustics_ && session_global.caustic_map_.get()->nPhotons() > 0)
		{
			Y_INFO << getName() << ": Building caustic photons kd-tree:" << YENDL;
			pb->setTag("Building caustic photons kd-tree...");
			session_global.caustic_map_.get()->updateTree();
			if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Done." << YENDL;
		}
	}

	if(use_photon_diffuse_ && session_global.diffuse_map_.get()->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		diffuse_map_build_kd_tree_thread.join();
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Diffuse photon map: done." << YENDL;
	}

	if(use_photon_diffuse_ && final_gather_) //create radiance map:
	{
		// == remove too close radiance points ==//
		auto r_tree = std::unique_ptr<kdtree::PointKdTree<RadData>>(new kdtree::PointKdTree<RadData>(pgdat.rad_points_, "FG Radiance Photon Map", scene_->getNumThreadsPhotons()));
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
		pgdat.pbar_->init(pgdat.rad_points_.size());
		pgdat.pbar_->setTag("Pregathering radiance data for final gathering...");

		std::vector<std::thread> threads;
		for(int i = 0; i < n_threads; ++i) threads.push_back(std::thread(&PhotonIntegrator::preGatherWorker, this, &pgdat, ds_radius_, n_diffuse_search_));
		for(auto &t : threads) t.join();

		session_global.radiance_map_.get()->swapVector(pgdat.radiance_vec_);
		pgdat.pbar_->done();
		pgdat.pbar_->setTag("Pregathering radiance data done...");
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Radiance tree built... Updating the tree..." << YENDL;
		session_global.radiance_map_.get()->updateTree();
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Done." << YENDL;
	}

	if(use_photon_caustics_ && session_global.caustic_map_.get()->nPhotons() > 0 && scene_->getNumThreadsPhotons() >= 2)
	{
		caustic_map_build_kd_tree_thread.join();
		if(Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Caustic photon map: done." << YENDL;
	}

	if(photon_map_processing_ == PhotonsGenerateAndSave)
	{
		if(use_photon_diffuse_)
		{
			pb->setTag("Saving diffuse photon map to file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_diffuse.photonmap";
			Y_INFO << getName() << ": Saving diffuse photon map to: " << filename << YENDL;
			if(session_global.diffuse_map_.get()->save(filename) && Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Diffuse map saved." << YENDL;
		}

		if(use_photon_caustics_)
		{
			pb->setTag("Saving caustic photon map to file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_caustic.photonmap";
			Y_INFO << getName() << ": Saving caustic photon map to: " << filename << YENDL;
			if(session_global.caustic_map_.get()->save(filename) && Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": Caustic map saved." << YENDL;
		}

		if(use_photon_diffuse_ && final_gather_)
		{
			pb->setTag("Saving FG radiance photon map to file...");
			const std::string filename = scene_->getImageFilm()->getFilmSavePath() + "_fg_radiance.photonmap";
			Y_INFO << getName() << ": Saving FG radiance photon map to: " << filename << YENDL;
			if(session_global.radiance_map_.get()->save(filename) && Y_LOG_HAS_VERBOSE) Y_VERBOSE << getName() << ": FG radiance map saved." << YENDL;
		}
	}

	g_timer_global.stop("prepass");
	Y_INFO << getName() << ": Photonmap building time: " << std::fixed << std::setprecision(1) << g_timer_global.getTime("prepass") << "s" << " (" << scene_->getNumThreadsPhotons() << " thread(s))" << YENDL;

	set << "| photon maps: " << std::fixed << std::setprecision(1) << g_timer_global.getTime("prepass") << "s" << " [" << scene_->getNumThreadsPhotons() << " thread(s)]";

	render_info_ += set.str();

	if(Y_LOG_HAS_VERBOSE)
	{
		for(std::string line; std::getline(set, line, '\n');) Y_VERBOSE << line << YENDL;
	}
	return true;
}

// final gathering: this is basically a full path tracer only that it uses the radiance map only
// at the path end. I.e. paths longer than 1 are only generated to overcome lack of local radiance detail.
// precondition: initBSDF of current spot has been called!
Rgb PhotonIntegrator::finalGathering(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb path_col(0.0);
	void *first_udat = render_data.arena_;
	alignas (16) unsigned char userdata[user_data_size_];
	void *n_udat = static_cast<void *>(userdata);
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
		Ray pRay;
		BsdfFlags mat_bsd_fs;
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
		scol = p_mat->sample(render_data, hit, pwo, pRay.dir_, s, w);

		scol *= w;
		if(scol.isBlack()) continue;

		pRay.tmin_ = scene_->ray_min_dist_;
		pRay.tmax_ = -1.0;
		pRay.from_ = hit.p_;
		throughput = scol;

		if(!(did_hit = scene_->getAccelerator()->intersect(pRay, hit))) continue;   //hit background

		p_mat = hit.material_;
		length = pRay.tmax_;
		render_data.arena_ = n_udat;
		mat_bsd_fs = p_mat->getFlags();
		bool has_spec = mat_bsd_fs.hasAny(BsdfFlags::Specular);
		bool caustic = false;
		bool close = length < gather_dist_;
		bool do_bounce = close || has_spec;
		// further bounces construct a path just as with path tracing:
		for(int depth = 0; depth < gather_bounces_ && do_bounce; ++depth)
		{
			int d_4 = 4 * depth;
			pwo = -pRay.dir_;
			p_mat->initBsdf(render_data, hit, mat_bsd_fs);

			if(mat_bsd_fs.hasAny(BsdfFlags::Volumetric) && (vol = p_mat->getVolumeHandler(hit.n_ * pwo < 0)))
			{
				if(vol->transmittance(render_data, pRay, vcol)) throughput *= vcol;
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
					const Photon *nearest = session_global.radiance_map_.get()->findNearest(hit.p_, sf, lookup_rad_);
					if(nearest) lcol = nearest->color();
				}

				if(close || caustic)
				{
					if(mat_bsd_fs.hasAny(BsdfFlags::Emit)) lcol += p_mat->emit(render_data, hit, pwo);
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
			scol = p_mat->sample(render_data, hit, pwo, pRay.dir_, sb, w);

			if(sb.pdf_ <= 1.0e-6f)
			{
				did_hit = false;
				break;
			}

			scol *= w;

			pRay.tmin_ = scene_->ray_min_dist_;
			pRay.tmax_ = -1.0;
			pRay.from_ = hit.p_;
			throughput *= scol;
			did_hit = scene_->getAccelerator()->intersect(pRay, hit);

			if(!did_hit) //hit background
			{
				const auto &background = scene_->getBackground();
				if(caustic && background && background->hasIbl() && background->shootsCaustic())
				{
					path_col += throughput * (*background)(pRay, render_data, true);
				}
				break;
			}

			p_mat = hit.material_;
			length += pRay.tmax_;
			caustic = (caustic || !depth) && sb.sampled_flags_.hasAny(BsdfFlags::Specular | BsdfFlags::Filter);
			close = length < gather_dist_;
			do_bounce = caustic || close;
		}

		if(did_hit)
		{
			p_mat->initBsdf(render_data, hit, mat_bsd_fs);
			if(mat_bsd_fs.hasAny(BsdfFlags::Diffuse | BsdfFlags::Glossy))
			{
				Vec3 sf = SurfacePoint::normalFaceForward(hit.ng_, hit.n_, -pRay.dir_);
				const Photon *nearest = session_global.radiance_map_.get()->findNearest(hit.p_, sf, lookup_rad_);
				if(nearest) lcol = nearest->color();
				if(mat_bsd_fs.hasAny(BsdfFlags::Emit)) lcol += p_mat->emit(render_data, hit, -pRay.dir_);
				path_col += lcol * throughput;
			}
		}
		render_data.arena_ = first_udat;
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

	void *o_udat = render_data.arena_;
	const bool old_lights_geometry_material_emit = render_data.lights_geometry_material_emit_;

	if(transp_background_) alpha = 0.0;
	else alpha = 1.0;

	if(scene_->getAccelerator()->intersect(ray, sp))
	{
		alignas (16) unsigned char userdata[user_data_size_];
		render_data.arena_ = static_cast<void *>(userdata);

		if(render_data.raylevel_ == 0)
		{
			render_data.chromatic_ = true;
			render_data.lights_geometry_material_emit_ = true;
		}
		BsdfFlags bsdfs;

		Vec3 wo = -ray.dir_;
		const Material *material = sp.material_;
		material->initBsdf(render_data, sp, bsdfs);

		if(additional_depth < material->getAdditionalDepth()) additional_depth = material->getAdditionalDepth();

		const Rgb col_emit = material->emit(render_data, sp, wo);
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
				const Photon *nearest = session_global.radiance_map_.get()->findNearest(sp.p_, n, lookup_rad_);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::Radiance))
					{
						Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
						const Photon *nearest = session_global.radiance_map_.get()->findNearest(sp.p_, n, lookup_rad_);
						if(nearest) color_layer->color_ = nearest->color();
					}
				}

				// contribution of light emitting surfaces
				if(bsdfs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = material->emit(render_data, sp, wo);
					col += col_tmp;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_tmp;
					}
				}

				if(bsdfs.hasAny(BsdfFlags::Diffuse))
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
				const Photon *nearest = session_global.diffuse_map_.get()->findNearest(sp.p_, n, ds_radius_);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(use_photon_diffuse_ && layers_used)
				{
					if(ColorLayer *color_layer = color_layers->find(Layer::Radiance))
					{
						Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
						const Photon *nearest = session_global.radiance_map_.get()->findNearest(sp.p_, n, lookup_rad_);
						if(nearest) color_layer->color_ = nearest->color();
					}
				}

				if(bsdfs.hasAny(BsdfFlags::Emit))
				{
					const Rgb col_tmp = material->emit(render_data, sp, wo);
					col += col_tmp;
					if(layers_used)
					{
						if(ColorLayer *color_layer = color_layers->find(Layer::Emit)) color_layer->color_ += col_tmp;
					}
				}

				if(bsdfs.hasAny(BsdfFlags::Diffuse))
				{
					col += estimateAllDirectLight(render_data, sp, wo, color_layers);
				}

				FoundPhoton *gathered = (FoundPhoton *)alloca(n_diffuse_search_ * sizeof(FoundPhoton));
				float radius = ds_radius_; //actually the square radius...

				int n_gathered = 0;

				if(use_photon_diffuse_ && session_global.diffuse_map_.get()->nPhotons() > 0) n_gathered = session_global.diffuse_map_.get()->gather(sp.p_, gathered, n_diffuse_search_, radius);
				if(use_photon_diffuse_ && n_gathered > 0)
				{
					if(n_gathered > n_max) n_max = n_gathered;

					float scale = 1.f / ((float)session_global.diffuse_map_.get()->nPaths() * radius * M_PI);
					for(int i = 0; i < n_gathered; ++i)
					{
						const Vec3 pdir = gathered[i].photon_->direction();
						const Rgb surf_col = material->eval(render_data, sp, wo, pdir, BsdfFlags::Diffuse);

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
		if(use_photon_caustics_ && bsdfs.hasAny(BsdfFlags::Diffuse))
		{
			Rgb col_tmp = estimateCausticPhotons(render_data, sp, wo);
			if(aa_noise_params_.clamp_indirect_ > 0.f) col_tmp.clampProportionalRgb(aa_noise_params_.clamp_indirect_);
			col += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Indirect)) color_layer->color_ = col_tmp;
			}
		}

		recursiveRaytrace(render_data, ray, bsdfs, sp, wo, col, alpha, additional_depth, color_layers);

		if(layers_used)
		{
			generateCommonLayers(render_data, sp, ray, color_layers);

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
			col += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Env)) color_layer->color_ = col_tmp;
			}
		}
	}

	render_data.arena_ = o_udat;
	render_data.lights_geometry_material_emit_ = old_lights_geometry_material_emit;

	Rgb col_vol_transmittance = scene_->vol_integrator_->transmittance(render_data, ray);
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

std::unique_ptr<Integrator> PhotonIntegrator::factory(ParamMap &params, const Scene &scene)
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

	auto inte = std::unique_ptr<PhotonIntegrator>(new PhotonIntegrator(num_photons, num_c_photons, transp_shad, shadow_depth, ds_rad, c_rad));

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
