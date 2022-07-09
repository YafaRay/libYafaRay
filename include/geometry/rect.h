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

#ifndef LIBYAFARAY_RECT_H
#define LIBYAFARAY_RECT_H

#include "geometry/vector.h"

namespace yafaray {

class Rect final
{
	public:
		Rect(const Point2i &point_start, const Point2i &point_end) : point_start_{point_start}, point_end_{point_end}, size_{point_end - point_start} { }
		Rect(Point2i &&point_start, Point2i &&point_end) : point_start_{std::move(point_start)}, point_end_{std::move(point_end)}, size_{point_end_ - point_start_ + Size2i{{1, 1}}} { }
		Rect(const Point2i &point_start, const Size2i &size) : point_start_{point_start}, point_end_{point_start + size - Size2i{{1, 1}}}, size_{size} { }
		Rect(Point2i &&point_start, Size2i &&size) : point_start_{std::move(point_start)}, point_end_{point_start_ + size - Size2i{{1, 1}}}, size_{std::move(size)} { }
		Rect(const Rect &rect) = default;
		Rect(Rect &&rect) = default;
		Rect &operator=(const Rect &rect) = default;
		Rect &operator=(Rect &&rect) = default;
		Point2i getPointStart() const { return point_start_; }
		Point2i getPointEnd() const { return point_end_; }
		Size2i getSize() const { return size_; }
		int getWidth() const { return size_[Axis::X]; }
		int getHeight() const { return size_[Axis::Y]; }
		int getArea() const { return getWidth() * getHeight(); }
		int getIndex(const Point2i &point) const;

	private:
		Point2i point_start_;
		Point2i point_end_;
		Size2i size_;
};

inline int Rect::getIndex(const Point2i &point) const
{
	return (point[Axis::Y] - point_start_[Axis::Y]) * size_[Axis::X] + point[Axis::X] - point_start_[Axis::X];
}

} //namespace yafaray

#endif //LIBYAFARAY_RECT_H
