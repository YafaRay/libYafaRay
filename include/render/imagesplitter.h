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

#include "common/enum.h"
#include "common/enum_map.h"
#include <vector>

namespace yafaray {

struct RenderArea
{
	int id_ = -1; //!< Area id, for client software to clearly reference flushed areas respect to previously highlighted areas
	int x_, y_, w_, h_; //!< Coordinates and dimensions of the area
	int sx_0_, sx_1_, sy_0_, sy_1_; //!< safe area unaffected by samples outside, does not need to be thread locked
};

/*!	Splits the image to be rendered into pieces, e.g. "buckets" for
	different threads.
	CAUTION! Some methods need to be thread save!
*/
class ImageSplitter final
{
	public:
		struct TilesOrderType : public Enum<TilesOrderType>
		{
			enum : ValueType_t { CentreRandom, Linear, Random  };
			inline static const EnumMap<ValueType_t> map_{{
					{"centre", CentreRandom, ""},
					{"linear", Linear, ""},
					{"random", Random, ""},
				}};
		};
		struct Region
		{
			int x_, y_, w_, h_;
		};
		ImageSplitter() = default;
		ImageSplitter(int w, int h, int x_0, int y_0, int bsize, TilesOrderType torder, int nthreads);
		/* return the n-th area to be rendered.
			\return false if n is out of range, true otherwise
		*/
		bool getArea(int n, RenderArea &area);

		bool empty() const {return regions_.empty();};
		int size() const {return static_cast<int>(regions_.size());};

	private:
		int blocksize_;
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


} //namespace yafaray

#endif // YAFARAY_IMAGESPLITTER_H
