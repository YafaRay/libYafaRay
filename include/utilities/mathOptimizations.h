/*
 * mathOptimizations.h - Some experimetal math optimizations and some macros
 *
 *  Created on: 26/03/2009
 *
 *      Author: Rodrigo Placencia (DarkTide)
 *
 *		fPow() based on the polynomials approach form Jose Fonséca's blog entry:
 *		Fast SSE2 pow: tables or polynomials?
 *		http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html
 *		
 *		fSin() based on Fast and Accurate Trigonometric Functions on the HP-12C Platinum
 *		Posted by Gerson W. Barbosa on 6 Aug 2006, 3:10 p.m.
 *		http://www.hpmuseum.org/cgi-sys/cgiwrap/hpmuseum/articles.cgi?read=654
 *
 */
#ifndef MATHOPTIMIZATIONS_H_
#define MATHOPTIMIZATIONS_H_

#include <cmath>
#include <algorithm>

//# define M_E		2.7182818284590452354	/* e */
//# define M_LOG2E	1.4426950408889634074	/* log_2 e */
//# define M_LOG10E	0.43429448190325182765	/* log_10 e */
//# define M_LN2		0.69314718055994530942	/* log_e 2 */
//# define M_LN10		2.30258509299404568402	/* log_e 10 */
//# define M_PI		3.14159265358979323846	/* pi */
//# define M_PI_2		1.57079632679489661923	/* pi/2 */
//# define M_PI_4		0.78539816339744830962	/* pi/4 */
//# define M_1_PI		0.31830988618379067154	/* 1/pi */
//# define M_2_PI		0.63661977236758134308	/* 2/pi */
//# define M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
//# define M_SQRT2	1.41421356237309504880	/* sqrt(2) */
//# define M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */
#define FAST_MATH
#define FAST_TRIG

__BEGIN_YAFRAY

#define M_2PI		6.28318530717958647692
#define M_1_2PI		0.15915494309189533577

#define degToRad(deg) (deg * 0.01745329251994329576922)
#define radToDeg(rad) (rad * 57.29577951308232087684636)

#define POLYEXP(x) (x * (x * (x * (x * (x * 1.8775767e-3f + 8.9893397e-3f) + 5.5826318e-2f) + 2.4015361e-1f) + 6.9315308e-1f) + 9.9999994e-1f)
#define POLYLOG(x) (x * (x * (x * (x * (x * -3.4436006e-2f + 3.1821337e-1f) + -1.2315303f) + 2.5988452) + -3.3241990f) + 3.1157899f)

#define ca1  1.00000000000e+00
#define ca2 -1.66666666666e-01
#define ca3  8.33333320429e-03
#define ca4 -1.98410347969e-04
#define ca5  2.74201854577e-06

#define POLYMINMAX(x, x2) (x * (ca1 + x2 * (ca2 + x2 * (ca3 + x2 * (ca4 + x2 * ca5)))))

#define f_HI 129.00000f
#define f_LOW -126.99999f

union bitTwiddler
{
	int i;
	float f;
};

inline float fExp2(float x)
{
	bitTwiddler ipart, fpart;
	bitTwiddler expipart;

	x = std::min(x, f_HI);
	x = std::max(x, f_LOW);

	ipart.i = (int)(x - 0.5f);
	fpart.f = (x - (float)(ipart.i));
	expipart.i = ((ipart.i + 127) << 23);

	return expipart.f * POLYEXP(fpart.f);
}

inline float fLog2(float x)
{
	int exp = 0x7F800000;
	int mant = 0x7FFFFF;
	bitTwiddler one, i, m, e;

	one.f = 1.0f;
	i.f = x;
	e.f = (float)(((i.i & exp) >> 23) - 127);
	m.i = ((i.i & mant) | one.i);

	return POLYLOG(m.f) * (m.f - one.f) + e.f;
}

inline float fPow(float a, float b)
{
#ifdef FAST_MATH
	return fExp2(fLog2(a) * b);
#else
	return pow(a,b);
#endif
}

inline float fExp(float a)
{
#ifdef FAST_MATH
	return fExp2(M_LOG2E * a);
#else
	return exp(a);
#endif
}

inline float fSqrt(float a)
{
#ifdef FAST_MATH
	return fExp2(fLog2(a) * 0.5);
#else
	return sqrt(a);
#endif
}

inline float fLdexp(float x, int a)
{
#ifdef FAST_MATH
	return x * fPow(2.0, a);
#else
	return ldexp(x,a);
#endif
}

inline float fSin(float x)
{
#ifdef FAST_TRIG
	float nx = x * 3.3333333333e-1f;
	float x2 = nx * nx;
	float y = POLYMINMAX(nx, x2);
	return (y * (3.0 - (4.0 * y * y)));
#else
	return sin(x);
#endif
}

inline float fCos(float x)
{
#ifdef FAST_TRIG
	float nx = (M_PI_2 - x) * 3.3333333333e-1f;
	float x2 = nx * nx;
	float y = POLYMINMAX(nx, x2);
	return (y * (3.0 - (4.0 * y * y)));
#else
	return cos(x);
#endif
}

inline float fTan(float x)
{
#ifdef FAST_TRIG
	return fSin(x) / fCos(x);
#else
	return tan(x);
#endif
}
inline float fArcTan(float x)
{
#ifdef FAST_TRIG
	float ax = fabs(x);
	return (M_PI_2 - fArcTan(1.0/ax)) * x/ax;
#else
	return tan(x);
#endif
}

__END_YAFRAY

#endif
