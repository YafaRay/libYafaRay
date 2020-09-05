#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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
 */

#ifndef YAFARAY_UTIL_MATH_H
#define YAFARAY_UTIL_MATH_H

#include <cmath>

//#if ( defined(__i386__) || defined(_M_IX86) || defined(_X86_) )
//#define FAST_INT 1
static constexpr double doublemagicroundeps__ = (.5 - 1.4e-11);
//almost .5f = .5f - 1e^(number of exp bit)
static constexpr double doublemagic__ = double (6755399441055744.0);
//2^52 * 1.5,  uses limited precision to floor
//#endif


inline int round2Int__(double val)
{
#ifdef FAST_INT
	val		= val + doublemagic__;
	return ((long *)&val)[0];
#else
	//	#warning "using slow rounding"
	return int (val + doublemagicroundeps__);
#endif
}

inline int float2Int__(double val)
{
#ifdef FAST_INT
	return (val < 0) ?  round2Int__(val + doublemagicroundeps__) :
		   (val - doublemagicroundeps__);
#else
	//	#warning "using slow rounding"
	return (int)val;
#endif
}

inline int floor2Int__(double val)
{
#ifdef FAST_INT
	return round2Int__(val - doublemagicroundeps__);
#else
	//	#warning "using slow rounding"
	return (int)std::floor(val);
#endif
}

inline int ceil2Int__(double val)
{
#ifdef FAST_INT
	return round2Int__(val + doublemagicroundeps__);
#else
	//	#warning "using slow rounding"
	return (int)std::ceil(val);
#endif
}

inline double roundFloatPrecision__(double val, double precision) //To round, for example 3.2384764 to 3.24 use precision 0.01
{
	if(precision <= 0.0) return 0.0;
	else return std::round(val / precision) * precision;
}

inline bool isValidFloat__(float value)	//To check a float is not a NaN for example, even while using --fast-math compile flag
{
	return (value >= std::numeric_limits<float>::lowest() && value <= std::numeric_limits<float>::max());
}

#endif // YAFARAY_UTIL_MATH_H
