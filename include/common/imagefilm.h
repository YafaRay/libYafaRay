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

#include "constants.h"
#include "imagesplitter.h"
#include "utility/util_image_buffers.h"
#include "utility/util_tiled_array.h"
#include "utility/util_thread.h"
#include "common/aa_noise_params.h"
#include "renderpasses.h"

BEGIN_YAFARAY

/*!	This class recieves all rendered image samples.
	You can see it as an enhanced render buffer;
	Holds RGBA and Density (for actual bidirectional pathtracing implementation) buffers.
*/

class ProgressBar;
class PassesSettings;
class Scene;
class ColorOutput;
class ColorPasses;

class LIBYAFARAY_EXPORT ImageFilm final
{
	public:
		enum class FilterType : int { Box, Mitchell, Gauss, Lanczos };
		enum Flags : unsigned int { Image = 1 << 0, Densityimage = 1 << 1, All = Image | Densityimage };
		enum class FilmSaveLoad : int { None, Save, LoadAndSave };
		struct AutoSaveParams
		{
			enum class IntervalType : int { None, Time, Pass };
			IntervalType interval_type_ = IntervalType::None;
			double interval_seconds_ = 300.0;
			int interval_passes_ = 1;
			double timer_ = 0.0; //Internal timer for AutoSave
			int pass_counter_ = 0; //Internal counter for AutoSave
		};

		/*! imageFilm_t Constructor */
		ImageFilm(int width, int height, int xstart, int ystart, ColorOutput &out, float filter_size = 1.0, FilterType filt = FilterType::Box,
				  Scene *e = nullptr, bool show_sam_mask = false, int t_size = 32,
				  ImageSplitter::TilesOrderType tiles_order_type = ImageSplitter::Linear, bool pm_a = false);
		/*! imageFilm_t Destructor */
		~ImageFilm();
		/*! Initialize imageFilm for new rendering, i.e. set pixels black etc */
		void init(int num_passes = 0);
		/*! Prepare for next pass, i.e. reset area_cnt, check if pixels need resample...
			\param adaptive_aa if true, flag pixels to be resampled
			\param threshold color threshold for adaptive antialiasing */
		int nextPass(int num_view, bool adaptive_aa, std::string integrator_name, bool skip_next_pass = false);
		/*! Return the next area to be rendered
			CAUTION! This method MUST be threadsafe!
			\return false if no area is left to be handed out, true otherwise */
		bool nextArea(int num_view, RenderArea &a);
		/*! Indicate that all pixels inside the area have been sampled for this pass */
		void finishArea(int num_view, RenderArea &a);
		/*! Output all pixels to the color output */
		void flush(int num_view, int flags = All, ColorOutput *out = nullptr);
		/*! query if sample (x,y) was flagged to need more samples.
			IMPORTANT! You may only call this after you have called nextPass(true, ...), otherwise
			no such flags have been created !! */
		bool doMoreSamples(int x, int y) const;
		/*!	Add image sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addSample(int x, int y, float dx, float dy, const RenderArea *a = nullptr, int num_sample = 0, int aa_pass_number = 0, float inv_aa_max_possible_samples = 0.1f, IntPasses *int_passes = nullptr);
		/*!	Add light density sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addDensitySample(const Rgb &c, int x, int y, float dx, float dy, const RenderArea *a = nullptr);
		//! Enables/Disables a light density estimation image
		void setDensityEstimation(bool enable);
		//! set number of samples for correct density estimation (if enabled)
		void setNumDensitySamples(int n) { num_density_samples_ = n; }
		/*! Sets the film color space and gamma correction */
		void setColorSpace(ColorSpace color_space, float gamma_val);
		/*! Sets the film color space and gamma correction for optional secondary file output */
		void setColorSpace2(ColorSpace color_space, float gamma_val);
		/*! Sets the film premultiply option for optional secondary file output */
		void setPremult2(bool premult);
		/*! Sets the adaptative AA sampling threshold */
		void setAaThreshold(float thresh) { aa_noise_params_.threshold_ = thresh; }
		/*! Sets a custom progress bar in the image film */
		void setProgressBar(ProgressBar *pb);
		/*! The following methods set the strings used for the parameters badge rendering */
		int getTotalPixels() const { return w_ * h_; };
		void setAaNoiseParams(const AaNoiseParams &aa_noise_params) { aa_noise_params_ = aa_noise_params; };
		/*! Methods for rendering the parameters badge; Note that FreeType lib is needed to render text */
		void drawRenderSettings(std::stringstream &ss);
		float darkThresholdCurveInterpolate(float pixel_brightness);
		int getWidth() const { return w_; }
		int getHeight() const { return h_; }
		int getCx0() const { return cx_0_; }
		int getCy0() const { return cy_0_; }
		int getTileSize() const { return tile_size_; }
		int getCurrentPass() const { return n_pass_; }
		int getNumPasses() const { return n_passes_; }
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
		void imageFilmLoadAllInFolder();
		bool imageFilmSave();
		void imageFilmFileBackup() const;
		void setFilmSaveLoad(const FilmSaveLoad &film_file_save_load) { film_file_save_load_ = film_file_save_load; }

		void setImagesAutoSaveParams(const AutoSaveParams &auto_save_params) { images_auto_save_params_ = auto_save_params; }
		void setFilmAutoSaveParams(const AutoSaveParams &auto_save_params) { film_auto_save_params_ = auto_save_params; }
		void resetImagesAutoSaveTimer() { images_auto_save_params_.timer_ = 0.0; }
		void resetFilmAutoSaveTimer() { film_auto_save_params_.timer_ = 0.0; }

		void generateDebugFacesEdges(int num_view, int idx_pass, int xstart, int width, int ystart, int height, bool drawborder, ColorOutput *out_1, int out_1_displacement = 0, ColorOutput *out_2 = nullptr, int out_2_displacement = 0);
		void generateToonAndDebugObjectEdges(int num_view, int idx_pass, int xstart, int width, int ystart, int height, bool drawborder, ColorOutput *out_1, int out_1_displacement = 0, ColorOutput *out_2 = nullptr, int out_2_displacement = 0);

		Rgba2DImageWeighed_t *getImagePassFromIntPassType(int pass_type);
		int getImagePassIndexFromIntPassType(int pass_type);

	private:
		std::vector<Rgba2DImageWeighed_t *> image_passes_; //!< rgba color buffers for the render passes
		Rgb2DImage_t *density_image_; //!< storage for z-buffer channel
		Rgba2DImage_t *dp_image_; //!< render parameters badge image
		TiledBitArray2D<3> *flags_ = nullptr; //!< flags for adaptive AA sampling;
		int dp_height_; //!< height of the rendering parameters badge;
		int w_, h_, cx_0_, cx_1_, cy_0_, cy_1_;
		int area_cnt_, completed_cnt_;
		volatile int next_area_;
		ColorSpace color_space_ = RawManualGamma;
		float gamma_ = 1.f;
		ColorSpace color_space_2_ = RawManualGamma;	//For optional secondary file output
		float gamma_2_ = 1.f;				//For optional secondary file output
		AaNoiseParams aa_noise_params_;
		float filterw_, table_scale_;
		float *filter_table_ = nullptr;
		ColorOutput *output_ = nullptr;
		// Thread mutes for shared access
		std::mutex image_mutex_, splitter_mutex_, out_mutex_, density_image_mutex_;
		bool split_ = true;
		bool abort_ = false;
		bool estimate_density_ = false;
		int num_density_samples_ = 0;
		ImageSplitter *splitter_ = nullptr;
		ProgressBar *pbar_ = nullptr;
		Scene *scene_;
		int n_pass_;
		bool show_mask_;
		int tile_size_;
		ImageSplitter::TilesOrderType tiles_order_;
		bool premult_alpha_;
		bool premult_alpha_2_ = false;	//For optional secondary file output
		int n_passes_;
		bool background_resampling_ = true;   //If false, the background will not be resampled in subsequent adaptative AA passes

		//Options for Film saving/loading correct sampling, as well as multi computer film saving
		unsigned int base_sampling_offset_ = 0;	//Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't "repeat" the same samples (user configurable)
		unsigned int sampling_offset_ = 0;	//To ensure sampling after loading the image film continues and does not repeat already done samples
		unsigned int computer_node_ = 0;	//Computer node in multi-computer render environments/render farms

		AutoSaveParams images_auto_save_params_;
		AutoSaveParams film_auto_save_params_;
		FilmSaveLoad film_file_save_load_ = FilmSaveLoad::None;
};

END_YAFARAY

#endif // YAFARAY_IMAGEFILM_H
