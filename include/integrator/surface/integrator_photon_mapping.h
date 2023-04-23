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

class PreGatherData final
{
	public:
		explicit PreGatherData(PhotonMap *dm) : diffuse_map_{dm} { }
		PhotonMap *getDiffuseMap() { return diffuse_map_; }
		const PhotonMap *getDiffuseMap() const { return diffuse_map_; }
		void lock() { mutx_.lock(); }
		void unlock() { mutx_.unlock(); }

		std::vector<RadData> rad_points_;
		std::vector<Photon> radiance_vec_;
		int fetched_{0};

	private:
		PhotonMap *diffuse_map_;
		std::mutex mutx_;
};

class PhotonIntegrator final : public CausticPhotonIntegrator
{
		using ThisClassType_t = PhotonIntegrator; using ParentClassType_t = CausticPhotonIntegrator;

	public:
		inline static std::string getClassName() { return "PhotonIntegrator"; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		PhotonIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::Photon; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
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
		bool preprocess(RenderControl &render_control, RenderMonitor &render_monitor, const Scene &scene) override;
		std::pair<Rgb, float> integrate(ImageFilm &image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier) override;
		void diffuseWorker(RenderControl &render_control, RenderMonitor &render_monitor, PreGatherData &pgdat, unsigned int &total_photons_shot, int thread_id, const Pdf1D *light_power_d, const std::vector<const Light *> &lights_diffuse, int pb_step);
		Rgb finalGathering(RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, unsigned int base_sampling_offset, int thread_id, const Camera *camera, bool chromatic_enabled, float wavelength, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier, const SurfacePoint &sp, const Vec3f &wo, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data);
		void enableDiffuse(const bool diffuse) { use_photon_diffuse_ = diffuse; }
		static void preGatherWorker(RenderControl &render_control, RenderMonitor &render_monitor, PreGatherData *gdata, float ds_rad, int n_search);
		static void photonMapKdTreeWorker(PhotonMap *photon_map, const RenderMonitor &render_monitor, const RenderControl &render_control);
		PhotonMap *getDiffuseMap() { return diffuse_map_.get(); }
		const PhotonMap *getDiffuseMap() const { return diffuse_map_.get(); }
		std::unique_ptr<PhotonMap> &getRadianceMap() { return radiance_map_; }
		const std::unique_ptr<PhotonMap> &getRadianceMap() const { return radiance_map_; }

		bool use_photon_diffuse_{params_.diffuse_}; //!< enable/disable diffuse photon processing
		int photons_diffuse_{params_.photons_diffuse_}; //!< Number of diffuse photons
		const float lookup_rad_{4.f * params_.diffuse_radius_ * params_.diffuse_radius_}; //!< square radius to lookup radiance photons, as infinity is no such good idea ;)
		std::unique_ptr<PhotonMap> diffuse_map_, radiance_map_;
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_PHOTON_MAPPING_H
