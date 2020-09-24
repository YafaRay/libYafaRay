/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "render/imagesplitter.h"
#include <iostream>
#include <math.h>
#include <algorithm>

BEGIN_YAFARAY

// currently only supports creation of scanrow-ordered tiles
// shuffling would of course be easy, but i don't find that too usefull really,
// it does maximum damage to the coherency gain and visual feedback is medicore too

ImageSplitter::ImageSplitter(int w, int h, int x_0, int y_0, int bsize, TilesOrderType torder, int nthreads): blocksize_(bsize), tilesorder_(torder)
{
	int nx, ny;
	nx = (w + blocksize_ - 1) / blocksize_;
	ny = (h + blocksize_ - 1) / blocksize_;

	std::vector<Region> regions_raw;

	for(int j = 0; j < ny; ++j)
	{
		for(int i = 0; i < nx; ++i)
		{
			Region r;
			r.x_ = x_0 + i * blocksize_;
			r.y_ = y_0 + j * blocksize_;
			r.w_ = std::min(blocksize_, x_0 + w - r.x_);
			r.h_ = std::min(blocksize_, y_0 + h - r.y_);
			regions_raw.push_back(r);
		}
	}

	switch(tilesorder_)
	{
		case Random:		std::random_shuffle(regions_raw.begin(), regions_raw.end());
		case CentreRandom:	std::random_shuffle(regions_.begin(), regions_.end());
			std::sort(regions_.begin(), regions_.end(), ImageSpliterCentreSorter(w, h, x_0, y_0));
		case Linear:		break;
		default:			break;
	}

	std::vector<Region> regions_subdivided;

	for(size_t rn = 0; rn < regions_raw.size(); ++rn)
	{
		if(nthreads == 1 || blocksize_ <= 4 || rn < regions_raw.size() - 2 * nthreads)	//If blocksize is more than 4, resubdivide the last (2 x nunber of threads) so their block size is progressively smaller (better CPU/thread usage in the last tiles to avoid/mitigate having one big tile at the end with only 1 CPU thread)
		{
			regions_.push_back(regions_raw[rn]);
		}
		else
		{
			int blocksize_2 = blocksize_;
			if(rn > regions_raw.size() - nthreads) blocksize_2 = std::max(4, blocksize_ / 4);
			else if(rn <= regions_raw.size() - nthreads) blocksize_2 = std::max(4, blocksize_ / 2);
			int nx_2, ny_2;
			nx_2 = (regions_raw[rn].w_ + blocksize_2 - 1) / blocksize_2;
			ny_2 = (regions_raw[rn].h_ + blocksize_2 - 1) / blocksize_2;

			for(int j = 0; j < ny_2; ++j)
			{
				for(int i = 0; i < nx_2; ++i)
				{
					Region r;
					r.x_ = regions_raw[rn].x_ + i * blocksize_2;
					r.y_ = regions_raw[rn].y_ + j * blocksize_2;
					r.w_ = std::min(blocksize_2, regions_raw[rn].x_ + regions_raw[rn].w_ - r.x_);
					r.h_ = std::min(blocksize_2, regions_raw[rn].y_ + regions_raw[rn].h_ - r.y_);
					regions_subdivided.push_back(r);
				}
			}
		}
	}

	switch(tilesorder_)
	{
		case Random:		std::random_shuffle(regions_.begin(), regions_.end());
			std::random_shuffle(regions_subdivided.begin(), regions_subdivided.end());
			break;
		case CentreRandom:	std::random_shuffle(regions_.begin(), regions_.end());
			std::sort(regions_.begin(), regions_.end(), ImageSpliterCentreSorter(w, h, x_0, y_0));
			std::random_shuffle(regions_subdivided.begin(), regions_subdivided.end());
			std::sort(regions_subdivided.begin(), regions_subdivided.end(), ImageSpliterCentreSorter(w, h, x_0, y_0));
			break;
		case Linear: 		break;
		default:			break;
	}

	regions_.insert(regions_.end(), regions_subdivided.begin(), regions_subdivided.end());
}

bool ImageSplitter::getArea(int n, RenderArea &area)
{
	if(n < 0 || n >= (int)regions_.size()) return false;
	Region &r = regions_[n];
	area.x_ = r.x_;
	area.y_ = r.y_;
	area.w_ = r.w_;
	area.h_ = r.h_;
	return true;
}

END_YAFARAY
