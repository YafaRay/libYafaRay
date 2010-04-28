#ifndef Y_EXR_IO_H
#define Y_EXR_IO_H

#include <core_api/color.h>
#include <utilities/buffer.h>
#include <core_api/output.h>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT outEXR_t : public colorOutput_t
{
	public:
		outEXR_t(int resx, int resy, const char *fname, const std::string &exr_flags);
		virtual bool putPixel(int x, int y, const float *c, bool alpha = true, bool depth = false, float z = 0.f);
		void flush() { SaveEXR(); }
		virtual void flushArea(int x0, int y0, int x1, int y1) {}; // no tiled file format used...yet
		virtual ~outEXR_t();
	protected:
		outEXR_t(const outEXR_t &o) {}; //forbidden
		bool SaveEXR();
		fcBuffer_t* fbuf;
		gBuf_t<float, 1>* zbuf;
		int sizex, sizey;
		const char* filename;
		std::string out_flags;
};

YAFRAYCORE_EXPORT fcBuffer_t* loadEXR(const char* fname);

__END_YAFRAY

#endif // Y_EXR_OUT_H
