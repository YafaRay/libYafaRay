#pragma once
/****************************************************************************
 *
 *      imagefilm.h: image data handling class
 *      This is part of the yafray package
 *		See AUTHORS for more information
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

#ifndef Y_IMAGEFILM_H
#define Y_IMAGEFILM_H

#include <yafray_constants.h>
#include "imagesplitter.h"
#include <utilities/image_buffers.h>
#include <utilities/tiled_array.h>
#include <utilities/threadUtils.h>

__BEGIN_YAFRAY

/*!	This class recieves all rendered image samples.
	You can see it as an enhanced render buffer;
	Holds RGBA and Density (for actual bidirectional pathtracing implementation) buffers.
*/

class progressBar_t;
class renderPasses_t;
class colorPasses_t;
class renderEnvironment_t;
class colorOutput_t;

// Image types define
#define IF_IMAGE 1
#define IF_DENSITYIMAGE 2
#define IF_ALL (IF_IMAGE | IF_DENSITYIMAGE)

enum darkDetectionType_t
{
	DARK_DETECTION_NONE,
	DARK_DETECTION_LINEAR,
	DARK_DETECTION_CURVE,
};

enum autoSaveIntervalType_t
{
	AUTOSAVE_NONE,
	AUTOSAVE_TIME_INTERVAL,
	AUTOSAVE_PASS_INTERVAL,
};

enum filmFileSaveLoad_t
{
	FILM_FILE_NONE,
	FILM_FILE_SAVE,
	FILM_FILE_LOAD_SAVE,
};

class YAFRAYCORE_EXPORT imageFilm_t
{
	public:
		enum filterType
		{
			BOX,
			MITCHELL,
			GAUSS,
			LANCZOS
		};

		/*! imageFilm_t Constructor */
		imageFilm_t(int width, int height, int xstart, int ystart, colorOutput_t &out, float filterSize=1.0, filterType filt=BOX,
		renderEnvironment_t *e = nullptr, bool showSamMask = false, int tSize = 32,
		imageSpliter_t::tilesOrderType tOrder=imageSpliter_t::LINEAR, bool pmA = false);
		/*! imageFilm_t Destructor */
		~imageFilm_t();
		/*! Initialize imageFilm for new rendering, i.e. set pixels black etc */
		void init(int numPasses = 0);
		/*! Prepare for next pass, i.e. reset area_cnt, check if pixels need resample...
			\param adaptive_AA if true, flag pixels to be resampled
			\param threshold color threshold for adaptive antialiasing */
		int nextPass(int numView, bool adaptive_AA, std::string integratorName, bool skipNextPass = false);
		/*! Return the next area to be rendered
			CAUTION! This method MUST be threadsafe!
			\return false if no area is left to be handed out, true otherwise */
		bool nextArea(int numView, renderArea_t &a);
		/*! Indicate that all pixels inside the area have been sampled for this pass */
		void finishArea(int numView, renderArea_t &a);
		/*! Output all pixels to the color output */
		void flush(int numView, int flags = IF_ALL, colorOutput_t *out = nullptr);
		/*! query if sample (x,y) was flagged to need more samples.
			IMPORTANT! You may only call this after you have called nextPass(true, ...), otherwise
			no such flags have been created !! */
		bool doMoreSamples(int x, int y) const;
		/*!	Add image sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addSample(colorPasses_t &colorPasses, int x, int y, float dx, float dy, const renderArea_t *a = nullptr, int numSample = 0, int AA_pass_number = 0, float inv_AA_max_possible_samples = 0.1f);
		/*!	Add light density sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addDensitySample(const color_t &c, int x, int y, float dx, float dy, const renderArea_t *a = nullptr);
		//! Enables/Disables a light density estimation image
		void setDensityEstimation(bool enable);
		//! set number of samples for correct density estimation (if enabled)
		void setNumDensitySamples(int n){ numDensitySamples = n; }
		/*! Sets the film color space and gamma correction */
		void setColorSpace(colorSpaces_t color_space, float gammaVal);
		/*! Sets the film color space and gamma correction for optional secondary file output */
		void setColorSpace2(colorSpaces_t color_space, float gammaVal);
		/*! Sets the film premultiply option for optional secondary file output */
		void setPremult2(bool premult);
		/*! Sets the adaptative AA sampling threshold */
		void setAAThreshold(float thresh){ AA_thesh=thresh; }
		/*! Sets a custom progress bar in the image film */
		void setProgressBar(progressBar_t *pb);
		/*! The following methods set the strings used for the parameters badge rendering */
		int getTotalPixels() const { return w*h; };
		void setAANoiseParams(bool detect_color_noise, int dark_detection_type, float dark_threshold_factor, int variance_edge_size, int variance_pixels, float clamp_samples);
		/*! Methods for rendering the parameters badge; Note that FreeType lib is needed to render text */
		void drawRenderSettings(std::stringstream & ss);
        float dark_threshold_curve_interpolate(float pixel_brightness);
        int getWidth() const { return w; }
        int getHeight() const { return h; }
        int getCX0() const { return cx0; }
        int getCY0() const { return cy0; }
        int getTileSize() const { return tileSize; }
        int getCurrentPass() const { return nPass; }
        int getNumPasses() const { return nPasses; }
        bool getBackgroundResampling() const { return backgroundResampling; }
        void setBackgroundResampling(bool background_resampling) { backgroundResampling = background_resampling; }
        unsigned int getComputerNode() const { return computerNode; }
        unsigned int getBaseSamplingOffset() const { return baseSamplingOffset + computerNode * 100000; } //We give to each computer node a "reserved space" of 100,000 samples
        unsigned int getSamplingOffset() const { return samplingOffset; }
        void setComputerNode(unsigned int computer_node) { computerNode = computer_node; }
        void setBaseSamplingOffset(unsigned int offset) { baseSamplingOffset = offset; }
        void setSamplingOffset(unsigned int offset) { samplingOffset = offset; }
		
		std::string getFilmPath() const;
        bool imageFilmLoad(const std::string &filename);
		void imageFilmLoadAllInFolder();
        bool imageFilmSave();
        void imageFilmFileBackup() const;

        void setImagesAutoSaveIntervalType(int interval_type) { imagesAutoSaveIntervalType = interval_type; }
        void setImagesAutoSaveIntervalSeconds(double interval_seconds) { imagesAutoSaveIntervalSeconds = interval_seconds; }
        void setImagesAutoSaveIntervalPasses(int interval_passes) { imagesAutoSaveIntervalPasses = interval_passes; }
        void resetImagesAutoSaveTimer() { imagesAutoSaveTimer = 0.0; }

        void setFilmFileSaveLoad(int film_file_save_load) { filmFileSaveLoad = film_file_save_load; }
        void setFilmAutoSaveIntervalType(int interval_type) { filmAutoSaveIntervalType = interval_type; }
        void setFilmAutoSaveIntervalSeconds(double interval_seconds) { filmAutoSaveIntervalSeconds = interval_seconds; }
        void setFilmAutoSaveIntervalPasses(int interval_passes) { filmAutoSaveIntervalPasses = interval_passes; }
        void resetFilmAutoSaveTimer() { filmAutoSaveTimer = 0.0; }

		void generateDebugFacesEdges(int numView, int idxPass, int xstart, int width, int ystart, int height, bool drawborder, colorOutput_t * out1, int out1displacement = 0, colorOutput_t * out2 = nullptr, int out2displacement = 0);
		void generateToonAndDebugObjectEdges(int numView, int idxPass, int xstart, int width, int ystart, int height, bool drawborder, colorOutput_t * out1, int out1displacement = 0, colorOutput_t * out2 = nullptr, int out2displacement = 0);
		
		rgba2DImage_t * getImagePassFromIntPassType(int intPassType);
        int getImagePassIndexFromIntPassType(int intPassType);
        int getAuxImagePassIndexFromIntPassType(int intPassType);
        
	protected:
		std::vector<rgba2DImage_t*> imagePasses; //!< rgba color buffers for the render passes
		std::vector<rgba2DImage_t*> auxImagePasses; //!< rgba color buffers for the auxiliary image passes
		rgb2DImage_nw_t *densityImage; //!< storage for z-buffer channel
		rgba2DImage_nw_t *dpimage; //!< render parameters badge image
		tiledBitArray2D_t<3> *flags = nullptr; //!< flags for adaptive AA sampling;
		int dpHeight; //!< height of the rendering parameters badge;
		int w, h, cx0, cx1, cy0, cy1;
		int area_cnt, completed_cnt;
		volatile int next_area;
		colorSpaces_t colorSpace = RAW_MANUAL_GAMMA;
		float gamma = 1.f;
		colorSpaces_t colorSpace2 = RAW_MANUAL_GAMMA;	//For optional secondary file output
		float gamma2 = 1.f;				//For optional secondary file output
		float AA_thesh;
		bool AA_detect_color_noise;
        int AA_dark_detection_type;
		float AA_dark_threshold_factor;
		int AA_variance_edge_size;
		int AA_variance_pixels;
		float AA_clamp_samples;
		float filterw, tableScale;
		float *filterTable;
		colorOutput_t *output;
		// Thread mutes for shared access
		std::mutex imageMutex, splitterMutex, outMutex, densityImageMutex;
		bool split = true;
		bool abort = false;
		bool estimateDensity = false;
		int numDensitySamples = 0;
		imageSpliter_t *splitter = nullptr;
		progressBar_t *pbar = nullptr;
		renderEnvironment_t *env;
		int nPass;
		bool showMask;
		int tileSize;
		imageSpliter_t::tilesOrderType tilesOrder;
		bool premultAlpha;
		bool premultAlpha2 = false;	//For optional secondary file output
		int nPasses;
        bool backgroundResampling = true;   //If false, the background will not be resampled in subsequent adaptative AA passes

        //Options for Film saving/loading correct sampling, as well as multi computer film saving
        unsigned int baseSamplingOffset = 0;	//Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't "repeat" the same samples (user configurable)
        unsigned int samplingOffset = 0;	//To ensure sampling after loading the image film continues and does not repeat already done samples
        unsigned int computerNode = 0;	//Computer node in multi-computer render environments/render farms

		//Options for AutoSaving output images
		int imagesAutoSaveIntervalType = AUTOSAVE_NONE;
		double imagesAutoSaveIntervalSeconds = 300.0;
		int imagesAutoSaveIntervalPasses = 1;
		double imagesAutoSaveTimer = 0.0; //Internal timer for images AutoSave
		int imagesAutoSavePassCounter = 0;	//Internal counter for images AutoSave

		//Options for Saving/AutoSaving/Loading the internal imageFilm image buffers
		int filmFileSaveLoad = FILM_FILE_NONE;
		int filmAutoSaveIntervalType = AUTOSAVE_NONE;
		double filmAutoSaveIntervalSeconds = 300.0;
		double filmAutoSaveTimer = 0.0; //Internal timer for Film AutoSave
		int filmAutoSavePassCounter = 0;	//Internal counter for Film AutoSave
		int filmAutoSaveIntervalPasses = 1;
};

__END_YAFRAY

#endif // Y_IMAGEFILM_H
