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

#ifndef YAFARAY_INTEGRATOR_SPPM_H
#define YAFARAY_INTEGRATOR_SPPM_H

#include "integrator_montecarlo.h"
#include "photon/hashgrid.h"
#include "sampler/halton.h"
#include "photon/photon.h"

BEGIN_YAFARAY

class Random;

struct HitPoint final // actually are per-pixel variable, use to record the sppm's shared statistics
{
	float radius_2_; // square search-radius, shrink during the passes
	int64_t acc_photon_count_; // record the total photon this pixel gathered
	Rgba acc_photon_flux_; // accumulated flux
	Rgba constant_randiance_; // record the direct light for this pixel

	bool radius_setted_; // used by IRE to direct whether the initial radius is set or not.
};

//used for gather ray to collect photon information
struct GatherInfo final
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
};


class SppmIntegrator final : public MonteCarloIntegrator
{
	public:
		static std::unique_ptr<Integrator> factory(Logger &logger, ParamMap &params, const Scene &scene);

	private:
		SppmIntegrator(Logger &logger, unsigned int d_photons, int passnum, bool transp_shad, int shadow_depth);
		virtual std::string getShortName() const override { return "SPPM"; }
		virtual std::string getName() const override { return "SPPM"; }
		virtual bool render(RenderControl &render_control, const RenderView *render_view) override;
		/*! render a tile; only required by default implementation of render() */
		virtual bool renderTile(RenderArea &a, const RenderView *render_view, const RenderControl &render_control, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number = 0) override;
		virtual Rgba integrate(RenderData &render_data, const DiffRay &ray, int additional_depth, ColorLayers *color_layers, const RenderView *render_view) const override;
		virtual bool preprocess(const RenderControl &render_control, const RenderView *render_view, ImageFilm *image_film) override; //not used for now
		// not used now
		virtual void prePass(int samples, int offset, bool adaptive, const RenderControl &render_control, const RenderView *render_view) override;
		/*! not used now, use traceGatherRay instead*/
		/*! initializing the things that PPM uses such as initial radius */
		void initializePpm(const RenderView *render_view);
		/*! based on integrate method to do the gatering trace, need double-check deadly. */
		GatherInfo traceGatherRay(RenderData &render_data, DiffRay &ray, HitPoint &hp, ColorLayers *color_layers = nullptr);
		void photonWorker(PhotonMap *diffuse_map, PhotonMap *caustic_map, int thread_id, const Scene *scene, const RenderView *render_view, const RenderControl &render_control, unsigned int n_photons, const Pdf1D *light_power_d, int num_d_lights, const std::vector<const Light *> &tmplights, ProgressBar *pb, int pb_step, unsigned int &total_photons_shot, int max_bounces, Random &prng);

		HashGrid  photon_grid_; // the hashgrid for holding photons
		unsigned int n_photons_; //photon number to scatter
		float ds_radius_; // used to do initial radius estimate
		int n_search_;// now used to do initial radius estimate
		int pass_num_; // the progressive pass number
		float initial_factor_; // used to time the initial radius
		uint64_t totaln_photons_; // amount of total photons that have been emited, used to normalize photon energy
		bool pm_ire_; // flag to  say if using PM for initial radius estimate
		bool b_hashgrid_; // flag to choose using hashgrid or not.
		Halton hal_1_{2, 0}, hal_2_{3, 0}, hal_3_{5, 0}, hal_4_{7, 0}; // halton sequence to do
		std::vector<HitPoint>hit_points_; // per-pixel refine data
		unsigned int n_refined_; // Debug info: Refined pixel per pass
		std::unique_ptr<PhotonMap> diffuse_map_;
		static constexpr int n_max_gather_ = 1000; //used to gather all the photon in the radius. seems could get a better way to do that
		std::mutex mutex_;
};

END_YAFARAY

#endif // SPPM
