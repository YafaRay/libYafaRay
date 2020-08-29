#pragma once

#ifndef YAFARAY_PHOTONINTEGR_H
#define YAFARAY_PHOTONINTEGR_H

#include <yafray_constants.h>

#include <yafraycore/timer.h>
#include <yafraycore/photon.h>
#include <yafraycore/spectrum.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/monitor.h>

#include <core_api/mcintegrator.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <core_api/imagefilm.h>
#include <core_api/camera.h>

#include <utilities/mcqmc.h>
#include <utilities/sample_utils.h>

#include <sstream>
#include <iomanip>

BEGIN_YAFRAY

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

class YAFRAYPLUGIN_EXPORT PhotonIntegrator: public MonteCarloIntegrator
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

END_YAFRAY

#endif // YAFARAY_PHOTONINTEGR_H
