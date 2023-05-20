#pragma once
/****************************************************************************
 *      integrator_surface.h: the interface definition for light surface integrators
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
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

#ifndef LIBYAFARAY_INTEGRATOR_SURFACE_H
#define LIBYAFARAY_INTEGRATOR_SURFACE_H

#include "geometry/bound.h"
#include "color/color.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include "accelerator/accelerator.h"
#include <vector>

namespace yafaray {

class RenderControl;
class RandomGenerator;
class Ray;
class ParamMap;
class ColorLayers;
class VolumeIntegrator;
class Camera;
class Background;
class Timer;
class Layers;
struct PixelSamplingData;
struct RayDivision;
class Scene;
class ImageFilm;
class Accelerator;
class FastRandom;
class Light;
struct EdgeToonParams;
struct AaNoiseParams;
struct MaskParams;

class SurfaceIntegrator
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "SurfaceIntegrator"; }
		[[nodiscard]] virtual Type type() const = 0;
		[[nodiscard]] std::string getName() const { return name_; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map);
		virtual ~SurfaceIntegrator();
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		[[nodiscard]] virtual std::map<std::string, const ParamMeta *> getParamMetaMap() const = 0;
		[[nodiscard]] static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const;
		/*! do whatever is required to render the image, if suitable for integrating whole image */
		bool render(RenderControl &render_control, RenderMonitor &render_monitor, ImageFilm *image_film);
		virtual std::pair<Rgb, float> integrate(Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) = 0; 	//!< chromatic_enabled indicates wether the full spectrum is calculated (true) or only a single wavelength (false). wavelength is the (normalized) wavelength being used when chromatic is false. The range is defined going from 400nm (0.0) to 700nm (1.0), although the widest range humans can perceive is ofteb given 380-780nm.
		/*! gets called before the scene rendering (i.e. before first call to integrate)
			\return false when preprocessing could not be done properly, true otherwise */
		virtual bool preprocess(RenderMonitor &render_monitor, const RenderControl &render_control, const Scene &scene);
		std::vector<const Light *> getLights() const { return lights_visible_; }
		const Light *getLight(size_t index) const { return lights_visible_[index]; }
		std::vector<const Light *> getLightsEmittingCausticPhotons() const;
		std::vector<const Light *> getLightsEmittingDiffusePhotons() const;
		size_t numLights() const { return lights_visible_.size(); }
		ParamResult defineVolumeIntegrator(const Scene &scene, const ParamMap &param_map);
		float getShadowBias() const { return params_.shadow_bias_; }

	private:
		std::string name_{getClassName()}; //Keep at the beginning of the list of members to ensure it is constructed before other methods called at construction

	protected:
		virtual bool render(RenderControl &render_control, RenderMonitor &render_monitor) = 0;
		Logger &logger_; //Keep at the beginning of the list of members to ensure it is constructed before other methods called at construction
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Bidirectional, Debug, DirectLight, Path, Photon, Sppm };
			inline static const EnumMap<ValueType_t> map_{{
					{"bidirectional", Bidirectional, ""},
					{"DebugIntegrator", Debug, ""},
					{"directlighting", DirectLight, ""},
					{"pathtracing", Path, ""},
					{"photonmapping", Photon, ""},
					{"SPPM", Sppm, ""},
				}};
		};
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(std::string, light_names_, "", "light_names", "Selection of the scene lights to be used in the integration, separated by a semicolon. If empty, all lights will be included");
			PARAM_DECL(bool, time_forced_, false, "time_forced", "");
			PARAM_DECL(float, time_forced_value_, 0.f, "time_forced_value", "");
			PARAM_DECL(int, nthreads_, -1, "threads", "Number of threads, -1 = auto detection");
			PARAM_DECL(bool, shadow_bias_auto_, true, "shadow_bias_auto", "Enable automatic shadow bias calculation");
			PARAM_DECL(float, shadow_bias_, Accelerator::shadowBias(), "shadow_bias", "Shadow bias to apply to shadows to avoid self-shadow artifacts. It gets overriden when automatic shadow bias is enabled.");
			PARAM_DECL(bool, ray_min_dist_auto_, true, "ray_min_dist_auto", "Enable automatic ray minimum distance calculation");
			PARAM_DECL(float, ray_min_dist_, Accelerator::minRayDist(), "ray_min_dist", "Ray minimum distance. It gets overriden when automatic ray min distance is enabled.");
		} params_;
		SurfaceIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);

		const int num_threads_{setNumThreads(params_.nthreads_)};
		const float shadow_bias_{params_.shadow_bias_auto_ ? Accelerator::shadowBias() : params_.shadow_bias_};
		const float ray_min_dist_{params_.ray_min_dist_auto_ ? Accelerator::minRayDist() : params_.ray_min_dist_};
		bool ray_differentials_enabled_ = false;  //!< By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials. This should avoid the (many) extra calculations when they are not necessary.
		Bound<float> scene_bound_{};
		unsigned int object_index_highest_{0};
		unsigned int material_index_highest_{0};
		std::unique_ptr<VolumeIntegrator> vol_integrator_;
		ImageFilm *image_film_{nullptr};
		const Background *background_{nullptr};
		const Accelerator *accelerator_{nullptr};
		const AaNoiseParams *aa_noise_params_{nullptr};
		FastRandom fast_random_;

	private:
		std::map<std::string, const Light *> getFilteredLights(const Scene &scene, const std::string &light_filter_string) const;
		std::vector<const Light *> getLightsVisible() const;
		[[nodiscard]] int setNumThreads(int threads_photons);

		std::map<std::string, const Light *> lights_map_filtered_;
		std::vector<const Light *> lights_visible_;
};

struct ColorLayerAccum
{
	explicit ColorLayerAccum(Rgba *color) : color_(color) { }
	Rgba *color_;
	Rgba accum_{0.f};
};

} //namespace yafaray

#endif //LIBYAFARAY_INTEGRATOR_SURFACE_H
