#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      colorConv.h: 	Color converter from CIE XYZ color space to CIE RGB
 *      Created on: 20/03/2009
 *      Author: Rodrigo Placencia (DarkTide)
 *      colorConv class
 *      based on "A review of RGB color spaces..." by Danny Pascale
 *      and info from http://www.brucelindbloom.com/
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
/*
 *
 */

#ifndef YAFARAY_COLOR_CONVERSION_H
#define YAFARAY_COLOR_CONVERSION_H

#include "common/yafaray_common.h"
#include "color.h"

BEGIN_YAFARAY

constexpr float cie_rgb_e_global[9] =
{
	2.3706743f, -0.9000405f, -0.4706338f,
	-0.5138850f,  1.4253036f,  0.0885814f,
	0.0052982f, -0.0146949f,  1.0093968f
};

constexpr float cie_rgb_d_50_global[9] =
{
	2.3638081f, -0.8676030f, -0.4988161f,
	-0.5005940f,  1.3962369f,  0.1047562f,
	0.0141712f, -0.0306400f,  1.2323842f
};

constexpr float s_rgb_d_65_global[9] =
{
	3.2404542f, -1.5371385f, -0.4985314f,
	-0.9692660f,  1.8760108f,  0.0415560f,
	0.0556434f, -0.2040259f,  1.0572252f
};

constexpr float s_rgb_d_50_global[9] =
{
	3.1338561f, -1.6168667f, -0.4906146f,
	-0.9787684f,  1.9161415f,  0.0334540f,
	0.0719453f, -0.2289914f,  1.4052427f
};

class ColorConv final
{
	public:
		enum ColorSpace { CieRgbECs, CieRgbD50Cs, SRgbD50Cs, SRgbD65Cs };
		ColorConv(bool cl = false, bool g_enc = false, ColorSpace cs = CieRgbECs, float exposure = 0.f);
		Rgb fromXyz(Rgb &c, bool force_gamma = false) const;
		Rgb fromXyz(float x, float y, float z, bool force_gamma = false) const;
		Rgb fromxyY(float x, float y, float Y) const;
		Rgb fromxyY2Xyz(float x, float y, float Y) const;

	private:
		float sGammaEnc(float v) const;

		float simple_g_enc_;
		bool clamp_;
		float exp_;
		ColorSpace color_space_;
		const float *mat_ = nullptr;
		bool encode_gamma_;
};


inline ColorConv::ColorConv(bool cl, bool g_enc, ColorSpace cs, float exposure) :
		simple_g_enc_(1.0 / 2.2), clamp_(cl), exp_(exposure), color_space_(cs), encode_gamma_(g_enc)
{
	switch(color_space_)
	{
		case CieRgbECs:
			mat_ = cie_rgb_e_global;
			break;

		case CieRgbD50Cs:
			mat_ = cie_rgb_d_50_global;
			break;

		case SRgbD50Cs:
			mat_ = s_rgb_d_50_global;
			break;

		case SRgbD65Cs:
			mat_ = s_rgb_d_65_global;
			break;
	}
}

inline Rgb ColorConv::fromXyz(float x, float y, float z, bool force_gamma) const
{
	Rgb ret(0.f);

	if(encode_gamma_ || force_gamma)
	{
		ret.set(
		    sGammaEnc((mat_[0] * x) + (mat_[1] * y) + (mat_[2] * z)),
		    sGammaEnc((mat_[3] * x) + (mat_[4] * y) + (mat_[5] * z)),
		    sGammaEnc((mat_[6] * x) + (mat_[7] * y) + (mat_[8] * z))
		);
	}
	else
	{
		ret.set(
		    ((mat_[0] * x) + (mat_[1] * y) + (mat_[2] * z)),
		    ((mat_[3] * x) + (mat_[4] * y) + (mat_[5] * z)),
		    ((mat_[6] * x) + (mat_[7] * y) + (mat_[8] * z))
		);
	}

	if(clamp_) ret.clampRgb01();

	return ret;
}

inline Rgb ColorConv::fromXyz(Rgb &c, bool force_gamma) const
{
	return fromXyz(c.r_, c.g_, c.b_, force_gamma);
}

inline Rgb ColorConv::fromxyY(float x, float y, float Y) const
{
	Rgb temp_col = fromxyY2Xyz(x, y, Y);
	return fromXyz(temp_col);
}

inline Rgb ColorConv::fromxyY2Xyz(float x, float y, float Y) const
{
	Rgb ret(0.0);
	float ratio, X, z;

	if(exp_ > 0.f) Y = math::exp(Y * exp_) - 1.f;

	if(y != 0.0)
	{
		ratio = (Y / y);
	}
	else
	{
		return ret;
	}

	X = x * ratio;
	z = (1.0 - x - y) * ratio;

	ret.set(X, Y, z);

	return ret;
}

inline float ColorConv::sGammaEnc(float v) const
{
	return math::pow(v, simple_g_enc_);
}

END_YAFARAY

#endif /* YAFARAY_COLOR_CONVERSION_H */
