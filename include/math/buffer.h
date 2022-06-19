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

namespace yafaray {

template <typename T, unsigned char num_dimensions>
class Buffer //! Generic num_dimensions-dimensional buffer. Unrolled starting from the highest dimension goind down in the dimensions (i.e. in 2D an x,y buffer would be unrolled so x=0,y=0->pos=0, x=0,y=1->pos=1, x=1,y=0->pos=height*x + y
{
	public:
		Buffer() noexcept = default;
		explicit constexpr Buffer(const std::array<int, num_dimensions> &dimensions) noexcept { resize(dimensions); }
		constexpr void zero() noexcept { data_.clear(); resize(dimensions_); }
		constexpr void resize(const std::array<int, num_dimensions> &dimensions) noexcept;
		constexpr void fill(const T &val) noexcept { const size_t size_prev = data_.size(); data_.clear(); data_.resize(size_prev, val); }
		constexpr void set(const std::array<int, num_dimensions> &coordinates, const T &val) noexcept { data_[calculateDataPosition(coordinates)] = val; }
		constexpr void set(const std::array<int, num_dimensions> &coordinates, T &&val) noexcept { data_[calculateDataPosition(coordinates)] = std::move(val); }
		constexpr T get(const std::array<int, num_dimensions> &coordinates) const noexcept { return data_[calculateDataPosition(coordinates)]; }
		constexpr T &operator()(const std::array<int, num_dimensions> &coordinates) noexcept { return data_[calculateDataPosition(coordinates)]; }
		constexpr const T &operator()(const std::array<int, num_dimensions> &coordinates) const noexcept { return data_[calculateDataPosition(coordinates)]; }
		constexpr const std::array<int, num_dimensions> &getDimensions() const noexcept { return dimensions_; }

	private:
		constexpr size_t calculateDataPosition(const std::array<int, num_dimensions> &coordinates) const noexcept;
		alignas(8) std::array<int, num_dimensions> dimensions_;
		alignas(8) std::vector<T> data_;
};

template<typename T, unsigned char num_dimensions>
inline constexpr void Buffer<T, num_dimensions>::resize(const std::array<int, num_dimensions> &dimensions) noexcept
{
	dimensions_ = dimensions;
	size_t size = 1;
	for(const auto &dim : dimensions) size *= dim;
	data_.resize(size);
}

template<typename T, unsigned char num_dimensions>
inline constexpr size_t Buffer<T, num_dimensions>::calculateDataPosition(const std::array<int, num_dimensions> &coordinates) const noexcept
{
	if(num_dimensions == 2) return coordinates[0] * dimensions_[1] + coordinates[1];
	else if(num_dimensions == 1) return coordinates[0];
	else
	{
		size_t data_position = 0;
		for(unsigned char dimension_id = 0; dimension_id < num_dimensions; ++dimension_id)
		{
			if(dimension_id == num_dimensions - 1) data_position += coordinates[dimension_id];
			else
			{
				for(unsigned char dimension_id_2 = dimension_id + 1; dimension_id_2 < num_dimensions; ++dimension_id_2)
				{
					data_position += coordinates[dimension_id] * dimensions_[dimension_id_2];
				}
			}
		}
		return data_position;
	}
}

} //namespace yafaray

#endif // YAFARAY_BUFFER_H
