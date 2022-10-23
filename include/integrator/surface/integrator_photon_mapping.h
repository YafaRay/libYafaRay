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

#ifndef LIBYAFARAY_INTEGRATOR_PHOTON_MAPPING_H
#define LIBYAFARAY_INTEGRATOR_PHOTON_MAPPING_H

#include "integrator_photon_caustic.h"
#include "photon/photon.h"
#include <vector>

namespace yafaray {

struct PreGatherData final
{
	explicit PreGatherData(RenderControl &render_control, PhotonMap *dm): render_control_{render_control}, diffuse_map_(dm), fetched_(0) {}
	RenderControl &render_control_;
	PhotonMap *diffuse_map_;
	std::vector<RadData> rad_points_;
	std::vector<Photon> radiance_vec_;
	volatile int fetched_;
	std::mutex mutx_;
};

class PhotonIntegrator final : public CausticPhotonIntegrator
{
		using ThisClassType_t = PhotonIntegrator; using ParentClassType_t = CausticPhotonIntegrator;

	public:
		inline static std::string getClassName() { return "PhotonIntegrator"; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, RenderControl &render_control, const ParamMap &params, const Scene &scene);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		PhotonIntegrator(RenderControl &render_control, Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::Photon; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(bool, diffuse_, true, "diffuse", "Enable/disable diffuse photon processing");
			PARAM_DECL(int , photons_diffuse_, 100000, "diffuse_photons", "Number of diffuse photons");
			PARAM_DECL(float, diffuse_radius_, 0.1f, "diffuse_radius", "Diffuse photons search radius");
			PARAM_DECL(int, num_photons_diffuse_search_, 50, "diffuse_search", "Num photons used for diffuse search");//FIXME DAVID: if caustic_mix not specified, the old code stated: "caustic_mix = diffuse_search;" by default However now that's not implemented because caustic_mix belongs to the parent class parameters. Is that really necessary??
			PARAM_DECL(bool, final_gather_, true, "finalGather", "Enable final gathering for diffuse photons");
			PARAM_DECL(int, fg_samples_, 32, "fg_samples", "Number of samples for Montecarlo raytracing");
			PARAM_DECL(int, bounces_, 3, "bounces", "Max. path depth for Montecarlo raytracing");
			PARAM_DECL(int, fg_bounces_, 2, "fg_bounces", "");
			PARAM_DECL(float, gather_dist_, 0.2f, "fg_min_pathlen", "Minimum distance to terminate path tracing (unless gatherBounces is reached). If not specified it defaults to the value set in '" + diffuse_radius_meta_.name() + "'");
			PARAM_DECL(bool, show_map_, false, "show_map", "Show radiance map");
		} params_;
		[[nodiscard]] std::string getName() const override { return "PhotonMap"; }
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;
		std::pair<Rgb, float> integrate(Ray &ray, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest) const override;
		void diffuseWorker(FastRandom &fast_random, PreGatherData &pgdat, unsigned int &total_photons_shot, int thread_id, const Pdf1D *light_power_d, const std::vector<const Light *> &lights_diffuse, int pb_step);
		Rgb finalGathering(FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, int thread_id, bool chromatic_enabled, float wavelength, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const;
		void enableDiffuse(const bool diffuse) { use_photon_diffuse_ = diffuse; }
		static void preGatherWorker(PreGatherData *gdata, float ds_rad, int n_search);
		static void photonMapKdTreeWorker(PhotonMap *photon_map);

		bool use_photon_diffuse_{params_.diffuse_}; //!< enable/disable diffuse photon processing
		int photons_diffuse_{params_.photons_diffuse_}; //!< Number of diffuse photons
		const float lookup_rad_{4.f * params_.diffuse_radius_ * params_.diffuse_radius_}; //!< square radius to lookup radiance photons, as infinity is no such good idea ;)
		std::unique_ptr<PhotonMap> diffuse_map_, radiance_map_;
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_PHOTON_MAPPING_H
