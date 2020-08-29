#pragma once

#ifndef YAFARAY_SPPM_H
#define YAFARAY_SPPM_H

#include <yafray_constants.h>
#include <yafraycore/timer.h>

#include <core_api/mcintegrator.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>

#include <sstream>

#include <core_api/imagefilm.h>
#include <core_api/camera.h>
#include <yafraycore/photon.h>
#include <yafraycore/monitor.h>
#include <yafraycore/spectrum.h>
#include <utilities/sample_utils.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/hashgrid.h>
#include <stdint.h>
#include <cmath>
#include <algorithm>

BEGIN_YAFRAY

typedef struct HitPoint // actually are per-pixel variable, use to record the sppm's shared statistics
{
	float radius_2_; // square search-radius, shrink during the passes
	int64_t acc_photon_count_; // record the total photon this pixel gathered
	Rgba acc_photon_flux_; // accumulated flux
	Rgba constant_randiance_; // record the direct light for this pixel

	bool radius_setted_; // used by IRE to direct whether the initial radius is set or not.
} HitPoint_t;

//used for gather ray to collect photon information
typedef struct GatherInfo
{
	int64_t photon_count_;  // the number of photons that the gather ray collected
	Rgba photon_flux_;   // the unnormalized flux of photons that the gather ray collected
	Rgba constant_randiance_; // the radiance from when the gather ray hit the lightsource

	GatherInfo(): photon_count_(0), photon_flux_(0.f), constant_randiance_(0.f) {}

	GatherInfo &operator +=(const GatherInfo &g)
	{
		photon_count_ += g.photon_count_;
		photon_flux_ += g.photon_flux_;
		constant_randiance_ += g.constant_randiance_;
		return (*this);
	}
} GatherInfo_t;


class YAFRAYPLUGIN_EXPORT SppmIntegrator: public MonteCarloIntegrator
{
	public:
		SppmIntegrator(unsigned int d_photons, int passnum, bool transp_shad, int shadow_depth);
		~SppmIntegrator();
		virtual bool render(int num_view, ImageFilm *image_film);
		/*! render a tile; only required by default implementation of render() */
		virtual bool renderTile(int num_view, RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number = 0);
		virtual bool preprocess(); //not used for now
		// not used now
		virtual void prePass(int samples, int offset, bool adaptive);
		/*! not used now, use traceGatherRay instead*/
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth = 0) const;
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);
		/*! initializing the things that PPM uses such as initial radius */
		void initializePpm();
		/*! based on integrate method to do the gatering trace, need double-check deadly. */
		GatherInfo_t traceGatherRay(RenderState &state, DiffRay &ray, HitPoint_t &hp, ColorPasses &color_passes);
		void photonWorker(PhotonMap *diffuse_map, PhotonMap *caustic_map, int thread_id, const Scene *scene, unsigned int n_photons, const Pdf1D *light_power_d, int num_d_lights, const std::string &integrator_name, const std::vector<Light *> &tmplights, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot, int max_bounces, Random &prng);

	protected:
		HashGrid  photon_grid_; // the hashgrid for holding photons
		PhotonMap diffuse_map_, caustic_map_; // photonmap
		unsigned int n_photons_; //photon number to scatter
		float ds_radius_; // used to do initial radius estimate
		int n_search_;// now used to do initial radius estimate
		int pass_num_; // the progressive pass number
		float initial_factor_; // used to time the initial radius
		uint64_t totaln_photons_; // amount of total photons that have been emited, used to normalize photon energy
		bool pm_ire_; // flag to  say if using PM for initial radius estimate
		bool b_hashgrid_; // flag to choose using hashgrid or not.

		Halton hal_1_, hal_2_, hal_3_, hal_4_, hal_7_, hal_8_, hal_9_, hal_10_; // halton sequence to do

		std::vector<HitPoint_t>hit_points_; // per-pixel refine data

		unsigned int n_refined_; // Debug info: Refined pixel per pass
};

END_YAFRAY

#endif // SPPM
