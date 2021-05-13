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

#ifndef YAFARAY_AXIS_H
#define YAFARAY_AXIS_H

#include "yafaray_conf.h"
#include <array>

BEGIN_YAFARAY

class Axis
{
	public:
		enum Code : int { None = -1, X = 0, Y, Z };
		Axis(int axis) : axis_(axis) { }
		bool operator == (Code axis_code) const { return axis_ == axis_code; }
		bool operator != (Code axis_code) const { return axis_ != axis_code; }
		int get() const { return axis_; }
		int next() const { return next(axis_); }
		int prev() const { return prev(axis_); }
		static constexpr int next(int axis) { return (axis + 1) % 3; }
		static constexpr int prev(int axis) { return ((axis + 3) - 1) % 3; }
//		static constexpr int next(int axis) { return axis_lut_[axis][1]; }
//		static constexpr int prev(int axis) { return axis_lut_[axis][2]; }
	private:
		int axis_;
//		static const std::array<std::array<int, 3>, 3> axis_lut_;
};

struct ClipPlane
{
	enum class Pos: int { None, Lower, Upper };
	ClipPlane(Pos pos = Pos::None) : pos_(pos) { }
	ClipPlane(int axis, Pos pos) : axis_(axis), pos_(pos) { }
	int axis_ = Axis::None;
	Pos pos_ = Pos::None;
};

END_YAFARAY

#endif //YAFARAY_AXIS_H
