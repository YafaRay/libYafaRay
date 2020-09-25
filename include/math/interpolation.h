#pragma once
/****************************************************************************
 *      interpolation.h: Some interpolation algorithms
 *      This is part of the libYafaRay package
 *      Copyright (C) 2009  Bert Buchholz
 *      Split into a header and some speedups by Rodrigo Placencia
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

#ifndef YAFARAY_INTERPOLATION_H
#define YAFARAY_INTERPOLATION_H

BEGIN_YAFARAY

// Algorithms from: http://local.wasp.uwa.edu.au/~pbourke/miscellaneous/interpolation/

namespace math
{

template<typename Y, typename X>
inline static Y lerp(
		Y const &y_1, Y const &y_2,
		const X &x)
{
	return y_1 * x + y_2 * (1.0 - x);
}

template<typename Y, typename X>
inline Y lerpSegment(const X &x, const Y &y_1, const X &x_1, const Y &y_2, const X &x_2)
{
	if(x == x_1 || x_1 == x_2) return y_1;
	else if(x == x_2) return y_2;

	const Y diff_y2_y1 = y_2 - y_1;
	const X diff_x2_x1 = x_2 - x_1;
	const X diff_alpha_x1 = x - x_1;

	return y_1 + ((diff_alpha_x1 / diff_x2_x1) * diff_y2_y1);
}

template<typename Y, typename X>
inline static Y cosineInterpolate(
		const Y &y_1, const Y &y_2,
		const X &x)
{
	const double x2 = (1 - math::cos(x * M_PI)) * 0.5f;
	return (y_1 * (1 - x2) + y_2 * x2);
}

template<class Y, typename X>
inline Y cubicInterpolate(
		const Y &y_0, const Y &y_1,
		const Y &y_2, const Y &y_3,
		const X &x)
{
	const X x_2 = x * x;
	const Y a_0 = y_3 - y_2 - y_0 + y_1;
	const Y a_1 = y_0 - y_1 - a_0;
	const Y a_2 = y_2 - y_0;
	const Y a_3 = y_1;

	return (a_0 * x * x_2 + a_1 * x_2 + a_2 * x + a_3);
}

} // namespace math

END_YAFARAY

#endif //YAFARAY_INTERPOLATION_H
