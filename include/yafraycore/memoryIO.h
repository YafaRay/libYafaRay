#ifndef Y_MEMORYIO_H
#define Y_MEMORYIO_H

#include <utilities/buffer.h>
#include <core_api/output.h>

__BEGIN_YAFRAY

class renderPasses_t;

class YAFRAYCORE_EXPORT memoryIO_t : public colorOutput_t
{
	public:
		memoryIO_t(int resx, int resy, float* iMem);
		virtual bool putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, int idx, const colorA_t &color, bool alpha = true);
		virtual bool putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, const std::vector<colorA_t> &colExtPasses, bool alpha = true);
		void flush(int numView, const renderPasses_t *renderPasses);
		virtual void flushArea(int numView, int x0, int y0, int x1, int y1, const renderPasses_t *renderPasses) {}; // no tiled file format used...yet
		virtual ~memoryIO_t();
	protected:
		int sizex, sizey;
		float* imageMem;
};


__END_YAFRAY

#endif // Y_MEMORYIO_H
