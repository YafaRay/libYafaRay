#pragma once

#ifndef YAFARAY_SPECTRUM_H
#define YAFARAY_SPECTRUM_H

#include <yafray_constants.h>
#include <core_api/color.h>

BEGIN_YAFRAY

YAFRAYCORE_EXPORT void wl2RgbFromCie__(float wl, Rgb &col);
//YAFRAYCORE_EXPORT void approxSpectrumRGB(float wl, Rgb &col);
//YAFRAYCORE_EXPORT void fakeSpectrum(float p, Rgb &col);
YAFRAYCORE_EXPORT void cauchyCoefficients__(float ior, float disp_pw, float &cauchy_a, float &cauchy_b);
YAFRAYCORE_EXPORT float getIoRcolor__(float w, float cauchy_a, float cauchy_b, Rgb &col);
YAFRAYCORE_EXPORT Rgb wl2Xyz__(float wl);

static inline float getIor__(float w, float cauchy_a, float cauchy_b)
{
	float wl = 300.0 * w + 400.0;
	return cauchy_a + cauchy_b / (wl * wl);
}

static inline void wl2Rgb__(float w, Rgb &wl_col)
{
	float wl = 300.0 * w + 400.0;
	wl2RgbFromCie__(wl, wl_col);
	wl_col *= 2.214032659670777114f;
}

END_YAFRAY

#endif // YAFARAY_SPECTRUM_H
