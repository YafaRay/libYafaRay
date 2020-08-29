#pragma once
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

#ifndef YAFARAY_COLORCONV_H
#define YAFARAY_COLORCONV_H

#include <yafray_constants.h>
#include <core_api/color.h>

BEGIN_YAFRAY

constexpr float cie_rgb_e__[9] =
{
	2.3706743, -0.9000405, -0.4706338,
	-0.5138850,  1.4253036,  0.0885814,
	0.0052982, -0.0146949,  1.0093968
};

constexpr float cie_rgb_d_50__[9] =
{
	2.3638081, -0.8676030, -0.4988161,
	-0.5005940,  1.3962369,  0.1047562,
	0.0141712, -0.0306400,  1.2323842
};

constexpr float s_rgb_d_65__[9] =
{
	3.2404542, -1.5371385, -0.4985314,
	-0.9692660,  1.8760108,  0.0415560,
	0.0556434, -0.2040259,  1.0572252
};

constexpr float s_rgb_d_50__[9] =
{
	3.1338561, -1.6168667, -0.4906146,
	-0.9787684,  1.9161415,  0.0334540,
	0.0719453, -0.2289914,  1.4052427
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
			mat_ = cie_rgb_e__;
			break;

		case CieRgbD50Cs:
			mat_ = cie_rgb_d_50__;
			break;

		case SRgbD50Cs:
			mat_ = s_rgb_d_50__;
			break;

		case SRgbD65Cs:
			mat_ = s_rgb_d_65__;
			break;
	}
};

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

	if(exp_ > 0.f) Y = fExp__(Y * exp_) - 1.f;

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
	return fPow__(v, simple_g_enc_);
}

END_YAFRAY

#endif /* YAFARAY_COLORCONV_H */
