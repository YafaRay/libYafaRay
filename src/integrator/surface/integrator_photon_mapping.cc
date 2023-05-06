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
#include "color/color_layers.h"
#include "param/param.h"
#include "sampler/halton.h"
#include "sampler/sample_pdf1d.h"
#include "light/light.h"
#include "background/background.h"
#include "render/imagefilm.h"
#include "render/render_data.h"
#include "math/interpolation.h"
#include "render/render_control.h"
#include "material/sample.h"
#include "photon/photon_sample.h"
#include "volume/handler/volume_handler.h"
#include "integrator/volume/integrator_volume.h"
#include "render/render_monitor.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> PhotonIntegrator::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(diffuse_);
	PARAM_META(photons_diffuse_);
	PARAM_META(diffuse_radius_);
	PARAM_META(num_photons_diffuse_search_);
	PARAM_META(final_gather_);
	PARAM_META(fg_samples_);
	PARAM_META(bounces_);
	PARAM_META(fg_bounces_);
	PARAM_META(gather_dist_);
	PARAM_META(show_map_);
	return param_meta_map;
}

PhotonIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(diffuse_);
	PARAM_LOAD(photons_diffuse_);
	PARAM_LOAD(diffuse_radius_);
	PARAM_LOAD(num_photons_diffuse_search_);
	PARAM_LOAD(final_gather_);
	PARAM_LOAD(fg_samples_);
	PARAM_LOAD(bounces_);
	PARAM_LOAD(fg_bounces_);
	gather_dist_ = diffuse_radius_;
	PARAM_LOAD(gather_dist_);
	PARAM_LOAD(show_map_);
}

ParamMap PhotonIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(diffuse_);
	PARAM_SAVE(photons_diffuse_);
	PARAM_SAVE(diffuse_radius_);
	PARAM_SAVE(num_photons_diffuse_search_);
	PARAM_SAVE(final_gather_);
	PARAM_SAVE(fg_samples_);
	PARAM_SAVE(bounces_);
	PARAM_SAVE(fg_bounces_);
	PARAM_SAVE(gather_dist_);
	PARAM_SAVE(show_map_);
	return param_map;
}

std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> PhotonIntegrator::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto integrator {std::make_unique<PhotonIntegrator>(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(getClassName(), {"type"}));
	return {std::move(integrator), param_result};
}

void PhotonIntegrator::preGatherWorker(RenderMonitor &render_monitor, PreGatherData *gdata, const RenderControl &render_control, float ds_rad, int n_search)
{
	const float ds_radius_2 = ds_rad * ds_rad;
	gdata->lock();
	unsigned int start = gdata->fetched_;
	const unsigned int total = gdata->rad_points_.size();
	unsigned int end = gdata->fetched_ = std::min(total, start + 32);
	gdata->unlock();

	auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_search]);

	float radius = 0.f;
	const float i_scale = 1.f / (static_cast<float>(gdata->getDiffuseMap()->nPaths()) * math::num_pi<>);
	float scale = 0.f;

	while(start < total)
	{
		if(render_control.canceled()) return;
		for(unsigned int n = start; n < end; ++n)
		{
			radius = ds_radius_2;//actually the square radius...
			const int n_gathered = gdata->getDiffuseMap()->gather(gdata->rad_points_[n].pos_, gathered.get(), n_search, radius);

			Vec3f rnorm{gdata->rad_points_[n].normal_};

			Rgb sum(0.0);

			if(n_gathered > 0)
			{
				scale = i_scale / radius;

				for(int i = 0; i < n_gathered; ++i)
				{
					Vec3f pdir{gathered[i].photon_->dir_};

					if(rnorm * pdir > 0.f) sum += gdata->rad_points_[n].refl_ * scale * gathered[i].photon_->col_;
					else sum += gdata->rad_points_[n].transm_ * scale * gathered[i].photon_->col_;
				}
			}

			gdata->radiance_vec_[n] = Photon{rnorm, gdata->rad_points_[n].pos_, sum, gdata->rad_points_[n].time_};
		}
		gdata->lock();
		start = gdata->fetched_;
		end = gdata->fetched_ = std::min(total, start + 32);
		render_monitor.updateProgressBar(32);
		gdata->unlock();
	}
}

PhotonIntegrator::PhotonIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : ParentClassType_t(logger, param_result, name, param_map), params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	diffuse_map_ = std::make_unique<PhotonMap>(logger);
	getDiffuseMap()->setName("Diffuse Photon Map");
	radiance_map_ = std::make_unique<PhotonMap>(logger);
	getRadianceMap()->setName("FG Radiance Photon Map");
}

void PhotonIntegrator::diffuseWorker(RenderMonitor &render_monitor, PreGatherData &pgdat, unsigned int &total_photons_shot, const RenderControl &render_control, int thread_id, const Pdf1D *light_power_d, const std::vector<const Light *> &lights_diffuse, int pb_step)
{
	//shoot photons
	bool done = false;
	unsigned int curr = 0;
	std::unique_ptr<const SurfacePoint> hit_prev, hit_curr;
	const int num_lights_diffuse = lights_diffuse.size();
	const auto f_num_lights = static_cast<float>(num_lights_diffuse);
	const unsigned int n_diffuse_photons_thread = 1 + ((photons_diffuse_ - 1) / num_threads_photons_);
	std::vector<Photon> local_diffuse_photons;
	std::vector<RadData> local_rad_points;
	local_diffuse_photons.clear();
	local_diffuse_photons.reserve(n_diffuse_photons_thread);
	local_rad_points.clear();
	const float inv_diff_photons = 1.f / static_cast<float>(photons_diffuse_);
	while(!done)
	{
		if(render_control.canceled()) return;
		unsigned int haltoncurr = curr + n_diffuse_photons_thread * thread_id;
		const float s_1 = sample::riVdC(haltoncurr);
		const float s_2 = Halton::lowDiscrepancySampling(fast_random_, 2, haltoncurr);
		const float s_3 = Halton::lowDiscrepancySampling(fast_random_, 3, haltoncurr);
		const float s_4 = Halton::lowDiscrepancySampling(fast_random_, 4, haltoncurr);
		const float s_l = float(haltoncurr) * inv_diff_photons;
		const auto [light_num, light_num_pdf]{light_power_d->dSample(s_l)};
		if(light_num >= num_lights_diffuse)
		{
			logger_.logError(getName(), ": lightPDF sample error! ", s_l, "/", light_num);
			return;
		}
		float time = TiledIntegrator::params_.time_forced_ ? TiledIntegrator::params_.time_forced_value_ : math::addMod1(static_cast<float>(curr) / static_cast<float>(n_diffuse_photons_thread), s_2); //FIXME: maybe we should use an offset for time that is independent from the space-related samples as s_2 now
		auto[ray, light_pdf, pcol]{lights_diffuse[light_num]->emitPhoton(s_1, s_2, s_3, s_4, time)};
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
			std::tie(hit_curr, ray.tmax_) = accelerator_->intersect(ray);
			if(!hit_curr) break;
			Rgb transm(1.f);
			if(material_prev && hit_prev && mat_bsdfs_prev.has(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = material_prev->getVolumeHandler(hit_prev->ng_ * -ray.dir_ < 0))
				{
					transm = vol->transmittance(ray);
				}
			}
			const Vec3f wi{-ray.dir_};
			const BsdfFlags mat_bsdfs = hit_curr->mat_data_->bsdf_flags_;
			if(mat_bsdfs.has(BsdfFlags::Diffuse))
			{
				//deposit photon on surface
				if(!caustic_photon)
				{
					local_diffuse_photons.emplace_back(Photon{wi, hit_curr->p_, pcol, ray.time_});
				}
				// create entry for radiance photon:
				// don't forget to choose subset only, face normal forward; geometric vs. smooth normal?
				if(params_.final_gather_ && fast_random_.getNextFloatNormalized() < 0.125 && !caustic_photon)
				{
					const Vec3f n{SurfacePoint::normalFaceForward(hit_curr->ng_, hit_curr->n_, wi)};
					RadData rd(hit_curr->p_, n, ray.time_);
					rd.refl_ = hit_curr->getReflectivity(fast_random_, BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Reflect, true);
					rd.transm_ = hit_curr->getReflectivity(fast_random_, BsdfFlags::Diffuse | BsdfFlags::Glossy | BsdfFlags::Transmit, true);
					local_rad_points.emplace_back(rd);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == params_.bounces_) break;
			// scatter photon
			const int d_5 = 3 * n_bounces + 5;
			const float s_5 = Halton::lowDiscrepancySampling(fast_random_, d_5, haltoncurr);
			const float s_6 = Halton::lowDiscrepancySampling(fast_random_, d_5 + 1, haltoncurr);
			const float s_7 = Halton::lowDiscrepancySampling(fast_random_, d_5 + 2, haltoncurr);
			PSample sample(s_5, s_6, s_7, BsdfFlags::All, pcol, transm);
			Vec3f wo;
			bool scattered = hit_curr->scatterPhoton(wi, wo, sample, true);
			if(!scattered) break; //photon was absorped.
			pcol = sample.color_;
			caustic_photon = (sample.sampled_flags_.has((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.has((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) &&
							  caustic_photon);
			direct_photon = sample.sampled_flags_.has(BsdfFlags::Filter) && direct_photon;
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
		done = (curr >= n_diffuse_photons_thread);
	}
	getDiffuseMap()->lock();
	getDiffuseMap()->appendVector(local_diffuse_photons, curr);
	total_photons_shot += curr;
	getDiffuseMap()->unlock();
	pgdat.lock();
	pgdat.rad_points_.insert(std::end(pgdat.rad_points_), std::begin(local_rad_points), std::end(local_rad_points));
	pgdat.unlock();
}

void PhotonIntegrator::photonMapKdTreeWorker(PhotonMap *photon_map, const RenderMonitor &render_monitor, const RenderControl &render_control)
{
	photon_map->updateTree(render_monitor, render_control);
}

bool PhotonIntegrator::preprocess(RenderMonitor &render_monitor, const RenderControl &render_control, const Scene &scene)
{
	bool success = SurfaceIntegrator::preprocess(render_monitor, render_control, scene);

	std::stringstream set;

	render_monitor.addTimerEvent("prepass");
	render_monitor.startTimer("prepass");

	logger_.logInfo(getName(), ": Starting preprocess...");

	set << "Photon Mapping  ";

	if(MonteCarloIntegrator::params_.transparent_shadows_)
	{
		set << "ShadowDepth=" << MonteCarloIntegrator::params_.shadow_depth_ << "  ";
	}
	set << "RayDepth=" << MonteCarloIntegrator::params_.r_depth_ << "  ";

	if(CausticPhotonIntegrator::params_.use_photon_caustics_)
	{
		set << "\nCaustic photons=" << n_caus_photons_ << " search=" << CausticPhotonIntegrator::params_.n_caus_search_ << " radius=" << CausticPhotonIntegrator::params_.caus_radius_ << " depth=" << CausticPhotonIntegrator::params_.caus_depth_ << "  ";
	}

	if(use_photon_diffuse_)
	{
		set << "\nDiffuse photons=" << photons_diffuse_ << " search=" << params_.num_photons_diffuse_search_ << " radius=" << params_.diffuse_radius_ << "  ";
	}

	if(params_.final_gather_)
	{
		set << " FG paths=" << params_.fg_samples_ << " bounces=" << params_.fg_bounces_ << "  ";
	}

	getDiffuseMap()->clear();
	getDiffuseMap()->setNumPaths(0);
	getDiffuseMap()->reserveMemory(photons_diffuse_);
	getDiffuseMap()->setNumThreadsPkDtree(num_threads_photons_);

	getCausticMap()->clear();
	getCausticMap()->setNumPaths(0);
	getCausticMap()->reserveMemory(n_caus_photons_);
	getCausticMap()->setNumThreadsPkDtree(num_threads_photons_);

	getRadianceMap()->clear();
	getRadianceMap()->setNumPaths(0);
	getRadianceMap()->setNumThreadsPkDtree(num_threads_photons_);

	//shoot photons
	unsigned int curr = 0;
	// for radiance map:
	PreGatherData pgdat(diffuse_map_.get());

	const auto lights_diffuse{getLightsEmittingDiffusePhotons()};
	if(lights_diffuse.empty())
	{
		logger_.logWarning(getName(), ": No lights found that can shoot diffuse photons, disabling Diffuse photon processing");
		enableDiffuse(false);
	}

	if(use_photon_diffuse_ && !lights_diffuse.empty())
	{
		const int num_lights_diffuse = lights_diffuse.size();
		const auto f_num_lights = static_cast<float>(num_lights_diffuse);
		std::vector<float> energies_diffuse(num_lights_diffuse);
		for(int i = 0; i < num_lights_diffuse; ++i) energies_diffuse[i] = lights_diffuse[i]->totalEnergy().energy();
		auto light_power_d_diffuse = std::make_unique<Pdf1D>(energies_diffuse);
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for diffuse map:");
		for(int i = 0; i < num_lights_diffuse; ++i)
		{
			auto[ray, light_pdf, pcol]{lights_diffuse[i]->emitPhoton(.5, .5, .5, .5, 0.f)}; //FIXME: what time to use?
			const float light_num_pdf = light_power_d_diffuse->function(i) * light_power_d_diffuse->invIntegral();
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", lights_diffuse[i]->getName(), "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}
		//shoot photons
		curr = 0;
		logger_.logInfo(getName(), ": Building diffuse photon map...");
		render_monitor.initProgressBar(128, logger_.getConsoleLogColorsEnabled());
		const int pb_step = std::max(1, photons_diffuse_ / 128);
		render_monitor.setProgressBarTag("Building diffuse photon map...");
		//Pregather diffuse photons
		photons_diffuse_ = std::max(num_threads_photons_, (photons_diffuse_ / num_threads_photons_) * num_threads_photons_); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread
		logger_.logParams(getName(), ": Shooting ", photons_diffuse_, " photons across ", num_threads_photons_, " threads (", (photons_diffuse_ / num_threads_photons_), " photons/thread)");
		std::vector<std::thread> threads;
		threads.reserve(num_threads_photons_);
		for(int i = 0; i < num_threads_photons_; ++i) threads.emplace_back(&PhotonIntegrator::diffuseWorker, this, std::ref(render_monitor), std::ref(pgdat), std::ref(curr), std::ref(render_control), i, light_power_d_diffuse.get(), lights_diffuse, pb_step);
		for(auto &t : threads) t.join();

		render_monitor.setProgressBarAsDone();
		render_monitor.setProgressBarTag("Diffuse photon map built.");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse photon map built.");
		logger_.logInfo(getName(), ": Shot ", curr, " photons from ", num_lights_diffuse, " light(s)");

		if(getDiffuseMap()->nPhotons() < 50)
		{
			logger_.logError(getName(), ": Too few diffuse photons, stopping now.");
			return false;
		}

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored diffuse photons: ", getDiffuseMap()->nPhotons());
	}
	else
	{
		logger_.logInfo(getName(), ": Diffuse photon mapping disabled, skipping...");
	}

	std::thread diffuse_map_build_kd_tree_thread;

	if(use_photon_diffuse_ && getDiffuseMap()->nPhotons() > 0)
	{
		if(num_threads_photons_ >= 2)
		{
			logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
			render_monitor.setProgressBarTag("Building diffuse photons kd-tree...");
			diffuse_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, diffuse_map_.get(), std::ref(render_monitor), std::ref(render_control));
		}
		else
		{
			logger_.logInfo(getName(), ": Building diffuse photons kd-tree:");
			render_monitor.setProgressBarTag("Building diffuse photons kd-tree...");
			getDiffuseMap()->updateTree(render_monitor, render_control);
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
	}

	const auto lights_caustic{getLightsEmittingCausticPhotons()};
	if(lights_caustic.empty())
	{
		logger_.logWarning(getName(), ": No lights found that can shoot caustic photons, disabling Caustic photon processing");
		enableCaustics(false);
	}

	if(CausticPhotonIntegrator::params_.use_photon_caustics_ && !lights_caustic.empty())
	{
		curr = 0;
		const int num_lights_caustic = lights_caustic.size();
		const auto f_num_lights = static_cast<float>(num_lights_caustic);
		std::vector<float> energies_caustic(num_lights_caustic);

		for(int i = 0; i < num_lights_caustic; ++i) energies_caustic[i] = lights_caustic[i]->totalEnergy().energy();

		auto light_power_d_caustic = std::make_unique<Pdf1D>(energies_caustic);

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for caustics map:");
		for(int i = 0; i < num_lights_caustic; ++i)
		{
			auto[ray, light_pdf, pcol]{lights_caustic[i]->emitPhoton(.5, .5, .5, .5, 0.f)}; //FIXME: what time to use?
			const float light_num_pdf = light_power_d_caustic->function(i) * light_power_d_caustic->invIntegral();
			pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", lights_caustic[i]->getName(), "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}

		logger_.logInfo(getName(), ": Building caustics photon map...");
		render_monitor.initProgressBar(128, logger_.getConsoleLogColorsEnabled());
		const int pb_step = std::max(1, n_caus_photons_ / 128);
		render_monitor.setProgressBarTag("Building caustics photon map...");
		//Pregather caustic photons

		n_caus_photons_ = std::max(num_threads_photons_, (n_caus_photons_ / num_threads_photons_) * num_threads_photons_); //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		logger_.logParams(getName(), ": Shooting ", n_caus_photons_, " photons across ", num_threads_photons_, " threads (", (n_caus_photons_ / num_threads_photons_), " photons/thread)");

		std::vector<std::thread> threads;
		threads.reserve(num_threads_photons_);
		for(int i = 0; i < num_threads_photons_; ++i) threads.emplace_back(&PhotonIntegrator::causticWorker, this, std::ref(render_monitor), std::ref(curr), std::ref(render_control), i, light_power_d_caustic.get(), lights_caustic, pb_step);
		for(auto &t : threads) t.join();

		render_monitor.setProgressBarAsDone();
		render_monitor.setProgressBarTag("Caustics photon map built.");
		logger_.logInfo(getName(), ": Shot ", curr, " caustic photons from ", num_lights_caustic, " light(s).");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored caustic photons: ", getCausticMap()->nPhotons());
	}
	else
	{
		logger_.logInfo(getName(), ": Caustics photon mapping disabled, skipping...");
	}

	std::thread caustic_map_build_kd_tree_thread;
	if(CausticPhotonIntegrator::params_.use_photon_caustics_ && getCausticMap()->nPhotons() > 0)
	{
		if(num_threads_photons_ >= 2)
		{
			logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
			render_monitor.setProgressBarTag("Building caustic photons kd-tree...");
			caustic_map_build_kd_tree_thread = std::thread(&PhotonIntegrator::photonMapKdTreeWorker, caustic_map_.get(), std::ref(render_monitor), std::ref(render_control));
		}
		else
		{
			logger_.logInfo(getName(), ": Building caustic photons kd-tree:");
			render_monitor.setProgressBarTag("Building caustic photons kd-tree...");
			getCausticMap()->updateTree(render_monitor, render_control);
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
	}

	if(use_photon_diffuse_ && getDiffuseMap()->nPhotons() > 0 && num_threads_photons_ >= 2)
	{
		diffuse_map_build_kd_tree_thread.join();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Diffuse photon map: done.");
	}

	if(use_photon_diffuse_ && params_.final_gather_) //create radiance map:
	{
		// == remove too close radiance points ==//
		auto r_tree = std::make_unique<kdtree::PointKdTree<RadData>>(logger_, render_monitor, render_control, pgdat.rad_points_, "FG Radiance Photon Map", num_threads_photons_);
		std::vector< RadData > cleaned;
		for(const auto &rad_point : pgdat.rad_points_)
		{
			if(rad_point.use_)
			{
				cleaned.emplace_back(rad_point);
				const EliminatePhoton elim_proc(rad_point.normal_);
				float maxrad = 0.01f * params_.diffuse_radius_; // 10% of diffuse search radius
				r_tree->lookup(rad_point.pos_, elim_proc, maxrad);
			}
		}
		pgdat.rad_points_.swap(cleaned);
		// ================ //
		int n_threads = num_threads_;
		pgdat.radiance_vec_.resize(pgdat.rad_points_.size());
		render_monitor.initProgressBar(pgdat.rad_points_.size(), logger_.getConsoleLogColorsEnabled());
		render_monitor.setProgressBarTag("Pregathering radiance data for final gathering...");

		std::vector<std::thread> threads;
		threads.reserve(n_threads);
		for(int i = 0; i < n_threads; ++i) threads.emplace_back(&PhotonIntegrator::preGatherWorker, std::ref(render_monitor), &pgdat, std::ref(render_control), params_.diffuse_radius_, params_.num_photons_diffuse_search_);
		for(auto &t : threads) t.join();

		getRadianceMap()->swapVector(pgdat.radiance_vec_);
		render_monitor.setProgressBarAsDone();
		render_monitor.setProgressBarTag("Pregathering radiance data done...");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Radiance tree built... Updating the tree...");
		getRadianceMap()->updateTree(render_monitor, render_control);
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
	}

	if(CausticPhotonIntegrator::params_.use_photon_caustics_ && getCausticMap()->nPhotons() > 0 && num_threads_photons_ >= 2)
	{
		caustic_map_build_kd_tree_thread.join();
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Caustic photon map: done.");
	}

	render_monitor.stopTimer("prepass");
	logger_.logInfo(getName(), ": Photonmap building time: ", std::fixed, std::setprecision(1), render_monitor.getTimerTime("prepass"), "s", " (", num_threads_photons_, " thread(s))");

	set << "| photon maps: " << std::fixed << std::setprecision(1) << render_monitor.getTimerTime("prepass") << "s" << " [" << num_threads_photons_ << " thread(s)]";

	render_monitor.setRenderInfo(render_monitor.getRenderInfo() + set.str());

	if(logger_.isVerbose())
	{
		for(std::string line; std::getline(set, line, '\n');) logger_.logVerbose(line);
	}
	return success;
}

// final gathering: this is basically a full path tracer only that it uses the radiance map only
// at the path end. I.e. paths longer than 1 are only generated to overcome lack of local radiance detail.
// precondition: initBSDF of current spot has been called!
Rgb PhotonIntegrator::finalGathering(RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data)
{
	Rgb path_col(0.0);
	float w = 0.f;

	int n_sampl = (int) ceilf(std::max(1, params_.fg_samples_ / ray_division.division_) * pixel_sampling_data.aa_indirect_sample_multiplier_);
	for(int i = 0; i < n_sampl; ++i)
	{
		Rgb throughput(1.0);
		float length = 0;
		Vec3f pwo{wo};
		Ray p_ray;
		p_ray.time_ = sp.time_;
		bool did_hit;
		unsigned int offs = params_.fg_samples_ * pixel_sampling_data.sample_ + pixel_sampling_data.offset_ + i; // some redundancy here...
		Rgb lcol, scol;
		// "zero'th" FG bounce:
		float s_1 = sample::riVdC(offs);
		float s_2 = Halton::lowDiscrepancySampling(fast_random_, 2, offs);
		if(ray_division.division_ > 1)
		{
			s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
			s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
		}

		Sample s(s_1, s_2, BsdfFlags::Diffuse | BsdfFlags::Reflect | BsdfFlags::Transmit); // glossy/dispersion/specular done via recursive raytracing
		scol = sp.sample(pwo, p_ray.dir_, s, w, chromatic_enabled, wavelength, image_film_->getCamera());

		scol *= w;
		if(scol.isBlack()) continue;

		p_ray.tmin_ = ray_min_dist_;
		p_ray.tmax_ = -1.f;
		p_ray.from_ = sp.p_;
		throughput = scol;
		std::unique_ptr<const SurfacePoint> hit;
		std::tie(hit, p_ray.tmax_) = accelerator_->intersect(p_ray, image_film_->getCamera());
		did_hit = static_cast<bool>(hit);
		if(!did_hit) continue;   //hit background
		length = p_ray.tmax_;
		const BsdfFlags mat_bsd_fs = hit->mat_data_->bsdf_flags_;
		bool has_spec = mat_bsd_fs.has(BsdfFlags::Specular);
		bool caustic = false;
		bool close = length < params_.gather_dist_;
		bool do_bounce = close || has_spec;
		// further bounces construct a path just as with path tracing:
		for(int depth = 0; depth < params_.fg_bounces_ && do_bounce; ++depth)
		{
			int d_4 = 4 * depth;
			pwo = -p_ray.dir_;
			if(mat_bsd_fs.has(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = hit->getMaterial()->getVolumeHandler(hit->n_ * pwo < 0))
				{
					throughput *= vol->transmittance(p_ray);
				}
			}
			if(mat_bsd_fs.has(BsdfFlags::Diffuse))
			{
				if(close)
				{
					lcol = estimateOneDirectLight(random_generator, correlative_sample_number, chromatic_enabled, wavelength, *hit, pwo, offs, ray_division, pixel_sampling_data);
				}
				else if(caustic)
				{
					Vec3f sf{SurfacePoint::normalFaceForward(hit->ng_, hit->n_, pwo)};
					const Photon *nearest = getRadianceMap()->findNearest(hit->p_, sf, lookup_rad_);
					if(nearest) lcol = nearest->col_;
				}

				if(close || caustic)
				{
					if(mat_bsd_fs.has(BsdfFlags::Emit)) lcol += hit->emit(pwo);
					path_col += lcol * throughput;
				}
			}

			s_1 = Halton::lowDiscrepancySampling(fast_random_, d_4 + 3, offs);
			s_2 = Halton::lowDiscrepancySampling(fast_random_, d_4 + 4, offs);

			if(ray_division.division_ > 1)
			{
				s_1 = math::addMod1(s_1, ray_division.decorrelation_1_);
				s_2 = math::addMod1(s_2, ray_division.decorrelation_2_);
			}

			Sample sb(s_1, s_2, (close) ? BsdfFlags::All : BsdfFlags::AllSpecular | BsdfFlags::Filter);
			scol = hit->sample(pwo, p_ray.dir_, sb, w, chromatic_enabled, wavelength, image_film_->getCamera());

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
			std::tie(hit, p_ray.tmax_) = accelerator_->intersect(p_ray, image_film_->getCamera());
			did_hit = static_cast<bool>(hit);
			if(!did_hit) break; //hit background
			length += p_ray.tmax_;
			caustic = (caustic || !depth) && sb.sampled_flags_.has(BsdfFlags::Specular | BsdfFlags::Filter);
			close = length < params_.gather_dist_;
			do_bounce = caustic || close;
		}

		if(did_hit)
		{
			if(mat_bsd_fs.has(BsdfFlags::Diffuse | BsdfFlags::Glossy))
			{
				Vec3f sf{SurfacePoint::normalFaceForward(hit->ng_, hit->n_, -p_ray.dir_)};
				const Photon *nearest = getRadianceMap()->findNearest(hit->p_, sf, lookup_rad_);
				if(nearest) lcol = nearest->col_; //FIXME should lcol be a local variable? Is it getting its value from previous functions or not??
				if(mat_bsd_fs.has(BsdfFlags::Emit)) lcol += hit->emit(-p_ray.dir_);
				path_col += lcol * throughput;
			}
		}
	}
	return path_col / (float)n_sampl;
}

std::pair<Rgb, float> PhotonIntegrator::integrate(Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data)
{
	static int n_max = 0;
	static int calls = 0;
	++calls;
	Rgb col {0.f};
	float alpha = 1.f;
	const auto [sp, tmax] = accelerator_->intersect(ray, image_film_->getCamera());
	ray.tmax_ = tmax;
	if(sp)
	{
		const Vec3f wo{-ray.dir_};
		const BsdfFlags mat_bsdfs = sp->mat_data_->bsdf_flags_;

		additional_depth = std::max(additional_depth, sp->getMaterial()->getAdditionalDepth());

		const Rgb col_emit = sp->emit(wo);
		col += col_emit;
		if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
		{
			if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_emit;
		}
		if(use_photon_diffuse_ && params_.final_gather_)
		{
			if(params_.show_map_)
			{
				const Vec3f n{SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo)};
				const Photon *nearest = getRadianceMap()->findNearest(sp->p_, n, lookup_rad_);
				if(nearest) col += nearest->col_;
			}
			else
			{
				if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
				{
					if(Rgba *color_layer = color_layers->find(LayerDef::Radiance))
					{
						const Vec3f n{SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo)};
						const Photon *nearest = getRadianceMap()->findNearest(sp->p_, n, lookup_rad_);
						if(nearest) *color_layer = Rgba{nearest->col_};
					}
				}

				// contribution of light emitting surfaces
				if(mat_bsdfs.has(BsdfFlags::Emit))
				{
					const Rgb col_tmp = sp->emit(wo);
					col += col_tmp;
					if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_tmp;
					}
				}

				if(mat_bsdfs.has(BsdfFlags::Diffuse))
				{
					col += estimateAllDirectLight(random_generator, color_layers, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
					Rgb col_tmp = finalGathering(random_generator, correlative_sample_number, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
					if(aa_noise_params_->clamp_indirect_ > 0.f) col_tmp.clampProportionalRgb(aa_noise_params_->clamp_indirect_);
					col += col_tmp;
					if(color_layers && color_layers->getFlags().has(LayerDef::Flags::DiffuseLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseIndirect)) *color_layer = Rgba{col_tmp};
					}
				}
			}
		}
		else
		{
			if(use_photon_diffuse_ && params_.show_map_)
			{
				const Vec3f n{SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo)};
				const Photon *nearest = getDiffuseMap()->findNearest(sp->p_, n, params_.diffuse_radius_);
				if(nearest) col += nearest->col_;
			}
			else
			{
				if(use_photon_diffuse_ && color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
				{
					if(Rgba *color_layer = color_layers->find(LayerDef::Radiance))
					{
						const Vec3f n{SurfacePoint::normalFaceForward(sp->ng_, sp->n_, wo)};
						const Photon *nearest = getRadianceMap()->findNearest(sp->p_, n, lookup_rad_);
						if(nearest) *color_layer = Rgba{nearest->col_};
					}
				}

				if(mat_bsdfs.has(BsdfFlags::Emit))
				{
					const Rgb col_tmp = sp->emit(wo);
					col += col_tmp;
					if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
					{
						if(Rgba *color_layer = color_layers->find(LayerDef::Emit)) *color_layer += col_tmp;
					}
				}

				if(mat_bsdfs.has(BsdfFlags::Diffuse))
				{
					col += estimateAllDirectLight(random_generator, color_layers, chromatic_enabled, wavelength, *sp, wo, ray_division, pixel_sampling_data);
				}

				auto *gathered = static_cast<FoundPhoton *>(alloca(params_.num_photons_diffuse_search_ * sizeof(FoundPhoton)));
				float radius = params_.diffuse_radius_; //actually the square radius...

				int n_gathered = 0;

				if(use_photon_diffuse_ && getDiffuseMap()->nPhotons() > 0) n_gathered = getDiffuseMap()->gather(sp->p_, gathered, params_.num_photons_diffuse_search_, radius);
				if(use_photon_diffuse_ && n_gathered > 0)
				{
					if(n_gathered > n_max) n_max = n_gathered;

					const float scale = 1.f / (static_cast<float>(getDiffuseMap()->nPaths()) * radius * math::num_pi<>);
					for(int i = 0; i < n_gathered; ++i)
					{
						const Vec3f pdir{gathered[i].photon_->dir_};
						const Rgb surf_col = sp->eval(wo, pdir, BsdfFlags::Diffuse);

						const Rgb col_tmp = surf_col * scale * gathered[i].photon_->col_;
						col += col_tmp;
						if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
						{
							if(Rgba *color_layer = color_layers->find(LayerDef::DiffuseIndirect)) *color_layer += col_tmp;
						}
					}
				}
			}
		}

		// add caustics
		if(CausticPhotonIntegrator::params_.use_photon_caustics_ && mat_bsdfs.has(BsdfFlags::Diffuse))
		{
			col += causticPhotons(color_layers, ray, *sp, wo, aa_noise_params_->clamp_indirect_, caustic_map_.get(), CausticPhotonIntegrator::params_.caus_radius_, CausticPhotonIntegrator::params_.n_caus_search_);
		}

		const auto [raytrace_col, raytrace_alpha]{recursiveRaytrace(random_generator, correlative_sample_number, color_layers, ray_level + 1, chromatic_enabled, wavelength, ray, mat_bsdfs, *sp, wo, additional_depth, ray_division, pixel_sampling_data)};
		col += raytrace_col;
		alpha = raytrace_alpha;
		if(color_layers)
		{
			generateCommonLayers(color_layers, *sp, *image_film_->getMaskParams(), object_index_highest_, material_index_highest_);
			generateOcclusionLayers(color_layers, *accelerator_, chromatic_enabled, wavelength, ray_division, image_film_->getCamera(), pixel_sampling_data, *sp, wo, MonteCarloIntegrator::params_.ao_samples_, SurfaceIntegrator::params_.shadow_bias_auto_, shadow_bias_, MonteCarloIntegrator::params_.ao_distance_, MonteCarloIntegrator::params_.ao_color_, MonteCarloIntegrator::params_.shadow_depth_);
			if(Rgba *color_layer = color_layers->find(LayerDef::DebugObjectTime))
			{
				const float col_combined_gray = col.col2Bri();
				if(sp->hasMotionBlur()) *color_layer += {col_combined_gray * ray.time_, 0.f, col_combined_gray * (1.f - ray.time_), 1.f};
				else *color_layer += {0.f, col_combined_gray, 0.f, 1.f};
			}
		}
	}
	else //nothing hit, return background
	{
		std::tie(col, alpha) = background(ray, color_layers, MonteCarloIntegrator::params_.transparent_background_, MonteCarloIntegrator::params_.transparent_background_refraction_, background_, ray_level);
	}
	if(vol_integrator_)
	{
		applyVolumetricEffects(col, alpha, color_layers, ray, random_generator, *vol_integrator_, MonteCarloIntegrator::params_.transparent_background_);
	}
	return {std::move(col), alpha};
}

} //namespace yafaray
