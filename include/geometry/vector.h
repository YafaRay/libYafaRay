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
#include "geometry/axis.h"
#include "uv.h"
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
		constexpr explicit Vec3(float f): vec_{f, f, f} {  }
		constexpr Vec3(float x, float y, float z = 0.f): vec_{x, y, z} { }
		constexpr Vec3(const Vec3 &v) = default;
		constexpr Vec3(Vec3 &&v) = default;
		explicit constexpr Vec3(const Point3 &p);
		explicit constexpr Vec3(Point3 &&p);

		constexpr float x() const { return vec_[0]; }
		constexpr float y() const { return vec_[1]; }
		constexpr float z() const { return vec_[2]; }
		constexpr float &x() { return vec_[0]; }
		constexpr float &y() { return vec_[1]; }
		constexpr float &z() { return vec_[2]; }

		constexpr void set(float x, float y, float z = 0.f) { vec_ = {x, y, z}; }
		Vec3 &normalize();
		constexpr Vec3 &reflect(const Vec3 &normal);
		float normLen(); // normalizes and returns length
		float normLenSqr(); // normalizes and returns length squared
		constexpr float lengthSqr() const { return vec_[0] * vec_[0] + vec_[1] * vec_[1] + vec_[2] * vec_[2]; }
		float length() const { return math::sqrt(lengthSqr()); }
		constexpr bool null() const { return ((vec_[0] == 0.f) && (vec_[1] == 0.f) && (vec_[2] == 0.f)); }
		float sinFromVectors(const Vec3 &v) const;

		constexpr Vec3 &operator = (const Vec3 &s) = default;
		constexpr Vec3 &operator = (Vec3 &&s) = default;
		constexpr Vec3 &operator +=(const Vec3 &s) { for(size_t i = 0; i < 3; ++i) vec_[i] += s.vec_[i]; return *this;}
		constexpr Vec3 &operator -=(const Vec3 &s) { for(size_t i = 0; i < 3; ++i) vec_[i] -= s.vec_[i];  return *this;}
		constexpr Vec3 &operator /=(float s) { for(float &v : vec_) v /= s;  return *this;}
		constexpr Vec3 &operator *=(float s) { for(float &v : vec_) v *= s;  return *this;}
		constexpr float operator[](Axis axis) const { return vec_[axis::getId(axis)]; }
		constexpr float &operator[](Axis axis) { return vec_[axis::getId(axis)]; }

		static constexpr Vec3 reflectDir(const Vec3 &normal, const Vec3 &v);
		static constexpr bool refract(const Vec3 &n, const Vec3 &wi, Vec3 &wo, float ior);
		static constexpr void fresnel(const Vec3 &i, const Vec3 &n, float ior, float &kr, float &kt);
		static constexpr void fastFresnel(const Vec3 &i, const Vec3 &n, float iorf, float &kr, float &kt);
		static constexpr Uv<Vec3> createCoordsSystem(const Vec3 &normal);
		static Uv<float> shirleyDisk(float r_1, float r_2);
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
		constexpr Point3(float x, float y, float z = 0.f) : Vec3{x, y, z} { }
		constexpr Point3(const Point3 &p) = default;
		constexpr Point3(Point3 &&p) = default;
		explicit constexpr Point3(const Vec3 &v): Vec3{v} { }
		explicit constexpr Point3(Vec3 &&v): Vec3{std::move(v)} { }
		constexpr Point3 &operator = (const Point3 &s) = default;
		constexpr Point3 &operator = (Point3 &&s) = default;
		static constexpr Point3 mult(const Point3 &a, const Vec3 &b);
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

inline constexpr Vec3::Vec3(const Point3 &p): vec_{p.vec_} { }
inline constexpr Vec3::Vec3(Point3 &&p): vec_{std::move(p.vec_)} { }

std::ostream &operator << (std::ostream &out, const Vec3 &v);
std::ostream &operator << (std::ostream &out, const Point3 &p);

inline constexpr float operator * (const Vec3 &a, const Vec3 &b)
{
	return a[Axis::X] * b[Axis::X] + a[Axis::Y] * b[Axis::Y] + a[Axis::Z] * b[Axis::Z];
}

inline constexpr Vec3 operator * (float f, const Vec3 &v)
{
	return {f * v[Axis::X], f * v[Axis::Y], f * v[Axis::Z]};
}

inline constexpr Vec3 operator * (const Vec3 &v, float f)
{
	return {f * v[Axis::X], f * v[Axis::Y], f * v[Axis::Z]};
}

inline constexpr Point3 operator * (float f, const Point3 &p)
{
	return {f * p[Axis::X], f * p[Axis::Y], f * p[Axis::Z]};
}

inline constexpr Vec3 operator / (const Vec3 &v, float f)
{
	return {v[Axis::X] / f, v[Axis::Y] / f, v[Axis::Z] / f};
}

inline constexpr Point3 operator / (const Point3 &p, float f)
{
	return {p[Axis::X] / f, p[Axis::Y] / f, p[Axis::Z] / f};
}

inline constexpr Point3 operator * (const Point3 &p, float f)
{
	return {p[Axis::X] * f, p[Axis::Y] * f, p[Axis::Z] * f};
}

inline constexpr Vec3 operator / (float f, const Vec3 &v)
{
	return {v[Axis::X] / f, v[Axis::Y] / f, v[Axis::Z] / f};
}

inline constexpr Vec3 operator ^ (const Vec3 &a, const Vec3 &b)
{
	return {a[Axis::Y] * b[Axis::Z] - a[Axis::Z] * b[Axis::Y], a[Axis::Z] * b[Axis::X] - a[Axis::X] * b[Axis::Z], a[Axis::X] * b[Axis::Y] - a[Axis::Y] * b[Axis::X]};
}

inline constexpr Vec3 operator - (const Vec3 &a, const Vec3 &b)
{
	return {a[Axis::X] - b[Axis::X], a[Axis::Y] - b[Axis::Y], a[Axis::Z] - b[Axis::Z]};
}

inline constexpr Vec3 operator - (const Point3 &a, const Point3 &b)
{
	return {a[Axis::X] - b[Axis::X], a[Axis::Y] - b[Axis::Y], a[Axis::Z] - b[Axis::Z]};
}

inline constexpr Point3 operator - (const Point3 &a, const Vec3 &b)
{
	return {a[Axis::X] - b[Axis::X], a[Axis::Y] - b[Axis::Y], a[Axis::Z] - b[Axis::Z]};
}

inline constexpr Vec3 operator - (const Vec3 &v)
{
	return {-v[Axis::X], -v[Axis::Y], -v[Axis::Z]};
}

inline constexpr Vec3 operator + (const Vec3 &a, const Vec3 &b)
{
	return {a[Axis::X] + b[Axis::X], a[Axis::Y] + b[Axis::Y], a[Axis::Z] + b[Axis::Z]};
}

inline constexpr Point3 operator + (const Point3 &a, const Point3 &b)
{
	return {a[Axis::X] + b[Axis::X], a[Axis::Y] + b[Axis::Y], a[Axis::Z] + b[Axis::Z]};
}

inline constexpr Point3 operator + (const Point3 &a, const Vec3 &b)
{
	return {a[Axis::X] + b[Axis::X], a[Axis::Y] + b[Axis::Y], a[Axis::Z] + b[Axis::Z]};
}

inline constexpr bool operator == (const Point3 &a, const Point3 &b)
{
	return ((a[Axis::X] == b[Axis::X]) && (a[Axis::Y] == b[Axis::Y]) && (a[Axis::Z] == b[Axis::Z]));
}

bool operator == (const Vec3 &a, const Vec3 &b);
bool operator != (const Vec3 &a, const Vec3 &b);

inline constexpr Point3 Point3::mult(const Point3 &a, const Vec3 &b)
{
	return {a[Axis::X] * b[Axis::X], a[Axis::Y] * b[Axis::Y], a[Axis::Z] * b[Axis::Z]};
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
inline constexpr Vec3 &Vec3::reflect(const Vec3 &normal)
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

inline constexpr Vec3 Vec3::reflectDir(const Vec3 &normal, const Vec3 &v)
{
	const float vn = v * normal;
	if(vn < 0.f) return -v;
	return 2.f * vn * normal - v;
}

inline constexpr Uv<Vec3> Vec3::createCoordsSystem(const Vec3 &normal)
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

// P.Shirley's concentric disk algorithm, maps square to disk
inline Uv<float> Vec3::shirleyDisk(float r_1, float r_2)
{
	float phi;
	float r;
	const float a = 2.f * r_1 - 1.f;
	const float b = 2.f * r_2 - 1.f;
	if(a > -b)
	{
		if(a > b)  	// Reg.1
		{
			r = a;
			phi = math::div_pi_by_4<> * (b / a);
		}
		else  			// Reg.2
		{
			r = b;
			phi = math::div_pi_by_4<> * (2.f - a / b);
		}
	}
	else
	{
		if(a < b)  	// Reg.3
		{
			r = -a;
			phi = math::div_pi_by_4<> * (4.f + b / a);
		}
		else  			// Reg.4
		{
			r = -b;
			if(b != 0)
				phi = math::div_pi_by_4<> * (6.f - a / b);
			else
				phi = 0.f;
		}
	}
	return {r * math::cos(phi), r * math::sin(phi)};
}

/*! refract a ray given the IOR. All directions (n, wi and wo) point away from the intersection point.
	\return true when refraction was possible, false when total inner reflrection occurs (wo is not computed then)
	\param ior Index of refraction, or precisely the ratio of eta_t/eta_i, where eta_i is by definition the
				medium in which n points. E.g. "outside" is air, "inside" is water, the normal points outside,
				IOR = eta_air / eta_water = 1.33
*/
inline constexpr bool Vec3::refract(const Vec3 &n, const Vec3 &wi, Vec3 &wo, float ior)
{
	Vec3 N{n};
	float eta = ior;
	const Vec3 i{-wi};
	float cos_v_n = wi * n;
	if((cos_v_n) < 0)
	{
		N = -n;
		cos_v_n = -cos_v_n;
	}
	else
	{
		eta = 1.f / ior;
	}
	const float k = 1 - eta * eta * (1 - cos_v_n * cos_v_n);
	if(k <= 0.f) return false;

	wo = eta * i + (eta * cos_v_n - math::sqrt(k)) * N;
	wo.normalize();

	return true;
}

inline constexpr void Vec3::fresnel(const Vec3 &i, const Vec3 &n, float ior, float &kr, float &kt)
{
	const bool negative = ((i * n) < 0.f);
	const float c = i * (negative ? (-n) : n);
	float g = ior * ior + c * c - 1.f;
	if(g <= 0) g = 0;
	else g = math::sqrt(g);
	const float aux = c * (g + c);
	kr = ((0.5f * (g - c) * (g - c)) / ((g + c) * (g + c))) *
		 (1.f + ((aux - 1.f) * (aux - 1.f)) / ((aux + 1.f) * (aux + 1.f)));
	if(kr < 1.f)
		kt = 1.f - kr;
	else
		kt = 0.f;
}

// 'Faster' Schlick fresnel approximation,
inline constexpr void Vec3::fastFresnel(const Vec3 &i, const Vec3 &n, float iorf, float &kr, float &kt)
{
	const float t = 1.f - (i * n);
	//t = (t<0)?0:((t>1)?1:t);
	const float t_2 = t * t;
	kr = iorf + (1.f - iorf) * t_2 * t_2 * t;
	kt = 1.f - kr;
}

END_YAFARAY

#endif // YAFARAY_VECTOR_H
