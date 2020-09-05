#pragma once
/****************************************************************************
 *
 *      mathOptimizations.h: Math aproximations to speed up things
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

#ifndef YAFARAY_UTIL_MATH_OPTIMIZATIONS_H
#define YAFARAY_UTIL_MATH_OPTIMIZATIONS_H

#include <cmath>
#include <algorithm>

BEGIN_YAFARAY

static constexpr double mult_pi_by_2__ = 6.28318530717958647692; // PI * 2
static constexpr double squared_pi__ = 9.86960440108935861882; // PI ^ 2
static constexpr double div_1_by_2pi__ = 0.15915494309189533577; // 1 / (2 * PI)
static constexpr double div_4_by_pi__ = 1.27323954473516268615; // 4 / PI
static constexpr double div_4_by_squared_pi__ = 0.40528473456935108578; // 4 / PI ^ 2

inline constexpr double degToRad__(double deg) { return (deg * 0.01745329251994329576922); }  // deg * PI / 180
inline constexpr double radToDeg__(double rad) { return (rad * 57.29577951308232087684636); } // rad * 180 / PI

//FIXME: All the overloaded double definitions have been added to fix the "white dots" problems and they seem to work fine. However several of them should be refined in the future to include constants with the correct precision for the doubles calculations.

union BitTwiddler
{
	int i_;
	float f_;
};

inline constexpr float polyexp__(float x) { return (x * (x * (x * (x * (x * 1.8775767e-3f + 8.9893397e-3f) + 5.5826318e-2f) + 2.4015361e-1f) + 6.9315308e-1f) + 9.9999994e-1f); }

inline float fExp2__(float x)
{
	BitTwiddler ipart, fpart;
	BitTwiddler expipart;

	static constexpr float f_hi = 129.00000f;
	static constexpr float f_lo = -126.99999f;

	x = std::min(x, f_hi);
	x = std::max(x, f_lo);

	ipart.i_ = (int)(x - 0.5f);
	fpart.f_ = (x - (float)(ipart.i_));
	expipart.i_ = ((ipart.i_ + 127) << 23);

	return (expipart.f_ * polyexp__(fpart.f_));
}

inline constexpr float polylog__(float x) { return (x * (x * (x * (x * (x * -3.4436006e-2f + 3.1821337e-1f) + -1.2315303f) + 2.5988452) + -3.3241990f) + 3.1157899f); }

inline float fLog2__(float x)
{
	BitTwiddler one, i, m, e;
	static constexpr int log_exp = 0x7F800000;
	static constexpr int log_mant = 0x7FFFFF;

	one.f_ = 1.0f;
	i.f_ = x;
	e.f_ = (float)(((i.i_ & log_exp) >> 23) - 127);
	m.i_ = ((i.i_ & log_mant) | one.i_);

	return (polylog__(m.f_) * (m.f_ - one.f_) + e.f_);
}

#ifdef FAST_MATH
inline float asmSqrt__(float n)
{
	float r = n;
#ifdef _MSC_VER
	r = sqrt(n);
#elif defined (__APPLE__)
	asm(
	    "flds %0;"
	    "fsqrt;"
	    "fstps %0"
	    :"=m"(r)
	    :"m"(r)
	);
#elif defined (__clang__)
	r = sqrtf(n);
#elif defined(__GNUC__) && defined(__i386__)
	asm(
	    "fld %0;"
	    "fsqrt;"
	    "fstp %0"
	    :"=m"(r)
	    :"m"(r)
	);
#elif defined(__GNUC__) && defined(__x86_64__)
	r = sqrt(n);
#else
	r = fsqrt(n);
#endif
	return r;
}
#endif

inline float fPow__(float a, float b)
{
#ifdef FAST_MATH
	return fExp2__(fLog2__(a) * b);
#else
	return pow(a, b);
#endif
}

inline float fLog__(float a)
{
#ifdef FAST_MATH
	return fLog2__(a) * (float) M_LN2;
#else
	return log(a);
#endif
}

inline float fExp__(float a)
{
#ifdef FAST_MATH
	return fExp2__((float) M_LOG2E * a);
#else
	return exp(a);
#endif
}

inline float fSqrt__(float a)
{
#ifdef FAST_MATH
	return asmSqrt__(a);
#else
	return sqrt(a);
#endif
}

inline float fLdexp__(float x, int a)
{
#ifdef FAST_MATH
	//return x * fPow(2.0, a);
	return ldexp(x, a);
#else
	return ldexp(x, a);
#endif
}

inline float fSin__(float x)
{
#ifdef FAST_TRIG
	if(x > mult_pi_by_2__ || x < -mult_pi_by_2__) x -= ((int)(x * (float)div_1_by_2pi__)) * (float)mult_pi_by_2__; //float modulo x % mult_pi_by_2__
	if(x < -M_PI)
	{
		x += (float)mult_pi_by_2__;
	}
	else if(x > M_PI)
	{
		x -= (float)mult_pi_by_2__;
	}

	x = ((float)div_4_by_pi__ * x) - ((float)div_4_by_squared_pi__ * x * std::fabs(x));
	static constexpr float const_p = 0.225f;
	const float result = const_p * (x * std::fabs(x) - x) + x;
	//Make sure that the function is in the valid range [-1.0,+1.0]
	if(result <= -1.0) return -1.0f;
	else if(result >= 1.0) return 1.0f;
	else return result;
#else
	return sin(x);
#endif
}

inline float fCos__(float x)
{
#ifdef FAST_TRIG
	return fSin__(x + (float) M_PI_2);
#else
	return cos(x);
#endif
}

inline float fAcos__(float x)
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.0) return((float)M_PI);
	else if(x >= 1.0) return(0.0);
	else return acos(x);
}

inline float fAsin__(float x)
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.0) return((float) - M_PI_2);
	else if(x >= 1.0) return((float)M_PI_2);
	else return asin(x);
}

END_YAFARAY

#endif
