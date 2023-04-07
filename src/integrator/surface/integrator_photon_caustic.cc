/****************************************************************************
 *      integrator_photon_caustic.cc: An abstract integrator for caustic
 *      photons integration
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010  Rodrigo Placencia (DarkTide)
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

#include "integrator/surface/integrator_photon_caustic.h"
#include "integrator/volume/integrator_volume.h"
#include "param/param.h"
#include <memory>
#include "geometry/surface.h"
#include "common/layers.h"
#include "color/color_layers.h"
#include "common/logger.h"
#include "material/material.h"
#include "light/light.h"
#include "color/spectrum.h"
#include "sampler/halton.h"
#include "render/imagefilm.h"
#include "photon/photon.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "render/render_data.h"
#include "render/render_control.h"
#include "photon/photon_sample.h"
#include "volume/handler/volume_handler.h"
#include "common/sysinfo.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> CausticPhotonIntegrator::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(use_photon_caustics_);
	PARAM_META(n_caus_photons_);
	PARAM_META(n_caus_search_);
	PARAM_META(caus_radius_);
	PARAM_META(caus_depth_);
	PARAM_META(threads_photons_);
	return param_meta_map;
}

CausticPhotonIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(use_photon_caustics_);
	PARAM_LOAD(n_caus_photons_);
	PARAM_LOAD(n_caus_search_);
	PARAM_LOAD(caus_radius_);
	PARAM_LOAD(caus_depth_);
	PARAM_LOAD(threads_photons_);
}

ParamMap CausticPhotonIntegrator::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(use_photon_caustics_);
	PARAM_SAVE(n_caus_photons_);
	PARAM_SAVE(n_caus_search_);
	PARAM_SAVE(caus_radius_);
	PARAM_SAVE(caus_depth_);
	PARAM_SAVE(threads_photons_);
	return param_map;
}

//Constructor and destructor defined here to avoid issues with std::unique_ptr<Pdf1D> being Pdf1D incomplete in the header (forward declaration)
CausticPhotonIntegrator::CausticPhotonIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : ParentClassType_t(logger, param_result, name, param_map), params_{param_result, param_map}
{
	caustic_map_ = std::make_unique<PhotonMap>(logger);
	caustic_map_->setName("Caustic Photon Map");
}

CausticPhotonIntegrator::~CausticPhotonIntegrator() = default;

Rgb CausticPhotonIntegrator::causticPhotons(ColorLayers *color_layers, const Ray &ray, const SurfacePoint &sp, const Vec3f &wo, float clamp_indirect, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search)
{
	Rgb col = CausticPhotonIntegrator::estimateCausticPhotons(sp, wo, caustic_map, caustic_radius, n_caus_search);
	if(clamp_indirect > 0.f) col.clampProportionalRgb(clamp_indirect);
	if(color_layers && color_layers->getFlags().has(LayerDef::Flags::BasicLayers))
	{
		if(Rgba *color_layer = color_layers->find(LayerDef::Indirect)) *color_layer += col;
	}
	return col;
}

void CausticPhotonIntegrator::causticWorker(RenderControl &render_control, FastRandom &fast_random, unsigned int &total_photons_shot, int thread_id, const Pdf1D *light_power_d_caustic, const std::vector<const Light *> &lights_caustic, int pb_step)
{
	bool done = false;
	const int num_lights_caustic = lights_caustic.size();
	const auto f_num_lights = static_cast<float>(num_lights_caustic);
	unsigned int curr = 0;
	const unsigned int n_caus_photons_thread = 1 + ((n_caus_photons_ - 1) / num_threads_photons_);
	std::vector<Photon> local_caustic_photons;
	std::unique_ptr<const SurfacePoint> hit_prev, hit_curr;
	local_caustic_photons.clear();
	local_caustic_photons.reserve(n_caus_photons_thread);
	while(!done)
	{
		const unsigned int haltoncurr = curr + n_caus_photons_thread * thread_id;
		const float wavelength = sample::riS(haltoncurr);
		const float s_1 = sample::riVdC(haltoncurr);
		const float s_2 = Halton::lowDiscrepancySampling(fast_random, 2, haltoncurr);
		const float s_3 = Halton::lowDiscrepancySampling(fast_random, 3, haltoncurr);
		const float s_4 = Halton::lowDiscrepancySampling(fast_random, 4, haltoncurr);
		const float s_l = static_cast<float>(haltoncurr) / static_cast<float>(n_caus_photons_);
		const auto [light_num, light_num_pdf]{light_power_d_caustic->dSample(s_l)};
		if(light_num >= num_lights_caustic)
		{
			logger_.logError(getName(), ": lightPDF sample error! ", s_l, "/", light_num);
			return;
		}
		float time = TiledIntegrator::params_.time_forced_ ? TiledIntegrator::params_.time_forced_value_ : math::addMod1(static_cast<float>(curr) / static_cast<float>(n_caus_photons_thread), s_2); //FIXME: maybe we should use an offset for time that is independent from the space-related samples as s_2 now
		auto[ray, light_pdf, pcol]{lights_caustic[light_num]->emitPhoton(s_1, s_2, s_3, s_4, time)};
		ray.tmin_ = ray_min_dist_;
		ray.tmax_ = -1.f;
		pcol *= f_num_lights * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= n_caus_photons_thread);
			continue;
		}
		else if(std::isnan(pcol.r_) || std::isnan(pcol.g_) || std::isnan(pcol.b_))
		{
			logger_.logWarning(getName(), ": NaN (photon color)");
			continue;
		}
		int n_bounces = 0;
		bool caustic_photon = false;
		bool direct_photon = true;
		const Material *material_prev = nullptr;
		BsdfFlags mat_bsdfs_prev = BsdfFlags::None;
		bool chromatic_enabled = true;
		while(true)
		{
			std::tie(hit_curr, ray.tmax_) = accelerator_->intersect(ray);
			if(!hit_curr) break;
			// check for volumetric effects, based on the material from the previous photon bounce
			Rgb transm(1.f);
			if(material_prev && hit_prev && mat_bsdfs_prev.has(BsdfFlags::Volumetric))
			{
				if(const VolumeHandler *vol = material_prev->getVolumeHandler(hit_prev->ng_ * ray.dir_ < 0))
				{
					transm = vol->transmittance(ray);
				}
			}
			const Vec3f wi{-ray.dir_};
			const BsdfFlags mat_bsdfs = hit_curr->mat_data_->bsdf_flags_;
			if(mat_bsdfs.has((BsdfFlags::Diffuse | BsdfFlags::Glossy)))
			{
				//deposit caustic photon on surface
				if(caustic_photon)
				{
					local_caustic_photons.emplace_back(Photon{wi, hit_curr->p_, pcol, ray.time_});
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(n_bounces == params_.caus_depth_) break;
			// scatter photon
			const int d_5 = 3 * n_bounces + 5;
			//int d6 = d5 + 1;

			const float s_5 = Halton::lowDiscrepancySampling(fast_random, d_5, haltoncurr);
			const float s_6 = Halton::lowDiscrepancySampling(fast_random, d_5 + 1, haltoncurr);
			const float s_7 = Halton::lowDiscrepancySampling(fast_random, d_5 + 2, haltoncurr);

			PSample sample(s_5, s_6, s_7, BsdfFlags::AllSpecular | BsdfFlags::Glossy | BsdfFlags::Filter | BsdfFlags::Dispersive, pcol, transm);
			Vec3f wo;
			bool scattered = hit_curr->scatterPhoton(wi, wo, sample, chromatic_enabled, wavelength);
			if(!scattered) break; //photon was absorped.
			pcol = sample.color_;
			// hm...dispersive is not really a scattering qualifier like specular/glossy/diffuse or the special case filter...
			caustic_photon = (sample.sampled_flags_.has((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Dispersive)) && direct_photon) ||
							 (sample.sampled_flags_.has((BsdfFlags::Glossy | BsdfFlags::Specular | BsdfFlags::Filter | BsdfFlags::Dispersive)) && caustic_photon);
			// light through transparent materials can be calculated by direct lighting, so still consider them direct!
			direct_photon = sample.sampled_flags_.has(BsdfFlags::Filter) && direct_photon;
			// caustic-only calculation can be stopped if:
			if(!(caustic_photon || direct_photon)) break;

			if(chromatic_enabled && sample.sampled_flags_.has(BsdfFlags::Dispersive))
			{
				chromatic_enabled = false;
				pcol *= spectrum::wl2Rgb(wavelength);
			}
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
			render_control.updateProgressBar();
			if(render_control.canceled()) { return; }
		}
		done = (curr >= n_caus_photons_thread);
	}
	caustic_map_->mutx_.lock();
	caustic_map_->appendVector(local_caustic_photons, curr);
	total_photons_shot += curr;
	caustic_map_->mutx_.unlock();
}

bool CausticPhotonIntegrator::createCausticMap(RenderControl &render_control, FastRandom &fast_random)
{
	caustic_map_->clear();
	caustic_map_->setNumPaths(0);
	caustic_map_->reserveMemory(n_caus_photons_);
	caustic_map_->setNumThreadsPkDtree(num_threads_photons_);

	const auto lights_caustic{getLightsEmittingCausticPhotons()};
	if(!lights_caustic.empty())
	{
		const int num_lights_caustic = lights_caustic.size();
		const auto f_num_lights_caustic = static_cast<float>(num_lights_caustic);
		std::vector<float> energies_caustic(num_lights_caustic);
		for(int i = 0; i < num_lights_caustic; ++i) energies_caustic[i] = lights_caustic[i]->totalEnergy().energy();
		auto light_power_d_caustic = std::make_unique<Pdf1D>(energies_caustic);

		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light(s) photon color testing for caustics map:");

		for(int i = 0; i < num_lights_caustic; ++i)
		{
			auto[ray, light_pdf, pcol]{lights_caustic[i]->emitPhoton(.5, .5, .5, .5, 0.f)}; //FIXME: What time to use?
			const float light_num_pdf = light_power_d_caustic->function(i) * light_power_d_caustic->invIntegral();
			pcol *= f_num_lights_caustic * light_pdf / light_num_pdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Light [", lights_caustic[i]->getName(), "] Photon col:", pcol, " | lnpdf: ", light_num_pdf);
		}

		logger_.logInfo(getName(), ": Building caustics photon map...");
		render_control.initProgressBar(128, logger_.getConsoleLogColorsEnabled());
		const int pb_step = std::max(1, n_caus_photons_ / 128);
		render_control.setProgressBarTag("Building caustics photon map...");

		unsigned int curr = 0;

		n_caus_photons_ = std::max(num_threads_photons_, (n_caus_photons_ / num_threads_photons_) * num_threads_photons_); //rounding the number of diffuse photons, so it's a number divisible by the number of threads (distribute uniformly among the threads). At least 1 photon per thread

		logger_.logParams(getName(), ": Shooting ", n_caus_photons_, " photons across ", num_threads_photons_, " threads (", (n_caus_photons_ / num_threads_photons_), " photons/thread)");

		std::vector<std::thread> threads;
		threads.reserve(num_threads_photons_);
		for(int i = 0; i < num_threads_photons_; ++i) threads.emplace_back(&CausticPhotonIntegrator::causticWorker, this, std::ref(render_control), std::ref(fast_random), std::ref(curr), i, light_power_d_caustic.get(), lights_caustic, pb_step);
		for(auto &t : threads) t.join();

		render_control.setProgressBarAsDone();
		render_control.setProgressBarTag("Caustic photon map built.");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		logger_.logInfo(getName(), ": Shot ", curr, " caustic photons from ", num_lights_caustic, " light(s).");
		if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Stored caustic photons: ", caustic_map_->nPhotons());

		if(caustic_map_->nPhotons() > 0)
		{
			render_control.setProgressBarTag("Building caustic photons kd-tree...");
			caustic_map_->updateTree();
			if(logger_.isVerbose()) logger_.logVerbose(getName(), ": Done.");
		}
	}
	else if(logger_.isVerbose()) logger_.logVerbose(getName(), ": No caustic source lights found, skiping caustic map building...");
	return true;
}

Rgb CausticPhotonIntegrator::estimateCausticPhotons(const SurfacePoint &sp, const Vec3f &wo, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search)
{
	if(!caustic_map->ready()) return Rgb{0.f};
	const auto gathered = std::unique_ptr<FoundPhoton[]>(new FoundPhoton[n_caus_search]);//(foundPhoton_t *)alloca(nCausSearch * sizeof(foundPhoton_t));
	float g_radius_square = caustic_radius * caustic_radius;
	const int n_gathered = caustic_map->gather(sp.p_, gathered.get(), n_caus_search, g_radius_square);
	g_radius_square = 1.f / g_radius_square;
	Rgb sum {0.f};
	if(n_gathered > 0)
	{
		for(int i = 0; i < n_gathered; ++i)
		{
			const Photon *photon = gathered[i].photon_;
			const Rgb surf_col = sp.eval(wo, photon->dir_, BsdfFlags::All);
			const float k = sample::kernel(gathered[i].dist_square_, g_radius_square);
			sum += surf_col * k * photon->col_;
		}
		sum *= 1.f / static_cast<float>(caustic_map->nPaths());
	}
	return sum;
}

int CausticPhotonIntegrator::setNumThreadsPhotons(int threads_photons)
{
	int result = threads_photons;
	if(threads_photons == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Active.");
		result = sysinfo::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported for Photon Mapping: [", result, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Inactive.");
	}
	logger_.logParams("Renderer '", getName(), "' using for Photon Mapping [", result, "] Threads.");
	return result;
}

} //namespace yafaray
