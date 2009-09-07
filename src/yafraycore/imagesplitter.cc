#include <core_api/imagesplitter.h>
#include <iostream>

__BEGIN_YAFRAY

// currently only supports creation of scanrow-ordered tiles
// shuffling would of course be easy, but i don't find that too usefull really,
// it does maximum damage to the coherency gain and visual feedback is medicore too

imageSpliter_t::imageSpliter_t(int w, int h, int x0,int y0, int bsize): blocksize(bsize)
{
	int nx, ny;
	nx = (w+blocksize-1)/blocksize;
	ny = (h+blocksize-1)/blocksize;
	for(int j=0; j<ny; ++j)
	{
		for(int i=0; i<nx; ++i)
		{
			region_t r;
			r.x = x0 + i*blocksize;
			r.y = y0 + j*blocksize;
			r.w = std::min(blocksize, x0+w-r.x);
			r.h = std::min(blocksize, y0+h-r.y);
			regions.push_back(r);
		}
	}
}

bool imageSpliter_t::getArea(int n, renderArea_t &area)
{
	if(n<0 || n>=(int)regions.size()) return false;
	region_t &r = regions[n];
	area.X = r.x;
	area.Y = r.y;
	area.W = r.w;
	area.H = r.h;
	return true;
}

__END_YAFRAY
