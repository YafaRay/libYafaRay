/****************************************************************************
 *
 *      vector3d.cc: Vector 3d and point manipulation implementation
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002 Alejandro Conty Estévez
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

#include "geometry/vector.h"
#include "geometry/matrix.h"

namespace yafaray {

template <typename T, size_t N>
Vec<T, 3> Vec<T, N>::discreteVectorCone(const Vec<T, 3> &dir, T cos_angle, int sample, int square)
{
	const T r_1{static_cast<T>(sample / square) / static_cast<T>(square)};
	const T r_2{static_cast<T>(sample % square) / static_cast<T>(square)};
	const T tt{math::mult_pi_by_2<T> * r_1};
	const T ss{math::acos(T{1} - (T{1} - cos_angle) * r_2)};
	const Vec<T, N> vx(math::cos(ss), math::sin(ss) * math::cos(tt), math::sin(ss) * math::sin(tt));
	const Vec<T, N> i(T{1}, T{0}, T{0});
	Matrix4f m(T{1});
	if((std::abs(dir.array_[1]) > T{0}) || (std::abs(dir.array_[2]) > T{0}))
	{
		m[0][0] = dir.array_[0];
		m[1][0] = dir.array_[1];
		m[2][0] = dir.array_[2];
		Vec<T, N> c{i ^ dir};
		c.normalize();
		m[0][1] = c.array_[0];
		m[1][1] = c.array_[1];
		m[2][1] = c.array_[2];
		c = dir ^ c;
		c.normalize();
		m[0][2] = c.array_[0];
		m[1][2] = c.array_[1];
		m[2][2] = c.array_[2];
	}
	else if(dir.array_[0] < T{0}) m[0][0] = T{-1};
	return m * vx;
}

} //namespace yafaray
