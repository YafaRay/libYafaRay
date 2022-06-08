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
#include "geometry/matrix4.h"

BEGIN_YAFARAY

std::ostream &operator << (std::ostream &out, const Vec3 &v)
{
	out << "(" << v[Axis::X] << "," << v[Axis::Y] << "," << v[Axis::Z] << ")";
	return out;
}

std::ostream &operator << (std::ostream &out, const Point3 &p)
{
	out << "(" << p[Axis::X] << "," << p[Axis::Y] << "," << p[Axis::Z] << ")";
	return out;
}

bool  operator == (const Vec3 &a, const Vec3 &b)
{
	for(const auto axis : axis::spatial)
	{
		if(a[axis] != b[axis]) return false;
	}
	return true;
}

bool  operator != (const Vec3 &a, const Vec3 &b)
{
	for(const auto axis : axis::spatial)
	{
		if(a[axis] != b[axis]) return true;
	}
	return false;
}

Vec3 Vec3::randomVectorCone(const Vec3 &d, const Vec3 &u, const Vec3 &v, float cosang, float z_1, float z_2)
{
	const float t_1 = math::mult_pi_by_2<> * z_1, t_2 = 1.f - (1.f - cosang) * z_2;
	return (u * math::cos(t_1) + v * math::sin(t_1)) * math::sqrt(1.f - t_2 * t_2) + d * t_2;
}

Vec3 Vec3::randomVectorCone(const Vec3 &dir, float cangle, float r_1, float r_2)
{
	const auto [u, v]{createCoordsSystem(dir)};
	return Vec3::randomVectorCone(dir, u, v, cangle, r_1, r_2);
}

Vec3 Vec3::discreteVectorCone(const Vec3 &dir, float cangle, int sample, int square)
{
	const float r_1 = static_cast<float>(sample / square) / static_cast<float>(square);
	const float r_2 = static_cast<float>(sample % square) / static_cast<float>(square);
	const float tt = math::mult_pi_by_2<> * r_1;
	const float ss = math::acos(1.f - (1.f - cangle) * r_2);
	const Vec3 vx(math::cos(ss), math::sin(ss) * math::cos(tt), math::sin(ss) * math::sin(tt));
	const Vec3 i(1.f, 0.f, 0.f);
	Matrix4 m(1.f);
	if((std::abs(dir.vec_[1]) > 0.f) || (std::abs(dir.vec_[2]) > 0.f))
	{
		m[0][0] = dir.vec_[0];
		m[1][0] = dir.vec_[1];
		m[2][0] = dir.vec_[2];
		Vec3 c = i ^ dir;
		c.normalize();
		m[0][1] = c.vec_[0];
		m[1][1] = c.vec_[1];
		m[2][1] = c.vec_[2];
		c = dir ^ c;
		c.normalize();
		m[0][2] = c.vec_[0];
		m[1][2] = c.vec_[1];
		m[2][2] = c.vec_[2];
	}
	else if(dir.vec_[0] < 0.f) m[0][0] = -1.f;
	return m * vx;
}

END_YAFARAY
