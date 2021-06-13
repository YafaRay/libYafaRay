#pragma once
/****************************************************************************
 *
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
 *
 */

#ifndef YAFARAY_FILTER_H
#define YAFARAY_FILTER_H

#include "yafaray_conf.h"
#include "math/math.h"

BEGIN_YAFARAY

namespace math
{
namespace filter
{

float box(float dx, float dy) { return 1.f; }

float gauss(float dx, float dy)
{
	constexpr float gauss_exp = 0.00247875f;
	const float r_2 = dx * dx + dy * dy;
	return std::max(0.f, math::exp(-6 * r_2) - gauss_exp);
}

//Lanczos sinc window size 2
float lanczos2(float dx, float dy)
{
	const float x = math::sqrt(dx * dx + dy * dy);
	if(x == 0.f) return 1.f;
	if(-2 < x && x < 2)
	{
		const float a = math::num_pi * x;
		const float b = math::div_pi_by_2 * x;
		return ((math::sin(a) * math::sin(b)) / (a * b));
	}
	return 0.f;
}

float mitchell(float dx, float dy)
{
/*!
Mitchell-Netravali constants
with B = 1/3 and C = 1/3 as suggested by the authors
mnX1 = constants for 1 <= |x| < 2
mna1 = (-B - 6 * C)/6
mnb1 = (6 * B + 30 * C)/6
mnc1 = (-12 * B - 48 * C)/6
mnd1 = (8 * B + 24 * C)/6

mnX2 = constants for 1 > |x|
mna2 = (12 - 9 * B - 6 * C)/6
mnb2 = (-18 + 12 * B + 6 * C)/6
mnc2 = (6 - 2 * B)/6

#define mna1 -0.38888889
#define mnb1  2.0
#define mnc1 -3.33333333
#define mnd1  1.77777778

#define mna2  1.16666666
#define mnb2 -2.0
#define mnc2  0.88888889
*/

	const float x = 2.f * math::sqrt(dx * dx + dy * dy);
	if(x >= 2.f) return (0.f);
	if(x >= 1.f) // from mitchell-netravali paper 1 <= |x| < 2
	{
		return x * (x * (x * -0.38888889f + 2.0f) - 3.33333333f) + 1.77777778f;
	}
	return x * x * (1.16666666f * x - 2.0f) + 0.88888889f;
}

} // namespace filter

} // namespace math

END_YAFARAY

#endif //YAFARAY_FILTER_H
