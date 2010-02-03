
#ifndef Y_IMAGEFILM_H
#define Y_IMAGEFILM_H

#include <yafray_config.h>

#include "color.h"
#include <yafraycore/ccthreads.h>
#include <core_api/output.h>
#include <core_api/imagesplitter.h>
#include <core_api/environment.h>
#include <utilities/tiled_array.h>

struct FT_Bitmap_;

__BEGIN_YAFRAY

/*!	This class recieves all rendered image samples.
	You can see it as an enhanced render buffer;
	May likely hold many more buffers than just RGB in the future.
*/

template<class T, int logBlockSize> class tiledArray2D_t;
template<int logBlockSize> class tiledBitArray2D_t;
class progressBar_t;

#define IF_IMAGE 1
#define IF_DENSITYIMAGE 2
#define IF_ALL (IF_IMAGE | IF_DENSITYIMAGE)

class YAFRAYCORE_EXPORT imageFilm_t
{
	public:
		enum filterType { BOX, MITCHELL, GAUSS, LANCZOS };
		imageFilm_t(int width, int height, int xstart, int ystart, colorOutput_t &out, float filterSize=1.0, filterType filt=BOX,
		renderEnvironment_t *e = NULL, bool showSamMask = false, int tSize = 32, imageSpliter_t::tilesOrderType tOrder=imageSpliter_t::LINEAR, bool pmA = false);
		~imageFilm_t();
		/*! initialize imageFilm for new rendering, i.e. set pixels black etc */
		void init(int numPasses = 0);
		/*! return the next area to be rendered
			CAUTION! This method MUST be threadsafe!
			\return false if no area is left to be handed out, true otherwise */
		bool nextArea(renderArea_t &a);
		/*! indicate that all pixels inside the area have been sampled for this pass */
		void finishArea(renderArea_t &a);
		/*!	add image sample; dx and dy describe the position in the pixel (x,y).
			IMPORTANT: when a is given, all samples within a are assumed to come from the same thread!
			use a=0 for contributions outside the area associated with current thread!
		*/
		void addSample(const colorA_t &c, int x, int y, float dx, float dy, const renderArea_t *a=0);
		void addDensitySample(const color_t &c, int x, int y, float dx, float dy, const renderArea_t *a=0);
		/*! prepare for next pass, i.e. reset area_cnt, check if pixels need resample...
			\param adaptive_AA if true, flag pixels to be resampled
			\param threshold color threshold for adaptive antialiasing */
		void nextPass(bool adaptive_AA);
		/*! query if sample (x,y) was flagged to need more samples.
			IMPORTANT! You may only call this after you have called nextPass(true, ...), otherwise
			no such flags have been created !! */
		void setChanPixel(float val, int chan, int x, int y);
		bool doMoreSamples(int x, int y) const;
		/*! output all pixels to the color output */
		void flush(int flags=IF_ALL, colorOutput_t *out=0);
		void setClamp(bool c){ clamp = c; }
		/*! set and enable/disable gamma correction of output; when gammaVal is <= 0 the current value is kept */
		void setGamma(float gammaVal, bool enable);
		void setDensityEstimation(bool enable);
		//! set number of samples for correct density estimation (if enabled)
		void setNumSamples(int n){ numSamples = n; }
		void setAAThreshold(CFLOAT thresh){ AA_thesh=thresh; }
		void setInteractive(bool ia){ interactive = ia; }
		/*! add a custom channel to image film, with the label "name".
			\return number of channel used to specify channel when adding samples */
		int addChannel(const std::string &name);

		/*! sets a custom progress bar in the image film */
		void setProgressBar(progressBar_t *pb);

#if HAVE_FREETYPE
		void drawRenderSettings();
		void drawFontBitmap( FT_Bitmap_* bitmap, int x, int y);
#endif

	protected:
		struct pixel_t
		{
			colorA_t normalized() const
			{
				if(weight>0.f) return col*(1.f/weight);
				else return colorA_t(0.f);
			}
			colorA_t col;
			CFLOAT weight;
		};
		tiledArray2D_t<pixel_t, 3> *image;
		tiledArray2D_t<color_t, 3> densityImage;
		std::vector< tiledArray2D_t<float, 3> > channels; //!< storage for custom channels
		tiledBitArray2D_t<3> *flags; //!< flags for adaptive AA;
		int w, h, cx0, cx1, cy0, cy1;
		int area_cnt, completed_cnt;
		volatile int next_area;
		float gamma;
		CFLOAT AA_thesh;
		double filterw, tableScale;
		float *filterTable;
		colorOutput_t *output;
		yafthreads::mutex_t imageMutex, splitterMutex, outMutex, densityImageMutex;
		bool clamp, split, interactive, abort, correctGamma;
		bool estimateDensity;
		int numSamples; //!< number of added samples; important for density estimation
		imageSpliter_t *splitter;
		progressBar_t *pbar;
		int _n_locked, _n_unlocked; //just debug crap...
		renderEnvironment_t *env;
		int nPass;
		bool showMask;
		int tileSize;
		imageSpliter_t::tilesOrderType tilesOrder;
		bool premultAlpha;
		int nPasses;
};

__END_YAFRAY

#endif // Y_IMAGEFILM_H
