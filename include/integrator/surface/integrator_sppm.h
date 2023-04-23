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

#ifndef LIBYAFARAY_INTEGRATOR_SPPM_H
#define LIBYAFARAY_INTEGRATOR_SPPM_H

#include "integrator_montecarlo.h"
#include "photon/hashgrid.h"
#include "sampler/halton.h"
#include "render/renderer.h"

namespace yafaray {

class PhotonMap;
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
		using ThisClassType_t = SppmIntegrator; using ParentClassType_t = MonteCarloIntegrator;

	public:
		inline static std::string getClassName() { return "SppmIntegrator"; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		SppmIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::Sppm; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(int , num_photons_, 500000, "photons", "Number of photons to scatter");
			PARAM_DECL(int , num_passes_, 1000, "passNums", "Number of passes");
			PARAM_DECL(int , bounces_, 5, "bounces", "");
			PARAM_DECL(float , times_, 1.f, "times", "");
			PARAM_DECL(float , photon_radius_, 1.f, "photonRadius", "Used to do initial radius estimate");
			PARAM_DECL(int , search_num_, 10, "searchNum", "Now used to do initial radius estimate");
			PARAM_DECL(bool , pm_ire_, false, "pmIRE", "Flag to say if using PM for initial radius estimate");
			PARAM_DECL(int, threads_photons_, -1, "threads_photons", "Number of threads for photon mapping, -1 = auto detection");
		} params_;
		bool render(RenderControl &render_control, RenderMonitor &render_monitor, ImageFilm &image_film, unsigned int object_index_highest, unsigned int material_index_highest) override;
		/*! render a tile; only required by default implementation of render() */
		bool renderTile(ImageFilm &image_film, std::vector<int> &correlative_sample_number, const RenderArea &a, int n_samples, int offset, bool adaptive, int thread_id, int aa_pass_number, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const RenderMonitor &render_monitor, const RenderControl &render_control) override;
		std::pair<Rgb, float> integrate(ImageFilm &image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier) override;
		void prePass(RenderControl &render_control, RenderMonitor &render_monitor, ImageFilm &image_film, int samples, int offset, bool adaptive) override;
		/*! not used now, use traceGatherRay instead*/
		/*! initializing the things that PPM uses such as initial radius */
		void initializePpm(const ImageFilm &image_film);
		/*! based on integrate method to do the gatering trace, need double-check deadly. */
		GatherInfo traceGatherRay(Ray &ray, HitPoint &hp, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, const Camera *camera, bool chromatic_enabled, float aa_light_sample_multiplier, float wavelength, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest);
		void photonWorker(RenderControl &render_control, RenderMonitor &render_monitor, unsigned int &total_photons_shot, int thread_id, int num_d_lights, const Pdf1D *light_power_d, const std::vector<const Light *> &tmplights, int pb_step);
		[[nodiscard]] int setNumThreadsPhotons(int threads_photons);
		PhotonMap *getCausticMap() { return caustic_map_.get(); }
		const PhotonMap *getCausticMap() const { return caustic_map_.get(); }
		PhotonMap *getDiffuseMap() { return diffuse_map_.get(); }
		const PhotonMap *getDiffuseMap() const { return diffuse_map_.get(); }

		const int num_threads_photons_{setNumThreadsPhotons(params_.threads_photons_)};
		HashGrid photon_grid_; //!< the hashgrid for holding photons
		bool pm_ire_{params_.pm_ire_}; // flag to  say if using PM for initial radius estimate
		int n_photons_{params_.num_photons_}; //!< Number of photons to scatter
		float initial_factor_{1.f}; //!< used to time the initial radius
		uint64_t totaln_photons_{0}; //!< amount of total photons that have been emited, used to normalize photon energy
		bool b_hashgrid_{false}; //!< flag to choose using hashgrid or not.
		Halton hal_1_{2, 0}, hal_2_{3, 0}, hal_3_{5, 0}, hal_4_{7, 0}; //!< halton sequence to do
		std::vector<HitPoint> hit_points_; //!< per-pixel refine data
		unsigned int n_refined_; //!< Debug info: Refined pixel per pass
		int n_max_gathered_ = 0; //!< Just for statistical information about max number of gathered photons
		std::unique_ptr<PhotonMap> caustic_map_;
		std::unique_ptr<PhotonMap> diffuse_map_;
		static constexpr inline int n_max_gather_ = 1000; //!< used to gather all the photon in the radius. FIXME seems could get a better way to do that
		std::mutex mutex_;
};

} //namespace yafaray

#endif // SPPM
