#pragma once
/****************************************************************************
 *
 *      imagehandler.h: image load and save abstract class
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
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

#ifndef YAFARAY_IMAGEHANDLER_H
#define YAFARAY_IMAGEHANDLER_H

#include "yafray_constants.h"
#include <utilities/image_buffers.h>


BEGIN_YAFRAY

typedef unsigned char YByte_t;
typedef unsigned short YWord_t;
class RenderPasses;

enum class TextureOptimization : int { None	= 1, Optimized = 2, Compressed = 3 };
enum class InterpolationType : int { None, Bilinear, Bicubic, Trilinear, Ewa };

class MipMapParams
{
	public:
		MipMapParams();
		MipMapParams(float force_image_level): force_image_level_(force_image_level) {}
		MipMapParams(float dsdx, float dtdx, float dsdy, float dtdy): ds_dx_(dsdx), dt_dx_(dtdx), ds_dy_(dsdy), dt_dy_(dtdy) {}

		float force_image_level_ = 0.f;
		float ds_dx_ = 0.f;
		float dt_dx_ = 0.f;
		float ds_dy_ = 0.f;
		float dt_dy_ = 0.f;
};

class YAFRAYCORE_EXPORT ImageBuffer final
{
	public:
		ImageBuffer(int width, int height, int num_channels, const TextureOptimization &optimization);
		~ImageBuffer();

		int getWidth() const { return width_; }
		int getHeight() const { return height_; }
		int getNumChannels() const { return num_channels_; }
		ImageBuffer getDenoisedLdrBuffer(float h_col, float h_lum, float mix) const; //!< Provides a denoised buffer, but only works with LDR images (that can be represented in 8-bit 0..255 values). If attempted with HDR images they would lose the HDR range and become unusable!

		Rgba getColor(int x, int y) const;
		void setColor(int x, int y, const Rgba &col);
		void setColor(int x, int y, const Rgba &col, ColorSpace color_space, float gamma);	// Set color after linearizing it from color space

	protected:
		int width_;
		int height_;
		int num_channels_;
		TextureOptimization optimization_;
		Rgba2DImage_t *rgba_128_float_img_ = nullptr;  //!< rgba standard float RGBA color buffer (for textures and mipmaps) or render passes depending on whether the image handler is used for input or output)
		RgbaOptimizedImage_t *rgba_40_optimized_img_ = nullptr;	//!< optimized RGBA (32bit/pixel) with alpha buffer (for textures and mipmaps)
		RgbaCompressedImage_t *rgba_24_compressed_img_ = nullptr;	//!< compressed RGBA (24bit/pixel) LOSSY! with alpha buffer (for textures and mipmaps)
		Rgb2DImage_t *rgb_96_float_img_ = nullptr;  //!< rgb standard float RGB color buffer (for textures and mipmaps) or render passes depending on whether the image handler is used for input or output)
		RgbOptimizedImage_t *rgb_32_optimized_img_ = nullptr;	//!< optimized RGB (24bit/pixel) without alpha buffer (for textures and mipmaps)
		RgbCompressedImage_t *rgb_16_compressed_img_ = nullptr;	//!< compressed RGB (16bit/pixel) LOSSY! without alpha buffer (for textures and mipmaps)
		Gray2DImage_t *gray_32_float_img_ = nullptr;	//!< grayscale float buffer (32bit/pixel) (for textures and mipmaps)
		GrayOptimizedImage_t *gray_8_optimized_img_ = nullptr;	//!< grayscale optimized buffer (8bit/pixel) (for textures and mipmaps)
};


class YAFRAYCORE_EXPORT ImageHandler
{
	public:
		virtual ~ImageHandler() = default;
		virtual bool loadFromFile(const std::string &name) = 0;
		virtual bool loadFromMemory(const YByte_t *data, size_t size) {return false; }
		virtual bool saveToFile(const std::string &name, int img_index = 0) = 0;
		virtual bool saveToFileMultiChannel(const std::string &name, const RenderPasses *render_passes) { return false; };
		virtual bool isHdr() const { return false; }
		virtual bool isMultiLayer() const { return multi_layer_; }
		virtual bool denoiseEnabled() const { return denoise_; }
		TextureOptimization getTextureOptimization() const { return texture_optimization_; }
		void setTextureOptimization(const TextureOptimization &texture_optimization) { texture_optimization_ = texture_optimization; }
		void setGrayScaleSetting(bool grayscale) { grayscale_ = grayscale; }
		int getWidth(int img_index = 0) const { return img_buffer_.at(img_index)->getWidth(); }
		int getHeight(int img_index = 0) const { return img_buffer_.at(img_index)->getHeight(); }
		std::string getDenoiseParams() const;
		void generateMipMaps();
		int getHighestImgIndex() const { return (int) img_buffer_.size() - 1; }
		void setColorSpace(ColorSpace color_space, float gamma) { color_space_ = color_space; gamma_ = gamma; }
		void putPixel(int x, int y, const Rgba &rgba, int img_index = 0);
		Rgba getPixel(int x, int y, int img_index = 0);
		void initForOutput(int width, int height, const RenderPasses *render_passes, bool denoise_enabled, int denoise_h_lum, int denoise_h_col, float denoise_mix, bool with_alpha = false, bool multi_layer = false, bool grayscale = false);
		void clearImgBuffers();

	protected:
		std::string handler_name_;
		int width_ = 0;
		int height_ = 0;
		bool has_alpha_ = false;
		bool grayscale_ = false;	//!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
		TextureOptimization texture_optimization_ = TextureOptimization::Optimized;
		ColorSpace color_space_ = RawManualGamma;
		float gamma_ = 1.f;
		std::vector<ImageBuffer *> img_buffer_;
		bool multi_layer_ = false;
		bool denoise_ = false;
		int denoise_hlum_ = 3;
		int denoise_hcol_ = 3;
		float denoise_mix_ = 0.8f;	//!< Mix factor between the de-noised image and the original "noisy" image to avoid banding artifacts in images with all noise removed.
};


inline Rgba ImageBuffer::getColor(int x, int y) const
{
	if(num_channels_ == 4)
	{
		if(rgba_40_optimized_img_) return (*rgba_40_optimized_img_)(x, y).getColor();
		else if(rgba_24_compressed_img_) return (*rgba_24_compressed_img_)(x, y).getColor();
		else if(rgba_128_float_img_) return (*rgba_128_float_img_)(x, y);
		else return Rgba(0.f);
	}

	else if(num_channels_ == 3)
	{
		if(rgb_32_optimized_img_) return (*rgb_32_optimized_img_)(x, y).getColor();
		else if(rgb_16_compressed_img_) return (*rgb_16_compressed_img_)(x, y).getColor();
		else if(rgb_96_float_img_) return (*rgb_96_float_img_)(x, y);
		else return Rgba(0.f);
	}

	else if(num_channels_ == 1)
	{
		if(gray_8_optimized_img_) return (*gray_8_optimized_img_)(x, y).getColor();
		else if(gray_32_float_img_) return Rgba((*gray_32_float_img_)(x, y), 1.f);
		else return Rgba(0.f);
	}

	else return Rgba(0.f);
}


inline void ImageBuffer::setColor(int x, int y, const Rgba &col)
{
	if(num_channels_ == 4)
	{
		if(rgba_40_optimized_img_)(*rgba_40_optimized_img_)(x, y).setColor(col);
		else if(rgba_24_compressed_img_)(*rgba_24_compressed_img_)(x, y).setColor(col);
		else if(rgba_128_float_img_)(*rgba_128_float_img_)(x, y) = col;
	}

	else if(num_channels_ == 3)
	{
		if(rgb_32_optimized_img_)(*rgb_32_optimized_img_)(x, y).setColor(col);
		else if(rgb_16_compressed_img_)(*rgb_16_compressed_img_)(x, y).setColor(col);
		else if(rgb_96_float_img_)(*rgb_96_float_img_)(x, y) = col;
	}

	else if(num_channels_ == 1)
	{
		if(gray_8_optimized_img_)(*gray_8_optimized_img_)(x, y).setColor(col);
		else if(gray_32_float_img_)
		{
			float f_gray_avg = (col.r_ + col.g_ + col.b_) / 3.f;
			(*gray_32_float_img_)(x, y) = f_gray_avg;
		}
	}
}

inline void ImageBuffer::setColor(int x, int y, const Rgba &col, ColorSpace color_space, float gamma)
{
	if(color_space == LinearRgb || (color_space == RawManualGamma && gamma == 1.f))
	{
		setColor(x, y, col);
	}
	else
	{
		Rgba col_linear = col;
		col_linear.linearRgbFromColorSpace(color_space, gamma);
		setColor(x, y, col_linear);
	}
}

END_YAFRAY

#endif
