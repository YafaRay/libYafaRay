#pragma once
/****************************************************************************
 *
 *      color.h: Color type and operators api
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002  Alejandro Conty Estévez
 *      Copyright (C) 2015  David Bluecame for Color Space modifications
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
 *
 */
#ifndef YAFARAY_COLOR_H
#define YAFARAY_COLOR_H

#include <yafray_constants.h>
#include <utilities/mathOptimizations.h>
#include <iostream>

BEGIN_YAFRAY

enum ColorSpace : int
{
	RawManualGamma	= 1,
	LinearRgb		= 2,
	Srgb			= 3,
	XyzD65			= 4
};

class YAFRAYCORE_EXPORT Rgb
{
	public:
		Rgb() = default;
		Rgb(float r, float g, float b) : r_(r), g_(g), b_(b) { }
		Rgb(float f) : r_(f), g_(f), b_(f) { }
		Rgb(float af[3]) : r_(af[0]), g_(af[1]), b_(af[2]) { }
		bool isBlack() const { return ((r_ == 0) && (g_ == 0) && (b_ == 0)); }
		bool isNaN() const { return (std::isnan(r_) || std::isnan(g_) || std::isnan(b_)); }
		bool isInf() const { return (std::isinf(r_) || std::isinf(g_) || std::isinf(b_)); }
		void set(float r, float g, float b) { r_ = r; g_ = g; b_ = b; }

		Rgb &operator +=(const Rgb &c);
		Rgb &operator -=(const Rgb &c);
		Rgb &operator *=(const Rgb &c);
		Rgb &operator *=(float f);

		float energy() const {return (r_ + g_ + b_) * 0.333333f;};
		// Using ITU/Photometric values Y = 0.2126 R + 0.7152 G + 0.0722 B
		float col2Bri() const { return (0.2126f * r_ + 0.7152f * g_ + 0.0722f * b_); }
		float abscol2Bri() const { return (0.2126f * std::fabs(r_) + 0.7152f * std::fabs(g_) + 0.0722f * std::fabs(b_)); }
		void gammaAdjust(float g) { r_ = fPow__(r_, g); g_ = fPow__(g_, g); b_ = fPow__(b_, g); }
		void expgamAdjust(float e, float g, bool clamp_rgb);
		float getR() const { return r_; }
		float getG() const { return g_; }
		float getB() const { return b_; }

		// used in blendershader
		void invertRgb()
		{
			if(r_ != 0.f) r_ = 1.f / r_;
			if(g_ != 0.f) g_ = 1.f / g_;
			if(b_ != 0.f) b_ = 1.f / b_;
		}
		void absRgb() { r_ = std::fabs(r_); g_ = std::fabs(g_); b_ = std::fabs(b_); }
		void darkenRgb(const Rgb &col)
		{
			if(r_ > col.r_) r_ = col.r_;
			if(g_ > col.g_) g_ = col.g_;
			if(b_ > col.b_) b_ = col.b_;
		}
		void lightenRgb(const Rgb &col)
		{
			if(r_ < col.r_) r_ = col.r_;
			if(g_ < col.g_) g_ = col.g_;
			if(b_ < col.b_) b_ = col.b_;
		}

		void black() { r_ = g_ = b_ = 0; }
		float minimum() const { return std::min(r_, std::min(g_, b_)); }
		float maximum() const { return std::max(r_, std::max(g_, b_)); }
		float absmax() const { return std::max(std::fabs(r_), std::max(std::fabs(g_), std::fabs(b_))); }
		void clampRgb0()
		{
			if(r_ < 0.0) r_ = 0.0;
			if(g_ < 0.0) g_ = 0.0;
			if(b_ < 0.0) b_ = 0.0;
		}

		void clampRgb01()
		{
			if(r_ < 0.0) r_ = 0.0; else if(r_ > 1.0) r_ = 1.0;
			if(g_ < 0.0) g_ = 0.0; else if(g_ > 1.0) g_ = 1.0;
			if(b_ < 0.0) b_ = 0.0; else if(b_ > 1.0) b_ = 1.0;
		}

		void blend(const Rgb &col, float blend_factor)
		{
			r_ = r_ * (1.f - blend_factor) + col.r_ * blend_factor;
			g_ = g_ * (1.f - blend_factor) + col.g_ * blend_factor;
			b_ = b_ * (1.f - blend_factor) + col.b_ * blend_factor;
		}

		void ceil() //Mainly used for Absolute Object/Material Index passes, to correct the antialiasing and ceil the "mixed" values to the upper integer
		{
			r_ = ceilf(r_);
			g_ = ceilf(g_);
			b_ = ceilf(b_);
		}

		void clampProportionalRgb(float max_value);

		float linearRgbFromSRgb(float value_s_rgb);
		float sRgbFromLinearRgb(float value_linear_rgb);

		void linearRgbFromColorSpace(ColorSpace color_space, float gamma);
		void colorSpaceFromLinearRgb(ColorSpace color_space, float gamma);
		void rgbToHsv(float &h, float &s, float &v) const;
		void hsvToRgb(const float &h, const float &s, const float &v);
		void rgbToHsl(float &h, float &s, float &l) const;
		void hslToRgb(const float &h, const float &s, const float &l);

		float r_ = 0.f;
		float g_ = 0.f;
		float b_ = 0.f;
};

class YAFRAYCORE_EXPORT Rgba final : public Rgb
{
	public:
		Rgba() = default;
		Rgba(const Rgb &c): Rgb(c), a_(1.f) { }
		Rgba(const Rgb &c, float a): Rgb(c), a_(a) { }
		Rgba(float r, float g, float b, float a = 1.f): Rgb(r, g, b), a_(a) { }
		Rgba(float g): Rgb(g), a_(g) { }
		Rgba(float g, float a): Rgb(g), a_(a) { }
		Rgba(float af[4]): Rgb(af), a_(af[3]) { }

		void set(float r, float g, float b, float a = 1.f) { Rgb::set(r, g, b); a_ = a; }

		Rgba &operator +=(const Rgba &c);
		Rgba &operator -=(const Rgba &c);
		Rgba &operator *=(const Rgba &c);
		Rgba &operator *=(float f);

		void alphaPremultiply() { r_ *= a_; g_ *= a_; b_ *= a_; }
		float getA() const { return a_; }
		void setAlpha(float a) { a_ = a; }

		void clampRgba0()
		{
			clampRgb0();
			if(a_ < 0.0) a_ = 0.0;
		}

		void clampRgba01()
		{
			clampRgb01();
			if(a_ < 0.0) a_ = 0.0; else if(a_ > 1.0) a_ = 1.0;
		}

		void blend(const Rgba &col, float blend_factor)
		{
			r_ = r_ * (1.f - blend_factor) + col.r_ * blend_factor;
			g_ = g_ * (1.f - blend_factor) + col.g_ * blend_factor;
			b_ = b_ * (1.f - blend_factor) + col.b_ * blend_factor;
			a_ = a_ * (1.f - blend_factor) + col.a_ * blend_factor;
		}

		void ceil() //Mainly used for Absolute Object/Material Index passes, to correct the antialiasing and ceil the "mixed" values to the upper integer
		{
			r_ = ceilf(r_);
			g_ = ceilf(g_);
			b_ = ceilf(b_);
			a_ = ceilf(a_);
		}

		float colorDifference(Rgba color_2, bool use_rg_bcomponents = false);

		float a_ = 1.f;
};

class YAFRAYCORE_EXPORT Rgbe final
{
	public:
		Rgbe() { rgbe_[3] = 0;};
		Rgbe(const Rgb &s);
		operator Rgb () const
		{
			Rgb res;
			float f;
			if(rgbe_[3])
			{
				/*nonzero pixel*/
				f = fLdexp__(1.0, rgbe_[3] - (int) (128 + 8));
				return Rgb(rgbe_[0] * f, rgbe_[1] * f, rgbe_[2] * f);
			}
			else return Rgb(0, 0, 0);
		}
		//		unsigned char& operator [] (int i){ return rgbe[i]; }

		//	protected:
		unsigned char rgbe_[4];
};

inline void Rgb::expgamAdjust(float e, float g, bool clamp_rgb)
{
	if((e == 0.f) && (g == 1.f))
	{
		if(clamp_rgb) clampRgb01();
		return;
	}
	if(e != 0.f)
	{
		// exposure adjust
		clampRgb0();
		r_ = 1.f - fExp__(r_ * e);
		g_ = 1.f - fExp__(g_ * e);
		b_ = 1.f - fExp__(b_ * e);
	}
	if(g != 1.f)
	{
		// gamma adjust
		clampRgb0();
		r_ = fPow__(r_, g);
		g_ = fPow__(g_, g);
		b_ = fPow__(b_, g);
	}
}

YAFRAYCORE_EXPORT void operator >> (unsigned char *data, Rgb &c);
YAFRAYCORE_EXPORT void operator << (unsigned char *data, const Rgb &c);
YAFRAYCORE_EXPORT void operator >> (float *data, Rgb &c);
YAFRAYCORE_EXPORT void operator << (float *data, const Rgb &c);
YAFRAYCORE_EXPORT std::ostream &operator << (std::ostream &out, const Rgb c);
YAFRAYCORE_EXPORT Rgb mix__(const Rgb &a, const Rgb &b, float point);

YAFRAYCORE_EXPORT void operator >> (unsigned char *data, Rgba &c);
YAFRAYCORE_EXPORT void operator << (unsigned char *data, const Rgba &c);
YAFRAYCORE_EXPORT void operator >> (float *data, Rgba &c);
YAFRAYCORE_EXPORT void operator << (float *data, const Rgba &c);
YAFRAYCORE_EXPORT std::ostream &operator << (std::ostream &out, const Rgba c);
YAFRAYCORE_EXPORT Rgba mix__(const Rgba &a, const Rgba &b, float point);


inline Rgb operator * (const Rgb &a, const Rgb &b)
{
	return Rgb(a.r_ * b.r_, a.g_ * b.g_, a.b_ * b.b_);
}

inline Rgb operator * (const float f, const Rgb &b)
{
	return Rgb(f * b.r_, f * b.g_, f * b.b_);
}

inline Rgb operator * (const Rgb &b, const float f)
{
	return Rgb(f * b.r_, f * b.g_, f * b.b_);
}

inline Rgb operator / (const Rgb &b, float f)
{
	return Rgb(b.r_ / f, b.g_ / f, b.b_ / f);
}

inline Rgb operator + (const Rgb &a, const Rgb &b)
{
	return Rgb(a.r_ + b.r_, a.g_ + b.g_, a.b_ + b.b_);
}

inline Rgb operator - (const Rgb &a, const Rgb &b)
{
	return Rgb(a.r_ - b.r_, a.g_ - b.g_, a.b_ - b.b_);
}

inline Rgb &Rgb::operator +=(const Rgb &c)
{ r_ += c.r_; g_ += c.g_; b_ += c.b_;  return *this; }
inline Rgb &Rgb::operator *=(const Rgb &c)
{ r_ *= c.r_; g_ *= c.g_; b_ *= c.b_;  return *this; }
inline Rgb &Rgb::operator *=(float f)
{ r_ *= f; g_ *= f; b_ *= f;  return *this; }
inline Rgb &Rgb::operator -=(const Rgb &c)
{ r_ -= c.r_; g_ -= c.g_; b_ -= c.b_;  return *this; }

inline Rgba operator * (const Rgba &a, const Rgba &b)
{
	return Rgba(a.r_ * b.r_, a.g_ * b.g_, a.b_ * b.b_, a.a_ * b.a_);
}

inline Rgba operator * (const float f, const Rgba &b)
{
	return Rgba(f * b.r_, f * b.g_, f * b.b_, f * b.a_);
}

inline Rgba operator * (const Rgba &b, const float f)
{
	return Rgba(f * b.r_, f * b.g_, f * b.b_, f * b.a_);
}

inline Rgba operator / (const Rgba &b, float f)
{
	if(f != 0) f = 1.0 / f;
	return Rgba(b.r_ * f, b.g_ * f, b.b_ * f, b.a_ * f);
}

inline Rgba operator + (const Rgba &a, const Rgba &b)
{
	return Rgba(a.r_ + b.r_, a.g_ + b.g_, a.b_ + b.b_, a.a_ + b.a_);
}

inline Rgba operator - (const Rgba &a, const Rgba &b)
{
	return Rgba(a.r_ - b.r_, a.g_ - b.g_, a.b_ - b.b_, a.a_ - b.a_);
}

inline Rgba &Rgba::operator +=(const Rgba &c) { r_ += c.r_; g_ += c.g_; b_ += c.b_; a_ += c.a_;  return *this; }
inline Rgba &Rgba::operator *=(const Rgba &c) { r_ *= c.r_; g_ *= c.g_; b_ *= c.b_; a_ *= c.a_;  return *this; }
inline Rgba &Rgba::operator *=(float f) { r_ *= f; g_ *= f; b_ *= f; a_ *= f;  return *this; }
inline Rgba &Rgba::operator -=(const Rgba &c) { r_ -= c.r_; g_ -= c.g_; b_ -= c.b_; a_ -= c.a_;  return *this; }

inline float maxAbsDiff__(const Rgb &a, const Rgb &b)
{
	return (a - b).absmax();
}

//Matrix information from: http://www.color.org/chardata/rgb/sRGB.pdf
static float linear_rgb_from_xyz_d_65__[3][3] =
{
	{ 3.2406255f, -1.537208f,  -0.4986286f },
	{-0.9689307f,  1.8757561f,  0.0415175f },
	{ 0.0557101f, -0.2040211f,  1.0569959f }
};

//Inverse matrices
static float xyz_d_65_from_linear_rgb__[3][3] =
{
	{ 0.412400f,   0.357600f,   0.180500f },
	{ 0.212600f,   0.715200f,   0.072200f },
	{ 0.019300f,   0.119200f,   0.950500f }
};

inline float Rgb::linearRgbFromSRgb(float value_s_rgb)
{
	//Calculations from http://www.color.org/chardata/rgb/sRGB.pdf
	if(value_s_rgb <= 0.04045f) return (value_s_rgb / 12.92f);
	else return fPow__(((value_s_rgb + 0.055f) / 1.055f), 2.4f);
}

inline float Rgb::sRgbFromLinearRgb(float value_linear_rgb)
{
	//Calculations from http://www.color.org/chardata/rgb/sRGB.pdf
	if(value_linear_rgb <= 0.0031308f) return (value_linear_rgb * 12.92f);
	else return ((1.055f * fPow__(value_linear_rgb, 0.416667f)) - 0.055f); //0,416667f = 1/2.4
}

inline void Rgb::linearRgbFromColorSpace(ColorSpace color_space, float gamma)
{
	//NOTE: Alpha value is not converted from linear to color space and vice versa. Should it be converted?
	if(color_space == Srgb)
	{
		r_ = linearRgbFromSRgb(r_);
		g_ = linearRgbFromSRgb(g_);
		b_ = linearRgbFromSRgb(b_);
	}
	else if(color_space == XyzD65)
	{
		float old_r = r_, old_g = g_, old_b = b_;
		r_ = linear_rgb_from_xyz_d_65__[0][0] * old_r + linear_rgb_from_xyz_d_65__[0][1] * old_g + linear_rgb_from_xyz_d_65__[0][2] * old_b;
		g_ = linear_rgb_from_xyz_d_65__[1][0] * old_r + linear_rgb_from_xyz_d_65__[1][1] * old_g + linear_rgb_from_xyz_d_65__[1][2] * old_b;
		b_ = linear_rgb_from_xyz_d_65__[2][0] * old_r + linear_rgb_from_xyz_d_65__[2][1] * old_g + linear_rgb_from_xyz_d_65__[2][2] * old_b;
	}
	else if(color_space == RawManualGamma && gamma != 1.f)
	{
		gammaAdjust(gamma);
	}
}

inline void Rgb::colorSpaceFromLinearRgb(ColorSpace color_space, float gamma)
{
	//NOTE: Alpha value is not converted from linear to color space and vice versa. Should it be converted?
	if(color_space == Srgb)
	{
		r_ = sRgbFromLinearRgb(r_);
		g_ = sRgbFromLinearRgb(g_);
		b_ = sRgbFromLinearRgb(b_);
	}
	else if(color_space == XyzD65)
	{
		float old_r = r_, old_g = g_, old_b = b_;
		r_ = xyz_d_65_from_linear_rgb__[0][0] * old_r + xyz_d_65_from_linear_rgb__[0][1] * old_g + xyz_d_65_from_linear_rgb__[0][2] * old_b;
		g_ = xyz_d_65_from_linear_rgb__[1][0] * old_r + xyz_d_65_from_linear_rgb__[1][1] * old_g + xyz_d_65_from_linear_rgb__[1][2] * old_b;
		b_ = xyz_d_65_from_linear_rgb__[2][0] * old_r + xyz_d_65_from_linear_rgb__[2][1] * old_g + xyz_d_65_from_linear_rgb__[2][2] * old_b;
	}
	else if(color_space == RawManualGamma && gamma != 1.f)
	{
		if(gamma <= 0.f) gamma = 1.0e-2f;	//Arbitrary lower boundary limit for the output gamma, to avoid division by 0
		float inv_gamma = 1.f / gamma;
		gammaAdjust(inv_gamma);
	}
}

inline void Rgb::clampProportionalRgb(float max_value)	//Function to clamp the current color to a maximum value, but keeping the relationship between the color components. So it will find the R,G,B component with the highest value, clamp it to the maxValue, and adjust proportionally the other two components
{
	if(max_value > 0.f)	//If maxValue is 0, no clamping is done at all.
	{
		//If we have to clamp the result, calculate the maximum RGB component, clamp it and scale the other components acordingly to preserve color information.

		float max_rgb = std::max(r_, std::max(g_, b_));
		float proportional_adjustment = max_value / max_rgb;

		if(max_rgb > max_value)
		{
			if(r_ >= max_rgb)
			{
				r_ = max_value;
				g_ *= proportional_adjustment;
				b_ *= proportional_adjustment;
			}

			else if(g_ >= max_rgb)
			{
				g_ = max_value;
				r_ *= proportional_adjustment;
				b_ *= proportional_adjustment;
			}

			else
			{
				b_ = max_value;
				r_ *= proportional_adjustment;
				g_ *= proportional_adjustment;
			}
		}
	}
}

inline float Rgba::colorDifference(Rgba color_2, bool use_rg_bcomponents)
{
	float color_difference = std::fabs(color_2.col2Bri() - col2Bri());

	if(use_rg_bcomponents)
	{
		float rdiff = std::fabs(color_2.r_ - r_);
		float gdiff = std::fabs(color_2.g_ - g_);
		float bdiff = std::fabs(color_2.b_ - b_);
		float adiff = std::fabs(color_2.a_ - a_);

		if(color_difference < rdiff) color_difference = rdiff;
		if(color_difference < gdiff) color_difference = gdiff;
		if(color_difference < bdiff) color_difference = bdiff;
		if(color_difference < adiff) color_difference = adiff;
	}

	return color_difference;
}

inline void Rgb::rgbToHsv(float &h, float &s, float &v) const
{
	//HSV-RGB Based on https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB

	float r_1 = std::max(r_, 0.f);
	float g_1 = std::max(g_, 0.f);
	float b_1 = std::max(b_, 0.f);

	float max_component = std::max(std::max(r_1, g_1), b_1);
	float min_component = std::min(std::min(r_1, g_1), b_1);
	float range = max_component - min_component;
	v = max_component;

	if(std::fabs(range) < 1.0e-6f) { h = 0.f; s = 0.f; }
	else if(max_component == r_1) { h = std::fmod((g_1 - b_1) / range, 6.f); s = range / std::max(v, 1.0e-6f); }
	else if(max_component == g_1) { h = ((b_1 - r_1) / range) + 2.f; s = range / std::max(v, 1.0e-6f); }
	else if(max_component == b_1) { h = ((r_1 - g_1) / range) + 4.f; s = range / std::max(v, 1.0e-6f); }
	else { h = 0.f; s = 0.f; v = 0.f; }

	if(h < 0.f) h += 6.f;
}

inline void Rgb::hsvToRgb(const float &h, const float &s, const float &v)
{
	//RGB-HSV Based on https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
	float c = v * s;
	float x = c * (1.f - std::fabs(std::fmod(h, 2.f) - 1.f));
	float m = v - c;
	float r_1 = 0.f, g_1 = 0.f, b_1 = 0.f;

	if(h >= 0.f && h < 1.f) { r_1 = c; g_1 = x; b_1 = 0.f; }
	else if(h >= 1.f && h < 2.f) { r_1 = x; g_1 = c; b_1 = 0.f; }
	else if(h >= 2.f && h < 3.f) { r_1 = 0.f; g_1 = c; b_1 = x; }
	else if(h >= 3.f && h < 4.f) { r_1 = 0.f; g_1 = x; b_1 = c; }
	else if(h >= 4.f && h < 5.f) { r_1 = x; g_1 = 0.f; b_1 = c; }
	else if(h >= 5.f && h < 6.f) { r_1 = c; g_1 = 0.f; b_1 = x; }

	r_ = r_1 + m;
	g_ = g_1 + m;
	b_ = b_1 + m;
}

inline void Rgb::rgbToHsl(float &h, float &s, float &l) const
{
	//hsl-RGB Based on https://en.wikipedia.org/wiki/HSL_and_hsl#Converting_to_RGB

	float r_1 = std::max(r_, 0.f);
	float g_1 = std::max(g_, 0.f);
	float b_1 = std::max(b_, 0.f);

	float max_component = std::max(std::max(r_1, g_1), b_1);
	float min_component = std::min(std::min(r_1, g_1), b_1);
	float range = max_component - min_component;
	l = 0.5f * (max_component + min_component);

	if(std::fabs(range) < 1.0e-6f) { h = 0.f; s = 0.f; }
	else if(max_component == r_1) { h = std::fmod((g_1 - b_1) / range, 6.f); s = range / std::max((1.f - std::fabs((2.f * l) - 1)), 1.0e-6f); }
	else if(max_component == g_1) { h = ((b_1 - r_1) / range) + 2.f; s = range / std::max((1.f - std::fabs((2.f * l) - 1)), 1.0e-6f); }
	else if(max_component == b_1) { h = ((r_1 - g_1) / range) + 4.f; s = range / std::max((1.f - std::fabs((2.f * l) - 1)), 1.0e-6f); }
	else { h = 0.f; s = 0.f; l = 0.f; }

	if(h < 0.f) h += 6.f;
}

inline void Rgb::hslToRgb(const float &h, const float &s, const float &l)
{
	//RGB-hsl Based on https://en.wikipedia.org/wiki/HSL_and_hsl#Converting_to_RGB
	float c = (1.f - std::fabs((2.f * l) - 1.f)) * s;
	float x = c * (1.f - std::fabs(std::fmod(h, 2.f) - 1.f));
	float m = l - 0.5f * c;
	float r_1 = 0.f, g_1 = 0.f, b_1 = 0.f;

	if(h >= 0.f && h < 1.f) { r_1 = c; g_1 = x; b_1 = 0.f; }
	else if(h >= 1.f && h < 2.f) { r_1 = x; g_1 = c; b_1 = 0.f; }
	else if(h >= 2.f && h < 3.f) { r_1 = 0.f; g_1 = c; b_1 = x; }
	else if(h >= 3.f && h < 4.f) { r_1 = 0.f; g_1 = x; b_1 = c; }
	else if(h >= 4.f && h < 5.f) { r_1 = x; g_1 = 0.f; b_1 = c; }
	else if(h >= 5.f && h < 6.f) { r_1 = c; g_1 = 0.f; b_1 = x; }

	r_ = r_1 + m;
	g_ = g_1 + m;
	b_ = b_1 + m;
}

END_YAFRAY

#endif // YAFARAY_COLOR_H
