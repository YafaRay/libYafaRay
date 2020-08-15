#pragma once

#ifndef Y_SPECTRUM_H
#define Y_SPECTRUM_H

#include <yafray_constants.h>
#include <core_api/color.h>

__BEGIN_YAFRAY

YAFRAYCORE_EXPORT void wl2rgb_fromCIE(float wl, color_t &col);
//YAFRAYCORE_EXPORT void approxSpectrumRGB(float wl, color_t &col);
//YAFRAYCORE_EXPORT void fakeSpectrum(float p, color_t &col);
YAFRAYCORE_EXPORT void CauchyCoefficients(float IOR, float disp_pw, float &CauchyA, float &CauchyB);
YAFRAYCORE_EXPORT float getIORcolor(float w, float CauchyA, float CauchyB, color_t &col);
YAFRAYCORE_EXPORT color_t wl2XYZ(float wl);

static inline float getIOR(float w, float CauchyA, float CauchyB)
{
	float wl = 300.0 * w + 400.0;
	return CauchyA + CauchyB / (wl * wl);
}

static inline void wl2rgb(float w, color_t &wl_col)
{
	float wl = 300.0 * w + 400.0;
	wl2rgb_fromCIE(wl, wl_col);
	wl_col *= 2.214032659670777114f;
}

__END_YAFRAY

#endif // Y_SPECTRUM_H
