#pragma once
/****************************************************************************
 *
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
 *
 */

#ifndef LIBYAFARAY_BUFFER_2D_H
#define LIBYAFARAY_BUFFER_2D_H

#include "math/buffer.h"
#include "geometry/vector.h"

namespace yafaray {

template <typename T>
class Buffer2D final : public Buffer<T, 2>
{
	public:
		explicit Buffer2D(const Size2i &size) : Buffer<T, 2>{size.getArray()} { }
		void set(const Point2i &point, const T &val) { Buffer<T, 2>::set(point.getArray(), val); }
		void set(const Point2i &point, T &&val) { Buffer<T, 2>::set(point.getArray(), std::move(val)); }
		T get(const Point2i &point) const { return Buffer<T, 2>::get(point.getArray()); }
		T &operator()(const Point2i &point) { return Buffer<T, 2>::operator()(point.getArray()); }
		const T &operator()(const Point2i &point) const { return Buffer<T, 2>::operator()(point.getArray()); }
		void clear() { Buffer<T, 2>::zero(); }
		int getWidth() const { return static_cast<int>(Buffer<T, 2>::getDimensions().at(0)); }
		int getHeight() const { return static_cast<int>(Buffer<T, 2>::getDimensions().at(1)); }
};

} //namespace yafaray

#endif //LIBYAFARAY_BUFFER_2D_H
