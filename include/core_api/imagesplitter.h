#pragma once
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

#ifndef YAFARAY_IMAGESPLITTER_H
#define YAFARAY_IMAGESPLITTER_H

#include <yafray_constants.h>

#include <vector>
#include <cmath>

BEGIN_YAFRAY

struct RenderArea
{
	RenderArea(int x, int y, int w, int h): x_(x), y_(y), w_(w), h_(h),
											real_x_(x), real_y_(y), real_w_(w), real_h_(h), resample_(w * h)
	{};
	RenderArea() {};

	void set(int x, int y, int w, int h)
	{
		real_x_ = x_ = x;
		real_y_ = y_ = y;
		real_w_ = w_ = w;
		real_h_ = h_ = h;
		//		image.resize(w*h);
		//		depth.resize(w*h);
		resample_.resize(w * h);
	}
	void setReal(int x, int y, int w, int h)
	{
		real_x_ = x;
		real_y_ = y;
		real_w_ = w;
		real_h_ = h;
	}
	bool checkResample(float threshold);
	//	bool out(colorOutput_t &o);

	//	Rgba & imagePixel(int x,int y) {return image[(y-Y)*W+(x-X)];};
	//	float & depthPixel(int x,int y)   {return depth[(y-Y)*W+(x-X)];};
	bool  resamplePixel(int x, int y)  {return resample_[(y - y_) * w_ + (x - x_)];};

	int x_, y_, w_, h_, real_x_, real_y_, real_w_, real_h_;
	int sx_0_, sx_1_, sy_0_, sy_1_; //!< safe area, i.e. region unaffected by samples outside (needs to be set by ImageFilm_t)
	//	std::vector<Rgba> image;
	//	std::vector<float> depth;
	std::vector<bool> resample_;
};

/*!	Splits the image to be rendered into pieces, e.g. "buckets" for
	different threads.
	CAUTION! Some methods need to be thread save!
*/
class ImageSplitter final
{
	public:
		enum TilesOrderType { Linear, Random, CentreRandom };
		struct Region
		{
			int x_, y_, w_, h_;
			//			int rx,ry,rw,rh;
		};
		ImageSplitter() {};
		ImageSplitter(int w, int h, int x_0, int y_0, int bsize, TilesOrderType torder, int nthreads);
		/* return the n-th area to be rendered.
			\return false if n is out of range, true otherwise
		*/
		bool getArea(int n, RenderArea &area);

		bool empty() const {return regions_.empty();};
		int size() const {return regions_.size();};

	private:
		int width_, height_, blocksize_;
		std::vector<Region> regions_;
		TilesOrderType tilesorder_;
};

class ImageSpliterCentreSorter final
{
	public:
		ImageSpliterCentreSorter(int image_w, int image_h, int image_x_0, int image_y_0) : image_w_(image_w), image_h_(image_h), image_x_0_(image_x_0), image_y_0_(image_y_0) {}
		bool operator()(ImageSplitter::Region const &a, ImageSplitter::Region const &b) const
		{
			return ((a.x_ - image_x_0_ - image_w_ / 2) * (a.x_ - image_x_0_ - image_w_ / 2) + (a.y_ - image_y_0_ - image_h_ / 2) * (a.y_ - image_y_0_ - image_h_ / 2)) < ((b.x_ - image_x_0_ - image_w_ / 2) * (b.x_ - image_x_0_ - image_w_ / 2) + (b.y_ - image_y_0_ - image_h_ / 2) * (b.y_ - image_y_0_ - image_h_ / 2));
		}

	private:
		int image_w_, image_h_, image_x_0_, image_y_0_;

};


END_YAFRAY

#endif // YAFARAY_IMAGESPLITTER_H
