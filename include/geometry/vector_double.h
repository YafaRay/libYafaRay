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

#ifndef YAFARAY_VECTOR_DOUBLE_H
#define YAFARAY_VECTOR_DOUBLE_H

#include "common/yafaray_common.h"
#include "geometry/clip_plane.h"
#include <iomanip>
#include <limits>
#include <sstream>

BEGIN_YAFARAY

class Vec3Double
{
	public:
		Vec3Double() = default;
		Vec3Double(const Vec3Double &v) = default;
		Vec3Double(Vec3Double &&v) = default;
		Vec3Double(double x, double y, double z) : vec_{x, y, z} { }
		Vec3Double &operator = (const Vec3Double &v) = default;
		Vec3Double &operator = (Vec3Double &&v) = default;
		double operator[](Axis axis) const { return vec_[axis::getId(axis)]; }
		double &operator[](Axis axis) { return vec_[axis::getId(axis)]; }
		std::string print() const;
		static Vec3Double cross(const Vec3Double &v_1, const Vec3Double &v_2);
		static double dot(const Vec3Double &v_1, const Vec3Double &v_2);
		static Vec3Double sub(const Vec3Double &v_1, const Vec3Double &v_2);

	private:
		alignas(8) std::array<double, 3> vec_;
};

inline std::string Vec3Double::print() const
{
	std::stringstream ss;
	ss << std::setprecision(std::numeric_limits<double>::digits10 + 1);
	ss << "<x=" <<  vec_[0] << ",y=" << vec_[1] << ",z=" << vec_[2] << ">";
	return ss.str();
}

inline Vec3Double Vec3Double::cross(const Vec3Double &v_1, const Vec3Double &v_2)
{
	return {
			v_1[Axis::Y] * v_2[Axis::Z] - v_1[Axis::Z] * v_2[Axis::Y],
			v_1[Axis::Z] * v_2[Axis::X] - v_1[Axis::X] * v_2[Axis::Z],
			v_1[Axis::X] * v_2[Axis::Y] - v_1[Axis::Y] * v_2[Axis::X]
	};
}

inline double Vec3Double::dot(const Vec3Double &v_1, const Vec3Double &v_2)
{
	return v_1[Axis::X] * v_2[Axis::X] + v_1[Axis::Y] * v_2[Axis::Y] + v_1[Axis::Z] * v_2[Axis::Z];
}

inline Vec3Double Vec3Double::sub(const Vec3Double &v_1, const Vec3Double &v_2)
{
	return {v_1[Axis::X] - v_2[Axis::X], v_1[Axis::Y] - v_2[Axis::Y], v_1[Axis::Z] - v_2[Axis::Z]};
}

END_YAFARAY

#endif //YAFARAY_VECTOR_DOUBLE_H
