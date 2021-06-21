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

#ifndef YAFARAY_IMAGEFILM_H
#define YAFARAY_IMAGEFILM_H

#include "common/yafaray_common.h"
#include "imagesplitter.h"
#include "common/aa_noise_params.h"
#include "common/layers.h"
#include "common/memory.h"
#include "image/image_buffers.h"
#include "image/image_layers.h"
#include <mutex>

BEGIN_YAFARAY

/*!	This class recieves all rendered image samples.
	You can see it as an enhanced render buffer;
	Holds RGBA and Density (for actual bidirectional pathtracing implementation) buffers.
*/

class ProgressBar;
class Scene;
class ColorOutput;
class Rgb;
class ColorLayers;
class ParamMap;
class RenderControl;
class RenderView;

class ImageFilm final
{
	public:
		enum class FilterType : int { Box, Mitchell, Gauss, Lanczos };
		enum Flags : unsigned int { RegularImage = 1 << 0, Densityimage = 1 << 1, All = RegularImage | Densityimage };
		struct AutoSaveParams
		{
			enum class IntervalType : int { None, Time, Pass };
			IntervalType interval_type_ = IntervalType::None;
			double interval_seconds_ = 300.0;
			int interval_passes_ = 1;
			double timer_ = 0.0; //Internal timer for AutoSave
			int pass_counter_ = 0; //Internal counter for AutoSave
		};
		struct FilmLoadSave
		{
			enum Mode : int { None, Save, LoadAndSave };
			Mode mode_ = Mode::None;
			std::string path_ = "./";
			AutoSaveParams auto_save_;
		};

		static std::unique_ptr<ImageFilm> factory(Logger &logger, const ParamMap &params, Scene *scene);
		/*! imageFilm_t Constructor */
		ImageFilm(Logger &logger, int width, int height, int xstart, int ystart, int num_threads, RenderControl &render_control, const Layers &layers, const std::map<std::string, UniquePtr_t<ColorOutput>> &outputs, float filter_size = 1.0, FilterType filt = FilterType::Box,
				  bool show_sam_mask = false, int t_size = 32,
				  ImageSplitter::TilesOrderType tiles_order_type = ImageSplitter::Linear);
		/*! Initialize imageFilm for new rendering, i.e. set pixels black etc */
		void init(RenderControl &render_control, int num_passes = 0);
		/*! Prepare for next pass, i.e. reset area_cnt, check if pixels need resample...
			\param adaptive_aa if true, flag pixels to be resampled
			\param threshold color threshold for adaptive antialiasing */
		int nextPass(const RenderView *render_view, RenderControl &render_control, bool adaptive_aa, std::string integrator_name, bool skip_nrender_layer = false);
		/*! Return the next area to be rendered
			CAUTION! This method MUST be threadsafe!
			\return false if no area is left to be handed out, true otherwise */
		bool nextArea(const RenderControl &render_control, RenderArea &a);
		/*! Indicate that all pixels inside the area have been sampled for this pass */
		void finishArea(const RenderView *render_view, RenderControl &render_control, RenderArea &a);
		/*! Output all pixels to the color output */
		void flush(const RenderView *render_view, const RenderControl &render_control, int flags = All);
		void cleanup() { weights_.clear(); }
		/*! query if sample (x,y) was flagged to need more samples.
			IMPORTANT! You may only call this after you have called nextPass(true, ...), otherwise
			no such flags have been created !! */
		bool doMoreSamples(int x, int y) const;
		/*!	Add image sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addSample(int x, int y, float dx, float dy, const RenderArea *a = nullptr, int num_sample = 0, int aa_pass_number = 0, float inv_aa_max_possible_samples = 0.1f, ColorLayers *color_layers = nullptr);
		/*!	Add light density sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addDensitySample(const Rgb &c, int x, int y, float dx, float dy, const RenderArea *a = nullptr);
		//! Enables/Disables a light density estimation image
		void setDensityEstimation(bool enable);
		//! set number of samples for correct density estimation (if enabled)
		void setNumDensitySamples(int n) { num_density_samples_ = n; }
		/*! Sets the adaptative AA sampling threshold */
		void setAaThreshold(float thresh) { aa_noise_params_.threshold_ = thresh; }
		/*! Sets a custom progress bar in the image film */
		void setProgressBar(std::shared_ptr<ProgressBar> pb);
		/*! The following methods set the strings used for the parameters badge rendering */
		int getTotalPixels() const { return width_ * height_; };
		void setAaNoiseParams(const AaNoiseParams &aa_noise_params) { aa_noise_params_ = aa_noise_params; };
		/*! Methods for rendering the parameters badge; Note that FreeType lib is needed to render text */
		float darkThresholdCurveInterpolate(float pixel_brightness);
		int getWidth() const { return width_; }
		int getHeight() const { return height_; }
		int getCx0() const { return cx_0_; }
		int getCy0() const { return cy_0_; }
		int getTileSize() const { return tile_size_; }
		float getWeight(int x, int y) const { return weights_(x, y).getFloat(); }
		bool getBackgroundResampling() const { return background_resampling_; }
		void setBackgroundResampling(bool background_resampling) { background_resampling_ = background_resampling; }
		unsigned int getComputerNode() const { return computer_node_; }
		unsigned int getBaseSamplingOffset() const { return base_sampling_offset_ + computer_node_ * 100000; } //We give to each computer node a "reserved space" of 100,000 samples
		unsigned int getSamplingOffset() const { return sampling_offset_; }
		void setComputerNode(unsigned int computer_node) { computer_node_ = computer_node; }
		void setBaseSamplingOffset(unsigned int offset) { base_sampling_offset_ = offset; }
		void setSamplingOffset(unsigned int offset) { sampling_offset_ = offset; }

		std::string getFilmPath() const;
		bool imageFilmLoad(const std::string &filename);
		void imageFilmLoadAllInFolder(RenderControl &render_control);
		bool imageFilmSave();
		void imageFilmFileBackup() const;
		void setImagesAutoSaveParams(const AutoSaveParams &auto_save_params) { images_auto_save_params_ = auto_save_params; }
		void setFilmLoadSaveParams(const FilmLoadSave &film_load_save) { film_load_save_ = film_load_save; }
		std::string getFilmSavePath() const { return film_load_save_.path_; }
		void resetImagesAutoSaveTimer() { images_auto_save_params_.timer_ = 0.0; }
		void resetFilmAutoSaveTimer() { film_load_save_.auto_save_.timer_ = 0.0; }

		void generateDebugFacesEdges(int xstart, int width, int ystart, int height, bool drawborder);
		void generateToonAndDebugObjectEdges(int xstart, int width, int ystart, int height, bool drawborder);
		const ImageLayers *getImageLayers() const { return &image_layers_; }

	private:
		int width_, height_, cx_0_, cx_1_, cy_0_, cy_1_;
		bool show_mask_;
		int tile_size_;
		ImageSplitter::TilesOrderType tiles_order_;
		int num_threads_ = 1;
		int n_passes_;
		unsigned int computer_node_ = 0;	//Computer node in multi-computer render environments/render farms
		int n_pass_;
		volatile int next_area_;
		int area_cnt_, completed_cnt_;
		bool split_ = true;
		bool cancel_ = false;
		bool background_resampling_ = true;   //If false, the background will not be resampled in subsequent adaptative AA passes
		//Options for Film saving/loading correct sampling, as well as multi computer film saving
		unsigned int base_sampling_offset_ = 0;	//Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't "repeat" the same samples (user configurable)
		unsigned int sampling_offset_ = 0;	//To ensure sampling after loading the image film continues and does not repeat already done samples
		bool estimate_density_ = false;
		int num_density_samples_ = 0;
		AaNoiseParams aa_noise_params_;
		const Layers &layers_;
		const std::map<std::string, UniquePtr_t<ColorOutput>> &outputs_;
		std::unique_ptr<ImageSplitter> splitter_;
		std::shared_ptr<ProgressBar> progress_bar_;
		//const Scene *scene_ = nullptr;
		//const RenderControl *render_control_;

		AutoSaveParams images_auto_save_params_;
		FilmLoadSave film_load_save_;

		float filterw_, table_scale_;
		std::unique_ptr<float[]> filter_table_;
		// Thread mutes for shared access
		std::mutex image_mutex_, splitter_mutex_, out_mutex_, density_image_mutex_;

		ImageBuffer2D<bool> flags_; //!< flags for adaptive AA sampling;
		ImageBuffer2D<Gray> weights_;
		ImageLayers image_layers_;
		std::unique_ptr<ImageBuffer2D<Rgb>> density_image_; //!< storage for z-buffer channel
		Logger &logger_;
};

END_YAFARAY

#endif // YAFARAY_IMAGEFILM_H
