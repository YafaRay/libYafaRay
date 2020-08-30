#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_INTEGRATOR_PHOTON_MAPPING_H
#define YAFARAY_INTEGRATOR_PHOTON_MAPPING_H

#include "constants.h"

#include "common/timer.h"
#include "common/photon.h"
#include "common/spectrum.h"
#include "common/scr_halton.h"
#include "common/monitor.h"

#include "integrator/integrator_montecarlo.h"
#include "common/environment.h"
#include "material/material.h"
#include "background/background.h"
#include "light/light.h"
#include "common/imagefilm.h"
#include "camera/camera.h"

#include "utility/util_mcqmc.h"
#include "utility/util_sample.h"

#include <sstream>
#include <iomanip>

BEGIN_YAFARAY

struct PreGatherData
{
	PreGatherData(PhotonMap *dm): diffuse_map_(dm), fetched_(0) {}
	PhotonMap *diffuse_map_;

	std::vector<RadData> rad_points_;
	std::vector<Photon> radiance_vec_;
	ProgressBar *pbar_;
	volatile int fetched_;
	std::mutex mutx_;
};

class PhotonIntegrator: public MonteCarloIntegrator
{
	public:
		PhotonIntegrator(unsigned int d_photons, unsigned int c_photons, bool transp_shad = false, int shadow_depth = 4, float ds_rad = 0.1f, float c_rad = 0.01f);
		~PhotonIntegrator();
		virtual bool preprocess();
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth = 0) const;
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);
		virtual void preGatherWorker(PreGatherData *gdata, float ds_rad, int n_search);
		virtual void causticWorker(PhotonMap *caustic_map, int thread_id, const Scene *scene, unsigned int n_caus_photons, const Pdf1D *light_power_d, int num_c_lights, const std::string &integrator_name, const std::vector<Light *> &tmplights, int caus_depth, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot, int max_bounces);
		virtual void diffuseWorker(PhotonMap *diffuse_map, int thread_id, const Scene *scene, unsigned int n_diffuse_photons, const Pdf1D *light_power_d, int num_d_lights, const std::string &integrator_name, const std::vector<Light *> &tmplights, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot, int max_bounces, bool final_gather, PreGatherData &pgdat);
		virtual void photonMapKdTreeWorker(PhotonMap *photon_map);

	protected:
		Rgb finalGathering(RenderState &state, const SurfacePoint &sp, const Vec3 &wo, ColorPasses &color_passes) const;

		void enableCaustics(const bool caustics) { use_photon_caustics_ = caustics; }
		void enableDiffuse(const bool diffuse) { use_photon_diffuse_ = diffuse; }

		bool use_photon_diffuse_; //!< enable/disable diffuse photon processing
		bool final_gather_, show_map_;
		bool prepass_;
		unsigned int n_diffuse_photons_;
		int n_diffuse_search_;
		int gather_bounces_;
		float ds_radius_; //!< diffuse search radius
		float lookup_rad_; //!< square radius to lookup radiance photons, as infinity is no such good idea ;)
		float gather_dist_; //!< minimum distance to terminate path tracing (unless gatherBounces is reached)
		friend class PrepassWorkerT;
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_PHOTON_MAPPING_H
