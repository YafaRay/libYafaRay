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

#ifndef YAFARAY_SPECTRUM_H
#define YAFARAY_SPECTRUM_H

#include "common/yafaray_common.h"
#include "color/color.h"

namespace yafaray {

namespace spectrum
{

Rgb wl2RgbFromCie(float wl);
void cauchyCoefficients(float ior, float disp_pw, float &cauchy_a, float &cauchy_b);
Rgb wl2Xyz(float wl);

inline float getIor(float w, float cauchy_a, float cauchy_b)
{
	const float wl = 300.f * w + 400.f;
	return cauchy_a + cauchy_b / (wl * wl);
}

inline Rgb wl2Rgb(float w)
{
	const float wl = 300.f * w + 400.f;
	Rgb wl_col = wl2RgbFromCie(wl);
	wl_col *= 2.214032659670777114f;
	return wl_col;
}

} //namespace spectrum

} //namespace yafaray

#endif // YAFARAY_SPECTRUM_H
