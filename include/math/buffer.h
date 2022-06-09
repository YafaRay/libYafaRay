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

#include "common/yafaray_common.h"
#include <vector>
#include <array>

BEGIN_YAFARAY

template <class T, unsigned char n>
class Buffer //! Generic n-dimensional buffer. Unrolled starting from the highest dimension goind down in the dimensions (i.e. in 2D an x,y buffer would be unrolled so x=0,y=0->pos=0, x=0,y=1->pos=1, x=1,y=0->pos=height*x + y
{
	public:
		Buffer() noexcept = default;
		explicit constexpr Buffer(const std::array<size_t, n> &dimensions) noexcept { resize(dimensions); }
		constexpr void zero() noexcept { data_.clear(); resize(dimensions_); }
		constexpr void resize(const std::array<size_t, n> &dimensions) noexcept;
		constexpr void fill(const T &val) noexcept { const size_t size_prev = data_.size(); data_.clear(); data_.resize(size_prev, val); }
		constexpr void set(const std::array<size_t, n> &coordinates, const T &val) noexcept { data_[calculateDataPosition(coordinates)] = val; }
		constexpr void set(const std::array<size_t, n> &coordinates, T &&val) noexcept { data_[calculateDataPosition(coordinates)] = std::move(val); }
		constexpr T get(const std::array<size_t, n> &coordinates) const noexcept { return data_[calculateDataPosition(coordinates)]; }
		constexpr T &operator()(const std::array<size_t, n> &coordinates) noexcept { return data_[calculateDataPosition(coordinates)]; }
		constexpr const T &operator()(const std::array<size_t, n> &coordinates) const noexcept { return data_[calculateDataPosition(coordinates)]; }
		constexpr const std::array<size_t, n> &getDimensions() const noexcept { return dimensions_; }

	private:
		constexpr size_t calculateDataPosition(const std::array<size_t, n> &coordinates) const noexcept;
		alignas(8) std::array<size_t, n> dimensions_;
		alignas(8) std::vector<T> data_;
};

template<class T, unsigned char n>
inline constexpr void Buffer<T, n>::resize(const std::array<size_t, n> &dimensions) noexcept
{
	dimensions_ = dimensions;
	size_t size = 1;
	for(const auto &dim : dimensions) size *= dim;
	data_.resize(size);
}

template<class T, unsigned char n>
inline constexpr size_t Buffer<T, n>::calculateDataPosition(const std::array<size_t, n> &coordinates) const noexcept
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
