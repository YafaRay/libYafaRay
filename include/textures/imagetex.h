#pragma once
/****************************************************************************
 *
 *		imagetex.h: Image specific functions
 *      This is part of the yafray package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef Y_IMAGETEX_H
#define Y_IMAGETEX_H

#include <core_api/texture.h>
#include <core_api/environment.h>
#include <utilities/interpolation.h>


__BEGIN_YAFRAY

#define EWA_WEIGHT_LUT_SIZE 128

enum TEX_CLIPMODE
{
	TCL_EXTEND,
	TCL_CLIP,
	TCL_CLIPCUBE,
	TCL_REPEAT,
	TCL_CHECKER
};

class textureImage_t : public texture_t
{
	public:
		textureImage_t(imageHandler_t *ih, int intp, float gamma, colorSpaces_t color_space = RAW_MANUAL_GAMMA);
		virtual ~textureImage_t();
		virtual bool discrete() const { return true; }
		virtual bool isThreeD() const { return false; }
		virtual bool isNormalmap() const { return normalmap; }
		virtual colorA_t getColor(const point3d_t &p, mipMapParams_t *mmParams = nullptr) const;
		virtual colorA_t getColor(int x, int y, int z, mipMapParams_t *mmParams = nullptr) const;
		virtual colorA_t getRawColor(const point3d_t &p, mipMapParams_t *mmParams = nullptr) const;
		virtual colorA_t getRawColor(int x, int y, int z, mipMapParams_t *mmParams = nullptr) const;
		virtual void resolution(int &x, int &y, int &z) const;
		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);
		virtual void generateMipMaps() { if(image->getHighestImgIndex() == 0) image->generateMipMaps(); }

	protected:
		void setCrop(float minx, float miny, float maxx, float maxy);
		void findTextureInterpolationCoordinates(int &coord, int &coord0, int &coord2, int &coord3, float &coord_decimal_part, float coord_float, int resolution, bool repeat, bool mirror) const;
		colorA_t noInterpolation(const point3d_t &p, int mipmaplevel = 0) const;
		colorA_t bilinearInterpolation(const point3d_t &p, int mipmaplevel = 0) const;
		colorA_t bicubicInterpolation(const point3d_t &p, int mipmaplevel = 0) const;
		colorA_t mipMapsTrilinearInterpolation(const point3d_t &p, mipMapParams_t *mmParams) const;
		colorA_t mipMapsEWAInterpolation(const point3d_t &p, float maxAnisotropy, mipMapParams_t *mmParams) const;
		colorA_t EWAEllipticCalculation(const point3d_t &p, float dS0, float dT0, float dS1, float dT1, int mipmaplevel = 0) const;
		void generateEWALookupTable();
		bool doMapping(point3d_t &texp) const;
		colorA_t interpolateImage(const point3d_t &p, mipMapParams_t *mmParams) const;

		bool use_alpha, calc_alpha, normalmap;
		bool grayscale = false;	//!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
		bool cropx, cropy, checker_odd, checker_even, rot90;
		float cropminx, cropmaxx, cropminy, cropmaxy;
		float checker_dist;
		int xrepeat, yrepeat;
		int tex_clipmode;
		imageHandler_t *image;
		colorSpaces_t colorSpace;
		float gamma;
		bool mirrorX;
		bool mirrorY;
		float trilinear_level_bias = 0.f; //!< manually specified delta to be added/subtracted from the calculated mipmap level. Negative values will choose higher resolution mipmaps than calculated, reducing the blurry artifacts at the cost of increasing texture noise. Positive values will choose lower resolution mipmaps than calculated. Default (and recommended) is 0.0 to use the calculated mipmaps as-is.
		float ewa_max_anisotropy = 8.f; //!< Maximum anisotropy allowed for mipmap EWA algorithm. Higher values give better quality in textures seen from an angle, but render will be slower. Lower values will give more speed but lower quality in textures seen in an angle.
		static float *ewaWeightLut;
};


__END_YAFRAY

#endif // Y_IMAGETEX_H
