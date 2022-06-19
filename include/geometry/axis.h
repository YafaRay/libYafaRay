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

#ifndef LIBYAFARAY_AXIS_H
#define LIBYAFARAY_AXIS_H

#include "common/yafaray_common.h"
#include <array>

namespace yafaray {

enum class Axis : unsigned char { X = 0, Y, Z, T, None };

namespace axis
{
	static inline constexpr std::array<Axis, 3> spatial {Axis::X, Axis::Y, Axis::Z};
	static inline constexpr Axis temporal {Axis::T};
	static inline constexpr Axis getNextSpatial(Axis current_axis)
	{
		switch(current_axis)
		{
			case Axis::X: return Axis::Y;
			case Axis::Y: return Axis::Z;
			case Axis::Z: return Axis::X;
			default: return Axis::None;
		}
	}
	static inline constexpr Axis getPrevSpatial(Axis current_axis)
	{
		switch(current_axis)
		{
			case Axis::X: return Axis::Z;
			case Axis::Y: return Axis::X;
			case Axis::Z: return Axis::Y;
			default: return Axis::None;
		}
	}
	static inline constexpr unsigned char getId(Axis axis) { return static_cast<unsigned char>(axis); }
}

} //namespace yafaray

#endif//LIBYAFARAY_AXIS_H
