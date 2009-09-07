#ifndef Y_MEMORYIO_H
#define Y_MEMORYIO_H

#include <core_api/color.h>
#include <utilities/buffer.h>
#include <core_api/output.h>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT memoryIO_t : public colorOutput_t
{
	public:
		memoryIO_t(int resx, int resy, float* iMem);
		virtual bool putPixel(int x, int y, const float *c, int channels);
		void flush();
		virtual void flushArea(int x0, int y0, int x1, int y1) {}; // no tiled file format used...yet
		virtual ~memoryIO_t();
	protected:
		int sizex, sizey;
		float* imageMem;
};


__END_YAFRAY

#endif // Y_MEMORYIO_H
