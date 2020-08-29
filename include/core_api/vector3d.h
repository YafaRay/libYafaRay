#pragma once
/****************************************************************************
 *
 * 			vector3d.h: Vector 3d and point representation and manipulation api
 *      This is part of the yafray package
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
#ifndef YAFARAY_VECTOR3D_H
#define YAFARAY_VECTOR3D_H

#include <yafray_constants.h>
#include <utilities/mathOptimizations.h>
#include <iostream>

BEGIN_YAFRAY

// useful trick found in trimesh2 lib by Szymon Rusinkiewicz
// makes code for vector dot- cross-products much more readable
#define VDOT *
#define VCROSS ^

#ifndef M_PI    //in most cases pi is defined as M_PI in cmath ohterwise we define it
#define M_PI 3.1415926535897932384626433832795
#endif

class Normal;
class Point3;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
class YAFRAYCORE_EXPORT Vec3
{
	public:
		Vec3() = default;
		Vec3(float v): x_(v), y_(v), z_(v) {  }
		Vec3(float ix, float iy, float iz = 0): x_(ix), y_(iy), z_(iz) { }
		Vec3(const Vec3 &s): x_(s.x_), y_(s.y_), z_(s.z_) { }
		explicit Vec3(const Normal &n);
		explicit Vec3(const Point3 &p);

		void set(float ix, float iy, float iz = 0) { x_ = ix; y_ = iy; z_ = iz; }
		Vec3 &normalize();
		Vec3 &reflect(const Vec3 &n);
		// normalizes and returns length
		float normLen()
		{
			float vl = x_ * x_ + y_ * y_ + z_ * z_;
			if(vl != 0.0)
			{
				vl = fSqrt__(vl);
				const float d = 1.0 / vl;
				x_ *= d; y_ *= d; z_ *= d;
			}
			return vl;
		}
		// normalizes and returns length squared
		float normLenSqr()
		{
			float vl = x_ * x_ + y_ * y_ + z_ * z_;
			if(vl != 0.0)
			{
				const float d = 1.0 / fSqrt__(vl);
				x_ *= d; y_ *= d; z_ *= d;
			}
			return vl;
		}
		float length() const;
		float lengthSqr() const { return x_ * x_ + y_ * y_ + z_ * z_; }
		bool null() const { return ((x_ == 0) && (y_ == 0) && (z_ == 0)); }
		float sinFromVectors(const Vec3 &v);

		Vec3 &operator = (const Vec3 &s) { x_ = s.x_; y_ = s.y_; z_ = s.z_;  return *this;}
		Vec3 &operator +=(const Vec3 &s) { x_ += s.x_; y_ += s.y_; z_ += s.z_;  return *this;}
		Vec3 &operator -=(const Vec3 &s) { x_ -= s.x_; y_ -= s.y_; z_ -= s.z_;  return *this;}
		Vec3 &operator /=(float s) { x_ /= s; y_ /= s; z_ /= s;  return *this;}
		Vec3 &operator *=(float s) { x_ *= s; y_ *= s; z_ *= s;  return *this;}
		float operator[](int i) const { return (&x_)[i]; } //Lynx
		void abs() { x_ = std::fabs(x_); y_ = std::fabs(y_); z_ = std::fabs(z_); }
		float x_, y_, z_;
};

class YAFRAYCORE_EXPORT Normal final
{
	public:
		Normal() = default;
		Normal(float nx, float ny, float nz): x_(nx), y_(ny), z_(nz) {}
		explicit Normal(const Vec3 &v): x_(v.x_), y_(v.y_), z_(v.z_) { }
		Normal &normalize();
		Normal &operator = (const Vec3 &v) { x_ = v.x_, y_ = v.y_, z_ = v.z_; return *this; }
		Normal &operator +=(const Vec3 &s) { x_ += s.x_; y_ += s.y_; z_ += s.z_;  return *this; }
		float x_, y_, z_;
};

class YAFRAYCORE_EXPORT Point3 final
{
	public:
		Point3() = default;
		Point3(float ix, float iy, float iz = 0): x_(ix), y_(iy), z_(iz) { }
		Point3(const Point3 &s): x_(s.x_), y_(s.y_), z_(s.z_) { }
		Point3(const Vec3 &v): x_(v.x_), y_(v.y_), z_(v.z_) { }
		void set(float ix, float iy, float iz = 0) { x_ = ix; y_ = iy; z_ = iz; }
		float length() const;

		Point3 &operator= (const Point3 &s) { x_ = s.x_; y_ = s.y_; z_ = s.z_;  return *this; }
		Point3 &operator *=(float s) { x_ *= s; y_ *= s; z_ *= s;  return *this;}
		Point3 &operator +=(float s) { x_ += s; y_ += s; z_ += s;  return *this;}
		Point3 &operator +=(const Point3 &s) { x_ += s.x_; y_ += s.y_; z_ += s.z_;  return *this;}
		Point3 &operator -=(const Point3 &s) { x_ -= s.x_; y_ -= s.y_; z_ -= s.z_;  return *this;}
		float operator[](int i) const { return (&x_)[i]; } //Lynx
		float &operator[](int i) { return (&x_)[i]; } //Lynx
		float x_, y_, z_;
};
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif


inline Vec3::Vec3(const Normal &n): x_(n.x_), y_(n.y_), z_(n.z_) { }
inline Vec3::Vec3(const Point3 &p): x_(p.x_), y_(p.y_), z_(p.z_) { }

#define FAST_ANGLE(a,b)  ( (a).x*(b).y - (a).y*(b).x )
#define FAST_SANGLE(a,b) ( (((a).x*(b).y - (a).y*(b).x) >= 0) ? 0 : 1 )
#define SIN(a,b) fSqrt(1-((a)*(b))*((a)*(b)))

YAFRAYCORE_EXPORT std::ostream &operator << (std::ostream &out, const Vec3 &v);
YAFRAYCORE_EXPORT std::ostream &operator << (std::ostream &out, const Point3 &p);

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

inline Normal operator + (const Normal &a, const Vec3 &b)
{
	return Normal(a.x_ + b.x_, a.y_ + b.y_, a.z_ + b.z_);
}


inline bool  operator == (const Point3 &a, const Point3 &b)
{
	return ((a.x_ == b.x_) && (a.y_ == b.y_) && (a.z_ == b.z_));
}

YAFRAYCORE_EXPORT bool  operator == (const Vec3 &a, const Vec3 &b);
YAFRAYCORE_EXPORT bool  operator != (const Vec3 &a, const Vec3 &b);

inline Point3 mult__(const Point3 &a, const Vec3 &b)
{
	return Point3(a.x_ * b.x_, a.y_ * b.y_, a.z_ * b.z_);
}

inline float Vec3::length() const
{
	return fSqrt__(x_ * x_ + y_ * y_ + z_ * z_);
}

inline Vec3 &Vec3::normalize()
{
	float len = x_ * x_ + y_ * y_ + z_ * z_;
	if(len != 0)
	{
		len = 1.0 / fSqrt__(len);
		x_ *= len;
		y_ *= len;
		z_ *= len;
	}
	return *this;
}


inline float Vec3::sinFromVectors(const Vec3 &v)
{
	float div = (length() * v.length()) * 0.99999f + 0.00001f;
	float asin_argument = ((*this ^ v).length() / div) * 0.99999f;
	//Fix to avoid black "nan" areas when this argument goes slightly over +1.0. Why that happens in the first place, maybe floating point rounding errors?
	if(asin_argument > 1.f) asin_argument = 1.f;
	return asin(asin_argument);
}

inline Normal &Normal::normalize()
{
	float len = x_ * x_ + y_ * y_ + z_ * z_;
	if(len != 0)
	{
		len = 1.0 / fSqrt__(len);
		x_ *= len;
		y_ *= len;
		z_ *= len;
	}
	return *this;
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

inline Vec3 reflectDir__(const Vec3 &n, const Vec3 &v)
{
	const float vn = v * n;
	if(vn < 0) return -v;
	return 2 * vn * n - v;
}


inline Vec3 toVector__(const Point3 &p)
{
	return Vec3(p.x_, p.y_, p.z_);
}

YAFRAYCORE_EXPORT bool refract__(const Vec3 &n, const Vec3 &wi, Vec3 &wo, float ior);
YAFRAYCORE_EXPORT bool refractTest__(const Vec3 &n, const Vec3 &wi, float ior);
YAFRAYCORE_EXPORT bool invRefractTest__(Vec3 &n, const Vec3 &wi, const Vec3 &wo, float ior);
YAFRAYCORE_EXPORT void fresnel__(const Vec3 &i, const Vec3 &n, float ior, float &kr, float &kt);
YAFRAYCORE_EXPORT void fastFresnel__(const Vec3 &i, const Vec3 &n, float iorf, float &kr, float &kt);

inline void createCs__(const Vec3 &n, Vec3 &u, Vec3 &v)
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
		const float d = 1.0 / fSqrt__(n.y_ * n.y_ + n.x_ * n.x_);
		u.set(n.y_ * d, -n.x_ * d, 0);
		v = n ^ u;
	}
}

YAFRAYCORE_EXPORT void shirleyDisk__(float r_1, float r_2, float &u, float &v);

extern YAFRAYCORE_EXPORT int myseed__;

inline int ourRandomI__()
{
	const int a = 0x000041A7;
	const int m = 0x7FFFFFFF;
	const int q = 0x0001F31D; // m/a
	const int r = 0x00000B14; // m%a;
	myseed__ = a * (myseed__ % q) - r * (myseed__ / q);
	if(myseed__ < 0)
		myseed__ += m;
	return myseed__;
}

inline float ourRandom__()
{
	const int a = 0x000041A7;
	const int m = 0x7FFFFFFF;
	const int q = 0x0001F31D; // m/a
	const int r = 0x00000B14; // m%a;
	myseed__ = a * (myseed__ % q) - r * (myseed__ / q);
	if(myseed__ < 0)
		myseed__ += m;
	return (float)myseed__ / (float)m;
}

inline float ourRandom__(int &seed)
{
	const int a = 0x000041A7;
	const int m = 0x7FFFFFFF;
	const int q = 0x0001F31D; // m/a
	const int r = 0x00000B14; // m%a;
	seed = a * (seed % q) - r * (seed / q);
	if(myseed__ < 0)
		myseed__ += m;
	return (float)seed / (float)m;
}

inline Vec3 randomSpherical__()
{
	float r;
	Vec3 v(0.0, 0.0, ourRandom__());
	if((r = 1.0 - v.z_ * v.z_) > 0.0)
	{
		float a = M_2PI * ourRandom__();
		r = fSqrt__(r);
		v.x_ = r * fCos__(a); v.y_ = r * fSin__(a);
	}
	else v.z_ = 1.0;
	return v;
}

YAFRAYCORE_EXPORT Vec3 randomVectorCone__(const Vec3 &d, const Vec3 &u, const Vec3 &v,
										  float cosang, float z_1, float z_2);
YAFRAYCORE_EXPORT Vec3 randomVectorCone__(const Vec3 &dir, float cosangle, float r_1, float r_2);
YAFRAYCORE_EXPORT Vec3 discreteVectorCone__(const Vec3 &dir, float cangle, int sample, int square);

END_YAFRAY

#endif // YAFARAY_VECTOR3D_H
