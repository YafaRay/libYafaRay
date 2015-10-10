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
bool memoryIO_t::putPixel(int numView, int x, int y, const renderPasses_t &renderPasses, const std::vector<colorA_t> &colExtPasses, bool alpha)
{
	imageMem[(x + sizex * y) * 4 + 0] = colExtPasses.at(0).R;
	imageMem[(x + sizex * y) * 4 + 0] = colExtPasses.at(0).G;
	imageMem[(x + sizex * y) * 4 + 0] = colExtPasses.at(0).B;
	if(!alpha) imageMem[(x + sizex * y) * 4 + 3] = 1.f;

	return true;
}

void memoryIO_t::flush(int numView, const renderPasses_t &renderPasses) { }

memoryIO_t::~memoryIO_t() { }


__END_YAFRAY

