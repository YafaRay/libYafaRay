#pragma once
/****************************************************************************
 *
 *      vector3d.h: Vector 3d and point representation and manipulation api
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
#ifndef YAFARAY_VECTOR_H
#define YAFARAY_VECTOR_H

#include "common/yafaray_common.h"
#include "math/math.h"
#include "math/random.h"
#include <iostream>
#include <array>

BEGIN_YAFARAY

class Point3;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
class Vec3
{
	public:
		Vec3() = default;
		explicit Vec3(float f): vec_{f, f, f} {  }
		Vec3(float x, float y, float z = 0.f): vec_{x, y, z} { }
		Vec3(const Vec3 &v) = default;
		Vec3(Vec3 &&v) = default;
		explicit Vec3(const Point3 &p);
		explicit Vec3(Point3 &&p);

		float x() const { return vec_[0]; }
		float y() const { return vec_[1]; }
		float z() const { return vec_[2]; }
		float &x() { return vec_[0]; }
		float &y() { return vec_[1]; }
		float &z() { return vec_[2]; }

		void set(float x, float y, float z = 0.f) { vec_ = {x, y, z}; }
		Vec3 &normalize();
		Vec3 &reflect(const Vec3 &normal);
		float normLen(); // normalizes and returns length
		float normLenSqr(); // normalizes and returns length squared
		float lengthSqr() const { return vec_[0] * vec_[0] + vec_[1] * vec_[1] + vec_[2] * vec_[2]; }
		float length() const { return math::sqrt(lengthSqr()); }
		bool null() const { return ((vec_[0] == 0.f) && (vec_[1] == 0.f) && (vec_[2] == 0.f)); }
		float sinFromVectors(const Vec3 &v) const;

		Vec3 &operator = (const Vec3 &s) = default;
		Vec3 &operator = (Vec3 &&s) = default;
		Vec3 &operator +=(const Vec3 &s) { for(size_t i = 0; i < 3; ++i) vec_[i] += s.vec_[i]; return *this;}
		Vec3 &operator -=(const Vec3 &s) { for(size_t i = 0; i < 3; ++i) vec_[i] -= s.vec_[i];  return *this;}
		Vec3 &operator /=(float s) { for(float &v : vec_) v /= s;  return *this;}
		Vec3 &operator *=(float s) { for(float &v : vec_) v *= s;  return *this;}
		float operator[](size_t i) const { return vec_[i]; }
		float &operator[](size_t i) { return vec_[i]; }

		static Vec3 reflectDir(const Vec3 &normal, const Vec3 &v);
		static bool refract(const Vec3 &n, const Vec3 &wi, Vec3 &wo, float ior);
		static void fresnel(const Vec3 &i, const Vec3 &n, float ior, float &kr, float &kt);
		static void fastFresnel(const Vec3 &i, const Vec3 &n, float iorf, float &kr, float &kt);
		static std::pair<Vec3, Vec3> createCoordsSystem(const Vec3 &normal);
		static void shirleyDisk(float r_1, float r_2, float &u, float &v);
		static Vec3 randomSpherical(FastRandom &fast_random);
		static Vec3 randomVectorCone(const Vec3 &d, const Vec3 &u, const Vec3 &v, float cosang, float z_1, float z_2);
		static Vec3 randomVectorCone(const Vec3 &dir, float cosangle, float r_1, float r_2);
		static Vec3 discreteVectorCone(const Vec3 &dir, float cangle, int sample, int square);

	private:
		alignas(4) std::array<float, 3> vec_;
};

class Point3 final : public Vec3
{
	public:
		Point3() = default;
		Point3(float x, float y, float z = 0.f) : Vec3{x, y, z} { }
		Point3(const Point3 &p) = default;
		Point3(Point3 &&p) = default;
		explicit Point3(const Vec3 &v): Vec3{v} { }
		explicit Point3(Vec3 &&v): Vec3{std::move(v)} { }
		Point3 &operator = (const Point3 &s) = default;
		Point3 &operator = (Point3 &&s) = default;
		static Point3 mult(const Point3 &a, const Vec3 &b);
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

inline Vec3::Vec3(const Point3 &p): vec_{p.vec_} { }
inline Vec3::Vec3(Point3 &&p): vec_{std::move(p.vec_)} { }

std::ostream &operator << (std::ostream &out, const Vec3 &v);
std::ostream &operator << (std::ostream &out, const Point3 &p);

inline float operator * (const Vec3 &a, const Vec3 &b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline Vec3 operator * (float f, const Vec3 &v)
{
	return {f * v[0], f * v[1], f * v[2]};
}

inline Vec3 operator * (const Vec3 &v, float f)
{
	return {f * v[0], f * v[1], f * v[2]};
}

inline Point3 operator * (float f, const Point3 &p)
{
	return {f * p[0], f * p[1], f * p[2]};
}

inline Vec3 operator / (const Vec3 &v, float f)
{
	return {v[0] / f, v[1] / f, v[2] / f};
}

inline Point3 operator / (const Point3 &p, float f)
{
	return {p[0] / f, p[1] / f, p[2] / f};
}

inline Point3 operator * (const Point3 &p, float f)
{
	return {p[0] * f, p[1] * f, p[2] * f};
}

inline Vec3 operator / (float f, const Vec3 &v)
{
	return {v[0] / f, v[1] / f, v[2] / f};
}

inline Vec3 operator ^ (const Vec3 &a, const Vec3 &b)
{
	return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

inline Vec3 operator - (const Vec3 &a, const Vec3 &b)
{
	return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline Vec3 operator - (const Point3 &a, const Point3 &b)
{
	return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline Point3 operator - (const Point3 &a, const Vec3 &b)
{
	return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline Vec3 operator - (const Vec3 &v)
{
	return {-v[0], -v[1], -v[2]};
}

inline Vec3 operator + (const Vec3 &a, const Vec3 &b)
{
	return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline Point3 operator + (const Point3 &a, const Point3 &b)
{
	return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline Point3 operator + (const Point3 &a, const Vec3 &b)
{
	return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline bool operator == (const Point3 &a, const Point3 &b)
{
	return ((a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]));
}

bool operator == (const Vec3 &a, const Vec3 &b);
bool operator != (const Vec3 &a, const Vec3 &b);

inline Point3 Point3::mult(const Point3 &a, const Vec3 &b)
{
	return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

inline Vec3 &Vec3::normalize()
{
	float len = lengthSqr();
	if(len != 0.f)
	{
		len = 1.f / math::sqrt(len);
		for(auto &v : vec_) v *= len;
	}
	return *this;
}

inline float Vec3::sinFromVectors(const Vec3 &v) const
{
	const float div = (length() * v.length()) * 0.99999f + 0.00001f;
	float asin_argument = ((*this ^ v).length() / div) * 0.99999f;
	//Fix to avoid black "nan" areas when this argument goes slightly over +1.0. Why that happens in the first place, maybe floating point rounding errors?
	if(asin_argument > 1.f) asin_argument = 1.f;
	return math::asin(asin_argument);
}

/** Vector reflection.
 *  Reflects the vector onto a surface whose normal is \a n
 *  @param	normal Surface normal
 *  @warning	\a n must be unit vector!
 *  @note	Lynn's formula: R = 2*(V dot N)*N -V (http://www.3dkingdoms.com/weekly/weekly.php?a=2)
 */
inline Vec3 &Vec3::reflect(const Vec3 &normal)
{
	const float vn = 2.f * (vec_[0] * normal.vec_[0] + vec_[1] * normal.vec_[1] + vec_[2] * normal.vec_[2]);
	for(size_t i = 0; i < 3; ++i) vec_[i] = vn * normal.vec_[i] - vec_[i];
	return *this;
}

inline float Vec3::normLen() {
	float vl = lengthSqr();
	if(vl != 0.f)
	{
		vl = math::sqrt(vl);
		const float d = 1.f / vl;
		for(float &v : vec_) v *= d;
	}
	return vl;
}

inline float Vec3::normLenSqr() {
	const float vl = lengthSqr();
	if(vl != 0.f)
	{
		const float d = 1.f / math::sqrt(vl);
		for(float &v : vec_) v *= d;
	}
	return vl;
}

inline Vec3 Vec3::reflectDir(const Vec3 &normal, const Vec3 &v)
{
	const float vn = v * normal;
	if(vn < 0.f) return -v;
	return 2.f * vn * normal - v;
}

inline std::pair<Vec3, Vec3> Vec3::createCoordsSystem(const Vec3 &normal)
{
	if((normal.vec_[0] == 0.f) && (normal.vec_[1] == 0.f))
	{
		return { (normal.vec_[2] < 0.f ? Vec3{-1.f, 0.f, 0.f} : Vec3{1.f, 0.f, 0.f}), Vec3{0.f, 1.f, 0.f} };
	}
	else
	{
		// Note: The root cannot become zero if
		// N.x==0 && N.y==0.
		const float d = 1.f / math::sqrt(normal.vec_[1] * normal.vec_[1] + normal.vec_[0] * normal.vec_[0]);
		const Vec3 u{normal.vec_[1] * d, -normal.vec_[0] * d, 0.f};
		return {u, normal ^ u};
	}
}

inline Vec3 Vec3::randomSpherical(FastRandom &fast_random)
{
	Vec3 v{0.f, 0.f, fast_random.getNextFloatNormalized()};
	float r = 1.f - v.vec_[2] * v.vec_[2];
	if(r > 0.f)
	{
		const float a = math::mult_pi_by_2<> * fast_random.getNextFloatNormalized();
		r = math::sqrt(r);
		v.vec_[0] = r * math::cos(a); v.vec_[1] = r * math::sin(a);
	}
	else v.vec_[2] = 1.f;
	return v;
}

END_YAFARAY

#endif // YAFARAY_VECTOR_H
