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

BEGIN_YAFRAY

// Algorithms from: http://local.wasp.uwa.edu.au/~pbourke/miscellaneous/interpolation/

template <typename T>
inline static T lerp__(
		T const &y_1, T const &y_2,
		double alpha)
{
	return y_1 * alpha + y_2 * (1.0 - alpha);
}

inline static double cosineInterpolate__(
		double y_1, double y_2,
		double mu)
{
	double mu_2;

	mu_2 = (1 - fCos__(mu * M_PI)) * 0.5f;
	return (y_1 * (1 - mu_2) + y_2 * mu_2);
}

template <class T>
inline T cubicInterpolate__(
		const T &y_0, const T &y_1,
		const T &y_2, const T &y_3,
		double mu)
{
	T a_0, a_1, a_2, a_3, mu_2;

	mu_2 = mu * mu;
	a_0 = y_3 - y_2 - y_0 + y_1;
	a_1 = y_0 - y_1 - a_0;
	a_2 = y_2 - y_0;
	a_3 = y_1;

	return (a_0 * mu * mu_2 + a_1 * mu_2 + a_2 * mu + a_3);
}

END_YAFRAY

#endif //YAFARAY_INTERPOLATION_H
