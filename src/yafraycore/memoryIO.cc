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

// Depth channel support?
bool memoryIO_t::putPixel ( int x, int y, const float *c, bool alpha, bool depth, float z )
{
	for (int i = 0; i < 4; ++i)
	{
		if(!alpha && i == 3) imageMem[(x + sizex * y) * 4 + i] = 1.f;
		else imageMem[(x + sizex * y) * 4 + i] = c[i];
	}
	return true;
}

void memoryIO_t::flush() { }

memoryIO_t::~memoryIO_t() { }


__END_YAFRAY

