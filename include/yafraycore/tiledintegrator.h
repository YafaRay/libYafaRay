
#ifndef Y_TILEDINTEGRATOR_H
#define Y_TILEDINTEGRATOR_H

#include <core_api/integrator.h>
#include <core_api/imagesplitter.h>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT tiledIntegrator_t: public surfaceIntegrator_t
{
	public:
		/*! do whatever is required to render the image; default implementation renders image in passes
		dividing each pass into tiles for multithreading. */
		virtual bool render(imageFilm_t *imageFilm);
		/*! render a pass; only required by the default implementation of render() */
		virtual bool renderPass(int samples, int offset, bool adaptive);
		/*! render a tile; only required by default implementation of render() */
		virtual bool renderTile(renderArea_t &a, int n_samples, int offset, bool adaptive, int threadID);
	protected:
		int AA_samples, AA_passes, AA_inc_samples;
		CFLOAT AA_threshold;
		imageFilm_t *imageFilm;
};

#ifdef USING_THREADS

struct threadControl_t
{
	threadControl_t():/* output(false),  */finishedThreads(0) {};
	yafthreads::conditionVar_t countCV; //!< condition variable to signal main thread
//	bool output; //!< indicate if area needs to be output (if not the thread just finished)
	std::vector<renderArea_t> areas; //!< area to be output to e.g. blender, if any
	volatile int finishedThreads; //!< number of finished threads, lock countCV when increasing/reading!
};

#endif

__END_YAFRAY

#endif // Y_TILEDINTEGRATOR_H
