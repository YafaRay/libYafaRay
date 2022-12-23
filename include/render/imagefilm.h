#pragma once
/****************************************************************************
 *
 *      imagefilm.h: image data handling class
 *      This is part of the libYafaRay package
 *      See AUTHORS for more information
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
 *
 */

#ifndef LIBYAFARAY_IMAGEFILM_H
#define LIBYAFARAY_IMAGEFILM_H

#include "public_api/yafaray_c_api.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include "imagesplitter.h"
#include "common/aa_noise_params.h"
#include "common/layers.h"
#include "image/image_pixel_types.h"
#include "math/buffer_2d.h"
#include "image/image_layers.h"
#include "render/render_callbacks.h"
#include "common/timer.h"
#include "geometry/rect.h"
#include "renderer.h"
#include <mutex>
#include <atomic>
#include <utility>

namespace yafaray {

class Scene;
class ImageOutput;
class ColorLayers;
class RenderControl;
class Timer;
class RenderView;
struct EdgeToonParams;
template <typename T> class Items;

/*!	This class recieves all rendered image samples.
	You can see it as an enhanced render buffer;
	Holds RGBA and Density (for actual bidirectional pathtracing implementation) buffers.
*/
class ImageFilm final
{
	public:
		enum Flags : unsigned char { RegularImage = 1 << 0, Densityimage = 1 << 1, All = RegularImage | Densityimage };
		inline static std::string getClassName() { return "ImageFilm"; }
		static std::pair<ImageFilm *, ParamResult> factory(Logger &logger, RenderControl &render_control, const ParamMap &param_map);
		ImageFilm(Logger &logger, ParamResult &param_result, RenderControl &render_control, const ParamMap &param_map);
		~ImageFilm();
		std::string name() const { return name_; }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		/*! Initialize imageFilm for new rendering, i.e. set pixels black etc */
		void init(RenderControl &render_control, const RenderView *render_view);
		/*! Prepare for next pass, i.e. reset area_cnt, check if pixels need resample...
			\param adaptive_aa if true, flag pixels to be resampled
			\param threshold color threshold for adaptive antialiasing */
		int nextPass(const RenderView *render_view, RenderControl &render_control, bool adaptive_aa, const std::string &integrator_name, const EdgeToonParams &edge_params, bool skip_nrender_layer = false);
		/*! Return the next area to be rendered
			CAUTION! This method MUST be threadsafe!
			\return false if no area is left to be handed out, true otherwise */
		bool nextArea(const RenderView *render_view, const RenderControl &render_control, RenderArea &a);
		/*! Indicate that all pixels inside the area have been sampled for this pass */
		void finishArea(const RenderView *render_view, RenderControl &render_control, const RenderArea &a, const EdgeToonParams &edge_params);
		/*! Output all pixels to the color output */
		void flush(const RenderView *render_view, RenderControl &render_control, Flags flags = All);
		/*! query if sample (x,y) was flagged to need more samples.
			IMPORTANT! You may only call this after you have called nextPass(true, ...), otherwise
			no such flags have been created !! */
		bool doMoreSamples(const Point2i &point) const;
		/*!	Add image sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addSample(const Point2i &point, float dx, float dy, const RenderArea *a = nullptr, int num_sample = 0, int aa_pass_number = 0, float inv_aa_max_possible_samples = 0.1f, const ColorLayers *color_layers = nullptr);
		/*!	Add light density sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addDensitySample(const Rgb &c, const Point2i &point, float dx, float dy);
		void setDensityEstimation(bool enable);
		void setNumDensitySamples(int n) { num_density_samples_ = n; }
		int getTotalPixels() const { return params_.width_ * params_.height_; };
		int getWidth() const { return params_.width_; }
		int getHeight() const { return params_.height_; }
		int getCx0() const { return params_.start_x_; }
		int getCy0() const { return params_.start_y_; }
		Size2i getSize() const { return {{params_.width_, params_.height_}}; }
		int getTileSize() const { return params_.tile_size_; }
		float getWeight(const Point2i &point) const { return weights_(point).getFloat(); }
		bool getBackgroundResampling() const { return params_.background_resampling_; }
		unsigned int getBaseSamplingOffset() const { return params_.base_sampling_offset_ + params_.computer_node_ * 100000; } //We give to each computer node a "reserved space" of 100,000 samples
		unsigned int getSamplingOffset() const { return sampling_offset_; }
		void setSamplingOffset(unsigned int offset) { sampling_offset_ = offset; }
		std::string getFilmSavePath() const { return film_load_save_.path_; }
		void resetImagesAutoSaveTimer() { images_auto_save_params_.timer_ = 0.0; }
		void resetFilmAutoSaveTimer() { film_load_save_.auto_save_.timer_ = 0.0; }
		const ImageLayers *getImageLayers() const { return &film_image_layers_; }
		const ImageLayers *getExportedImageLayers() const { return &exported_image_layers_; }
		const Layers * getLayers() const { return &layers_; }
		Timer * getTimer() { return &timer_; }
		void defineLayer(const ParamMap &param_map);
		void defineLayer(std::string &&layer_type_name, std::string &&image_type_name, std::string &&exported_image_type_name, std::string &&exported_image_name);
		void defineLayer(LayerDef::Type layer_type, Image::Type image_type = Image::Type{Image::Type::None}, Image::Type exported_image_type = Image::Type{Image::Type::None}, const std::string &exported_image_name = "");
		const Items<ImageOutput> &getOutputs() const { return outputs_; }
		MaskParams getMaskParams() const { return mask_params_; }
		EdgeToonParams getEdgeToonParams() const { return edge_toon_params_; }
		AaNoiseParams getAaParameters() const { return aa_noise_params_; }
		std::pair<size_t, ParamResult> createOutput(const std::string &name, const ParamMap &param_map);
		std::pair<size_t, ResultFlags> getOutput(const std::string &name) const;
		bool disableOutput(const std::string &name);
		void clearOutputs();

		void setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback callback, void *callback_data);
		void setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback callback, void *callback_data);
		void setRenderPutPixelCallback(yafaray_RenderPutPixelCallback callback, void *callback_data);
		void setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback callback, void *callback_data);
		void setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback callback, void *callback_data);
		void setRenderFlushCallback(yafaray_RenderFlushCallback callback, void *callback_data);
		void setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback callback, void *callback_data);

	private:
		struct FilterType : public Enum<FilterType>
		{
			enum : ValueType_t { Box, Mitchell, Gauss, Lanczos };
			inline static const EnumMap<ValueType_t> map_{{
					{"box", Box, ""},
					{"mitchell", Mitchell, ""},
					{"gauss", Gauss, ""},
					{"lanczos", Lanczos, ""},
				}};
		};
		struct AutoSaveParams
		{
			struct IntervalType : public Enum<IntervalType>
			{
				enum : ValueType_t { None, Time, Pass };
				inline static const EnumMap<ValueType_t> map_{{
						{"none", None, ""},
						{"time-interval", Time, ""},
						{"pass-interval", Pass, ""},
					}};
			};
			AutoSaveParams(float interval_seconds, int interval_passes, IntervalType interval_type) : interval_seconds_{interval_seconds}, interval_passes_{interval_passes}, interval_type_{interval_type} { }
			double interval_seconds_ = 300.0;
			int interval_passes_ = 1;
			double timer_ = 0.0; //Internal timer for AutoSave
			int pass_counter_ = 0; //Internal counter for AutoSave
			IntervalType interval_type_{IntervalType::None};
		};
		struct FilmLoadSave
		{
			struct Mode : public Enum<Mode>
			{
				enum : ValueType_t { None, Save, LoadAndSave };
				inline static const EnumMap<ValueType_t> map_{{
						{"none", None, ""},
						{"save", Save, ""},
						{"load-save", LoadAndSave, ""},
					}};
			};
			FilmLoadSave(std::string path, AutoSaveParams auto_save_params, Mode mode) : path_{std::move(path)}, auto_save_{auto_save_params}, mode_{mode} { }
			std::string path_ = "./";
			AutoSaveParams auto_save_;
			Mode mode_{Mode::None};
		};
		const struct Params
		{
			PARAM_INIT;
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
			PARAM_DECL(int, threads_, -1, "threads", "Number of threads, -1 = auto detection");
			PARAM_DECL(bool , background_resampling_, true, "background_resampling", "If false, the background will not be resampled in subsequent adaptative AA passes");
			PARAM_DECL(int , base_sampling_offset_, 0, "adv_base_sampling_offset", "Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't \"repeat\" the same samples (user configurable)");
			PARAM_DECL(int , computer_node_, 0, "adv_computer_node", "Computer node in multi-computer render environments/render farms");
			PARAM_DECL(float, aa_pixel_width_, 1.5f, "AA_pixelwidth", "");
			PARAM_DECL(int, width_, 320, "width", "Width of rendered image");
			PARAM_DECL(int, height_, 240, "height", "Height of rendered image");
			PARAM_DECL(int, start_x_, 0, "xstart", "x-offset (for cropped rendering)");
			PARAM_DECL(int, start_y_, 0, "ystart", "y-offset (for cropped rendering)");
			PARAM_ENUM_DECL(FilterType, filter_type_, FilterType::Box, "filter_type", "AA filter type");
			PARAM_DECL(int, tile_size_, 32, "tile_size", "Size of the render buckets or tiles");
			PARAM_ENUM_DECL(ImageSplitter::TilesOrderType, tiles_order_, ImageSplitter::TilesOrderType::CentreRandom, "tiles_order", "Order of the render buckets or tiles");
			PARAM_ENUM_DECL(AutoSaveParams::IntervalType, images_autosave_interval_type_, AutoSaveParams::IntervalType::None, "images_autosave_interval_type", "");
			PARAM_DECL(int, images_autosave_interval_passes_, 1, "images_autosave_interval_passes", "");
			PARAM_DECL(float, images_autosave_interval_seconds_, 300.f, "images_autosave_interval_seconds", "");
			PARAM_ENUM_DECL(FilmLoadSave::Mode, film_load_save_mode_, FilmLoadSave::Mode::None, "film_load_save_mode", "");
			PARAM_DECL(std::string, film_load_save_path_, "./", "film_load_save_path", "");
			PARAM_ENUM_DECL(AutoSaveParams::IntervalType, film_autosave_interval_type_, AutoSaveParams::IntervalType::None, "film_autosave_interval_type", "");
			PARAM_DECL(int, film_autosave_interval_passes_, 1, "film_autosave_interval_passes", "");
			PARAM_DECL(float, film_autosave_interval_seconds_, 300.f, "film_autosave_interval_seconds", "");
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
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const;

		std::string getFilmPath() const;
		bool imageFilmLoad(const std::string &filename);
		void imageFilmLoadAllInFolder(RenderControl &render_control);
		bool imageFilmSave(RenderControl &render_control);
		void imageFilmFileBackup(RenderControl &render_control) const;
		static std::string printRenderStats(const RenderControl &render_control, const Timer &timer, const Size2i &size);
		static float darkThresholdCurveInterpolate(float pixel_brightness);
		void initLayersImages();
		void initLayersExportedImages();
		static int roundToIntWithBias(double val); //!< Asymmetrical rounding function with a +0.5 bias
		void defineBasicLayers();
		void defineDependentLayers(); //!< This function generates the basic/auxiliary layers. Must be called *after* defining all render layers with the defineLayer function.

		std::string name_{"Imagefilm"};
		int n_pass_{1};
		std::atomic<int> next_area_;
		int area_cnt_{0}, completed_cnt_{0};
		bool split_ = true;
		bool cancel_ = false;
		unsigned int sampling_offset_ = 0;	//To ensure sampling after loading the image film continues and does not repeat already done samples
		bool estimate_density_ = false;
		int num_density_samples_ = 0;
		Layers layers_;
		std::unique_ptr<ImageSplitter> splitter_;

		AutoSaveParams images_auto_save_params_{params_.images_autosave_interval_seconds_, params_.images_autosave_interval_passes_, params_.images_autosave_interval_type_};
		FilmLoadSave film_load_save_{params_.film_load_save_path_, {params_.film_autosave_interval_seconds_, params_.film_autosave_interval_passes_, params_.film_autosave_interval_type_}, params_.film_load_save_mode_};

		static constexpr inline int max_filter_size_ = 8;
		static constexpr inline int filter_table_size_ = 16;
		static constexpr inline float filter_scale_ = 1.f / static_cast<float>(filter_table_size_);
		alignas(16) std::array<float, filter_table_size_ * filter_table_size_> filter_table_;

		std::mutex image_mutex_, out_mutex_, density_image_mutex_; // Thread mutes for shared access

		Buffer2D<bool> flags_{{{params_.width_, params_.height_}}}; //!< flags for adaptive AA sampling;
		Buffer2D<Gray> weights_{{{params_.width_, params_.height_}}};
		ImageLayers film_image_layers_;
		ImageLayers exported_image_layers_;
		std::unique_ptr<Buffer2D<Rgb>> density_image_; //!< storage for z-buffer channel

		float filter_width_{params_.aa_pixel_width_ * 0.5f};
		float filter_table_scale_;

		Items<ImageOutput> outputs_;
		RenderCallbacks render_callbacks_;
		Timer timer_;
		Logger &logger_;

		const AaNoiseParams aa_noise_params_{
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
};

inline int ImageFilm::roundToIntWithBias(double val)
{
	static constexpr double doublemagicroundeps {.5 - 1.4e-11}; //almost .5f = .5f - 1e^(number of exp bit)
	return static_cast<int>(val + doublemagicroundeps);
}


} //namespace yafaray

#endif // LIBYAFARAY_IMAGEFILM_H
