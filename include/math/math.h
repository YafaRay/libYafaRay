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

#ifndef YAFARAY_MATH_H
#define YAFARAY_MATH_H

#include <cmath>
#include <algorithm>

BEGIN_YAFARAY

namespace math
{

static constexpr double mult_pi_by_2 = 6.28318530717958647692; // PI * 2
static constexpr double squared_pi = 9.86960440108935861882; // PI ^ 2
static constexpr double div_1_by_2pi = 0.15915494309189533577; // 1 / (2 * PI)
static constexpr double div_4_by_pi = 1.27323954473516268615; // 4 / PI
static constexpr double div_4_by_squared_pi = 0.40528473456935108578; // 4 / PI ^ 2

inline constexpr double degToRad(double deg) { return (deg * 0.01745329251994329576922); }  // deg * PI / 180
inline constexpr double radToDeg(double rad) { return (rad * 57.29577951308232087684636); } // rad * 180 / PI

//#if ( defined(__i386__) || defined(_M_IX86) || defined(_X86_) )
//#define FAST_INT 1
static constexpr double doublemagicroundeps = (.5 - 1.4e-11);
//almost .5f = .5f - 1e^(number of exp bit)
static constexpr double doublemagic = double (6755399441055744.0);
//2^52 * 1.5,  uses limited precision to floor
//#endif

// fast base-2 van der Corput, Sobel, and Larcher & Pillichshammer sequences,
// all from "Efficient Multidimensional Sampling" by Alexander Keller
static constexpr double sample_mult_ratio = 0.00000000023283064365386962890625;


union BitTwiddler
{
	int i_;
	float f_;
};

inline constexpr float polyexp(float x) { return (x * (x * (x * (x * (x * 1.8775767e-3f + 8.9893397e-3f) + 5.5826318e-2f) + 2.4015361e-1f) + 6.9315308e-1f) + 9.9999994e-1f); }

inline float exp2(float x)
{
	BitTwiddler ipart, fpart;
	BitTwiddler expipart;

	static constexpr float f_hi = 129.00000f;
	static constexpr float f_lo = -126.99999f;

	x = std::min(x, f_hi);
	x = std::max(x, f_lo);

	ipart.i_ = static_cast<int>(x - 0.5f);
	fpart.f_ = (x - static_cast<float>(ipart.i_));
	expipart.i_ = ((ipart.i_ + 127) << 23);

	return (expipart.f_ * math::polyexp(fpart.f_));
}

inline constexpr float polylog__(float x) { return (x * (x * (x * (x * (x * -3.4436006e-2f + 3.1821337e-1f) + -1.2315303f) + 2.5988452) + -3.3241990f) + 3.1157899f); }

inline float log2(float x)
{
	BitTwiddler one, i, m, e;
	static constexpr int log_exp = 0x7F800000;
	static constexpr int log_mant = 0x7FFFFF;

	one.f_ = 1.0f;
	i.f_ = x;
	e.f_ = static_cast<float>(((i.i_ & log_exp) >> 23) - 127);
	m.i_ = ((i.i_ & log_mant) | one.i_);
	return (polylog__(m.f_) * (m.f_ - one.f_) + e.f_);
}

#ifdef FAST_MATH
inline float sqrt_asm(float n)
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

inline float pow(float a, float b)
{
#ifdef FAST_MATH
	return math::exp2(math::log2(a) * b);
#else
	return std::pow(a, b);
#endif
}

inline float log(float a)
{
#ifdef FAST_MATH
	return math::log2(a) * (float) M_LN2;
#else
	return std::log(a);
#endif
}

inline float exp(float a)
{
#ifdef FAST_MATH
	return math::exp2((float) M_LOG2E * a);
#else
	return std::exp(a);
#endif
}

inline float sqrt(float a)
{
#ifdef FAST_MATH
	return math::sqrt_asm(a);
#else
	return std::sqrt(a);
#endif
}

inline float ldexp(float x, int a)
{
#ifdef FAST_MATH
	//return x * fPow(2.0, a);
	return std::ldexp(x, a);
#else
	return std::ldexp(x, a);
#endif
}

inline float sin(float x)
{
#ifdef FAST_TRIG
	if(x > math::mult_pi_by_2 || x < -math::mult_pi_by_2) x -= ((int) (x * static_cast<float>(math::div_1_by_2pi))) * static_cast<float>(math::mult_pi_by_2); //float modulo x % math::mult_pi_by_2
	if(x < -M_PI)
	{
		x += static_cast<float>(math::mult_pi_by_2);
	}
	else if(x > M_PI)
	{
		x -= static_cast<float>(math::mult_pi_by_2);
	}

	x = (static_cast<float>(math::div_4_by_pi * x)) - (static_cast<float>(math::div_4_by_squared_pi * x * std::abs(x)));
	static constexpr float const_p = 0.225f;
	const float result = const_p * (x * std::abs(x) - x) + x;
	//Make sure that the function is in the valid range [-1.0,+1.0]
	if(result <= -1.0) return -1.0f;
	else if(result >= 1.0) return 1.0f;
	else return result;
#else
	return std::sin(x);
#endif
}

inline float cos(float x)
{
#ifdef FAST_TRIG
	return math::sin(x + static_cast<float>(M_PI_2));
#else
	return std::cos(x);
#endif
}

inline float acos(float x)
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.0) return (static_cast<float>(M_PI));
	else if(x >= 1.0) return (0.0);
	else return std::acos(x);
}

inline float asin(float x)
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.0) return (static_cast<float>(-M_PI_2));
	else if(x >= 1.0) return (static_cast<float>(M_PI_2));
	else return std::asin(x);
}

inline int roundToInt(double val)
{
#ifdef FAST_INT
	val		= val + math::doublemagic;
	return static_cast<long *>(&val)[0];
#else
	//	#warning "using slow rounding"
	return static_cast<int>(val + math::doublemagicroundeps);
#endif
}

inline int floatToInt(double val)
{
#ifdef FAST_INT
	return (val < 0) ?  math::roundToInt(val + math::doublemagicroundeps) :
		   (val - math::doublemagicroundeps);
#else
	//	#warning "using slow rounding"
	return static_cast<int>(val);
#endif
}

inline int floorToInt(double val)
{
#ifdef FAST_INT
	return math::roundToInt(val - math::doublemagicroundeps);
#else
	//	#warning "using slow rounding"
	return static_cast<int>(std::floor(val));
#endif
}

inline int ceilToInt(double val)
{
#ifdef FAST_INT
	return math::roundToInt(val + math::doublemagicroundeps);
#else
	//	#warning "using slow rounding"
	return static_cast<int>(std::ceil(val));
#endif
}

inline double roundFloatPrecision(double val, double precision) //To round, for example 3.2384764 to 3.24 use precision 0.01
{
	if(precision <= 0.0) return 0.0;
	else return std::round(val / precision) * precision;
}

template<typename T>
inline bool isValid(const T &value)	//To check a floating point number is not a NaN for example, even while using --fast-math compile flag
{
	return (value >= std::numeric_limits<T>::lowest() && value <= std::numeric_limits<T>::max());
}

template<typename T>
inline T mod(const T &a, const T &b)
{
	const T n = static_cast<T>(a / b);
	T result = a - n * b;
	if(a < 0) result += b;
	return result;
}

//! Just a "modulo 1" addition, assumed that both values are in range [0;1]
template<typename T>
inline T addMod1(const T &a, const T &b)
{
	const T s = a + b;
	return s > 1 ? s - 1 : s;
}

inline int nextPrime(int last_prime)
{
	int new_prime = last_prime + (last_prime & 1) + 1;
	for(;;)
	{
		int dv = 3;
		bool ispr = true;
		while((ispr) && (dv * dv <= new_prime))
		{
			ispr = ((new_prime % dv) != 0);
			dv += 2;
		}
		if(ispr) break;
		new_prime += 2;
	}
	return new_prime;
}

} //namespace math

END_YAFARAY

#endif // YAFARAY_MATH_H
