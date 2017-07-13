/****************************************************************************
 * 		imagetex.cc: a texture class for images
 *      This is part of the yafray package
 *      Based on the original by: Mathias Wein; Copyright (C) 2006 Mathias Wein
 *		Copyright (C) 2010 Rodrigo Placencia Vazquez (DarkTide)
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

#include <cstring>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <textures/imagetex.h>
#include <utilities/stringUtils.h>

__BEGIN_YAFRAY

float * textureImage_t::ewaWeightLut = nullptr;

textureImage_t::textureImage_t(imageHandler_t *ih, int intp, float gamma, colorSpaces_t color_space):
				image(ih), colorSpace(color_space), gamma(gamma), mirrorX(false), mirrorY(false)
{
	intp_type = intp;
}

textureImage_t::~textureImage_t()
{
	// Here we simply clear the pointer, yafaray's core will handle the memory cleanup
	image = nullptr;
}

void textureImage_t::resolution(int &x, int &y, int &z) const
{
	x=image->getWidth();
	y=image->getHeight();
	z=0;
}

colorA_t textureImage_t::interpolateImage(const point3d_t &p, mipMapParams_t * mmParams) const
{
	if(mmParams && mmParams->forceImageLevel > 0.f) return mipMapsTrilinearInterpolation(p, mmParams);

	colorA_t interpolatedColor(0.f);

	switch(intp_type)
	{
		case INTP_NONE: interpolatedColor = noInterpolation(p); break;
		case INTP_BICUBIC: interpolatedColor = bicubicInterpolation(p); break;
		case INTP_MIPMAP_TRILINEAR:
									if(mmParams) interpolatedColor = mipMapsTrilinearInterpolation(p, mmParams);
									else interpolatedColor = bilinearInterpolation(p); 
									break;
		case INTP_MIPMAP_EWA:
									if(mmParams) interpolatedColor = mipMapsEWAInterpolation(p, ewa_max_anisotropy, mmParams);
									else interpolatedColor = bilinearInterpolation(p); 
									break;
		case INTP_BILINEAR:
		default: 			interpolatedColor = bilinearInterpolation(p); break;	//By default use Bilinear
	}

	return interpolatedColor;
}

colorA_t textureImage_t::getColor(const point3d_t &p, mipMapParams_t * mmParams) const
{
	point3d_t p1 = point3d_t(p.x, -p.y, p.z);
	colorA_t ret(0.f);

	bool outside = doMapping(p1);

	if(outside) return ret;
	
	ret = interpolateImage(p1, mmParams);
		
	return applyAdjustments(ret);
}

colorA_t textureImage_t::getRawColor(const point3d_t &p, mipMapParams_t * mmParams) const
{
	// As from v3.2.0 all color buffers are already Linear RGB, if any part of the code requires the original "raw" color, a "de-linearization" (encoding again into the original color space) takes place in this function.
	
	// For example for Non-RGB / Stencil / Bump / Normal maps, etc, textures are typically already linear and the user should select "linearRGB" in the texture properties, but if the user (by mistake) keeps the default sRGB for them, for example, the default linearization would apply a sRGB to linearRGB conversion that messes up the texture values. This function will try to reconstruct the original texture values. In this case (if the user selected incorrectly sRGB for a normal map, for example), this function will prevent wrong results, but it will be slower and it could be slightly inaccurate as the interpolation will take place in the (incorrectly) linearized texels. 
	
	//If the user correctly selected "linearRGB" for these kind of textures, the function below will not make any changes to the color and will keep the texture "as is" without any linearization nor de-linearization, which is the ideal case (fast and correct).
	
	//The user is responsible to select the correct textures color spaces, if the user does not do it, results may not be the expected. This function is only a coarse "fail safe"
	
	colorA_t ret = getColor(p, mmParams);
	ret.ColorSpace_from_linearRGB(colorSpace, gamma);
	
	return ret;
}

colorA_t textureImage_t::getColor(int x, int y, int z, mipMapParams_t * mmParams) const
{
	int resx=image->getWidth();
	int resy=image->getHeight();

	y = resy - y; //on occasion change image storage from bottom to top...

	x = std::max(0, std::min(resx-1, x));
	y = std::max(0, std::min(resy-1, y));

	colorA_t ret(0.f);
	
	if(mmParams && mmParams->forceImageLevel > 0.f) ret = image->getPixel(x, y, (int) floorf(mmParams->forceImageLevel * image->getHighestImgIndex()));
	else ret = image->getPixel(x, y);

	return applyAdjustments(ret);
}

colorA_t textureImage_t::getRawColor(int x, int y, int z, mipMapParams_t * mmParams) const
{
	// As from v3.2.0 all color buffers are already Linear RGB, if any part of the code requires the original "raw" color, a "de-linearization" (encoding again into the original color space) takes place in this function.
	
	// For example for Non-RGB / Stencil / Bump / Normal maps, etc, textures are typically already linear and the user should select "linearRGB" in the texture properties, but if the user (by mistake) keeps the default sRGB for them, for example, the default linearization would apply a sRGB to linearRGB conversion that messes up the texture values. This function will try to reconstruct the original texture values. In this case (if the user selected incorrectly sRGB for a normal map, for example), this function will prevent wrong results, but it will be slower and it could be slightly inaccurate as the interpolation will take place in the (incorrectly) linearized texels. 
	
	//If the user correctly selected "linearRGB" for these kind of textures, the function below will not make any changes to the color and will keep the texture "as is" without any linearization nor de-linearization, which is the ideal case (fast and correct).
	
	//The user is responsible to select the correct textures color spaces, if the user does not do it, results may not be the expected. This function is only a coarse "fail safe"
	
	colorA_t ret = getColor(x, y, z, mmParams);
	ret.ColorSpace_from_linearRGB(colorSpace, gamma);
		
	return ret;
}

bool textureImage_t::doMapping(point3d_t &texpt) const
{
	bool outside = false;
	
	texpt = 0.5f*texpt + 0.5f;
	// repeat, only valid for REPEAT clipmode
	if (tex_clipmode==TCL_REPEAT) {
		
		if(xrepeat > 1) texpt.x *= (float)xrepeat;
		if(yrepeat > 1) texpt.y *= (float)yrepeat;
		
		if (mirrorX && int(ceilf(texpt.x)) % 2 == 0) texpt.x = -texpt.x;
		if (mirrorY && int(ceilf(texpt.y)) % 2 == 0) texpt.y = -texpt.y;
		
		if (texpt.x>1.f) texpt.x -= int(texpt.x);
		else if (texpt.x<0.f) texpt.x += 1 - int(texpt.x);
		
		if (texpt.y>1.f) texpt.y -= int(texpt.y);
		else if (texpt.y<0.f) texpt.y += 1 - int(texpt.y);
	}
	
	// crop
	if (cropx) texpt.x = cropminx + texpt.x*(cropmaxx - cropminx);
	if (cropy) texpt.y = cropminy + texpt.y*(cropmaxy - cropminy);
	
	// rot90
	if(rot90) std::swap(texpt.x, texpt.y);
	
	// clipping
	switch (tex_clipmode)
	{
		case TCL_CLIPCUBE: {
			if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1) || (texpt.z<-1) || (texpt.z>1))
				outside = true;
			break;
		}
		case TCL_CHECKER: {
			int xs=(int)floor(texpt.x), ys=(int)floor(texpt.y);
			texpt.x -= xs;
			texpt.y -= ys;
			if ( !checker_odd && !((xs+ys) & 1) )
			{
				outside = true;
				break;
			}
			if ( !checker_even && ((xs+ys) & 1) )
			{
				outside = true;
				break;
			}
			// scale around center, (0.5, 0.5)
			if (checker_dist<1.0) {
				texpt.x = (texpt.x-0.5) / (1.0-checker_dist) + 0.5;
				texpt.y = (texpt.y-0.5) / (1.0-checker_dist) + 0.5;
			}
			// continue to TCL_CLIP
		}
		case TCL_CLIP: {
			if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1))
				outside = true;
			break;
		}
		case TCL_EXTEND: {
			if (texpt.x>0.99999f) texpt.x=0.99999f; else if (texpt.x<0) texpt.x=0;
			if (texpt.y>0.99999f) texpt.y=0.99999f; else if (texpt.y<0) texpt.y=0;
			// no break, fall thru to TEX_REPEAT
		}
		default:
		case TCL_REPEAT: outside = false;
	}
	return outside;
}

void textureImage_t::setCrop(float minx, float miny, float maxx, float maxy)
{
	cropminx=minx, cropmaxx=maxx, cropminy=miny, cropmaxy=maxy;
	cropx = ((cropminx!=0.0) || (cropmaxx!=1.0));
	cropy = ((cropminy!=0.0) || (cropmaxy!=1.0));
}

void textureImage_t::findTextureInterpolationCoordinates(int &coord0, int &coord1, int &coord2, int &coord3, float &coord_decimal_part, float coord_float, int resolution, bool repeat, bool mirror) const
{
	if(repeat)
	{
		coord1 = ((int)coord_float) % resolution;

		if(mirror)
		{

			if(coord_float < 0.f)
			{
				coord0 = 1 % resolution;
				coord2 = coord1;
				coord3 = coord0;
				coord_decimal_part = -coord_float;
			}
			else if(coord_float >= resolution - 1.f)
			{
				coord0 = (resolution+resolution-1) % resolution;
				coord2 = coord1;
				coord3 = coord0;
				coord_decimal_part = coord_float - ((int)coord_float);
			}
			else
			{
				coord0 = (resolution+coord1-1) % resolution;
				coord2 = coord1+1;
				if(coord2 >= resolution) coord2 = (resolution + resolution - coord2) % resolution;
				coord3 = coord1+2;
				if(coord3 >= resolution) coord3 = (resolution + resolution - coord3) % resolution;
				coord_decimal_part = coord_float - ((int)coord_float);
			}			
		}
		else
		{
			if(coord_float > 0.f)
			{
				coord0 = (resolution+coord1-1) % resolution;
				coord2 = (coord1+1) % resolution;
				coord3 = (coord1+2) % resolution;
				coord_decimal_part = coord_float - ((int)coord_float);
			}
			else
			{
				coord0 = 1 % resolution;
				coord2 = (resolution-1) % resolution;
				coord3 = (resolution-2) % resolution;
				coord_decimal_part = -coord_float;
			}
		}
	}
	else
	{
		coord1 = std::max(0, std::min(resolution-1, ((int)coord_float)));

		if(coord_float > 0.f) coord2 = std::min(resolution-1, coord1+1);
		else coord2 = 0;
		
		coord0 = std::max(0, coord1-1);
		coord3 = std::min(resolution-1, coord2+1);
		
		coord_decimal_part = coord_float - floor(coord_float);
	}
}

colorA_t textureImage_t::noInterpolation(const point3d_t &p, int mipmaplevel) const
{
	int resx = image->getWidth(mipmaplevel);
	int resy = image->getHeight(mipmaplevel);
	
	float xf = ((float)resx * (p.x - floor(p.x)));
	float yf = ((float)resy * (p.y - floor(p.y)));

	int x0, x1, x2, x3, y0, y1, y2, y3;
	float dx, dy;
	findTextureInterpolationCoordinates(x0, x1, x2, x3, dx, xf, resx, tex_clipmode==TCL_REPEAT, mirrorX);
	findTextureInterpolationCoordinates(y0, y1, y2, y3, dy, yf, resy, tex_clipmode==TCL_REPEAT, mirrorY);

	return image->getPixel(x1, y1, mipmaplevel);
}

colorA_t textureImage_t::bilinearInterpolation(const point3d_t &p, int mipmaplevel) const
{
	int resx = image->getWidth(mipmaplevel);
	int resy = image->getHeight(mipmaplevel);
	
	float xf = ((float)resx * (p.x - floor(p.x))) - 0.5f;
	float yf = ((float)resy * (p.y - floor(p.y))) - 0.5f;

	int x0, x1, x2, x3, y0, y1, y2, y3;
	float dx, dy;
	findTextureInterpolationCoordinates(x0, x1, x2, x3, dx, xf, resx, tex_clipmode==TCL_REPEAT, mirrorX);
	findTextureInterpolationCoordinates(y0, y1, y2, y3, dy, yf, resy, tex_clipmode==TCL_REPEAT, mirrorY);

	colorA_t c11 = image->getPixel(x1, y1, mipmaplevel);
	colorA_t c21 = image->getPixel(x2, y1, mipmaplevel);
	colorA_t c12 = image->getPixel(x1, y2, mipmaplevel);
	colorA_t c22 = image->getPixel(x2, y2, mipmaplevel);

	float w11 = (1-dx) * (1-dy);
	float w12 = (1-dx) * dy;
	float w21 = dx * (1-dy);
	float w22 = dx * dy;
	
	return (w11 * c11) + (w12 * c12) + (w21 * c21) + (w22 * c22);
}

colorA_t textureImage_t::bicubicInterpolation(const point3d_t &p, int mipmaplevel) const
{
	int resx = image->getWidth(mipmaplevel);
	int resy = image->getHeight(mipmaplevel);
	
	float xf = ((float)resx * (p.x - floor(p.x))) - 0.5f;
	float yf = ((float)resy * (p.y - floor(p.y))) - 0.5f;

	int x0, x1, x2, x3, y0, y1, y2, y3;
	float dx, dy;
	findTextureInterpolationCoordinates(x0, x1, x2, x3, dx, xf, resx, tex_clipmode==TCL_REPEAT, mirrorX);
	findTextureInterpolationCoordinates(y0, y1, y2, y3, dy, yf, resy, tex_clipmode==TCL_REPEAT, mirrorY);

	colorA_t c00 = image->getPixel(x0, y0, mipmaplevel);
	colorA_t c01 = image->getPixel(x0, y1, mipmaplevel);
	colorA_t c02 = image->getPixel(x0, y2, mipmaplevel);
	colorA_t c03 = image->getPixel(x0, y3, mipmaplevel);

	colorA_t c10 = image->getPixel(x1, y0, mipmaplevel);
	colorA_t c11 = image->getPixel(x1, y1, mipmaplevel);
	colorA_t c12 = image->getPixel(x1, y2, mipmaplevel);
	colorA_t c13 = image->getPixel(x1, y3, mipmaplevel);

	colorA_t c20 = image->getPixel(x2, y0, mipmaplevel);
	colorA_t c21 = image->getPixel(x2, y1, mipmaplevel);
	colorA_t c22 = image->getPixel(x2, y2, mipmaplevel);
	colorA_t c23 = image->getPixel(x2, y3, mipmaplevel);

	colorA_t c30 = image->getPixel(x3, y0, mipmaplevel);
	colorA_t c31 = image->getPixel(x3, y1, mipmaplevel);
	colorA_t c32 = image->getPixel(x3, y2, mipmaplevel);
	colorA_t c33 = image->getPixel(x3, y3, mipmaplevel);

	colorA_t cy0 = CubicInterpolate(c00, c10, c20, c30, dx);
	colorA_t cy1 = CubicInterpolate(c01, c11, c21, c31, dx);
	colorA_t cy2 = CubicInterpolate(c02, c12, c22, c32, dx);
	colorA_t cy3 = CubicInterpolate(c03, c13, c23, c33, dx);

	return CubicInterpolate(cy0, cy1, cy2, cy3, dy);
}

colorA_t textureImage_t::mipMapsTrilinearInterpolation(const point3d_t &p, mipMapParams_t * mmParams) const
{
	float dS = std::max(fabsf(mmParams->dSdx), fabsf(mmParams->dSdy)) * image->getWidth();
	float dT = std::max(fabsf(mmParams->dTdx), fabsf(mmParams->dTdy)) * image->getHeight();
	float mipmaplevel = 0.5f * log2(dS*dS + dT*dT);
	
	if(mmParams->forceImageLevel > 0.f) mipmaplevel = mmParams->forceImageLevel * image->getHighestImgIndex();
	
	mipmaplevel += trilinear_level_bias;

	mipmaplevel = std::min(std::max(0.f, mipmaplevel), (float) image->getHighestImgIndex());
	
	int mipmaplevelA = (int) floor(mipmaplevel);
	int mipmaplevelB = (int) ceil(mipmaplevel);
	float mipmaplevelDelta = mipmaplevel - (float) mipmaplevelA;
	
	colorA_t col = bilinearInterpolation(p, mipmaplevelA);
	colorA_t colB = bilinearInterpolation(p, mipmaplevelB);

	col.blend(colB, mipmaplevelDelta);
	
	return col;
}

//All EWA interpolation/calculation code has been adapted from PBRT v2 (https://github.com/mmp/pbrt-v2). see LICENSES file

colorA_t textureImage_t::mipMapsEWAInterpolation(const point3d_t &p, float maxAnisotropy, mipMapParams_t * mmParams) const
{
	float dS0 = fabsf(mmParams->dSdx);
	float dS1 = fabsf(mmParams->dSdy);
	float dT0 = fabsf(mmParams->dTdx);
	float dT1 = fabsf(mmParams->dTdy);
	
	if((dS0*dS0 + dT0*dT0) < (dS1*dS1 + dT1*dT1))
	{
		std::swap(dS0, dS1);
		std::swap(dT0, dT1);
	}
	
	float majorLength = sqrtf(dS0*dS0 + dT0*dT0);
	float minorLength = sqrtf(dS1*dS1 + dT1*dT1);

	if((minorLength * maxAnisotropy < majorLength) && (minorLength > 0.f))
	{
		float scale = majorLength / (minorLength * maxAnisotropy);
		dS1 *= scale;
		dT1 *= scale;
		minorLength *= scale;
	}

	if(minorLength <= 0.f) return bilinearInterpolation(p);

	float mipmaplevel = image->getHighestImgIndex() - 1.f + log2(minorLength);

	mipmaplevel = std::min(std::max(0.f, mipmaplevel), (float) image->getHighestImgIndex());
	
	
	int mipmaplevelA = (int) floor(mipmaplevel);
	int mipmaplevelB = (int) ceil(mipmaplevel);
	float mipmaplevelDelta = mipmaplevel - (float) mipmaplevelA;
	
	colorA_t col = EWAEllipticCalculation(p, dS0, dT0, dS1, dT1, mipmaplevelA);
	colorA_t colB = EWAEllipticCalculation(p, dS0, dT0, dS1, dT1, mipmaplevelB);
	
	col.blend(colB, mipmaplevelDelta);
	
	return col;
}

inline int Mod(int a, int b) {
    int n = int(a/b);
    a -= n*b;
    if (a < 0) a += b;
    return a;
}

colorA_t textureImage_t::EWAEllipticCalculation(const point3d_t &p, float dS0, float dT0, float dS1, float dT1, int mipmaplevel) const
{
	if(mipmaplevel >= image->getHighestImgIndex())
	{
		int resx = image->getWidth(mipmaplevel);
		int resy = image->getHeight(mipmaplevel);
		
		return image->getPixel(Mod(p.x, resx), Mod(p.y, resy), image->getHighestImgIndex());
	}
	
	int resx = image->getWidth(mipmaplevel);
	int resy = image->getHeight(mipmaplevel);
	
	float xf = ((float)resx * (p.x - floor(p.x))) - 0.5f;
	float yf = ((float)resy * (p.y - floor(p.y))) - 0.5f;
	
	dS0 *= resx;
	dS1 *= resx;
	dT0 *= resy;
	dT1 *= resy;

	float A = dT0*dT0 + dT1*dT1 + 1;
	float B = -2.f * (dS0*dT0 + dS1*dT1);
	float C = dS0*dS0 + dS1*dS1 + 1;
	float invF = 1.f / (A*C - B*B*0.25f);
	A *= invF;
	B *= invF;
	C *= invF;
	
	float det = -B*B + 4.f*A*C;
	float invDet = 1.f / det;
	float uSqrt = sqrtf(det * C);
	float vSqrt = sqrtf(A * det);
	
	int s0 = (int) ceilf(xf - 2.f * invDet * uSqrt);
	int s1 = (int) floorf(xf + 2.f * invDet * uSqrt);
	int t0 = (int) ceilf(yf - 2.f * invDet * vSqrt);
	int t1 = (int) floorf(yf + 2.f * invDet * vSqrt);
	
	colorA_t sumCol(0.f);
	
	float sumWts = 0.f;
	for(int it = t0; it <= t1; ++it)
	{
		float tt = it - yf;
		for(int is = s0; is <= s1; ++is)
		{
			float ss = is - xf;
			
			float r2 = A*ss*ss + B*ss*tt + C*tt*tt;
			if(r2 < 1.f)
			{
				float weight = ewaWeightLut[std::min((int)floorf(r2*EWA_WEIGHT_LUT_SIZE), EWA_WEIGHT_LUT_SIZE-1)];
				int ismod = Mod(is, resx);
				int itmod = Mod(it, resy);

				sumCol += image->getPixel(ismod, itmod, mipmaplevel) * weight;
				sumWts += weight;
			}
		}
	}
	
	if(sumWts > 0.f) sumCol = sumCol / sumWts;
	else sumCol = colorA_t(0.f);
	
	return sumCol;
}

void textureImage_t::generateEWALookupTable()
{
	if(!ewaWeightLut)
	{
		Y_DEBUG << "** GENERATING EWA LOOKUP **" << yendl;
		ewaWeightLut = (float *) malloc(sizeof(float) * EWA_WEIGHT_LUT_SIZE);
		for(int i = 0; i < EWA_WEIGHT_LUT_SIZE; ++i)
		{
			float alpha = 2.f;
			float r2 = float(i) / float(EWA_WEIGHT_LUT_SIZE - 1);
			ewaWeightLut[i] = expf(-alpha * r2) - expf(-alpha);
		}
	}
}

int string2cliptype(const std::string *clipname)
{
	// default "repeat"
	int	tex_clipmode = TCL_REPEAT;
	if(!clipname) return tex_clipmode;
	if (*clipname=="extend")			tex_clipmode = TCL_EXTEND;
	else if (*clipname=="clip")		tex_clipmode = TCL_CLIP;
	else if (*clipname=="clipcube")	tex_clipmode = TCL_CLIPCUBE;
	else if (*clipname=="checker")	tex_clipmode = TCL_CHECKER;
	return tex_clipmode;
}

texture_t *textureImage_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	const std::string *name = nullptr;
	const std::string *intpstr = nullptr;
	double gamma = 1.0;
	double expadj = 0.0;
	bool normalmap = false;
	std::string color_space_string = "Raw_Manual_Gamma";
	colorSpaces_t color_space = RAW_MANUAL_GAMMA;
	std::string texture_optimization_string = "optimized";
	textureOptimization_t texture_optimization = TEX_OPTIMIZATION_OPTIMIZED;
	bool img_grayscale = false;
	textureImage_t *tex = nullptr;
	imageHandler_t *ih = nullptr;
	params.getParam("interpolate", intpstr);
	params.getParam("color_space", color_space_string);
	params.getParam("gamma", gamma);
	params.getParam("exposure_adjust", expadj);
	params.getParam("normalmap", normalmap);
	params.getParam("filename", name);
	params.getParam("texture_optimization", texture_optimization_string);
	params.getParam("img_grayscale", img_grayscale);
	
	if(!name)
	{
		Y_ERROR << "ImageTexture: Required argument filename not found for image texture" << yendl;
		return nullptr;
	}
	
	// interpolation type, bilinear default
	int intp = INTP_BILINEAR;
	
	if(intpstr)
	{
		if (*intpstr == "none") intp = INTP_NONE;
		else if (*intpstr == "bicubic") intp = INTP_BICUBIC;
		else if (*intpstr == "mipmap_trilinear") intp = INTP_MIPMAP_TRILINEAR;
		else if (*intpstr == "mipmap_ewa") intp = INTP_MIPMAP_EWA;
	}
		
	size_t lDot = name->rfind(".") + 1;
	size_t lSlash = name->rfind("/") + 1;
	
	std::string ext = toLower(name->substr(lDot));
	
	std::string fmt = render.getImageFormatFromExtension(ext);
	
	if(fmt == "")
	{
		Y_ERROR << "ImageTexture: Image extension not recognized, dropping texture." << yendl;
		return nullptr;
	}
	
	paraMap_t ihpm;
	ihpm["type"] = fmt;
	ihpm["for_output"] = false;
	std::string ihname = "ih";
	ihname.append(toLower(name->substr(lSlash, lDot - lSlash - 1)));
	
	ih = render.createImageHandler(ihname, ihpm);
	
	if(!ih)
	{
		Y_ERROR << "ImageTexture: Couldn't create image handler, dropping texture." << yendl;
		return nullptr;
	}
	

	if(ih->isHDR())
	{
		if(color_space_string != "LinearRGB") Y_VERBOSE << "ImageTexture: The image is a HDR/EXR file: forcing linear RGB and ignoring selected color space '" << color_space_string <<"' and the gamma setting." << yendl;
		color_space = LINEAR_RGB;
		if(texture_optimization_string != "none") Y_VERBOSE << "ImageTexture: The image is a HDR/EXR file: forcing texture optimization to 'none' and ignoring selected texture optimization '" << texture_optimization_string <<"'" << yendl;
		texture_optimization = TEX_OPTIMIZATION_NONE;	//FIXME DAVID: Maybe we should leave this to imageHandler factory code...
	}
	else
	{
		if(color_space_string == "sRGB") color_space = SRGB;
		else if(color_space_string == "XYZ") color_space = XYZ_D65;
		else if(color_space_string == "LinearRGB") color_space = LINEAR_RGB;
		else if(color_space_string == "Raw_Manual_Gamma") color_space = RAW_MANUAL_GAMMA;
		else color_space = SRGB;
		
		if(texture_optimization_string == "none") texture_optimization = TEX_OPTIMIZATION_NONE;
		else if(texture_optimization_string == "optimized") texture_optimization = TEX_OPTIMIZATION_OPTIMIZED;
		else if(texture_optimization_string == "compressed") texture_optimization = TEX_OPTIMIZATION_COMPRESSED;
		else texture_optimization = TEX_OPTIMIZATION_NONE;
	}
	
	ih->setColorSpace(color_space, gamma);
	ih->setTextureOptimization(texture_optimization);	//FIXME DAVID: Maybe we should leave this to imageHandler factory code...
	ih->setGrayScaleSetting(img_grayscale);
	
	if(!ih->loadFromFile(*name))
	{
		Y_ERROR << "ImageTexture: Couldn't load image file, dropping texture." << yendl;
		return nullptr;
	}

	tex = new textureImage_t(ih, intp, gamma, color_space);

	if(!tex)
	{
		Y_ERROR << "ImageTexture: Couldn't create image texture." << yendl;
		return nullptr;
	}

	if(intp == INTP_MIPMAP_TRILINEAR || intp == INTP_MIPMAP_EWA)
	{
		ih->generateMipMaps();
		if(!session.getDifferentialRaysEnabled())
		{
			Y_VERBOSE << "At least one texture using mipmaps interpolation, enabling ray differentials." << yendl;
			session.setDifferentialRaysEnabled(true);	//If there is at least one texture using mipmaps, then enable differential rays in the rendering process.
		}
		
		/*//FIXME DAVID: TEST SAVING MIPMAPS. CAREFUL: IT COULD CAUSE CRASHES!
		for(int i=0; i<=ih->getHighestImgIndex(); ++i)
		{
			std::stringstream ss;
			ss << "//tmp//saved_mipmap_" << ihname << "__" << i;
			ih->saveToFile(ss.str(), i);
		}*/
	}
	
	// setup image
	bool rot90 = false;
	bool even_tiles=false, odd_tiles=true;
	bool use_alpha=true, calc_alpha=false;
	int xrep=1, yrep=1;
	double minx=0.0, miny=0.0, maxx=1.0, maxy=1.0;
	double cdist=0.0;
	const std::string *clipmode = nullptr;
	bool mirror_x = false;
	bool mirror_y = false;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	float trilinear_level_bias = 0.f;
	float ewa_max_anisotropy = 8.f;
	
	params.getParam("xrepeat", xrep);
	params.getParam("yrepeat", yrep);
	params.getParam("cropmin_x", minx);
	params.getParam("cropmin_y", miny);
	params.getParam("cropmax_x", maxx);
	params.getParam("cropmax_y", maxy);
	params.getParam("rot90", rot90);
	params.getParam("clipping", clipmode);
	params.getParam("even_tiles", even_tiles);
	params.getParam("odd_tiles", odd_tiles);
	params.getParam("checker_dist", cdist);
	params.getParam("use_alpha", use_alpha);
	params.getParam("calc_alpha", calc_alpha);
	params.getParam("mirror_x", mirror_x);
	params.getParam("mirror_y", mirror_y);
	params.getParam("trilinear_level_bias", trilinear_level_bias);
	params.getParam("ewa_max_anisotropy", ewa_max_anisotropy);
	
	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);
		
	tex->xrepeat = xrep;
	tex->yrepeat = yrep;
	tex->rot90 = rot90;
	tex->setCrop(minx, miny, maxx, maxy);
	tex->use_alpha = use_alpha;
	tex->calc_alpha = calc_alpha;
	tex->normalmap = normalmap;
	tex->tex_clipmode = string2cliptype(clipmode);
	tex->checker_even = even_tiles;
	tex->checker_odd = odd_tiles;
	tex->checker_dist = cdist;
	tex->mirrorX = mirror_x;
	tex->mirrorY = mirror_y;
	
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	
	tex->trilinear_level_bias = trilinear_level_bias;
	tex->ewa_max_anisotropy = ewa_max_anisotropy;

	if(intp == INTP_MIPMAP_EWA) tex->generateEWALookupTable();
	
	return tex;
}

__END_YAFRAY
