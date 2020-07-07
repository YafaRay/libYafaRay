/****************************************************************************
 *
 * 		imagehandler.h: image load and save abstract class
 *      This is part of the yafray package
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
 
#ifndef Y_IMAGE_HANDLER_H
#define Y_IMAGE_HANDLER_H

#include "yafray_constants.h"
#include <utilities/image_buffers.h>


__BEGIN_YAFRAY

typedef unsigned char yByte;
typedef unsigned short yWord;
class renderPasses_t;

enum textureOptimization_t
{
	TEX_OPTIMIZATION_NONE				= 1,
	TEX_OPTIMIZATION_OPTIMIZED			= 2,
	TEX_OPTIMIZATION_COMPRESSED			= 3
};

enum interpolationType_t
{
	INTP_NONE,
	INTP_BILINEAR,
	INTP_BICUBIC,
	INTP_MIPMAP_TRILINEAR,
	INTP_MIPMAP_EWA
};

class mipMapParams_t
{
	public:
		mipMapParams_t();
		mipMapParams_t(float force_image_level):forceImageLevel(force_image_level) {}
		mipMapParams_t(float dsdx, float dtdx, float dsdy, float dtdy):dSdx(dsdx),dTdx(dtdx),dSdy(dsdy),dTdy(dtdy) {}
		
		float forceImageLevel = 0.f;
		float dSdx = 0.f;
		float dTdx = 0.f;
		float dSdy = 0.f;
		float dTdy = 0.f;
};

class YAFRAYCORE_EXPORT imageBuffer_t
{
public:
	imageBuffer_t(int width, int height, int num_channels, int optimization);
	~imageBuffer_t();
	
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	int getNumChannels() const { return m_num_channels; }
	imageBuffer_t getDenoisedLDRBuffer(int h_lum, int h_col, float mix) const; //!< Provides a denoised buffer, but only works with LDR images (that can be represented in 8-bit 0..255 values). If attempted with HDR images they would lose the HDR range and become unusable!

	colorA_t getColor(int x, int y) const;
	void setColor(int x, int y, const colorA_t & col);
	void setColor(int x, int y, const colorA_t & col, colorSpaces_t color_space, float gamma);	// Set color after linearizing it from color space 

protected:
	int m_width;
	int m_height;
	int m_num_channels;
	int m_optimization;
	rgba2DImage_nw_t * rgba128_FloatImg = nullptr; //!< rgba standard float RGBA color buffer (for textures and mipmaps) or render passes depending on whether the image handler is used for input or output)
	rgbaOptimizedImage_nw_t * rgba40_OptimizedImg = nullptr;	//!< optimized RGBA (32bit/pixel) with alpha buffer (for textures and mipmaps)
	rgbaCompressedImage_nw_t * rgba24_CompressedImg = nullptr;	//!< compressed RGBA (24bit/pixel) LOSSY! with alpha buffer (for textures and mipmaps)
	rgb2DImage_nw_t * rgb96_FloatImg = nullptr; //!< rgb standard float RGB color buffer (for textures and mipmaps) or render passes depending on whether the image handler is used for input or output)
	rgbOptimizedImage_nw_t * rgb32_OptimizedImg = nullptr;	//!< optimized RGB (24bit/pixel) without alpha buffer (for textures and mipmaps)
	rgbCompressedImage_nw_t * rgb16_CompressedImg = nullptr;	//!< compressed RGB (16bit/pixel) LOSSY! without alpha buffer (for textures and mipmaps)
	gray2DImage_nw_t * gray32_FloatImg = nullptr;	//!< grayscale float buffer (32bit/pixel) (for textures and mipmaps)
	grayOptimizedImage_nw_t * gray8_OptimizedImg = nullptr;	//!< grayscale optimized buffer (8bit/pixel) (for textures and mipmaps)
};


class YAFRAYCORE_EXPORT imageHandler_t
{
public:	
	virtual ~imageHandler_t() {}
	virtual bool loadFromFile(const std::string &name) = 0;
	virtual bool loadFromMemory(const yByte *data, size_t size) {return false; }
	virtual bool saveToFile(const std::string &name, int imgIndex = 0) = 0;
	virtual bool saveToFileMultiChannel(const std::string &name, const renderPasses_t *renderPasses) { return false; };
	virtual bool isHDR() { return false; }
	virtual bool isMultiLayer() { return m_MultiLayer; }
	virtual bool denoiseEnabled() { return m_Denoise; }
	int getTextureOptimization() { return m_textureOptimization; }
	void setTextureOptimization(int texture_optimization) { m_textureOptimization = texture_optimization; }
	void setGrayScaleSetting(bool grayscale) { m_grayscale = grayscale; }
	int getWidth(int imgIndex = 0) { return imgBuffer.at(imgIndex)->getWidth(); }
	int getHeight(int imgIndex = 0) { return imgBuffer.at(imgIndex)->getHeight(); }
	std::string getDenoiseParams() const;
	void generateMipMaps();
	int getHighestImgIndex() const { return (int) imgBuffer.size() - 1; }
	void setColorSpace(colorSpaces_t color_space, float gamma) { m_colorSpace = color_space; m_gamma = gamma; }
	void putPixel(int x, int y, const colorA_t &rgba, int imgIndex = 0);
	colorA_t getPixel(int x, int y, int imgIndex = 0);
	void initForOutput(int width, int height, const renderPasses_t *renderPasses, bool denoiseEnabled, int denoiseHLum, int denoiseHCol, float denoiseMix, bool withAlpha = false, bool multi_layer = false, bool grayscale = false);
	void clearImgBuffers();
	
protected:
	std::string handlerName;
	int m_width = 0;
	int m_height = 0;
	bool m_hasAlpha = false;
	bool m_grayscale = false;	//!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
	int m_textureOptimization = TEX_OPTIMIZATION_OPTIMIZED;
	colorSpaces_t m_colorSpace = RAW_MANUAL_GAMMA;
	float m_gamma = 1.f;
	std::vector<imageBuffer_t *> imgBuffer;
	bool m_MultiLayer = false;
	bool m_Denoise = false;
	int m_DenoiseHLum = 3;
	int m_DenoiseHCol = 3;
	float m_DenoiseMix = 0.8f;	//!< Mix factor between the de-noised image and the original "noisy" image to avoid banding artifacts in images with all noise removed.
};


inline colorA_t imageBuffer_t::getColor(int x, int y) const
{
	if(m_num_channels == 4)
	{
		if(rgba40_OptimizedImg) return (*rgba40_OptimizedImg)(x, y).getColor();
		else if(rgba24_CompressedImg) return (*rgba24_CompressedImg)(x, y).getColor();
		else if(rgba128_FloatImg) return (*rgba128_FloatImg)(x, y);
		else return colorA_t(0.f);
	}
	
	else if(m_num_channels == 3)
	{
		if(rgb32_OptimizedImg) return (*rgb32_OptimizedImg)(x, y).getColor();
		else if(rgb16_CompressedImg) return (*rgb16_CompressedImg)(x, y).getColor();
		else if(rgb96_FloatImg) return (*rgb96_FloatImg)(x, y);
		else return colorA_t(0.f);
	}

	else if(m_num_channels == 1)
	{
		if(gray8_OptimizedImg) return (*gray8_OptimizedImg)(x, y).getColor();
		else if(gray32_FloatImg) return colorA_t((*gray32_FloatImg)(x, y), 1.f);
		else return colorA_t(0.f);
	}
	
	else return colorA_t(0.f);
}


inline void imageBuffer_t::setColor(int x, int y, const colorA_t & col)
{
	if(m_num_channels == 4)
	{
		if(rgba40_OptimizedImg) (*rgba40_OptimizedImg)(x, y).setColor(col);
		else if(rgba24_CompressedImg) (*rgba24_CompressedImg)(x, y).setColor(col);
		else if(rgba128_FloatImg) (*rgba128_FloatImg)(x, y) = col;
	}
	
	else if(m_num_channels == 3)
	{
		if(rgb32_OptimizedImg) (*rgb32_OptimizedImg)(x, y).setColor(col);
		else if(rgb16_CompressedImg) (*rgb16_CompressedImg)(x, y).setColor(col);
		else if(rgb96_FloatImg) (*rgb96_FloatImg)(x, y) = col;
	}

	else if(m_num_channels == 1)
	{
		if(gray8_OptimizedImg) (*gray8_OptimizedImg)(x, y).setColor(col);
		else if(gray32_FloatImg)
		{
			float fGrayAvg = (col.R + col.G + col.B) / 3.f;
			(*gray32_FloatImg)(x, y) = fGrayAvg;
		}
	}
}

inline void imageBuffer_t::setColor(int x, int y, const colorA_t & col, colorSpaces_t color_space, float gamma)
{
	if(color_space == LINEAR_RGB || (color_space == RAW_MANUAL_GAMMA && gamma == 1.f))
	{
		setColor(x, y, col);
	}
	else
	{
		colorA_t colLinear = col;
		colLinear.linearRGB_from_ColorSpace(color_space, gamma);
		setColor(x, y, colLinear);
	}
}

__END_YAFRAY

#endif
