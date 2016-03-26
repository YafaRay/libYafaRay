#include <core_api/imagesplitter.h>
#include <iostream>

#include <algorithm>

__BEGIN_YAFRAY

// currently only supports creation of scanrow-ordered tiles
// shuffling would of course be easy, but i don't find that too usefull really,
// it does maximum damage to the coherency gain and visual feedback is medicore too

imageSpliter_t::imageSpliter_t(int w, int h, int x0,int y0, int bsize, tilesOrderType torder): blocksize(bsize), tilesorder(torder)
{
	int nx, ny;
	nx = (w+blocksize-1)/blocksize;
	ny = (h+blocksize-1)/blocksize;

	std::vector<region_t> regions_raw;

	for(int j=0; j<ny; ++j)
	{
		for(int i=0; i<nx; ++i)
		{
			region_t r;
			r.x = x0 + i*blocksize;
			r.y = y0 + j*blocksize;
			r.w = std::min(blocksize, x0+w-r.x);
			r.h = std::min(blocksize, y0+h-r.y);
			regions_raw.push_back(r);
		}
	}
	switch(tilesorder)
	{
		case RANDOM:	std::random_shuffle( regions_raw.begin(), regions_raw.end() );
		case LINEAR:
		default:	break;
	}
	
	for(size_t rn=0; rn<regions_raw.size(); ++rn)
	{
		if(blocksize <= 4 || rn<regions_raw.size()-16)	//If blocksize is more than 4, resubdivide the last 16 tiles so their block size is 4 (better CPU/thread usage in the last tiles to avoid having one big tile at the end with only 1 CPU thread)
		{
			regions.push_back(regions_raw[rn]);
		}
		else
		{
			int blocksize2 = 4;
			int nx2, ny2;
			nx2 = (regions_raw[rn].w+blocksize2-1)/blocksize2;
			ny2 = (regions_raw[rn].h+blocksize2-1)/blocksize2;

			for(int j=0; j<ny2; ++j)
			{
				for(int i=0; i<nx2; ++i)
				{
					region_t r;
					r.x = regions_raw[rn].x + i*blocksize2;
					r.y = regions_raw[rn].y + j*blocksize2;
					r.w = std::min(blocksize2, regions_raw[rn].x+regions_raw[rn].w-r.x);
					r.h = std::min(blocksize2, regions_raw[rn].y+regions_raw[rn].h-r.y);
					regions.push_back(r);
				}
			}
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
