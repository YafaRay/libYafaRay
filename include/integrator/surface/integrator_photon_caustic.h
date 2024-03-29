#pragma once
/****************************************************************************
 *      integrator_photon_caustic.h: An abstract integrator for caustic
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

#ifndef LIBYAFARAY_INTEGRATOR_PHOTON_CAUSTIC_H
#define LIBYAFARAY_INTEGRATOR_PHOTON_CAUSTIC_H

#include "integrator_montecarlo.h"
#include "color/color.h"

namespace yafaray {

class PhotonMap;

class CausticPhotonIntegrator: public MonteCarloIntegrator
{
		using ThisClassType_t = CausticPhotonIntegrator; using ParentClassType_t = MonteCarloIntegrator;

	public:
		inline static std::string getClassName() { return "CausticPhotonIntegrator"; }
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }

	protected:
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(bool, use_photon_caustics_, false, "caustics", "Use photon caustics");
			PARAM_DECL(int, n_caus_photons_, 500000, "caustic_photons", "Number of caustic photons to be shoot but it should be the target");
			PARAM_DECL(int, n_caus_search_, 50, "caustic_mix", "Amount of caustic photons to be gathered in estimation");
			PARAM_DECL(float, caus_radius_, 0.25f, "caustic_radius", "Caustic search radius for estimation");
			PARAM_DECL(int, caus_depth_, 10, "caustic_depth", "Caustic photons max path depth");
			PARAM_DECL(int, threads_photons_, -1, "threads_photons", "Number of threads for photon mapping, -1 = auto detection");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		CausticPhotonIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		~CausticPhotonIntegrator() override;
		void enableCaustics(const bool caustics) { use_photon_caustics_ = caustics; }
		/*! Creates and prepares the caustic photon map */
		bool createCausticMap(RenderMonitor &render_monitor, const RenderControl &render_control);
		void causticWorker(RenderMonitor &render_monitor, unsigned int &total_photons_shot, const RenderControl &render_control, int thread_id, const Pdf1D *light_power_d_caustic, const std::vector<const Light *> &lights_caustic, int pb_step);
		/*! Estimates caustic photons for a given surface point */
		static Rgb estimateCausticPhotons(const SurfacePoint &sp, const Vec3f &wo, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search);
		static Rgb causticPhotons(ColorLayers *color_layers, const Ray &ray, const SurfacePoint &sp, const Vec3f &wo, float clamp_indirect, const PhotonMap *caustic_map, float caustic_radius, int n_caus_search);
		PhotonMap *getCausticMap() { return caustic_map_.get(); }
		const std::unique_ptr<PhotonMap> &getCausticMap() const { return caustic_map_; }

		const int num_threads_photons_{setNumThreadsPhotons(params_.threads_photons_)};
		bool use_photon_caustics_{params_.use_photon_caustics_};
		int n_caus_photons_{params_.n_caus_photons_}; //! Number of caustic photons (to be shoot but it should be the target
		std::unique_ptr<PhotonMap> caustic_map_;

	private:
		[[nodiscard]] int setNumThreadsPhotons(int threads_photons);
};

} //namespace yafaray

#endif
