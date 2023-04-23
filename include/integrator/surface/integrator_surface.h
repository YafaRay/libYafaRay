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
#include "common/aa_noise_params.h"
#include "common/mask_edge_toon_params.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include "render/renderer.h"
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

class SurfaceIntegrator
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "SurfaceIntegrator"; }
		[[nodiscard]] virtual Type type() const = 0;
		[[nodiscard]] std::string getName() const { return name_; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		virtual ~SurfaceIntegrator();
		/*! do whatever is required to render the image, if suitable for integrating whole image */
		virtual bool render(RenderControl &render_control, ImageFilm &image_film, unsigned int object_index_highest, unsigned int material_index_highest) = 0;
		virtual std::pair<Rgb, float> integrate(ImageFilm &image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier) = 0; 	//!< chromatic_enabled indicates wether the full spectrum is calculated (true) or only a single wavelength (false). wavelength is the (normalized) wavelength being used when chromatic is false. The range is defined going from 400nm (0.0) to 700nm (1.0), although the widest range humans can perceive is ofteb given 380-780nm.
		/*! gets called before the scene rendering (i.e. before first call to integrate)
			\return false when preprocessing could not be done properly, true otherwise */
		virtual bool preprocess(RenderControl &render_control, const Scene &scene);
		/*! allow the integrator to do some cleanup when an image is done
		(possibly also important for multiframe rendering in the future)	*/
		virtual void cleanup(ImageFilm &image_film) const { /*FIXME render_info_.clear(); aa_noise_info_.clear(); */ }
		std::vector<const Light *> getLights() const { return lights_visible_; }
		const Light *getLight(size_t index) const { return lights_visible_[index]; }
		std::vector<const Light *> getLightsEmittingCausticPhotons() const;
		std::vector<const Light *> getLightsEmittingDiffusePhotons() const;
		size_t numLights() const { return lights_visible_.size(); }
		ParamResult defineVolumeIntegrator(const Scene &scene, const ParamMap &param_map);
		float getShadowBias() const { return params_.shadow_bias_; }
		const EdgeToonParams *getEdgeToonParams() const { return &edge_toon_params_; }
		const AaNoiseParams *getAaParameters() const { return &aa_noise_params_; }

	private:
		std::string name_{getClassName()}; //Keep at the beginning of the list of members to ensure it is constructed before other methods called at construction

	protected:
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
			PARAM_DECL(int, aa_passes_, 1, "AA_passes", "");
			PARAM_DECL(int, aa_samples_, 1, "AA_minsamples", "Sample count for first pass");
			PARAM_DECL(int, aa_inc_samples_, 1, "AA_inc_samples", "Sample count for additional passes");
			PARAM_DECL(float , aa_threshold_, 0.05f, "AA_threshold", "");
			PARAM_DECL(float , aa_resampled_floor_, 0.f, "AA_resampled_floor", "Minimum amount of resampled pixels (% of the total pixels) below which we will automatically decrease the threshold value for the next pass");
			PARAM_DECL(float , aa_sample_multiplier_factor_, 1.f, "AA_sample_multiplier_factor", "");
			PARAM_DECL(float , aa_light_sample_multiplier_factor_, 1.f, "AA_light_sample_multiplier_factor", "");
			PARAM_DECL(float , aa_indirect_sample_multiplier_factor_, 1.f, "AA_indirect_sample_multiplier_factor", "");
			PARAM_DECL(bool , aa_detect_color_noise_, false, "AA_detect_color_noise", "");
			PARAM_ENUM_DECL(AaNoiseParams::DarkDetectionType, aa_dark_detection_type_, AaNoiseParams::DarkDetectionType::None, "AA_dark_detection_type", "");
			PARAM_DECL(float , aa_dark_threshold_factor_, 0.f, "AA_dark_threshold_factor", "");
			PARAM_DECL(int, aa_variance_edge_size_, 10, "AA_variance_edge_size", "");
			PARAM_DECL(int, aa_variance_pixels_, 0, "AA_variance_pixels", "");
			PARAM_DECL(float , aa_clamp_samples_, 0.f, "AA_clamp_samples", "");
			PARAM_DECL(float , aa_clamp_indirect_, 0.f, "AA_clamp_indirect", "");
			PARAM_DECL(int , layer_mask_obj_index_, 0, "layer_mask_obj_index", "Object Index used for masking in/out in the Mask Render Layers");
			PARAM_DECL(int , layer_mask_mat_index_, 0, "layer_mask_mat_index", "Material Index used for masking in/out in the Mask Render Layers");
			PARAM_DECL(bool , layer_mask_invert, false, "layer_mask_invert", "False=mask in, True=mask out");
			PARAM_DECL(bool , layer_mask_only_, false, "layer_mask_only", "False=rendered image is masked, True=only the mask is shown without rendered image");
			PARAM_DECL(Rgb , layer_toon_edge_color_, Rgb{0.f}, "layer_toon_edge_color", "Color of the edges used in the Toon Render Layers");
			PARAM_DECL(int , layer_object_edge_thickness_, 2, "layer_object_edge_thickness", "Thickness of the edges used in the Object Edge and Toon Render Layers");
			PARAM_DECL(float , layer_object_edge_threshold_, 0.3f, "layer_object_edge_threshold", "Threshold for the edge detection process used in the Object Edge and Toon Render Layers");
			PARAM_DECL(float , layer_object_edge_smoothness_, 0.75f, "layer_object_edge_smoothness", "Smoothness (blur) of the edges used in the Object Edge and Toon Render Layers");
			PARAM_DECL(float , layer_toon_pre_smooth_, 3.f, "layer_toon_pre_smooth", "Toon effect: smoothness applied to the original image");
			PARAM_DECL(float , layer_toon_quantization_, 0.1f, "layer_toon_quantization", "Toon effect: color Quantization applied to the original image");
			PARAM_DECL(float , layer_toon_post_smooth_, 3.f, "layer_toon_post_smooth", "Toon effect: smoothness applied after Quantization");
			PARAM_DECL(int , layer_faces_edge_thickness_, 1, "layer_faces_edge_thickness", "Thickness of the edges used in the Faces Edge Render Layers");
			PARAM_DECL(float , layer_faces_edge_threshold_, 0.01f, "layer_faces_edge_threshold", "Threshold for the edge detection process used in the Faces Edge Render Layers");
			PARAM_DECL(float , layer_faces_edge_smoothness_, 0.5f, "layer_faces_edge_smoothness", "Smoothness (blur) of the edges used in the Faces Edge Render Layers");
		} params_;
		SurfaceIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);

		const int num_threads_{setNumThreads(params_.nthreads_)};
		const float shadow_bias_{params_.shadow_bias_auto_ ? Accelerator::shadowBias() : params_.shadow_bias_};
		const float ray_min_dist_{params_.ray_min_dist_auto_ ? Accelerator::minRayDist() : params_.ray_min_dist_};
		bool ray_differentials_enabled_ = false;  //!< By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials. This should avoid the (many) extra calculations when they are not necessary.
		Bound<float> scene_bound_{};
		std::unique_ptr<VolumeIntegrator> vol_integrator_;
		const Background *background_ = nullptr;
		const Accelerator *accelerator_ = nullptr;

		AaNoiseParams aa_noise_params_{
				params_.aa_samples_,
				params_.aa_passes_,
				params_.aa_inc_samples_,
				params_.aa_threshold_,
				params_.aa_resampled_floor_,
				params_.aa_sample_multiplier_factor_,
				params_.aa_light_sample_multiplier_factor_,
				params_.aa_indirect_sample_multiplier_factor_,
				params_.aa_detect_color_noise_,
				params_.aa_dark_detection_type_,
				params_.aa_dark_threshold_factor_,
				params_.aa_variance_edge_size_,
				params_.aa_variance_pixels_,
				params_.aa_clamp_samples_,
				params_.aa_clamp_indirect_,
		};
		const MaskParams mask_params_{
				params_.layer_mask_obj_index_,
				params_.layer_mask_mat_index_,
				params_.layer_mask_invert,
				params_.layer_mask_only_,
		};
		const EdgeToonParams edge_toon_params_{
				params_.layer_object_edge_thickness_,
				params_.layer_object_edge_threshold_,
				params_.layer_object_edge_smoothness_,
				params_.layer_toon_edge_color_,
				params_.layer_toon_pre_smooth_,
				params_.layer_toon_quantization_,
				params_.layer_toon_post_smooth_,
				params_.layer_faces_edge_thickness_,
				params_.layer_faces_edge_threshold_,
				params_.layer_faces_edge_smoothness_,
		};
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
