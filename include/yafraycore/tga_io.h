
#ifndef Y_TARGAIO_H
#define Y_TARGAIO_H

#include <yafray_config.h>

#include <core_api/output.h>
#include <string>


__BEGIN_YAFRAY

/*! color output to write to tga file
*/
class YAFRAYCORE_EXPORT outTga_t : public colorOutput_t
{
	public:
		outTga_t(int resx, int resy, const char *fname, bool sv_alpha=false);
		//virtual bool putPixel(int x, int y, const color_t &c, 
		//		CFLOAT alpha=0,PFLOAT depth=0);
		virtual bool putPixel(int x, int y, const float *c, int channels);
		virtual void flush() { savetga(outfile.c_str()); }
		virtual void flushArea(int x0, int y0, int x1, int y1) {}; // no tiled file format...useless
		virtual ~outTga_t();
	protected:
		outTga_t(const outTga_t &o) {}; //forbidden
		bool savetga(const char* filename);
		bool save_alpha;
		unsigned char *data;
		unsigned char *alpha_buf;
		int sizex, sizey;
		std::string outfile;
};

__END_YAFRAY


#endif // Y_TARGAIO_H
