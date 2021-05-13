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

#ifndef YAFARAY_BUFFER_H
#define YAFARAY_BUFFER_H

#include "yafaray_conf.h"
#include <vector>
#include <array>

BEGIN_YAFARAY

template <class T, unsigned char n>
class Buffer //! Generic n-dimensional buffer. Unrolled starting from the highest dimension goind down in the dimensions (i.e. in 2D an x,y buffer would be unrolled so x=0,y=0->pos=0, x=0,y=1->pos=1, x=1,y=0->pos=height*x + y
{
	public:
		Buffer() = default;
		Buffer(const std::array<size_t, n> &dimensions) { resize(dimensions); }
		void zero() { data_.clear(); resize(dimensions_); }
		void resize(const std::array<size_t, n> &dimensions);
		void fill(const T &val) { for(size_t i = 0; i < data_.size(); ++i) data_[i] = val; }
		void set(const std::array<size_t, n> &coordinates, const T &val) { data_[calculateDataPosition(coordinates)] = val; }
		T get(const std::array<size_t, n> &coordinates) const { return data_[calculateDataPosition(coordinates)]; }
		T &operator()(const std::array<size_t, n> &coordinates) { return data_[calculateDataPosition(coordinates)]; }
		const T &operator()(const std::array<size_t, n> &coordinates) const { return data_[calculateDataPosition(coordinates)]; }
		const std::array<size_t, n> &getDimensions() const { return dimensions_; }

	private:
		size_t calculateDataPosition(const std::array<size_t, n> &coordinates) const;

		std::array<size_t, n> dimensions_;
		std::vector<T> data_;
};

template<class T, unsigned char n>
inline void Buffer<T, n>::resize(const std::array<size_t, n> &dimensions)
{
	dimensions_ = dimensions;
	size_t size = 1;
	for(const auto &dim : dimensions) size *= dim;
	data_.resize(size);
}

template<class T, unsigned char n>
inline size_t Buffer<T, n>::calculateDataPosition(const std::array<size_t, n> &coordinates) const
{
	if(n == 2) return coordinates[0] * dimensions_[1] + coordinates[1];
	else if(n == 1) return coordinates[0];
	else
	{
		size_t data_position = 0;
		for(size_t coord_idx = 0; coord_idx < n; ++coord_idx)
		{
			if(coord_idx == n - 1) data_position += coordinates[coord_idx];
			else
			{
				for(size_t dimension_idx = coord_idx + 1; dimension_idx < n; ++dimension_idx)
				{
					data_position += coordinates[coord_idx] * dimensions_[dimension_idx];
				}
			}
		}
		return data_position;
	}
}

END_YAFARAY

#endif // YAFARAY_BUFFER_H
