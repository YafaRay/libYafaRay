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
#include <textures/imagetex.h>
#include <utilities/stringUtils.h>

__BEGIN_YAFRAY

textureImage_t::textureImage_t(imageHandler_t *ih, interpolationType intp, colorSpaces_t color_space, float gamma):
				image(ih), intp_type(intp), colorSpace(color_space), gamma(gamma)
{
	// Empty
}

textureImage_t::~textureImage_t()
{
	// Here we simply clear the pointer, yafaray's core will handle the memory cleanup
	image = NULL;
}

void textureImage_t::resolution(int &x, int &y, int &z) const
{
	x=image->getWidth();
	y=image->getHeight();
	z=0;
}

colorA_t textureImage_t::interpolateImage(const point3d_t &p) const
{
	int x, y, x2, y2;
	
	int resx=image->getWidth();
	int resy=image->getHeight();
	
	float xf = ((float)resx * (p.x - floor(p.x)));
	float yf = ((float)resy * (p.y - floor(p.y)));
	
	if (intp_type != INTP_NONE)
	{
		xf -= 0.5f;
		yf -= 0.5f;
	}
	
	x = std::max(0, std::min(resx-1, (int)xf));
	y = std::max(0, std::min(resy-1, (int)yf));

	colorA_t c1 = image->getPixel(x, y);
	
	if (intp_type == INTP_NONE) return c1;
	
	colorA_t c2, c3, c4;
	
	x2 = std::min(resx-1, x+1);
	y2 = std::min(resy-1, y+1);

	c2 = image->getPixel(x2, y);
	c3 = image->getPixel(x, y2);
	c4 = image->getPixel(x2, y2);

	float dx = xf - floor(xf);
	float dy = yf - floor(yf);
	
	if (intp_type == INTP_BILINEAR)
	{
		float w0 = (1-dx) * (1-dy);
		float w1 = (1-dx) * dy;
		float w2 = dx * (1-dy);
		float w3 = dx * dy;
		
		return (w0 * c1) + (w1 * c3) + (w2 * c2) + (w3 * c4);
	}

	colorA_t c0, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF, ret;

	int x0 = std::max(0, x-1);
	int x3 = std::min(resx-1, x2+1);
	int y0 = std::max(0, y-1);
	int y3 = std::min(resy-1, y2+1);

	c0 = image->getPixel(x0, y0);
	c5 = image->getPixel(x,  y0);
	c6 = image->getPixel(x2, y0);
	c7 = image->getPixel(x3, y0);
	c8 = image->getPixel(x0, y);
	c9 = image->getPixel(x3, y);
	cA = image->getPixel(x0, y2);
	cB = image->getPixel(x3, y2);
	cC = image->getPixel(x0, y3);
	cD = image->getPixel(x,  y3);
	cE = image->getPixel(x2, y3);
	cF = image->getPixel(x3, y3);

	c0 = CubicInterpolate(c0, c5, c6, c7, dx);
	c8 = CubicInterpolate(c8, c1, c2, c9, dx);
	cA = CubicInterpolate(cA, c3, c4, cB, dx);
	cC = CubicInterpolate(cC, cD, cE, cF, dx);
	
	c0 = CubicInterpolate(c0, c8, cA, cC, dy);
	
	return c0;
}

colorA_t textureImage_t::getColor(const point3d_t &p) const
{
	colorA_t ret = getRawColor(p);
	ret.linearRGB_from_ColorSpace(colorSpace, gamma);
	return ret;
}

colorA_t textureImage_t::getRawColor(const point3d_t &p) const
{
	point3d_t p1 = point3d_t(p.x, -p.y, p.z);
	colorA_t ret(0.f);

	bool outside = doMapping(p1);

	if(outside) return ret;
	
	ret = interpolateImage(p1);
	
	return ret;
}

colorA_t textureImage_t::getColor(int x, int y, int z) const
{
	colorA_t ret = getRawColor(x, y, z);
	ret.linearRGB_from_ColorSpace(colorSpace, gamma);
	return ret;
}

colorA_t textureImage_t::getRawColor(int x, int y, int z) const
{
	int resx=image->getWidth();
	int resy=image->getHeight();

	y = resy - y; //on occasion change image storage from bottom to top...

	x = std::max(0, std::min(resx-1, x));
	y = std::max(0, std::min(resy-1, y));

	colorA_t c1(0.f);
	
	return image->getPixel(x, y);
}

bool textureImage_t::doMapping(point3d_t &texpt) const
{
	bool outside = false;
	texpt = 0.5f*texpt + 0.5f;
	// repeat, only valid for REPEAT clipmode
	if (tex_clipmode==TCL_REPEAT) {
		if (xrepeat>1) {
			texpt.x *= (PFLOAT)xrepeat;
			if (texpt.x>1.0) texpt.x -= int(texpt.x); else if (texpt.x<0.0) texpt.x += 1-int(texpt.x);
		}
		if (yrepeat>1) {
			texpt.y *= (PFLOAT)yrepeat;
			if (texpt.y>1.0) texpt.y -= int(texpt.y); else if (texpt.y<0.0) texpt.y += 1-int(texpt.y);
		}
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
	const std::string *name = NULL;
	const std::string *intpstr = NULL;
	double gamma = 1.0;
	double expadj = 0.0;
	bool normalmap = false;
	std::string color_space_string = "sRGB";
	colorSpaces_t color_space = SRGB;
	textureImage_t *tex = NULL;
	imageHandler_t *ih = NULL;
	params.getParam("interpolate", intpstr);
	params.getParam("color_space", color_space_string);
	params.getParam("gamma", gamma);
	params.getParam("exposure_adjust", expadj);
	params.getParam("normalmap", normalmap);
	params.getParam("filename", name);
	
	if(!name)
	{
		Y_ERROR << "ImageTexture: Required argument filename not found for image texture" << yendl;
		return NULL;
	}
	
	// interpolation type, bilinear default
	interpolationType intp = INTP_BILINEAR;
	
	if(intpstr)
	{
		if (*intpstr == "none") intp = INTP_NONE;
		else if (*intpstr == "bicubic") intp = INTP_BICUBIC;
	}
	
	size_t lDot = name->rfind(".") + 1;
	size_t lSlash = name->rfind("/") + 1;
	
	std::string ext = toLower(name->substr(lDot));
	
	std::string fmt = render.getImageFormatFromExtension(ext);
	
	if(fmt == "")
	{
		Y_ERROR << "ImageTexture: Image extension not recognized, dropping texture." << yendl;
		return NULL;
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
		return NULL;
	}
	
	if(!ih->loadFromFile(*name))
	{
		Y_ERROR << "ImageTexture: Couldn't load image file, dropping texture." << yendl;
		return NULL;
	}

	if(ih->isHDR())
	{
		if(color_space_string != "LinearRGB") Y_INFO << "ImageTexture: The image is a HDR/EXR file: forcing linear RGB and ignoring selected color space '" << color_space_string <<"' and the gamma setting." << yendl;
		color_space = LINEAR_RGB;
	}
	else
	{
		if(color_space_string == "sRGB") color_space = SRGB;
		else if(color_space_string == "XYZ") color_space = XYZ_D65;
		else if(color_space_string == "LinearRGB") color_space = LINEAR_RGB;
		else if(color_space_string == "Raw_Manual_Gamma") color_space = RAW_MANUAL_GAMMA;
		else color_space = SRGB;
	}
	
	tex = new textureImage_t(ih, intp, color_space, gamma);

	if(!tex)
	{
		Y_ERROR << "ImageTexture: Couldn't create image texture." << yendl;
		return NULL;
	}

	// setup image
	bool rot90 = false;
	bool even_tiles=false, odd_tiles=true;
	bool use_alpha=true, calc_alpha=false;
	int xrep=1, yrep=1;
	double minx=0.0, miny=0.0, maxx=1.0, maxy=1.0;
	double cdist=0.0;
	const std::string *clipmode=0;
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
	
	return tex;
}

__END_YAFRAY
