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

#include "yafaray_conf.h"
#include "math/math.h"
#include "math/random.h"
#include <iostream>

BEGIN_YAFARAY

#ifndef M_PI    //in most cases pi is defined as M_PI in cmath ohterwise we define it
#define M_PI 3.1415926535897932384626433832795
#endif

class Point3;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
class Vec3
{
	public:
		Vec3() = default;
		Vec3(float v): x_(v), y_(v), z_(v) {  }
		Vec3(float ix, float iy, float iz = 0): x_(ix), y_(iy), z_(iz) { }
		Vec3(const Vec3 &s): x_(s.x_), y_(s.y_), z_(s.z_) { }
		explicit Vec3(const Point3 &p);

		void set(float ix, float iy, float iz = 0) { x_ = ix; y_ = iy; z_ = iz; }
		Vec3 &normalize();
		Vec3 &reflect(const Vec3 &n);
		float normLen(); // normalizes and returns length
		float normLenSqr(); // normalizes and returns length squared
		float lengthSqr() const { return x_ * x_ + y_ * y_ + z_ * z_; }
		float length() const { return math::sqrt(lengthSqr()); }
		bool null() const { return ((x_ == 0.f) && (y_ == 0.f) && (z_ == 0.f)); }
		float sinFromVectors(const Vec3 &v) const;

		Vec3 &operator = (const Vec3 &s) { x_ = s.x_; y_ = s.y_; z_ = s.z_;  return *this;}
		Vec3 &operator +=(const Vec3 &s) { x_ += s.x_; y_ += s.y_; z_ += s.z_;  return *this;}
		Vec3 &operator -=(const Vec3 &s) { x_ -= s.x_; y_ -= s.y_; z_ -= s.z_;  return *this;}
		Vec3 &operator /=(float s) { x_ /= s; y_ /= s; z_ /= s;  return *this;}
		Vec3 &operator *=(float s) { x_ *= s; y_ *= s; z_ *= s;  return *this;}
		float operator[](int i) const { return (&x_)[i]; } //Lynx
		float &operator[](int i) { return (&x_)[i]; } //Lynx

		static Vec3 reflectDir(const Vec3 &n, const Vec3 &v);
		static bool refract(const Vec3 &n, const Vec3 &wi, Vec3 &wo, float ior);
		static void fresnel(const Vec3 &i, const Vec3 &n, float ior, float &kr, float &kt);
		static void fastFresnel(const Vec3 &i, const Vec3 &n, float iorf, float &kr, float &kt);
		static void createCs(const Vec3 &n, Vec3 &u, Vec3 &v);
		static void shirleyDisk(float r_1, float r_2, float &u, float &v);
		static Vec3 randomSpherical();
		static Vec3 randomVectorCone(const Vec3 &d, const Vec3 &u, const Vec3 &v, float cosang, float z_1, float z_2);
		static Vec3 randomVectorCone(const Vec3 &dir, float cosangle, float r_1, float r_2);
		static Vec3 discreteVectorCone(const Vec3 &dir, float cangle, int sample, int square);

		float x_, y_, z_;
};

class Point3 final : public Vec3
{
	public:
		Point3() = default;
		Point3(float ix, float iy, float iz = 0) : Vec3(ix, iy, iz) { }
		Point3(const Point3 &s) : Vec3(s.x_, s.y_, s.z_) { }
		Point3(const Vec3 &v): Vec3(v) { }
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

inline Vec3::Vec3(const Point3 &p): x_(p.x_), y_(p.y_), z_(p.z_) { }

std::ostream &operator << (std::ostream &out, const Vec3 &v);
std::ostream &operator << (std::ostream &out, const Point3 &p);

inline float operator * (const Vec3 &a, const Vec3 &b)
{
	return (a.x_ * b.x_ + a.y_ * b.y_ + a.z_ * b.z_);
}

inline Vec3 operator * (float f, const Vec3 &b)
{
	return Vec3(f * b.x_, f * b.y_, f * b.z_);
}

inline Vec3 operator * (const Vec3 &b, float f)
{
	return Vec3(f * b.x_, f * b.y_, f * b.z_);
}

inline Point3 operator * (float f, const Point3 &b)
{
	return Point3(f * b.x_, f * b.y_, f * b.z_);
}

inline Vec3 operator / (const Vec3 &b, float f)
{
	return Vec3(b.x_ / f, b.y_ / f, b.z_ / f);
}

inline Point3 operator / (const Point3 &b, float f)
{
	return Point3(b.x_ / f, b.y_ / f, b.z_ / f);
}

inline Point3 operator * (const Point3 &b, float f)
{
	return Point3(b.x_ * f, b.y_ * f, b.z_ * f);
}

inline Vec3 operator / (float f, const Vec3 &b)
{
	return Vec3(b.x_ / f, b.y_ / f, b.z_ / f);
}

inline Vec3 operator ^ (const Vec3 &a, const Vec3 &b)
{
	return Vec3(a.y_ * b.z_ - a.z_ * b.y_, a.z_ * b.x_ - a.x_ * b.z_, a.x_ * b.y_ - a.y_ * b.x_);
}

inline Vec3  operator - (const Vec3 &a, const Vec3 &b)
{
	return Vec3(a.x_ - b.x_, a.y_ - b.y_, a.z_ - b.z_);
}

inline Vec3  operator - (const Point3 &a, const Point3 &b)
{
	return Vec3(a.x_ - b.x_, a.y_ - b.y_, a.z_ - b.z_);
}

inline Point3  operator - (const Point3 &a, const Vec3 &b)
{
	return Point3(a.x_ - b.x_, a.y_ - b.y_, a.z_ - b.z_);
}

inline Vec3  operator - (const Vec3 &b)
{
	return Vec3(-b.x_, -b.y_, -b.z_);
}

inline Vec3  operator + (const Vec3 &a, const Vec3 &b)
{
	return Vec3(a.x_ + b.x_, a.y_ + b.y_, a.z_ + b.z_);
}

inline Point3  operator + (const Point3 &a, const Point3 &b)
{
	return Point3(a.x_ + b.x_, a.y_ + b.y_, a.z_ + b.z_);
}

inline Point3  operator + (const Point3 &a, const Vec3 &b)
{
	return Point3(a.x_ + b.x_, a.y_ + b.y_, a.z_ + b.z_);
}

inline bool  operator == (const Point3 &a, const Point3 &b)
{
	return ((a.x_ == b.x_) && (a.y_ == b.y_) && (a.z_ == b.z_));
}

bool  operator == (const Vec3 &a, const Vec3 &b);
bool  operator != (const Vec3 &a, const Vec3 &b);

inline Point3 mult_global(const Point3 &a, const Vec3 &b)
{
	return Point3(a.x_ * b.x_, a.y_ * b.y_, a.z_ * b.z_);
}

inline Vec3 &Vec3::normalize()
{
	float len = lengthSqr();
	if(len != 0.f)
	{
		len = 1.f / math::sqrt(len);
		x_ *= len;
		y_ *= len;
		z_ *= len;
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
 *  @param	n Surface normal
 *  @warning	\a n must be unit vector!
 *  @note	Lynn's formula: R = 2*(V dot N)*N -V (http://www.3dkingdoms.com/weekly/weekly.php?a=2)
 */
inline Vec3 &Vec3::reflect(const Vec3 &n)
{
	const float vn = 2.0f * (x_ * n.x_ + y_ * n.y_ + z_ * n.z_);
	x_ = vn * n.x_ - x_;
	y_ = vn * n.y_ - y_;
	z_ = vn * n.z_ - z_;
	return *this;
}

inline float Vec3::normLen() {
	float vl = lengthSqr();
	if(vl != 0.f)
	{
		vl = math::sqrt(vl);
		const float d = 1.f / vl;
		x_ *= d; y_ *= d; z_ *= d;
	}
	return vl;
}

inline float Vec3::normLenSqr() {
	const float vl = lengthSqr();
	if(vl != 0.f)
	{
		const float d = 1.f / math::sqrt(vl);
		x_ *= d; y_ *= d; z_ *= d;
	}
	return vl;
}

inline Vec3 Vec3::reflectDir(const Vec3 &n, const Vec3 &v)
{
	const float vn = v * n;
	if(vn < 0) return -v;
	return 2 * vn * n - v;
}

inline void Vec3::createCs(const Vec3 &n, Vec3 &u, Vec3 &v)
{
	if((n.x_ == 0) && (n.y_ == 0))
	{
		if(n.z_ < 0)
			u.set(-1, 0, 0);
		else
			u.set(1, 0, 0);
		v.set(0, 1, 0);
	}
	else
	{
		// Note: The root cannot become zero if
		// N.x==0 && N.y==0.
		const float d = 1.f / math::sqrt(n.y_ * n.y_ + n.x_ * n.x_);
		u.set(n.y_ * d, -n.x_ * d, 0);
		v = n ^ u;
	}
}

inline Vec3 Vec3::randomSpherical()
{
	Vec3 v(0.0, 0.0, FastRandom::getNextFloatNormalized());
	float r = 1.0 - v.z_ * v.z_;
	if(r > 0.0)
	{
		const float a = math::mult_pi_by_2 * FastRandom::getNextFloatNormalized();
		r = math::sqrt(r);
		v.x_ = r * math::cos(a); v.y_ = r * math::sin(a);
	}
	else v.z_ = 1.0;
	return v;
}

END_YAFARAY

#endif // YAFARAY_VECTOR_H
