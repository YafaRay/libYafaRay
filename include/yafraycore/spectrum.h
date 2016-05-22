#ifndef Y_SPECTRUM_H
#define Y_SPECTRUM_H

#include <core_api/color.h>

__BEGIN_YAFRAY

YAFRAYCORE_EXPORT void wl2rgb_fromCIE(CFLOAT wl, color_t &col);
//YAFRAYCORE_EXPORT void approxSpectrumRGB(CFLOAT wl, color_t &col);
//YAFRAYCORE_EXPORT void fakeSpectrum(CFLOAT p, color_t &col);
YAFRAYCORE_EXPORT void CauchyCoefficients(PFLOAT IOR, PFLOAT disp_pw, PFLOAT &CauchyA, PFLOAT &CauchyB);
YAFRAYCORE_EXPORT PFLOAT getIORcolor(PFLOAT w, PFLOAT CauchyA, PFLOAT CauchyB, color_t &col);
YAFRAYCORE_EXPORT color_t wl2XYZ(CFLOAT wl);

static inline PFLOAT getIOR(PFLOAT w, PFLOAT CauchyA, PFLOAT CauchyB)
{
	PFLOAT wl = 300.0*w + 400.0;
	return CauchyA + CauchyB/(wl*wl);
}

static inline void wl2rgb(float w, color_t &wl_col)
{
	PFLOAT wl = 300.0*w + 400.0;
	wl2rgb_fromCIE(wl, wl_col);
	wl_col *= 2.214032659670777114f;
}

__END_YAFRAY

#endif // Y_SPECTRUM_H
