#include <core_api/color.h>
#include <utilities/buffer.h>
#include <core_api/output.h>
#include <yafraycore/memoryIO.h>
#include <cstdlib>

__BEGIN_YAFRAY


memoryIO_t::memoryIO_t ( int resx, int resy, float* iMem )
{
	sizex = resx;
	sizey = resy;
	imageMem = iMem; // iMem must be a valid pointer to memory of the size: sizex * sizey * 4 * sizeof(float)
}

bool memoryIO_t::putPixel ( int x, int y, const float *c, int channels )
{
	for (int i = 0; i < 4; ++i) {
		imageMem[(x + sizex * y) * 4 + i] = c[i];
	}
	return true;
}

void memoryIO_t::flush() { }

memoryIO_t::~memoryIO_t() { }


__END_YAFRAY

