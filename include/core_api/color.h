#pragma once
/****************************************************************************
 *
 * 			color.h: Color type and operators api
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
 *		Copyright (C) 2015  David Bluecame for Color Space modifications
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
#ifndef Y_COLOR_H
#define Y_COLOR_H

#include <yafray_constants.h>
#include <utilities/mathOptimizations.h>
#include <iostream>

__BEGIN_YAFRAY

enum colorSpaces_t : int
{
	RAW_MANUAL_GAMMA	= 1,
	LINEAR_RGB		= 2,
	SRGB			= 3,
	XYZ_D65			= 4	
};

class YAFRAYCORE_EXPORT color_t
{
	friend color_t operator * (const color_t &a, const color_t &b);
	friend color_t operator * (const float f, const color_t &b);
	friend color_t operator * (const color_t &b, const float f);
	friend color_t operator / (const color_t &b, const float f);
	friend color_t operator + (const color_t &a, const color_t &b);
	friend color_t operator - (const color_t &a, const color_t &b);
	friend float maxAbsDiff(const color_t &a, const color_t &b);
	friend YAFRAYCORE_EXPORT void operator >> (unsigned char *data, color_t &c);
	friend YAFRAYCORE_EXPORT void operator << (unsigned char *data, const color_t &c);
	friend YAFRAYCORE_EXPORT void operator >> (float *data, color_t &c);
	friend YAFRAYCORE_EXPORT void operator << (float *data, const color_t &c);
	friend YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream & out, const color_t c);
	friend YAFRAYCORE_EXPORT color_t mix(const color_t &a, const color_t &b, float point);
	friend YAFRAYCORE_EXPORT color_t convergenceAccell(const color_t &cn_1, const color_t &cn0, const color_t &cn1);
	public:
		color_t() { R=G=B=0; }
		color_t(float r, float g, float b) {R=r;G=g;B=b;};
		color_t(float g) { R=G=B=g; }
		color_t(float af[3]) { R=af[0];  G=af[1];  B=af[2]; }
		bool isBlack() const { return ((R==0) && (G==0) && (B==0)); }
		bool isNaN() const { return (std::isnan(R) || std::isnan(G) || std::isnan(B)); }
		bool isInf() const { return (std::isinf(R) || std::isinf(G) || std::isinf(B)); }
		~color_t() {}
		void set(float r, float g, float b) { R=r;  G=g;  B=b; }

		color_t & operator +=(const color_t &c);
		color_t & operator -=(const color_t &c);
		color_t & operator *=(const color_t &c);
		color_t & operator *=(float f);

		float energy() const {return (R+G+B)*0.333333f;};
		// Using ITU/Photometric values Y = 0.2126 R + 0.7152 G + 0.0722 B
		float col2bri() const { return (0.2126f*R + 0.7152f*G + 0.0722f*B); }
		float abscol2bri() const { return (0.2126f*std::fabs(R) + 0.7152f*std::fabs(G) + 0.0722f*std::fabs(B)); }
		void gammaAdjust(float g){ R = fPow(R, g); G = fPow(G, g); B = fPow(B, g); }
		void expgam_Adjust (float e, float g, bool clamp_rgb);
		float getR() const { return R; }
		float getG() const { return G; }
		float getB() const { return B; }

		// used in blendershader
		void invertRGB()
		{
			if (R!=0.f) R=1.f/R;
			if (G!=0.f) G=1.f/G;
			if (B!=0.f) B=1.f/B;
		}
		void absRGB() { R=std::fabs(R);  G=std::fabs(G);  B=std::fabs(B); }
		void darkenRGB(const color_t &col)
		{
			if (R>col.R) R=col.R;
			if (G>col.G) G=col.G;
			if (B>col.B) B=col.B;
		}
		void lightenRGB(const color_t &col)
		{
			if (R<col.R) R=col.R;
			if (G<col.G) G=col.G;
			if (B<col.B) B=col.B;
		}

		void black() { R=G=B=0; }
		float minimum() const { return std::min(R, std::min(G, B)); }
		float maximum() const { return std::max(R, std::max(G, B)); }
		float absmax() const { return std::max(std::fabs(R), std::max(std::fabs(G), std::fabs(B))); }
		void clampRGB0()
		{
			if (R<0.0) R=0.0;
			if (G<0.0) G=0.0;
			if (B<0.0) B=0.0;
		}

		void clampRGB01()
		{
			if (R<0.0) R=0.0; else if (R>1.0) R=1.0;
			if (G<0.0) G=0.0; else if (G>1.0) G=1.0;
			if (B<0.0) B=0.0; else if (B>1.0) B=1.0;
		}
		
		void blend(const color_t &col, float blend_factor)
		{
			R = R * (1.f - blend_factor) + col.R * blend_factor;
			G = G * (1.f - blend_factor) + col.G * blend_factor;
			B = B * (1.f - blend_factor) + col.B * blend_factor;
		}

		void ceil() //Mainly used for Absolute Object/Material Index passes, to correct the antialiasing and ceil the "mixed" values to the upper integer 
		{
			R = ceilf(R);
			G = ceilf(G);
			B = ceilf(B);
		}
		
		void clampProportionalRGB(float maxValue);
		
		float linearRGB_from_sRGB(float value_sRGB);
		float sRGB_from_linearRGB(float value_linearRGB);
		
		void linearRGB_from_ColorSpace(colorSpaces_t colorSpace, float gamma);
		void ColorSpace_from_linearRGB(colorSpaces_t colorSpace, float gamma);		
		void rgb_to_hsv(float & h, float & s, float & v) const;
		void hsv_to_rgb(const float & h, const float & s, const float & v);
		void rgb_to_hsl(float & h, float & s, float & l) const;
		void hsl_to_rgb(const float & h, const float & s, const float & l);
				
//	protected:
		float R, G, B;
};

class YAFRAYCORE_EXPORT colorA_t : public color_t
{
	friend colorA_t operator * (const colorA_t &a, const colorA_t &b);
	friend colorA_t operator * (const float f, const colorA_t &b);
	friend colorA_t operator * (const colorA_t &b, const float f);
	friend colorA_t operator / (const colorA_t &b, const float f);
	friend colorA_t operator + (const colorA_t &a, const colorA_t &b);
	friend colorA_t operator - (const colorA_t &a, const colorA_t &b);
	friend YAFRAYCORE_EXPORT void operator >> (unsigned char *data, colorA_t &c);
	friend YAFRAYCORE_EXPORT void operator << (unsigned char *data, const colorA_t &c);
	friend YAFRAYCORE_EXPORT void operator >> (float *data, colorA_t &c);
	friend YAFRAYCORE_EXPORT void operator << (float *data, const colorA_t &c);
	friend YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream & out, const colorA_t c);
	friend YAFRAYCORE_EXPORT colorA_t mix(const colorA_t &a, const colorA_t &b, float point);
	public:
		colorA_t():A(1.f) { }
		colorA_t(const color_t &c):color_t(c), A(1.f) { }
		colorA_t(const color_t &c, float a):color_t(c), A(a) { }
		colorA_t(float r, float g, float b, float a=1.f):color_t(r,g,b), A(a) { }
		colorA_t(float g):color_t(g), A(g) { }
		colorA_t(float g, float a):color_t(g), A(a) { }
		colorA_t(float af[4]):color_t(af), A(af[3]) { }
		~colorA_t() {};
		
		void set(float r, float g, float b, float a=1.f) { color_t::set(r,g,b);  A=a; }

		colorA_t & operator +=(const colorA_t &c);
		colorA_t & operator -=(const colorA_t &c);
		colorA_t & operator *=(const colorA_t &c);
		colorA_t & operator *=(float f);

		void alphaPremultiply() { R*=A; G*=A; B*=A; }
		float getA() const { return A; }
		void setAlpha(float a) { A=a; }

		void clampRGBA0()
		{
			clampRGB0();
			if (A<0.0) A=0.0;
		}

		void clampRGBA01()
		{
			clampRGB01();
			if (A<0.0) A=0.0; else if (A>1.0) A=1.0;
		}

		void blend(const colorA_t &col, float blend_factor)
		{
			R = R * (1.f - blend_factor) + col.R * blend_factor;
			G = G * (1.f - blend_factor) + col.G * blend_factor;
			B = B * (1.f - blend_factor) + col.B * blend_factor;
			A = A * (1.f - blend_factor) + col.A * blend_factor;
		}

		void ceil() //Mainly used for Absolute Object/Material Index passes, to correct the antialiasing and ceil the "mixed" values to the upper integer 
		{
			R = ceilf(R);
			G = ceilf(G);
			B = ceilf(B);
			A = ceilf(A);
		}

		float colorDifference(colorA_t color2, bool useRGBcomponents = false);

//	protected:
		float A;
};

class YAFRAYCORE_EXPORT rgbe_t
{
	public:
		rgbe_t() {rgbe[3]=0;};
		rgbe_t(const color_t &s);
		operator color_t ()const
		{
			color_t res;
			float f;
			if (rgbe[3])
			{   /*nonzero pixel*/
				f = fLdexp(1.0,rgbe[3]-(int)(128+8));
				return color_t(rgbe[0] * f,rgbe[1] * f,rgbe[2] * f);
			}
			else return color_t(0,0,0);
		}
//		unsigned char& operator [] (int i){ return rgbe[i]; }

//	protected:
		unsigned char rgbe[4];
};

inline void color_t::expgam_Adjust(float e, float g, bool clamp_rgb)
{
	if ((e==0.f) && (g==1.f)) {
		if (clamp_rgb) clampRGB01();
		return;
	}
	if (e!=0.f) {
		// exposure adjust
		clampRGB0();
		R = 1.f - fExp(R*e);
		G = 1.f - fExp(G*e);
		B = 1.f - fExp(B*e);
	}
	if (g!=1.f) {
		// gamma adjust
		clampRGB0();
		R = fPow(R, g);
		G = fPow(G, g);
		B = fPow(B, g);
	}
}

YAFRAYCORE_EXPORT void operator >> (unsigned char *data,color_t &c);
YAFRAYCORE_EXPORT void operator << (unsigned char *data,const color_t &c);
YAFRAYCORE_EXPORT void operator >> (float *data, color_t &c);
YAFRAYCORE_EXPORT void operator << (float *data, const color_t &c);
YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream & out,const color_t c);
YAFRAYCORE_EXPORT color_t mix(const color_t &a,const color_t &b,float point);

YAFRAYCORE_EXPORT void operator >> (unsigned char *data,colorA_t &c);
YAFRAYCORE_EXPORT void operator << (unsigned char *data,const colorA_t &c);
YAFRAYCORE_EXPORT void operator >> (float *data, colorA_t &c);
YAFRAYCORE_EXPORT void operator << (float *data, const colorA_t &c);
YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream & out,const colorA_t c);
YAFRAYCORE_EXPORT colorA_t mix(const colorA_t &a,const colorA_t &b,float point);


inline color_t operator * (const color_t &a,const color_t &b)
{
	return color_t(a.R*b.R,a.G*b.G,a.B*b.B);
}

inline color_t operator * (const float f,const color_t &b)
{
	return color_t(f*b.R,f*b.G,f*b.B);
}

inline color_t operator * (const color_t &b,const float f)
{
	return color_t(f*b.R,f*b.G,f*b.B);
}

inline color_t operator / (const color_t &b,float f)
{
	return color_t(b.R/f,b.G/f,b.B/f);
}

inline color_t operator + (const color_t &a,const color_t &b)
{
	return color_t(a.R+b.R,a.G+b.G,a.B+b.B);
}

inline color_t operator - (const color_t &a, const color_t &b)
{
	return color_t(a.R-b.R, a.G-b.G, a.B-b.B);
}

/*
inline color_t & color_t::operator *=(const color_t &c)
{
	FLUSH_3DNOW();
	MMX_LOAD64(MM0,c.R);
	MMX_LOAD32(MM1,c.B);
	MMX_LOAD64(MM2,R);
	MMX_LOAD32(MM3,B);
	MMX_MULF(MM0,MM2);
	MMX_MULF(MM1,MM3);
	MMX_STORE64(R,MM0);
	MMX_STORE32(B,MM1);
	FLUSH_3DNOW();
	return *this;
}*/

inline color_t & color_t::operator +=(const color_t &c)
{ R += c.R;  G += c.G;  B += c.B;  return *this; }
inline color_t & color_t::operator *=(const color_t &c)
{ R *= c.R;  G *= c.G;  B *= c.B;  return *this; }
inline color_t & color_t::operator *=(float f)
{ R *= f;  G*= f;  B *= f;  return *this; }
inline color_t & color_t::operator -=(const color_t &c)
{ R -= c.R;  G -= c.G;  B -= c.B;  return *this; }

inline colorA_t operator * (const colorA_t &a,const colorA_t &b)
{
	return colorA_t(a.R*b.R, a.G*b.G, a.B*b.B, a.A*b.A);
}

inline colorA_t operator * (const float f,const colorA_t &b)
{
	return colorA_t(f*b.R, f*b.G, f*b.B, f*b.A);
}

inline colorA_t operator * (const colorA_t &b,const float f)
{
	return colorA_t(f*b.R, f*b.G, f*b.B, f*b.A);
}

inline colorA_t operator / (const colorA_t &b,float f)
{
	if (f!=0) f=1.0/f;
	return colorA_t(b.R*f, b.G*f, b.B*f, b.A*f);
}

inline colorA_t operator + (const colorA_t &a,const colorA_t &b)
{
	return colorA_t(a.R+b.R, a.G+b.G, a.B+b.B, a.A+b.A);
}

inline colorA_t operator - (const colorA_t &a, const colorA_t &b)
{
	return colorA_t(a.R-b.R, a.G-b.G, a.B-b.B, a.A-b.A);
}

inline colorA_t & colorA_t::operator +=(const colorA_t &c) { R += c.R;  G += c.G;  B += c.B;  A += c.A;  return *this; }
inline colorA_t & colorA_t::operator *=(const colorA_t &c) { R *= c.R;  G *= c.G;  B *= c.B;  A *= c.A;  return *this; }
inline colorA_t & colorA_t::operator *=(float f) { R *= f;  G*= f;  B *= f;  A *= f;  return *this; }
inline colorA_t & colorA_t::operator -=(const colorA_t &c) { R -= c.R;  G -= c.G;  B -= c.B;  A -= c.A;  return *this; }

inline float maxAbsDiff(const color_t &a,const color_t &b)
{
	return (a - b).absmax();
}

YAFRAYCORE_EXPORT color_t convergenceAccell(const color_t &cn_1,const color_t &cn0,const color_t &cn1);

//Matrix information from: http://www.color.org/chardata/rgb/sRGB.pdf
static float linearRGB_from_XYZ_D65[3][3] =
{
	 { 3.2406255f, -1.537208f,  -0.4986286f },
	 {-0.9689307f,  1.8757561f,  0.0415175f },
	 { 0.0557101f, -0.2040211f,  1.0569959f }
};

//Inverse matrices
static float XYZ_D65_from_linearRGB[3][3] =
{
	{ 0.412400f,   0.357600f,   0.180500f },
	{ 0.212600f,   0.715200f,   0.072200f },
	{ 0.019300f,   0.119200f,   0.950500f }
};

inline float color_t::linearRGB_from_sRGB(float value_sRGB)
{
	//Calculations from http://www.color.org/chardata/rgb/sRGB.pdf
	if(value_sRGB <= 0.04045f) return (value_sRGB / 12.92f);
	else return fPow(((value_sRGB + 0.055f) / 1.055f), 2.4f);
}

inline float color_t::sRGB_from_linearRGB(float value_linearRGB)
{
	//Calculations from http://www.color.org/chardata/rgb/sRGB.pdf
	if(value_linearRGB <= 0.0031308f) return (value_linearRGB * 12.92f);
	else return ((1.055f * fPow(value_linearRGB, 0.416667f)) - 0.055f); //0,416667f = 1/2.4
}

inline void color_t::linearRGB_from_ColorSpace(colorSpaces_t colorSpace, float gamma)
{
	//NOTE: Alpha value is not converted from linear to color space and vice versa. Should it be converted?
	if(colorSpace == SRGB)
	{
		R = linearRGB_from_sRGB(R);
		G = linearRGB_from_sRGB(G);
		B = linearRGB_from_sRGB(B);
	}
	else if(colorSpace == XYZ_D65)
	{
		float oldR = R, oldG = G, oldB = B;
		R = linearRGB_from_XYZ_D65[0][0] * oldR + linearRGB_from_XYZ_D65[0][1] * oldG + linearRGB_from_XYZ_D65[0][2] * oldB;
		G = linearRGB_from_XYZ_D65[1][0] * oldR + linearRGB_from_XYZ_D65[1][1] * oldG + linearRGB_from_XYZ_D65[1][2] * oldB;
		B = linearRGB_from_XYZ_D65[2][0] * oldR + linearRGB_from_XYZ_D65[2][1] * oldG + linearRGB_from_XYZ_D65[2][2] * oldB;
	}
	else if(colorSpace == RAW_MANUAL_GAMMA && gamma != 1.f)
	{
		gammaAdjust(gamma);
	}
}

inline void color_t::ColorSpace_from_linearRGB(colorSpaces_t colorSpace, float gamma)
{
	//NOTE: Alpha value is not converted from linear to color space and vice versa. Should it be converted?
	if(colorSpace == SRGB)
	{
		R = sRGB_from_linearRGB(R);
		G = sRGB_from_linearRGB(G);
		B = sRGB_from_linearRGB(B);
	}
	else if(colorSpace == XYZ_D65)
	{
		float oldR = R, oldG = G, oldB = B;
		R = XYZ_D65_from_linearRGB[0][0] * oldR + XYZ_D65_from_linearRGB[0][1] * oldG + XYZ_D65_from_linearRGB[0][2] * oldB;
		G = XYZ_D65_from_linearRGB[1][0] * oldR + XYZ_D65_from_linearRGB[1][1] * oldG + XYZ_D65_from_linearRGB[1][2] * oldB;
		B = XYZ_D65_from_linearRGB[2][0] * oldR + XYZ_D65_from_linearRGB[2][1] * oldG + XYZ_D65_from_linearRGB[2][2] * oldB;
	}
	else if(colorSpace == RAW_MANUAL_GAMMA && gamma != 1.f)
	{
		if(gamma <= 0.f) gamma = 1.0e-2f;	//Arbitrary lower boundary limit for the output gamma, to avoid division by 0
		float invGamma = 1.f / gamma;
		gammaAdjust(invGamma);
	}
}

inline void color_t::clampProportionalRGB(float maxValue)	//Function to clamp the current color to a maximum value, but keeping the relationship between the color components. So it will find the R,G,B component with the highest value, clamp it to the maxValue, and adjust proportionally the other two components 
{
	if(maxValue > 0.f)	//If maxValue is 0, no clamping is done at all.
	{
		//If we have to clamp the result, calculate the maximum RGB component, clamp it and scale the other components acordingly to preserve color information.
		
		float maxRGB = std::max(R, std::max(G, B));
		float proportionalAdjustment = maxValue / maxRGB;
		
		if(maxRGB > maxValue)
		{
			if(R >= maxRGB)
			{
				R = maxValue;
				G *= proportionalAdjustment;
				B *= proportionalAdjustment;
			}
			
			else if(G >= maxRGB)
			{
				G = maxValue;
				R *= proportionalAdjustment;
				B *= proportionalAdjustment;
			}

			else
			{
				B = maxValue;
				R *= proportionalAdjustment;
				G *= proportionalAdjustment;
			}
		}
	}
}

inline float colorA_t::colorDifference(colorA_t color2, bool useRGBcomponents)
{	
	float colorDifference = std::fabs(color2.col2bri() - col2bri());

	if(useRGBcomponents)
	{
		float Rdiff = std::fabs(color2.R - R);
		float Gdiff = std::fabs(color2.G - G);
		float Bdiff = std::fabs(color2.B - B);
		float Adiff = std::fabs(color2.A - A);
		
		if(colorDifference < Rdiff) colorDifference = Rdiff; 
		if(colorDifference < Gdiff) colorDifference = Gdiff; 
		if(colorDifference < Bdiff) colorDifference = Bdiff; 
		if(colorDifference < Adiff) colorDifference = Adiff; 
	}
	
	return colorDifference;
}

inline void color_t::rgb_to_hsv(float & h, float & s, float & v) const
{
	//HSV-RGB Based on https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
	
	float R1 = std::max(R, 0.f);
	float G1 = std::max(G, 0.f);
	float B1 = std::max(B, 0.f);
	
	float M = std::max(std::max(R1,G1),B1);
	float m = std::min(std::min(R1,G1),B1);
	float C = M - m;
	v = M;
	
	if(std::fabs(C) < 1.0e-6f) { h = 0.f; s = 0.f; }
	else if(M == R1) { h = std::fmod((G1-B1)/C, 6.f);	s = C / std::max(v, 1.0e-6f); }
	else if(M == G1) { h = ((B1-R1)/C) + 2.f;	s = C / std::max(v, 1.0e-6f); }
	else if(M == B1) { h = ((R1-G1)/C) + 4.f;	s = C / std::max(v, 1.0e-6f); }
	else { h = 0.f; s = 0.f; v = 0.f; }
	
	if(h < 0.f) h += 6.f;
}

inline void color_t::hsv_to_rgb(const float & h, const float & s, const float & v)
{
	//RGB-HSV Based on https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
	float C = v * s;
	float X = C * (1.f - std::fabs(std::fmod(h, 2.f) - 1.f));
	float m = v - C;
	float R1 = 0.f, G1 = 0.f, B1 = 0.f;

	if(h >= 0.f && h < 1.f) { R1 = C; G1 = X; B1 = 0.f; }
	else if(h >= 1.f && h < 2.f) { R1 = X; G1 = C; B1 = 0.f; }
	else if(h >= 2.f && h < 3.f) { R1 = 0.f; G1 = C; B1 = X; }
	else if(h >= 3.f && h < 4.f) { R1 = 0.f; G1 = X; B1 = C; }
	else if(h >= 4.f && h < 5.f) { R1 = X; G1 = 0.f; B1 = C; }
	else if(h >= 5.f && h < 6.f) { R1 = C; G1 = 0.f; B1 = X; }

	R = R1 + m;
	G = G1 + m;
	B = B1 + m;
}

inline void color_t::rgb_to_hsl(float & h, float & s, float & l) const
{
	//hsl-RGB Based on https://en.wikipedia.org/wiki/HSL_and_hsl#Converting_to_RGB
	
	float R1 = std::max(R, 0.f);
	float G1 = std::max(G, 0.f);
	float B1 = std::max(B, 0.f);
	
	float M = std::max(std::max(R1,G1),B1);
	float m = std::min(std::min(R1,G1),B1);
	float C = M - m;
	l = 0.5f * (M + m);
	
	if(std::fabs(C) < 1.0e-6f) { h = 0.f; s = 0.f; }
	else if(M == R1) { h = std::fmod((G1-B1)/C, 6.f);	s = C / std::max((1.f - std::fabs((2.f * l) - 1)), 1.0e-6f); }
	else if(M == G1) { h = ((B1-R1)/C) + 2.f;	s = C / std::max((1.f - std::fabs((2.f * l) - 1)), 1.0e-6f); }
	else if(M == B1) { h = ((R1-G1)/C) + 4.f;	s = C / std::max((1.f - std::fabs((2.f * l) - 1)), 1.0e-6f); }
	else { h = 0.f; s = 0.f; l = 0.f; }
	
	if(h < 0.f) h += 6.f;
}

inline void color_t::hsl_to_rgb(const float & h, const float & s, const float & l)
{
	//RGB-hsl Based on https://en.wikipedia.org/wiki/HSL_and_hsl#Converting_to_RGB
	float C = (1.f - std::fabs((2.f * l) - 1.f)) * s;
	float X = C * (1.f - std::fabs(std::fmod(h, 2.f) - 1.f));
	float m = l - 0.5f * C;
	float R1 = 0.f, G1 = 0.f, B1 = 0.f;

	if(h >= 0.f && h < 1.f) { R1 = C; G1 = X; B1 = 0.f; }
	else if(h >= 1.f && h < 2.f) { R1 = X; G1 = C; B1 = 0.f; }
	else if(h >= 2.f && h < 3.f) { R1 = 0.f; G1 = C; B1 = X; }
	else if(h >= 3.f && h < 4.f) { R1 = 0.f; G1 = X; B1 = C; }
	else if(h >= 4.f && h < 5.f) { R1 = X; G1 = 0.f; B1 = C; }
	else if(h >= 5.f && h < 6.f) { R1 = C; G1 = 0.f; B1 = X; }

	R = R1 + m;
	G = G1 + m;
	B = B1 + m;
}

__END_YAFRAY

#endif // Y_COLOR_H
