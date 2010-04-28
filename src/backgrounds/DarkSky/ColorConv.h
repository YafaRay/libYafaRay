/*
 * colorConv.h: 	Color converter from CIE XYZ color space to CIE RGB
 *
 *  Created on: 20/03/2009
 *
 *      Author: Rodrigo Placencia (DarkTide)
 *
 * 		colorConv class
 *		based on "A review of RGB color spaces..." by Danny Pascale
 *		and info from http://www.brucelindbloom.com/
 *
 */

#ifndef COLOR_CONV_H_
#define COLOR_CONV_H_

#include <core_api/color.h>
#include <utilities/mathOptimizations.h>


__BEGIN_YAFRAY

enum ColorSpaces
{
	cieRGB_E_CS,
	cieRGB_D50_CS,
	sRGB_D50_CS,
	sRGB_D65_CS
};

static float cieRGB_E[9] =
{
	 2.3706743, -0.9000405, -0.4706338,
	-0.5138850,  1.4253036,  0.0885814,
	 0.0052982, -0.0146949,  1.0093968
};

static float cieRGB_D50[9] =
{
	 2.3638081, -0.8676030, -0.4988161,
	-0.5005940,  1.3962369,  0.1047562,
	 0.0141712, -0.0306400,  1.2323842
};

static float sRGB_D65[9] =
{
	 3.2404542, -1.5371385, -0.4985314,
	-0.9692660,  1.8760108,  0.0415560,
	 0.0556434, -0.2040259,  1.0572252
};

static float sRGB_D50[9] =
{
	 3.1338561, -1.6168667, -0.4906146,
	-0.9787684,  1.9161415,  0.0334540,
	 0.0719453, -0.2289914,  1.4052427
};

class ColorConv
{
public:
	ColorConv(bool cl = false, bool gEnc = false, ColorSpaces cs = cieRGB_E_CS, float exposure = 0.f) :
	simpleGEnc(1.0/2.2), clamp(cl), exp(exposure), colorSpace(cs), encodeGamma(gEnc)
	{
		switch(colorSpace)
		{
			case cieRGB_E_CS:
				mat = cieRGB_E;
				break;

			case cieRGB_D50_CS:
				mat = cieRGB_D50;
				break;

			case sRGB_D50_CS:
				mat = sRGB_D50;
				break;

			case sRGB_D65_CS:
				mat = sRGB_D65;
				break;
		}
	};
	color_t fromXYZ(color_t &c, bool forceGamma = false) const;
	color_t fromXYZ(float x, float y, float z, bool forceGamma = false) const;
	color_t fromxyY(float x, float y, float Y) const;
	color_t fromxyY2XYZ(float x, float y, float Y) const;

private:

	float simpleGEnc;
	bool clamp;
	float exp;
	ColorSpaces colorSpace;
	float *mat;
	bool encodeGamma;

	float sGammaEnc(float v) const;
};

inline color_t ColorConv::fromXYZ(float x, float y, float z, bool forceGamma) const
{
	color_t ret(0.f);
	
	if(encodeGamma || forceGamma)
	{
		ret.set(
			sGammaEnc((mat[0] * x) + (mat[1] * y) + (mat[2] * z)),
			sGammaEnc((mat[3] * x) + (mat[4] * y) + (mat[5] * z)),
			sGammaEnc((mat[6] * x) + (mat[7] * y) + (mat[8] * z))
		);
	}
	else
	{
		ret.set(
			((mat[0] * x) + (mat[1] * y) + (mat[2] * z)),
			((mat[3] * x) + (mat[4] * y) + (mat[5] * z)),
			((mat[6] * x) + (mat[7] * y) + (mat[8] * z))
		);
	}

	if(clamp) ret.clampRGB01();

	return ret;
}

inline color_t ColorConv::fromXYZ(color_t &c, bool forceGamma) const
{
	return fromXYZ(c.R, c.G, c.B, forceGamma);
}

inline color_t ColorConv::fromxyY(float x, float y, float Y) const
{
	color_t tempCol = fromxyY2XYZ(x,y,Y);
	return fromXYZ(tempCol);
}

inline color_t ColorConv::fromxyY2XYZ(float x, float y, float Y) const
{
	color_t ret(0.0);
	float ratio, X, Z;
	
	if(exp > 0.f) Y = fExp(Y * exp) - 1.f;
	
	if(y != 0.0)
	{
		ratio = (Y / y);
	}
	else
	{
		return ret;
	}

	X = x * ratio;
	Z = (1.0 - x - y) * ratio;

	ret.set(X, Y, Z);

	return ret;
}

inline float ColorConv::sGammaEnc(float v) const
{
	return fPow(v, simpleGEnc);;
}

__END_YAFRAY

#endif /* COLOR_CONV_H_ */
