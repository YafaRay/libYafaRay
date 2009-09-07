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

static const float cieRGB_E[3][3] =
{
	{ 2.3706743,-0.5138850, 0.0052982},
	{-0.9000405, 1.4253036,-0.0146949},
	{-0.4706338, 0.0885814, 1.0093968}
};

/*static const float sRGB_D65[3][3] =
{
	{ 3.2404542, -0.9692660,  0.0556434},
	{-1.5371385,  1.8760108, -0.2040259},
	{-0.4985314,  0.0415560,  1.0572252}
};*/

class ColorConv
{
public:
	ColorConv(bool cl = false):simpleGEnc(1.0/2.2), scale(0.01), clamp(cl), exp(1.1)
	{
	};
	color_t fromXYZ(color_t &c) const;
	color_t fromXYZ(float x, float y, float z) const;
	color_t fromxyY(float x, float y, float Y) const;
	color_t fromxyY2XYZ(float x, float y, float Y) const;
private:
	float simpleGEnc;
	float scale;
	bool clamp;
	float exp;

	float sGammaEnc(float v) const;
};

inline color_t ColorConv::fromXYZ(float x, float y, float z) const
{
	color_t ret(x,y,z);

 	ret.set(
		sGammaEnc((cieRGB_E[0][0] * ret.R) + (cieRGB_E[1][0] * ret.G) + (cieRGB_E[2][0] * ret.B)),
		sGammaEnc((cieRGB_E[0][1] * ret.R) + (cieRGB_E[1][1] * ret.G) + (cieRGB_E[2][1] * ret.B)),
		sGammaEnc((cieRGB_E[0][2] * ret.R) + (cieRGB_E[1][2] * ret.G) + (cieRGB_E[2][2] * ret.B))
	);

	if(clamp) ret.clampRGB01();

	return ret;
}

inline color_t ColorConv::fromXYZ(color_t &c) const
{
	return fromXYZ(c.R, c.G, c.B);
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
	float ret = fPow(v, simpleGEnc) * scale; //we need to scale the resulting value because is in [0,100] range
	return ret;
}

__END_YAFRAY

#endif /* COLOR_CONV_H_ */
