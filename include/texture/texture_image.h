#pragma once
/****************************************************************************
 *
 *      imagetex.h: Image specific functions
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_TEXTURE_IMAGE_H
#define YAFARAY_TEXTURE_IMAGE_H

#include "texture/texture.h"
#include "common/environment.h"
#include "utility/util_interpolation.h"

BEGIN_YAFARAY

class ImageTexture : public Texture
{
	public:
		enum class TexClipMode : int { Extend, Clip, ClipCube, Repeat, Checker };
		ImageTexture(ImageHandler *ih, const InterpolationType &interpolation_type, float gamma, const ColorSpace &color_space = RawManualGamma);
		virtual ~ImageTexture();
		virtual bool discrete() const { return true; }
		virtual bool isThreeD() const { return false; }
		virtual bool isNormalmap() const { return normalmap_; }
		virtual Rgba getColor(const Point3 &p, const MipMapParams *mipmap_params = nullptr) const;
		virtual Rgba getColor(int x, int y, int z, const MipMapParams *mipmap_params = nullptr) const;
		virtual Rgba getRawColor(const Point3 &p, const MipMapParams *mipmap_params = nullptr) const;
		virtual Rgba getRawColor(int x, int y, int z, const MipMapParams *mipmap_params = nullptr) const;
		virtual void resolution(int &x, int &y, int &z) const;
		static Texture *factory(ParamMap &params, RenderEnvironment &render);
		virtual void generateMipMaps() { if(image_->getHighestImgIndex() == 0) image_->generateMipMaps(); }

	protected:
		void setCrop(float minx, float miny, float maxx, float maxy);
		void findTextureInterpolationCoordinates(int &coord_0, int &coord_1, int &coord_2, int &coord_3, float &coord_decimal_part, float coord_float, int resolution, bool repeat, bool mirror) const;
		Rgba noInterpolation(const Point3 &p, int mipmaplevel = 0) const;
		Rgba bilinearInterpolation(const Point3 &p, int mipmaplevel = 0) const;
		Rgba bicubicInterpolation(const Point3 &p, int mipmaplevel = 0) const;
		Rgba mipMapsTrilinearInterpolation(const Point3 &p, const MipMapParams *mipmap_params) const;
		Rgba mipMapsEwaInterpolation(const Point3 &p, float max_anisotropy, const MipMapParams *mipmap_params) const;
		Rgba ewaEllipticCalculation(const Point3 &p, float d_s_0, float d_t_0, float d_s_1, float d_t_1, int mipmaplevel = 0) const;
		void generateEwaLookupTable();
		bool doMapping(Point3 &texp) const;
		Rgba interpolateImage(const Point3 &p, const MipMapParams *mipmap_params) const;

		const int ewa_weight_lut_size_ = 128;
		bool use_alpha_, calc_alpha_, normalmap_;
		bool grayscale_ = false;	//!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
		bool cropx_, cropy_, checker_odd_, checker_even_, rot_90_;
		float cropminx_, cropmaxx_, cropminy_, cropmaxy_;
		float checker_dist_;
		int xrepeat_, yrepeat_;
		TexClipMode tex_clip_mode_;
		ImageHandler *image_;
		ColorSpace color_space_;
		float gamma_;
		bool mirror_x_;
		bool mirror_y_;
		float trilinear_level_bias_ = 0.f; //!< manually specified delta to be added/subtracted from the calculated mipmap level. Negative values will choose higher resolution mipmaps than calculated, reducing the blurry artifacts at the cost of increasing texture noise. Positive values will choose lower resolution mipmaps than calculated. Default (and recommended) is 0.0 to use the calculated mipmaps as-is.
		float ewa_max_anisotropy_ = 8.f; //!< Maximum anisotropy allowed for mipmap EWA algorithm. Higher values give better quality in textures seen from an angle, but render will be slower. Lower values will give more speed but lower quality in textures seen in an angle.
		static float *ewa_weight_lut_;
};


END_YAFARAY

#endif // YAFARAY_TEXTURE_IMAGE_H
