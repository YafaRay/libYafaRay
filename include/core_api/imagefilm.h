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

#include <yafray_config.h>

#include "color.h"
#include <yafraycore/ccthreads.h>
#include <core_api/output.h>
#include <core_api/imagesplitter.h>
#include <core_api/environment.h>
#include <utilities/image_buffers.h>
#include <utilities/tiled_array.h>

#if HAVE_FREETYPE
struct FT_Bitmap_;
#endif

__BEGIN_YAFRAY

/*!	This class recieves all rendered image samples.
	You can see it as an enhanced render buffer;
	Holds RGBA and Density (for actual bidirectional pathtracing implementation) buffers.
*/

class progressBar_t;
class renderPasses_t;
class colorPasses_t;

// Image types define
#define IF_IMAGE 1
#define IF_DENSITYIMAGE 2
#define IF_ALL (IF_IMAGE | IF_DENSITYIMAGE)

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
		renderEnvironment_t *e = NULL, bool showSamMask = false, int tSize = 32,
		imageSpliter_t::tilesOrderType tOrder=imageSpliter_t::LINEAR, bool pmA = false, bool drawParams = false);
		/*! imageFilm_t Destructor */
		~imageFilm_t();
		/*! Initialize imageFilm for new rendering, i.e. set pixels black etc */
		void init(int numPasses = 0);
		/*! Prepare for next pass, i.e. reset area_cnt, check if pixels need resample...
			\param adaptive_AA if true, flag pixels to be resampled
			\param threshold color threshold for adaptive antialiasing */
		int nextPass(int numView, bool adaptive_AA, std::string integratorName);
		/*! Return the next area to be rendered
			CAUTION! This method MUST be threadsafe!
			\return false if no area is left to be handed out, true otherwise */
		bool nextArea(int numView, renderArea_t &a);
		/*! Indicate that all pixels inside the area have been sampled for this pass */
		void finishArea(int numView, renderArea_t &a);
		/*! Output all pixels to the color output */
		void flush(int numView, int flags = IF_ALL, colorOutput_t *out = 0);
		/*! query if sample (x,y) was flagged to need more samples.
			IMPORTANT! You may only call this after you have called nextPass(true, ...), otherwise
			no such flags have been created !! */
		bool doMoreSamples(int x, int y) const;
		/*!	Add image sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addSample(colorPasses_t &colorPasses, int x, int y, float dx, float dy, const renderArea_t *a = 0, int numSample = 0, int AA_pass_number = 0, float inv_AA_max_possible_samples = 0.1f);
		/*!	Add light density sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addDensitySample(const color_t &c, int x, int y, float dx, float dy, const renderArea_t *a = 0);
		//! Enables/Disables a light density estimation image
		void setDensityEstimation(bool enable);
		//! set number of samples for correct density estimation (if enabled)
		void setNumSamples(int n){ numSamples = n; }
		/*! Sets the film color space and gamma correction */
		void setColorSpace(colorSpaces_t color_space, float gammaVal);
		/*! Sets the film color space and gamma correction for optional secondary file output */
		void setColorSpace2(colorSpaces_t color_space, float gammaVal);
		/*! Sets the film premultiply option for optional secondary file output */
		void setPremult2(bool premult);
		/*! Sets the adaptative AA sampling threshold */
		void setAAThreshold(CFLOAT thresh){ AA_thesh=thresh; }
		/*! Enables interactive color buffer output for preview during render */
		void setInteractive(bool ia){ interactive = ia; }
        /*! Enables partial image saving during render every time_interval seconds. Time=0.0 (default) disables partial saving. */
		void setImageOutputPartialSaveTimeInterval(double time_interval){ imageOutputPartialSaveTimeInterval = time_interval; }
		/*! Sets a custom progress bar in the image film */
		void setProgressBar(progressBar_t *pb);
		/*! The following methods set the strings used for the parameters badge rendering */
		void setAAParams(const std::string &aa_params);
		void setIntegParams(const std::string &integ_params);
		void setUseParamsBadge(bool on = true) { drawParams = on; }
		bool getUseParamsBadge() { return drawParams; }
		int getTotalPixels() const { return w*h; };
		void setAANoiseParams(bool detect_color_noise, float dark_threshold_factor, int variance_edge_size, int variance_pixels, float clamp_samples);

		/*! Methods for rendering the parameters badge; Note that FreeType lib is needed to render text */
		void drawRenderSettings();
        void reset_accumulated_image_area_flush_time() { accumulated_image_area_flush_time = 0.0; }

#if HAVE_FREETYPE
		void drawFontBitmap( FT_Bitmap_* bitmap, int x, int y);
#endif

	protected:
		std::vector<rgba2DImage_t*> imagePasses; //!< rgba color buffers for the render passes
		rgb2DImage_nw_t *densityImage; //!< storage for z-buffer channel
		rgba2DImage_nw_t *dpimage; //!< render parameters badge image
		tiledBitArray2D_t<3> *flags; //!< flags for adaptive AA sampling;
		int dpHeight; //!< height of the rendering parameters badge;
		int w, h, cx0, cx1, cy0, cy1;
		int area_cnt, completed_cnt;
		volatile int next_area;
		colorSpaces_t colorSpace;
		float gamma;
		colorSpaces_t colorSpace2;	//For optional secondary file output
		float gamma2;				//For optional secondary file output
		CFLOAT AA_thesh;
		bool AA_detect_color_noise;
		float AA_dark_threshold_factor;
		int AA_variance_edge_size;
		int AA_variance_pixels;
		float AA_clamp_samples;
		float filterw, tableScale;
		float *filterTable;
		colorOutput_t *output;
		// Thread mutes for shared access
		yafthreads::mutex_t imageMutex, splitterMutex, outMutex, densityImageMutex;
		bool split, interactive, abort;
        double imageOutputPartialSaveTimeInterval;
		bool estimateDensity;
		int numSamples;
		imageSpliter_t *splitter;
		progressBar_t *pbar;
		renderEnvironment_t *env;
		int nPass;
		bool showMask;
		int tileSize;
		imageSpliter_t::tilesOrderType tilesOrder;
		bool premultAlpha;
		bool premultAlpha2;	//For optional secondary file output
		int nPasses;
		bool drawParams;
		std::string aaSettings;
		std::string integratorSettings;
        double accumulated_image_area_flush_time;
};

__END_YAFRAY

#endif // Y_IMAGEFILM_H
