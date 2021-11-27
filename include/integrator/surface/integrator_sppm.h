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

BEGIN_YAFARAY

class RandomGenerator;

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
		static std::unique_ptr<Integrator> factory(Logger &logger, ParamMap &params, const Scene &scene, RenderControl &render_control);

	private:
		SppmIntegrator(RenderControl &render_control, Logger &logger, unsigned int d_photons, int passnum, bool transp_shad, int shadow_depth);
		virtual std::string getShortName() const override { return "SPPM"; }
		virtual std::string getName() const override { return "SPPM"; }
		virtual bool render() override;
		/*! render a tile; only required by default implementation of render() */
		virtual bool renderTile(const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number) override;
		virtual std::pair<Rgb, float> integrate(Ray &ray, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const override;
		virtual bool preprocess(ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override; //not used for now
		// not used now
		virtual void prePass(int samples, int offset, bool adaptive) override;
		/*! not used now, use traceGatherRay instead*/
		/*! initializing the things that PPM uses such as initial radius */
		void initializePpm();
		/*! based on integrate method to do the gatering trace, need double-check deadly. */
		GatherInfo traceGatherRay(Ray &ray, HitPoint &hp, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data);
		void photonWorker(unsigned int &total_photons_shot, int thread_id, int num_d_lights, const Pdf1D *light_power_d, const std::vector<const Light *> &tmplights, int pb_step);

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
		int n_max_gathered_ = 0; //Just for statistical information about max number of gathered photons
		std::unique_ptr<PhotonMap> diffuse_map_;
		static constexpr int n_max_gather_ = 1000; //used to gather all the photon in the radius. seems could get a better way to do that
		std::mutex mutex_;
};

END_YAFARAY

#endif // SPPM
