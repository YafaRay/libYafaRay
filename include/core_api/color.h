/****************************************************************************
 *
 * 			color.h: Color type and operators api
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
 *		Copyright (C) 2015  David Bluecame for Color Space and Render Passes
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

#include<yafray_config.h>

#include <iostream>
#include <vector>
#include <string>
#include <utilities/mathOptimizations.h>
#include <map>

#define COLOR_SIZE 3

// ensure isnan and isinf are available. I *hope* it works with OSX w. gcc 4.x too
#ifdef _MSC_VER
#include <float.h>
#define isnan _isnan
#define isinf _isinf
#else
using std::isnan; // from cmath
using std::isinf; // from cmath
#endif

__BEGIN_YAFRAY

enum colorSpaces_t
{
	RAW_MANUAL_GAMMA	= 1,
	LINEAR_RGB		= 2,
	SRGB			= 3,
	XYZ_D65			= 4	
};

class YAFRAYCORE_EXPORT color_t
{
	friend color_t operator * (const color_t &a, const color_t &b);
	friend color_t operator * (const CFLOAT f, const color_t &b);
	friend color_t operator * (const color_t &b, const CFLOAT f);
	friend color_t operator / (const color_t &b, const CFLOAT f);
	friend color_t operator + (const color_t &a, const color_t &b);
	friend color_t operator - (const color_t &a, const color_t &b);
	friend CFLOAT maxAbsDiff(const color_t &a, const color_t &b);
	friend YAFRAYCORE_EXPORT void operator >> (unsigned char *data, color_t &c);
	friend YAFRAYCORE_EXPORT void operator << (unsigned char *data, const color_t &c);
	friend YAFRAYCORE_EXPORT void operator >> (float *data, color_t &c);
	friend YAFRAYCORE_EXPORT void operator << (float *data, const color_t &c);
	friend YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream & out, const color_t c);
	friend YAFRAYCORE_EXPORT color_t mix(const color_t &a, const color_t &b, CFLOAT point);
	friend YAFRAYCORE_EXPORT color_t convergenceAccell(const color_t &cn_1, const color_t &cn0, const color_t &cn1);
	public:
		color_t() { R=G=B=0; }
		color_t(CFLOAT r, CFLOAT g, CFLOAT b) {R=r;G=g;B=b;};
		color_t(CFLOAT g) { R=G=B=g; }
		color_t(CFLOAT af[3]) { R=af[0];  G=af[1];  B=af[2]; }
		bool isBlack() const { return ((R==0) && (G==0) && (B==0)); }
		bool isNaN() const { return (isnan(R) || isnan(G) || isnan(B)); }
		bool isInf() const { return (isinf(R) || isinf(G) || isinf(B)); }
		~color_t() {}
		void set(CFLOAT r, CFLOAT g, CFLOAT b) { R=r;  G=g;  B=b; }

		color_t & operator +=(const color_t &c);
		color_t & operator -=(const color_t &c);
		color_t & operator *=(const color_t &c);
		color_t & operator *=(CFLOAT f);

		CFLOAT energy() const {return (R+G+B)*0.333333f;};
		// Using ITU/Photometric values Y = 0.2126 R + 0.7152 G + 0.0722 B
		CFLOAT col2bri() const { return (0.2126f*R + 0.7152f*G + 0.0722f*B); }
		CFLOAT abscol2bri() const { return (0.2126f*std::fabs(R) + 0.7152f*std::fabs(G) + 0.0722f*std::fabs(B)); }
		void gammaAdjust(CFLOAT g){ R = fPow(R, g); G = fPow(G, g); B = fPow(B, g); }
		void expgam_Adjust (CFLOAT e, CFLOAT g, bool clamp_rgb);
		CFLOAT getR() const { return R; }
		CFLOAT getG() const { return G; }
		CFLOAT getB() const { return B; }

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
		CFLOAT minimum() const { return std::min(R, std::min(G, B)); }
		CFLOAT maximum() const { return std::max(R, std::max(G, B)); }
		CFLOAT absmax() const { return std::max(std::fabs(R), std::max(std::fabs(G), std::fabs(B))); }
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
		
		CFLOAT linearRGB_from_sRGB(CFLOAT value_sRGB);
		CFLOAT sRGB_from_linearRGB(CFLOAT value_linearRGB);
		
		void linearRGB_from_ColorSpace(colorSpaces_t colorSpace, float gamma);
		void ColorSpace_from_linearRGB(colorSpaces_t colorSpace, float gamma);		
		
//	protected:
		CFLOAT R, G, B;
};

class YAFRAYCORE_EXPORT colorA_t : public color_t
{
	friend colorA_t operator * (const colorA_t &a, const colorA_t &b);
	friend colorA_t operator * (const CFLOAT f, const colorA_t &b);
	friend colorA_t operator * (const colorA_t &b, const CFLOAT f);
	friend colorA_t operator / (const colorA_t &b, const CFLOAT f);
	friend colorA_t operator + (const colorA_t &a, const colorA_t &b);
	friend colorA_t operator - (const colorA_t &a, const colorA_t &b);
	friend YAFRAYCORE_EXPORT void operator >> (unsigned char *data, colorA_t &c);
	friend YAFRAYCORE_EXPORT void operator << (unsigned char *data, const colorA_t &c);
	friend YAFRAYCORE_EXPORT void operator >> (float *data, colorA_t &c);
	friend YAFRAYCORE_EXPORT void operator << (float *data, const colorA_t &c);
	friend YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream & out, const colorA_t c);
	friend YAFRAYCORE_EXPORT colorA_t mix(const colorA_t &a, const colorA_t &b, CFLOAT point);
	public:
		colorA_t() { /* A=0; */ }
		colorA_t(const color_t &c):color_t(c), A(1.f) { /* A=0; */ }
		colorA_t(const color_t &c, CFLOAT a):color_t(c), A(a) {}
		colorA_t(CFLOAT r, CFLOAT g, CFLOAT b, CFLOAT a=0):color_t(r,g,b), A(a) {}
		colorA_t(CFLOAT g):color_t(g) { A=g; }
		colorA_t(CFLOAT af[4]):color_t(af) { A=af[3]; }
		~colorA_t() {};
		void set(CFLOAT r, CFLOAT g, CFLOAT b, CFLOAT a=0) { color_t::set(r,g,b);  A=a; }

		colorA_t & operator +=(const colorA_t &c);
		colorA_t & operator -=(const colorA_t &c);
		colorA_t & operator *=(const colorA_t &c);
		colorA_t & operator *=(CFLOAT f);

		void alphaPremultiply() { R*=A; G*=A; B*=A; }
		CFLOAT getA() const { return A; }
		void setAlpha(CFLOAT a) { A=a; }

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

//	protected:
		CFLOAT A;
};

class YAFRAYCORE_EXPORT rgbe_t
{
	public:
		rgbe_t() {rgbe[3]=0;};
		rgbe_t(const color_t &s);
		operator color_t ()const
		{
			color_t res;
			CFLOAT f;
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

inline void color_t::expgam_Adjust(CFLOAT e, CFLOAT g, bool clamp_rgb)
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
YAFRAYCORE_EXPORT color_t mix(const color_t &a,const color_t &b,CFLOAT point);

YAFRAYCORE_EXPORT void operator >> (unsigned char *data,colorA_t &c);
YAFRAYCORE_EXPORT void operator << (unsigned char *data,const colorA_t &c);
YAFRAYCORE_EXPORT void operator >> (float *data, colorA_t &c);
YAFRAYCORE_EXPORT void operator << (float *data, const colorA_t &c);
YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream & out,const colorA_t c);
YAFRAYCORE_EXPORT colorA_t mix(const colorA_t &a,const colorA_t &b,CFLOAT point);


inline color_t operator * (const color_t &a,const color_t &b)
{
	return color_t(a.R*b.R,a.G*b.G,a.B*b.B);
}

inline color_t operator * (const CFLOAT f,const color_t &b)
{
	return color_t(f*b.R,f*b.G,f*b.B);
}

inline color_t operator * (const color_t &b,const CFLOAT f)
{
	return color_t(f*b.R,f*b.G,f*b.B);
}

inline color_t operator / (const color_t &b,CFLOAT f)
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
inline color_t & color_t::operator *=(CFLOAT f)
{ R *= f;  G*= f;  B *= f;  return *this; }
inline color_t & color_t::operator -=(const color_t &c)
{ R -= c.R;  G -= c.G;  B -= c.B;  return *this; }

inline colorA_t operator * (const colorA_t &a,const colorA_t &b)
{
	return colorA_t(a.R*b.R, a.G*b.G, a.B*b.B, a.A*b.A);
}

inline colorA_t operator * (const CFLOAT f,const colorA_t &b)
{
	return colorA_t(f*b.R, f*b.G, f*b.B, f*b.A);
}

inline colorA_t operator * (const colorA_t &b,const CFLOAT f)
{
	return colorA_t(f*b.R, f*b.G, f*b.B, f*b.A);
}

inline colorA_t operator / (const colorA_t &b,CFLOAT f)
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
inline colorA_t & colorA_t::operator *=(CFLOAT f) { R *= f;  G*= f;  B *= f;  A *= f;  return *this; }
inline colorA_t & colorA_t::operator -=(const colorA_t &c) { R -= c.R;  G -= c.G;  B -= c.B;  A -= c.A;  return *this; }

inline CFLOAT maxAbsDiff(const color_t &a,const color_t &b)
{
	return (a - b).absmax();
}

YAFRAYCORE_EXPORT color_t convergenceAccell(const color_t &cn_1,const color_t &cn0,const color_t &cn1);

//Matrix information from: http://www.color.org/chardata/rgb/sRGB.pdf
static float linearRGB_from_XYZ_D65[3][3] =
{
	 { 3.2406255, -1.537208,  -0.4986286 },
	 {-0.9689307,  1.8757561,  0.0415175 },
	 { 0.0557101, -0.2040211,  1.0569959 }
};

//Inverse matrices
static float XYZ_D65_from_linearRGB[3][3] =
{
	{ 0.412400,   0.357600,   0.180500 },
	{ 0.212600,   0.715200,   0.072200 },
	{ 0.019300,   0.119200,   0.950500 }
};

inline CFLOAT color_t::linearRGB_from_sRGB(CFLOAT value_sRGB)
{
	//Calculations from http://www.color.org/chardata/rgb/sRGB.pdf
	if(value_sRGB <= 0.04045f) return (value_sRGB / 12.92f);
	else return fPow(((value_sRGB + 0.055f) / 1.055f), 2.4f);
}

inline CFLOAT color_t::sRGB_from_linearRGB(CFLOAT value_linearRGB)
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

enum externalPassTypes_t
{
	PASS_EXT_DISABLED				=	-1,
	PASS_EXT_COMBINED				=	0,
	PASS_EXT_Z_DEPTH,
	PASS_EXT_VECTOR,
	PASS_EXT_NORMAL,
	PASS_EXT_UV,
	PASS_EXT_COLOR,
	PASS_EXT_EMIT,
	PASS_EXT_MIST,
	PASS_EXT_DIFFUSE,
	PASS_EXT_SPECULAR,
	PASS_EXT_AO,
	PASS_EXT_ENV,
	PASS_EXT_INDIRECT,
	PASS_EXT_SHADOW,
	PASS_EXT_REFLECT,
	PASS_EXT_REFRACT,
	PASS_EXT_OBJ_INDEX,
	PASS_EXT_MAT_INDEX,
	PASS_EXT_DIFFUSE_DIRECT,
	PASS_EXT_DIFFUSE_INDIRECT,
	PASS_EXT_DIFFUSE_COLOR,
	PASS_EXT_GLOSSY_DIRECT,
	PASS_EXT_GLOSSY_INDIRECT,
	PASS_EXT_GLOSSY_COLOR,
	PASS_EXT_TRANS_DIRECT,
	PASS_EXT_TRANS_INDIRECT,
	PASS_EXT_TRANS_COLOR,
	PASS_EXT_SUBSURFACE_DIRECT,
	PASS_EXT_SUBSURFACE_INDIRECT,
	PASS_EXT_SUBSURFACE_COLOR,
	PASS_EXT_SURFACE_INTEGRATION,
	PASS_EXT_VOLUME_INTEGRATION,
	PASS_EXT_VOLUME_TRANSMITTANCE,
	PASS_EXT_TOTAL_PASSES			//IMPORTANT: KEEP THIS ALWAYS IN THE LAST POSITION
};

enum externalPassTileTypes_t
{
	PASS_EXT_TILE_1_GRAYSCALE		=	 1,
	PASS_EXT_TILE_3_RGB				=	 3,
	PASS_EXT_TILE_4_RGBA			=	 4
};

enum internalYafPassTypes_t
{
	PASS_YAF_DISABLED				=	-1,
	PASS_YAF_COMBINED				=	0,
	PASS_YAF_Z_DEPTH_NORM,
	PASS_YAF_Z_DEPTH_ABS,
	PASS_YAF_NORMAL_SMOOTH,
	PASS_YAF_NORMAL_GEOM,
	PASS_YAF_UV,
	PASS_YAF_RADIANCE,
	PASS_YAF_EMIT,
	PASS_YAF_DIFFUSE,
	PASS_YAF_DIFFUSE_NO_SHADOW,
	PASS_YAF_AO,
	PASS_YAF_ENV,
    PASS_YAF_MIST,
    PASS_YAF_INDIRECT,
	PASS_YAF_INDIRECT_ALL,
	PASS_YAF_SHADOW,
	PASS_YAF_REFLECT_PERFECT,
	PASS_YAF_REFRACT_PERFECT,
	PASS_YAF_REFLECT_ALL,
	PASS_YAF_REFRACT_ALL,
	PASS_YAF_OBJ_INDEX_ABS,
	PASS_YAF_OBJ_INDEX_NORM,
	PASS_YAF_OBJ_INDEX_AUTO,
	PASS_YAF_MAT_INDEX_ABS,
	PASS_YAF_MAT_INDEX_NORM,
	PASS_YAF_MAT_INDEX_AUTO,
    PASS_YAF_OBJ_INDEX_MASK,
	PASS_YAF_OBJ_INDEX_MASK_SHADOW,
	PASS_YAF_OBJ_INDEX_MASK_ALL,
	PASS_YAF_MAT_INDEX_MASK,
	PASS_YAF_MAT_INDEX_MASK_SHADOW,
	PASS_YAF_MAT_INDEX_MASK_ALL,
	PASS_YAF_DIFFUSE_INDIRECT,
	PASS_YAF_DIFFUSE_COLOR,
	PASS_YAF_GLOSSY,
	PASS_YAF_GLOSSY_INDIRECT,
	PASS_YAF_GLOSSY_COLOR,
	PASS_YAF_TRANS,
	PASS_YAF_TRANS_INDIRECT,
	PASS_YAF_TRANS_COLOR,
	PASS_YAF_SUBSURFACE,
	PASS_YAF_SUBSURFACE_INDIRECT,
	PASS_YAF_SUBSURFACE_COLOR,
	PASS_YAF_SURFACE_INTEGRATION,
	PASS_YAF_VOLUME_INTEGRATION,
	PASS_YAF_VOLUME_TRANSMITTANCE,
	PASS_YAF_DEBUG_NU,
	PASS_YAF_DEBUG_NV,
	PASS_YAF_DEBUG_DPDU,
	PASS_YAF_DEBUG_DPDV,
	PASS_YAF_DEBUG_DSDU,
	PASS_YAF_DEBUG_DSDV,
    PASS_YAF_AA_SAMPLES,
	PASS_YAF_TOTAL_PASSES			//IMPORTANT: KEEP THIS ALWAYS IN THE LAST POSITION
};


class YAFRAYCORE_EXPORT extPass_t  //Render pass to be exported, for example, to Blender, and mapping to the internal YafaRay render passes generated in different points of the rendering process
{
	public:
		extPass_t(int extPassType, int intPassType):
			externalPassType(extPassType), internalYafPassType(intPassType)
		{ 
			switch(extPassType)  //These are the tyle types needed for Blender
			{
				case PASS_EXT_COMBINED:		externalTyleType = PASS_EXT_TILE_4_RGBA;		break;
				case PASS_EXT_Z_DEPTH:		externalTyleType = PASS_EXT_TILE_1_GRAYSCALE;	break;
				case PASS_EXT_VECTOR:		externalTyleType = PASS_EXT_TILE_4_RGBA;		break;
				case PASS_EXT_COLOR:		externalTyleType = PASS_EXT_TILE_4_RGBA;		break;
				case PASS_EXT_MIST:			externalTyleType = PASS_EXT_TILE_1_GRAYSCALE;	break;
				case PASS_EXT_OBJ_INDEX:	externalTyleType = PASS_EXT_TILE_1_GRAYSCALE;	break;
				case PASS_EXT_MAT_INDEX:	externalTyleType = PASS_EXT_TILE_1_GRAYSCALE;	break;
				default: 					externalTyleType = PASS_EXT_TILE_3_RGB;			break;
			}
			
			enabled = ((extPassType == PASS_EXT_DISABLED || extPassType >= PASS_EXT_TOTAL_PASSES) ? false : true);
		}
		
		bool enabled;		
		int	externalPassType;
		int externalTyleType;
		int internalYafPassType;
};


class YAFRAYCORE_EXPORT colorIntPasses_t  //Internal YafaRay color passes generated in different points of the rendering process
{
	public:
		colorIntPasses_t(): highestInternalPassUsed(PASS_YAF_COMBINED) 
        {
            highestInternalPassUsed = PASS_YAF_DISABLED;
            
            //for performance, even if we don't actually use all the possible internal passes, we reserve a contiguous memory block
            intPasses.reserve(PASS_YAF_TOTAL_PASSES);
            enabledIntPasses.reserve(PASS_YAF_TOTAL_PASSES);
            
            //by default, if no passes are explicitally enabled, we create the Combined pass by default
            enable_pass(PASS_YAF_COMBINED);
        }
        
		bool enabled(int pass) const
		{
			if(pass <= highestInternalPassUsed) return enabledIntPasses[pass];
            
            else return false;
		}
        
        void enable_pass(int pass)
        {
            if(enabled(pass) || pass == PASS_YAF_DISABLED) return;
            
            if(pass > highestInternalPassUsed)
            {
                for(int idx = highestInternalPassUsed+1; idx <= pass; ++idx)
                {
                    intPasses.push_back(init_color(idx));
                                        
                    if(idx == pass) enabledIntPasses.push_back(true);
                    else enabledIntPasses.push_back(false);
                }
                
                highestInternalPassUsed = pass;
            }
            enabledIntPasses[pass] = true;
        }
        
        colorA_t& color(int pass)
        {
            return intPasses[pass];
        }
                
        colorA_t& operator()(int pass)
        {
            return color(pass);
        }

		void reset_colors()
		{
			for(int idx = PASS_YAF_COMBINED; idx <= highestInternalPassUsed; ++idx)
			{
                color(idx) = init_color(idx);
			}
		}
        
        colorA_t init_color(int pass)
        {
            switch(pass)    //Default initialization color in general is black/opaque, except for SHADOW and MASK passes where the default is black/transparent for easier masking
            {
                case PASS_YAF_SHADOW:
                case PASS_YAF_OBJ_INDEX_MASK:
                case PASS_YAF_OBJ_INDEX_MASK_SHADOW:
                case PASS_YAF_OBJ_INDEX_MASK_ALL:
                case PASS_YAF_MAT_INDEX_MASK:
                case PASS_YAF_MAT_INDEX_MASK_SHADOW:
                case PASS_YAF_MAT_INDEX_MASK_ALL: return colorA_t(0.f, 0.f, 0.f, 0.f); break;
                default: return colorA_t(0.f, 0.f, 0.f, 1.f); break;
            }            
        }
		
		void multiply_colors(float factor)
		{
			for(int idx = PASS_YAF_COMBINED; idx <= highestInternalPassUsed; ++idx)
			{
                color(idx) *= factor;
			}
		}

		colorA_t probe_set(const int& pass, const colorA_t& renderedColor, const bool& condition = true)
		{
			if(condition && enabled(pass)) color(pass) = renderedColor;
            
			return renderedColor;
		}
        
		colorA_t probe_set(const int& pass, const colorIntPasses_t& colorPasses, const bool& condition = true)
		{
			if(condition && enabled(pass) && colorPasses.enabled(pass))
            {
                intPasses[pass] = colorPasses.intPasses[pass];	
                return  colorPasses.intPasses[pass];
            }
            else return colorA_t(0.f);
		}
        
		colorA_t probe_add(const int& pass, const colorA_t& renderedColor, const bool& condition = true)
		{
			if(condition && enabled(pass)) color(pass) += renderedColor;
			
			return renderedColor;
		}

		colorA_t probe_add(const int& pass, const colorIntPasses_t& colorPasses, const bool& condition = true)
		{
			if(condition && enabled(pass) && colorPasses.enabled(pass))
            {
                intPasses[pass] += colorPasses.intPasses[pass];	
                return  colorPasses.intPasses[pass];
            }
            else return colorA_t(0.f);
		}
        
		colorA_t probe_mult(const int& pass, const colorA_t& renderedColor, const bool& condition = true)
		{
			if(condition && enabled(pass)) color(pass) *= renderedColor;
			
			return renderedColor;
		}

		colorA_t probe_mult(const int& pass, const colorIntPasses_t& colorPasses, const bool& condition = true)
		{
			if(condition && enabled(pass) && colorPasses.enabled(pass))
            {
                intPasses[pass] *= colorPasses.intPasses[pass];	
                return  colorPasses.intPasses[pass];
            }
            else return colorA_t(0.f);
		}

        int get_highest_internal_pass_used() const { return highestInternalPassUsed; }
		
		colorIntPasses_t & operator *=(CFLOAT f);
		colorIntPasses_t & operator *=(color_t &a);
		colorIntPasses_t & operator *=(colorA_t &a);
		colorIntPasses_t & operator +=(colorIntPasses_t &a);

		float pass_mask_obj_index;	//Object Index used for masking in/out in the Mask Render Passes
		float pass_mask_mat_index;	//Material Index used for masking in/out in the Mask Render Passes
		bool pass_mask_invert;	//False=mask in, True=mask out
		bool pass_mask_only;	//False=rendered image is masked, True=only the mask is shown without rendered image
    
    protected:
		int highestInternalPassUsed;
		std::vector <bool> enabledIntPasses;
		std::vector <colorA_t> intPasses;
};


inline colorIntPasses_t & colorIntPasses_t::operator *=(CFLOAT f)
{
	for(int idx = PASS_YAF_COMBINED; idx <= highestInternalPassUsed; ++idx)
	{
		color(idx) *= f;
	}
	return *this;
}

inline colorIntPasses_t & colorIntPasses_t::operator *=(color_t &a)
{
	for(int idx = PASS_YAF_COMBINED; idx <= highestInternalPassUsed; ++idx)
	{
		color(idx) *= a;
	}
	return *this;
}

inline colorIntPasses_t & colorIntPasses_t::operator *=(colorA_t &a)
{
	for(int idx = PASS_YAF_COMBINED; idx <= highestInternalPassUsed; ++idx)
	{
		color(idx) *= a;
	}
	return *this;
}

inline colorIntPasses_t & colorIntPasses_t::operator +=(colorIntPasses_t &a)
{
	for(int idx = PASS_YAF_COMBINED; idx <= highestInternalPassUsed; ++idx)
	{
		color(idx) += a.color(idx);
	}
	return *this;
}


class YAFRAYCORE_EXPORT renderPasses_t
{
	public:

		renderPasses_t()
		{ 
			extPasses.reserve(PASS_EXT_TOTAL_PASSES);

			extPasses.push_back(extPass_t(PASS_EXT_COMBINED, PASS_YAF_COMBINED));	//by default we will have an external Combined pass
			
			this->generate_pass_maps();
		}

		void generate_pass_maps();	//Generate text strings <-> pass type maps
	
		void pass_add(const std::string& sExternalPass, const std::string& sInternalPass);	//Adds a new External Pass associated to an internal pass. Strings are used as parameters and they must match the strings in the maps generated by generate_pass_maps()
       
        size_t numExtPasses() const { return extPasses.size(); }
        
        int externalPassType(size_t pass_seq) const { return extPasses[pass_seq].externalPassType; }
	
	std::string externalPassTypeString(size_t pass_seq) const { return extPassMapIntString.find(extPasses[pass_seq].externalPassType)->second; }
        
        int externalTyleType(size_t pass_seq) const { return extPasses[pass_seq].externalTyleType; }

        int internalYafPassType(externalPassTypes_t pass) const { return extPasses[pass].internalYafPassType; }
        
        int internalYafPassType(int pass) const { return extPasses[pass].internalYafPassType; }
        
	std::map<int, std::string> extPassMapIntString; //Map int-string for external passes
	std::map<std::string, int> extPassMapStringInt; //Reverse map string-int for external passes
	std::map<int, std::string> intPassMapIntString; //Map int-string for internal passes
	std::map<std::string, int> intPassMapStringInt; //Reverse map string-int for internal passes
	colorIntPasses_t colorPassesTemplate;
        
    protected:
	std::vector<extPass_t> extPasses;		//List of the external Render passes to be exported
};

inline void renderPasses_t::generate_pass_maps()
{
	//External Render passes - mapping String and External Pass Type
	//IMPORTANT: the external strings MUST MATCH the pass property names in Blender. These must also match the property names in Blender-Exporter without the "pass_" prefix. 
	extPassMapStringInt["Combined"] = PASS_EXT_COMBINED;
	extPassMapStringInt["Depth"] = PASS_EXT_Z_DEPTH;
	extPassMapStringInt["Vector"] = PASS_EXT_VECTOR;
	extPassMapStringInt["Normal"] = PASS_EXT_NORMAL;
	extPassMapStringInt["UV"] = PASS_EXT_UV;
	extPassMapStringInt["Color"] = PASS_EXT_COLOR;
	extPassMapStringInt["Emit"] = PASS_EXT_EMIT;
	extPassMapStringInt["Mist"] = PASS_EXT_MIST;
	extPassMapStringInt["Diffuse"] = PASS_EXT_DIFFUSE;
	extPassMapStringInt["Spec"] = PASS_EXT_SPECULAR;
	extPassMapStringInt["AO"] = PASS_EXT_AO;
	extPassMapStringInt["Env"] = PASS_EXT_ENV;
	extPassMapStringInt["Indirect"] = PASS_EXT_INDIRECT;
	extPassMapStringInt["Shadow"] = PASS_EXT_SHADOW;
	extPassMapStringInt["Reflect"] = PASS_EXT_REFLECT;
	extPassMapStringInt["Refract"] = PASS_EXT_REFRACT;
	extPassMapStringInt["IndexOB"] = PASS_EXT_OBJ_INDEX;
	extPassMapStringInt["IndexMA"] = PASS_EXT_MAT_INDEX;
	extPassMapStringInt["DiffDir"] = PASS_EXT_DIFFUSE_DIRECT;
	extPassMapStringInt["DiffInd"] = PASS_EXT_DIFFUSE_INDIRECT;
	extPassMapStringInt["DiffCol"] = PASS_EXT_DIFFUSE_COLOR;
	extPassMapStringInt["GlossDir"] = PASS_EXT_GLOSSY_DIRECT;
	extPassMapStringInt["GlossInd"] = PASS_EXT_GLOSSY_INDIRECT;
	extPassMapStringInt["GlossCol"] = PASS_EXT_GLOSSY_COLOR;
	extPassMapStringInt["TransDir"] = PASS_EXT_TRANS_DIRECT;
	extPassMapStringInt["TransInd"] = PASS_EXT_TRANS_INDIRECT;
	extPassMapStringInt["TransCol"] = PASS_EXT_TRANS_COLOR;
	extPassMapStringInt["SubsurfaceDir"] = PASS_EXT_SUBSURFACE_DIRECT;
	extPassMapStringInt["SubsurfaceInd"] = PASS_EXT_SUBSURFACE_INDIRECT;
	extPassMapStringInt["SubsurfaceCol"] = PASS_EXT_SUBSURFACE_COLOR;

	//Generation of reverse map (pass type -> pass_string)
	for(std::map<std::string, int>::const_iterator it = extPassMapStringInt.begin(); it != extPassMapStringInt.end(); ++it)
	{
		extPassMapIntString[it->second] = it->first;
	}

	//Internal YafaRay Render passes - mapping String and Internal YafaRay Render passes
	//IMPORTANT: the internal strings MUST MATCH the valid values for the pass properties in Blender Exporter
	intPassMapStringInt["combined"] = PASS_YAF_COMBINED;
	intPassMapStringInt["z-depth-norm"] = PASS_YAF_Z_DEPTH_NORM;
	intPassMapStringInt["z-depth-abs"] = PASS_YAF_Z_DEPTH_ABS;
	intPassMapStringInt["debug-normal-smooth"] = PASS_YAF_NORMAL_SMOOTH;
	intPassMapStringInt["debug-normal-geom"] = PASS_YAF_NORMAL_GEOM;
	intPassMapStringInt["adv-radiance"] = PASS_YAF_RADIANCE;
	intPassMapStringInt["debug-uv"] = PASS_YAF_UV;
	intPassMapStringInt["emit"] = PASS_YAF_EMIT;
	intPassMapStringInt["mist"] = PASS_YAF_MIST;
	intPassMapStringInt["diffuse"] = PASS_YAF_DIFFUSE;
	intPassMapStringInt["diffuse-noshadow"] = PASS_YAF_DIFFUSE_NO_SHADOW;
	intPassMapStringInt["ao"] = PASS_YAF_AO;
	intPassMapStringInt["env"] = PASS_YAF_ENV;
	intPassMapStringInt["indirect"] = PASS_YAF_INDIRECT_ALL;
	intPassMapStringInt["adv-indirect"] = PASS_YAF_INDIRECT;
	intPassMapStringInt["shadow"] = PASS_YAF_SHADOW;
	intPassMapStringInt["reflect"] = PASS_YAF_REFLECT_ALL;
	intPassMapStringInt["refract"] = PASS_YAF_REFRACT_ALL;
	intPassMapStringInt["adv-reflect"] = PASS_YAF_REFLECT_PERFECT;
	intPassMapStringInt["adv-refract"] = PASS_YAF_REFRACT_PERFECT;
	intPassMapStringInt["obj-index-abs"] = PASS_YAF_OBJ_INDEX_ABS;
	intPassMapStringInt["obj-index-norm"] = PASS_YAF_OBJ_INDEX_NORM;
	intPassMapStringInt["obj-index-auto"] = PASS_YAF_OBJ_INDEX_AUTO;
	intPassMapStringInt["obj-index-mask"] = PASS_YAF_OBJ_INDEX_MASK;
	intPassMapStringInt["obj-index-mask-shadow"] = PASS_YAF_OBJ_INDEX_MASK_SHADOW;
	intPassMapStringInt["obj-index-mask-all"] = PASS_YAF_OBJ_INDEX_MASK_ALL;
	intPassMapStringInt["mat-index-abs"] = PASS_YAF_MAT_INDEX_ABS;
	intPassMapStringInt["mat-index-norm"] = PASS_YAF_MAT_INDEX_NORM;
	intPassMapStringInt["mat-index-auto"] = PASS_YAF_MAT_INDEX_AUTO;
	intPassMapStringInt["mat-index-mask"] = PASS_YAF_MAT_INDEX_MASK;
	intPassMapStringInt["mat-index-mask-shadow"] = PASS_YAF_MAT_INDEX_MASK_SHADOW;
	intPassMapStringInt["mat-index-mask-all"] = PASS_YAF_MAT_INDEX_MASK_ALL;
	intPassMapStringInt["adv-diffuse-indirect"] = PASS_YAF_DIFFUSE_INDIRECT;
	intPassMapStringInt["adv-diffuse-color"] = PASS_YAF_DIFFUSE_COLOR;
	intPassMapStringInt["adv-glossy"] = PASS_YAF_GLOSSY;
	intPassMapStringInt["adv-glossy-indirect"] = PASS_YAF_GLOSSY_INDIRECT;
	intPassMapStringInt["adv-glossy-color"] = PASS_YAF_GLOSSY_COLOR;
	intPassMapStringInt["adv-trans"] = PASS_YAF_TRANS;
	intPassMapStringInt["adv-trans-indirect"] = PASS_YAF_TRANS_INDIRECT;
	intPassMapStringInt["adv-trans-color"] = PASS_YAF_TRANS_COLOR;
	intPassMapStringInt["adv-subsurface"] = PASS_YAF_SUBSURFACE;
	intPassMapStringInt["adv-subsurface-indirect"] = PASS_YAF_SUBSURFACE_INDIRECT;
	intPassMapStringInt["adv-subsurface-color"] = PASS_YAF_SUBSURFACE_COLOR;
	intPassMapStringInt["debug-normal-smooth"] = PASS_YAF_NORMAL_SMOOTH;
	intPassMapStringInt["debug-normal-geom"] = PASS_YAF_NORMAL_GEOM;
	intPassMapStringInt["debug-nu"] = PASS_YAF_DEBUG_NU;
	intPassMapStringInt["debug-nv"] = PASS_YAF_DEBUG_NV;
	intPassMapStringInt["debug-dpdu"] = PASS_YAF_DEBUG_DPDU;
	intPassMapStringInt["debug-dpdv"] = PASS_YAF_DEBUG_DPDV;
	intPassMapStringInt["debug-dsdu"] = PASS_YAF_DEBUG_DSDU;
	intPassMapStringInt["debug-dsdv"] = PASS_YAF_DEBUG_DSDV;
	intPassMapStringInt["adv-surface-integration"] = PASS_YAF_SURFACE_INTEGRATION;
	intPassMapStringInt["adv-volume-integration"] = PASS_YAF_VOLUME_INTEGRATION;
	intPassMapStringInt["adv-volume-transmittance"] = PASS_YAF_VOLUME_TRANSMITTANCE;
	intPassMapStringInt["debug-aa-samples"] = PASS_YAF_AA_SAMPLES;

	//Generation of reverse map (pass type -> pass_string)
	for(std::map<std::string, int>::const_iterator it = intPassMapStringInt.begin(); it != intPassMapStringInt.end(); ++it)
	{
		intPassMapIntString[it->second] = it->first;
	}
}

inline void renderPasses_t::pass_add(const std::string& sExternalPass, const std::string& sInternalPass)
{
	//This function adds a new external pass, linked to a certain internal pass, based on the text strings indicated in the parameters.
	
	//By default, in case the strings are not found in the maps, set the types to "disabled"
	int extPassType = PASS_EXT_DISABLED;
	int intPassType = PASS_YAF_DISABLED;
	
	//Convert the string into the external pass type using the pass type maps
	std::map<std::string, int>::const_iterator extPassMapIterator = extPassMapStringInt.find(sExternalPass);
	if(extPassMapIterator != extPassMapStringInt.end()) extPassType = extPassMapIterator->second;
	
	//Convert the string into the internal pass type using the pass type maps
	std::map<std::string, int>::const_iterator intPassMapIterator = intPassMapStringInt.find(sInternalPass);
	if(intPassMapIterator != intPassMapStringInt.end()) intPassType = intPassMapIterator->second;
	
	if(extPassType != PASS_EXT_COMBINED && extPassType != PASS_EXT_DISABLED && intPassType != PASS_YAF_DISABLED)
	{
		//If both external and internal pass types exist and are not disabled, then add the External Pass with the appropiate link to the associated internal pass
		//Also, don't add another external Combined pass, as it's added by default, to avoid duplication of the Combined pass.
		extPasses.push_back(extPass_t(extPassType, intPassType));
		colorPassesTemplate.enable_pass(intPassType);
		
		Y_INFO << "Render Pass added: \"" << sExternalPass << "\" [" << extPassType << "]  (internal pass: \"" << sInternalPass << "\" " << intPassType << "]), highestInternalPassUsed = " << colorPassesTemplate.get_highest_internal_pass_used() << yendl;
	}
    
    //If any internal pass needs an auxiliary internal pass, enable also the auxiliary passes.
    switch(intPassType)
    {
        case PASS_YAF_REFLECT_ALL:
            colorPassesTemplate.enable_pass(PASS_YAF_REFLECT_PERFECT);
            colorPassesTemplate.enable_pass(PASS_YAF_GLOSSY);
            colorPassesTemplate.enable_pass(PASS_YAF_GLOSSY_INDIRECT);
            break;
            
        case PASS_YAF_REFRACT_ALL:
            colorPassesTemplate.enable_pass(PASS_YAF_REFRACT_PERFECT);
            colorPassesTemplate.enable_pass(PASS_YAF_TRANS);
            colorPassesTemplate.enable_pass(PASS_YAF_TRANS_INDIRECT);
            break;

        case PASS_YAF_INDIRECT_ALL:
            colorPassesTemplate.enable_pass(PASS_YAF_INDIRECT);
            colorPassesTemplate.enable_pass(PASS_YAF_DIFFUSE_INDIRECT);
            break;

        case PASS_YAF_OBJ_INDEX_MASK_ALL:
            colorPassesTemplate.enable_pass(PASS_YAF_OBJ_INDEX_MASK);
            colorPassesTemplate.enable_pass(PASS_YAF_OBJ_INDEX_MASK_SHADOW);
            break;

        case PASS_YAF_MAT_INDEX_MASK_ALL:
            colorPassesTemplate.enable_pass(PASS_YAF_MAT_INDEX_MASK);
            colorPassesTemplate.enable_pass(PASS_YAF_MAT_INDEX_MASK_SHADOW);
            break;
    }
}

__END_YAFRAY

#endif // Y_COLOR_H
