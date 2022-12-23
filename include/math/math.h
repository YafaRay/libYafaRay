#pragma once
/****************************************************************************
 *
 *      math.h: Math aproximations to speed up things
 *      This is part of the libYafaRay package
 *      Copyright (C) 2009 Rodrigo Placencia Vazquez (DarkTide)
 *      Creation date: 2009-03-26
 *
 *      fPow() based on the polynomials approach form Jose Fonséca's blog entry:
 *      Fast SSE2 pow: tables or polynomials?
 *      http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html
 *
 *      fSin() and fCos() based on Fast and Accurate sine/cosine
 *      thread on DevMaster.net forum, posted by Nick
 *      http://www.devmaster.net/forums/showthread.php?t=5784
 *
 ****************************************************************************
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

#ifndef LIBYAFARAY_MATH_H
#define LIBYAFARAY_MATH_H

#include <cmath>
#include <algorithm>
#include <limits>

namespace yafaray::math
{
template<typename T> static inline constexpr T invalid {std::numeric_limits<T>::max()}; //For some operations where we need to consider certain values as "invalid" (for example similar to the "-1" result but for unsigned integers or floats) we can set a "libYafaRay-specific" criteria that the maxium value of that particular type can be considered as an invalid value. We could use that for, for example, large float values or unsigned "-1" equivalent.

template<typename T = float> static inline constexpr T num_e {T{2.7182818284590452353602874713527L}}; // Number e
template<typename T = float> static inline constexpr T log2e {T{1.4426950408889634073599246810019L}};
template<typename T = float> static inline constexpr T log10e {T{0.43429448190325182765112891891661L}};
template<typename T = float> static inline constexpr T ln2 {T{0.69314718055994530941723212145818L}};
template<typename T = float> static inline constexpr T ln10 {T{2.3025850929940456840179914546844L}};
template<typename T = float> static inline constexpr T num_pi {T{3.1415926535897932384626433832795L}}; // Number pi
template<typename T = float> static inline constexpr T div_pi_by_2 {T{1.5707963267948966192313216916398L}};
template<typename T = float> static inline constexpr T div_pi_by_4 {T{0.78539816339744830961566084581988L}};
template<typename T = float> static inline constexpr T div_1_by_pi {T{0.31830988618379067153776752674503L}};
template<typename T = float> static inline constexpr T div_2_by_pi {T{0.63661977236758134307553505349006L}};
template<typename T = float> static inline constexpr T div_2_by_sqrt_pi {T{1.1283791670955125738961589031215L}};
template<typename T = float> static inline constexpr T sqrt2 {T{1.4142135623730950488016887242097L}};
template<typename T = float> static inline constexpr T div_1_by_sqrt2 {T{0.70710678118654752440084436210485L}};
template<typename T = float> static inline constexpr T mult_pi_by_2 {T{6.283185307179586476925286766559L}}; // pi * 2
template<typename T = float> static inline constexpr T squared_pi {T{9.8696044010893586188344909998762L}}; // pi ^ 2
template<typename T = float> static inline constexpr T div_1_by_2pi {T{0.15915494309189533576888376337251L}}; // 1 / (2 * pi)
template<typename T = float> static inline constexpr T div_4_by_pi {T{1.2732395447351626861510701069801L}}; // 4 / pi
template<typename T = float> static inline constexpr T div_4_by_squared_pi {T{0.40528473456935108577551785283891L}}; // 4 / (pi ^ 2)
template<typename T = float> static inline constexpr T div_pi_by_180 {T{0.01745329251994329576923690768489L}}; // pi / 180
template<typename T = float> static inline constexpr T div_180_by_pi {T{57.295779513082320876798154814105L}}; // 180 / pi

// fast base-2 van der Corput, Sobel, and Larcher & Pillichshammer sequences,
// all from "Efficient Multidimensional Sampling" by Alexander Keller
template<typename T = float> static inline constexpr T sample_mult_ratio {0.00000000023283064365386962890625L};

template<typename T> inline constexpr T inverse(T val) noexcept
{
	//To avoid division by zero
	static_assert(std::is_floating_point<T>::value, "This function can only be instantiated for floating point arguments");
	if(val == T{0}) return std::numeric_limits<T>::max();
	else return T{1} / val;
}

template<typename T> inline constexpr T degToRad(T deg) noexcept
{
	static_assert(std::is_floating_point<T>::value, "This function can only be instantiated for floating point arguments");
	return deg * math::div_pi_by_180<T>;
}

template<typename T> inline constexpr T radToDeg(T rad) noexcept
{
	static_assert(std::is_floating_point<T>::value, "This function can only be instantiated for floating point arguments");
	return rad * math::div_180_by_pi<T>;
}

template <typename T> inline constexpr T min(T a, T b, T c) noexcept
{
	static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "This function can only be instantiated for simple 'plain old data' types");
	return std::min(a, std::min(b, c));
}

template <typename T> inline constexpr T max(T a, T b, T c) noexcept
{
	static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "This function can only be instantiated for simple 'plain old data' types");
	return std::max(a, std::max(b, c));
}

union BitTwiddler
{
	public:
		explicit constexpr BitTwiddler(int i) noexcept : i_{i} { }
		explicit constexpr BitTwiddler(float f) noexcept : f_{f} { }
		constexpr int getAsInt() const noexcept { return i_; }
		constexpr float getAsFloat() const noexcept { return f_; }

	private:
		int i_;
		float f_;
};

inline constexpr float polyexp(float x) noexcept
{
	return x * (x * (x * (x * (x * 1.8775767e-3f + 8.9893397e-3f) + 5.5826318e-2f) + 2.4015361e-1f) + 6.9315308e-1f) + 9.9999994e-1f;
}

inline constexpr float exp2(float x) noexcept
{
	constexpr float f_hi = 129.00000f;
	constexpr float f_lo = -126.99999f;
	const float x_limited = std::max(std::min(x, f_hi), f_lo);
	const auto ipart = static_cast<int>(x_limited - 0.5f);
	const float fpart = x_limited - static_cast<float>(ipart);
	const BitTwiddler expipart{((ipart + 127) << 23)};
	return expipart.getAsFloat() * math::polyexp(fpart);
}

inline constexpr float polylog(float x) noexcept
{
	return x * (x * (x * (x * (x * -3.4436006e-2f + 3.1821337e-1f) + -1.2315303f) + 2.5988452f) + -3.3241990f) + 3.1157899f;
}

inline constexpr float log2(float x) noexcept
{
	constexpr int log_mant = 0x7FFFFF;
	const BitTwiddler i{x};
	const BitTwiddler one{1.f};
	const BitTwiddler m{(i.getAsInt() & log_mant) | one.getAsInt()};
	constexpr int log_exp = 0x7F800000;
	const auto e = static_cast<float>(((i.getAsInt() & log_exp) >> 23) - 127);
	return polylog(m.getAsFloat()) * (m.getAsFloat() - 1.f) + e;
}

#ifdef FAST_MATH
inline float sqrt_asm(float n) noexcept
{
#if defined (__APPLE__)
	float r = n;
	asm(
		"flds %0;"
		"fsqrt;"
		"fstps %0"
		:"=m"(r)
		:"m"(r)
	);
	return r;
#elif defined(__GNUC__) && defined(__i386__)
	float r = n;
	asm(
		"fld %0;"
		"fsqrt;"
		"fstp %0"
		:"=m"(r)
		:"m"(r)
	);
	return r;
#else
	return std::sqrt(n);
#endif
}
#endif // FAST_MATH

inline constexpr float pow(float a, float b) noexcept
{
#ifdef FAST_MATH
	return math::exp2(math::log2(a) * b);
#else
	return std::pow(a, b);
#endif
}

inline constexpr float log(float a) noexcept
{
#ifdef FAST_MATH
	return math::log2(a) * math::ln2<>;
#else
	return std::log(a);
#endif
}

inline double log(double a) noexcept
{
	return std::log(a);
}

inline constexpr float exp(float a) noexcept
{
#ifdef FAST_MATH
	return math::exp2(math::log2e<> * a);
#else
	return std::exp(a);
#endif
}

inline double exp(double a) noexcept
{
	return std::exp2(a);
}

inline float sqrt(float a) noexcept
{
#ifdef FAST_MATH
	return math::sqrt_asm(a);
#else
	return std::sqrt(a);
#endif
}

inline double sqrt(double a) noexcept
{
	return std::sqrt(a);
}

inline float sin(float x) noexcept
{
#ifdef FAST_TRIG
	if(x > math::mult_pi_by_2<> || x < -math::mult_pi_by_2<>) x -= static_cast<int>(x * math::div_1_by_2pi<>) * math::mult_pi_by_2<>; //float modulo x % math::mult_pi_by_2<>
	if(x < -math::num_pi<>)
	{
		x += math::mult_pi_by_2<>;
	}
	else if(x > math::num_pi<>)
	{
		x -= math::mult_pi_by_2<>;
	}

	x = math::div_4_by_pi<> * x - math::div_4_by_squared_pi<> * x * std::abs(x);
	constexpr float const_p = 0.225f;
	const float result = const_p * (x * std::abs(x) - x) + x;
	//Make sure that the function is in the valid range [-1.0,+1.0]
	if(result <= -1.f) return -1.f;
	else if(result >= 1.f) return 1.f;
	else return result;
#else
	return std::sin(x);
#endif
}

inline double sin(double x) noexcept
{
	return std::sin(x);
}

inline float cos(float x) noexcept
{
#ifdef FAST_TRIG
	return math::sin(x + math::div_pi_by_2<>);
#else
	return std::cos(x);
#endif
}

inline double cos(double x) noexcept
{
	return std::cos(x);
}

template<typename T> inline constexpr T acos(T x) noexcept
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.f) return math::num_pi<>;
	else if(x >= 1.f) return (0.f);
	else return std::acos(x);
}

inline constexpr float asin(float x) noexcept
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.f) return -math::div_pi_by_2<>;
	else if(x >= 1.f) return math::div_pi_by_2<>;
	else return std::asin(x);
}

inline constexpr double roundFloatPrecision(double val, double precision) noexcept //To round, for example 3.2384764 to 3.24 use precision 0.01
{
	if(precision <= 0.0) return 0.0;
	else return std::round(val / precision) * precision;
}

template<typename T>
inline constexpr T mod(const T &a, const T &b) noexcept
{
	const T n = static_cast<T>(a / b);
	const T result = a - n * b;
	if(a < 0) return result + b;
	else return result;
}

//! Just a "modulo 1" addition, assumed that both values are in range [0;1]
template<typename T>
inline constexpr T addMod1(const T &a, const T &b) noexcept
{
	const T s = a + b;
	return s > 1 ? s - 1 : s;
}

} //namespace yafaray::math

#endif // LIBYAFARAY_MATH_H
