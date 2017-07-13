/****************************************************************************
 *      imagehandler.cc: common code for all imagehandlers
 *      This is part of the yafaray package
 *      Copyright (C) 2016  David Bluecame
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

#include <core_api/imagehandler.h>


__BEGIN_YAFRAY


imageBuffer_t::imageBuffer_t(int width, int height, int num_channels, int optimization):m_width(width),m_height(height),m_num_channels(num_channels)
{
	switch(optimization)
	{
		case TEX_OPTIMIZATION_NONE:
			if(m_num_channels == 4) rgba128_FloatImg = new rgba2DImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb96_FloatImg = new rgb2DImage_nw_t(width, height);
			else if(m_num_channels == 1) gray32_FloatImg = new gray2DImage_nw_t(width, height);
			break;
			
		case TEX_OPTIMIZATION_OPTIMIZED:
			if(m_num_channels == 4) rgba40_OptimizedImg = new rgbaOptimizedImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb32_OptimizedImg = new rgbOptimizedImage_nw_t(width, height);
			else if(m_num_channels == 1) gray8_OptimizedImg = new grayOptimizedImage_nw_t(width, height);
			break;
			
		case TEX_OPTIMIZATION_COMPRESSED:
			if(m_num_channels == 4) rgba24_CompressedImg = new rgbaCompressedImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb16_CompressedImg = new rgbCompressedImage_nw_t(width, height);
			else if(m_num_channels == 1) gray8_OptimizedImg = new grayOptimizedImage_nw_t(width, height);
			break;
			
#ifdef HAVE_OPENEXR
		case TEX_OPTIMIZATION_HALF_FLOAT:
			if(m_num_channels == 4) rgba64_HalfFloatImg = new rgbaHalfImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb48_HalfFloatImg = new rgbHalfImage_nw_t(width, height);
			else if(m_num_channels == 1) gray16_HalfFloatImg = new grayHalfImage_nw_t(width, height);
			break;
#endif
		default: break;
	}
}

imageBuffer_t::~imageBuffer_t()
{
	if(rgba40_OptimizedImg) { delete rgba40_OptimizedImg; rgba40_OptimizedImg = nullptr; }
	if(rgba24_CompressedImg) { delete rgba24_CompressedImg; rgba24_CompressedImg = nullptr; }
	if(rgba128_FloatImg) { delete rgba128_FloatImg; rgba128_FloatImg = nullptr; }
	if(rgb32_OptimizedImg) { delete rgb32_OptimizedImg; rgb32_OptimizedImg = nullptr; }
	if(rgb16_CompressedImg) { delete rgb16_CompressedImg; rgb16_CompressedImg = nullptr; }
	if(rgb96_FloatImg) { delete rgb96_FloatImg; rgb96_FloatImg = nullptr; }
	if(gray32_FloatImg) { delete gray32_FloatImg; gray32_FloatImg = nullptr; }
	if(gray8_OptimizedImg) { delete gray8_OptimizedImg; gray8_OptimizedImg = nullptr; }
	
#ifdef HAVE_OPENEXR
	if(rgba64_HalfFloatImg) { delete rgba64_HalfFloatImg; rgba64_HalfFloatImg = nullptr; }
	if(rgb48_HalfFloatImg) { delete rgb48_HalfFloatImg; rgb48_HalfFloatImg = nullptr; }
	if(gray16_HalfFloatImg) { delete gray16_HalfFloatImg; gray16_HalfFloatImg = nullptr; }
#endif
}


std::string imageHandler_t::getDenoiseParams() const
{
#ifdef HAVE_OPENCV	//Denoise only works if YafaRay is built with OpenCV support
	if(!m_Denoise) return "";
	std::stringstream paramString;
	paramString << "| Image file denoise enabled [mix=" << m_DenoiseMix << ", h(Luminance)=" << m_DenoiseHLum << ", h(Chrominance)=" <<  m_DenoiseHCol << "]" << yendl;
	return paramString.str();
#else
	return "";
#endif
}


void imageHandler_t::generateMipMaps()
{
	if(imgBuffer.empty()) return;

#ifdef HAVE_OPENCV	
	int imgIndex = 0;
	//bool blur_seamless = true;
	int w = m_width, h = m_height;

	Y_VERBOSE << "ImageHandler: generating mipmaps for texture of resolution [" << w << " x " << h << "]" << yendl;
	
	cv::Mat A(h, w, CV_32FC4);
	cv::Mat_<cv::Vec4f> _A = A;
	
	for(int j = 0; j < h; ++j)
	{
		for(int i = 0; i < w; ++i)
		{
			colorA_t color = imgBuffer.at(imgIndex)->getColor(i, j);

			_A(j, i)[0] = color.getR();
			_A(j, i)[1] = color.getG();
			_A(j, i)[2] = color.getB();
			_A(j, i)[3] = color.getA();
		}
	}
	
	//Mipmap generation using the temporary full float buffer to reduce information loss
	while(w > 1 || h > 1)
	{
		int w2 = (w + 1) / 2;
		int h2 = (h + 1) / 2;
		++imgIndex;
		imgBuffer.push_back(new imageBuffer_t(w2, h2, imgBuffer.at(imgIndex-1)->getNumChannels(), getTextureOptimization()));
		
		cv::Mat B(h2, w2, CV_32FC4);
		cv::Mat_<cv::Vec4f> _B = B;
		cv::resize(A, B, cv::Size(w2, h2), 0, 0, cv::INTER_AREA);
		//A = B;

		for(int j = 0; j < h2; ++j)
		{
			for(int i = 0; i < w2; ++i)
			{
				colorA_t tmpCol(0.f);
				tmpCol.R = _B(j, i)[0];
				tmpCol.G = _B(j, i)[1];
				tmpCol.B = _B(j, i)[2];
				tmpCol.A = _B(j, i)[3];

				imgBuffer.at(imgIndex)->setColor(i, j, tmpCol);
			}
		}

		w = w2;
		h = h2;
		Y_DEBUG << "ImageHandler: generated mipmap " << imgIndex << " [" << w2 << " x " << h2 << "]" << yendl;
	}
	
	Y_VERBOSE << "ImageHandler: mipmap generation done: " << imgIndex << " mipmaps generated." << yendl;
#else
	Y_WARNING << "ImageHandler: cannot generate mipmaps, YafaRay was not built with OpenCV support which is needed for mipmap processing." << yendl;
#endif
}


void imageHandler_t::putPixel(int x, int y, const colorA_t &rgba, int imgIndex)
{
	imgBuffer.at(imgIndex)->setColor(x, y, rgba);
}

colorA_t imageHandler_t::getPixel(int x, int y, int imgIndex)
{
	return imgBuffer.at(imgIndex)->getColor(x, y);
}

void imageHandler_t::initForOutput(int width, int height, const renderPasses_t *renderPasses, bool denoiseEnabled, int denoiseHLum, int denoiseHCol, float denoiseMix, bool withAlpha, bool multi_layer, bool grayscale)
{
	m_hasAlpha = withAlpha;
	m_MultiLayer = multi_layer;
	m_Denoise = denoiseEnabled;
	m_DenoiseHLum = denoiseHLum;
	m_DenoiseHCol = denoiseHCol;
	m_DenoiseMix = denoiseMix;
	m_grayscale = grayscale;
    
	int nChannels = 3;
	if(m_grayscale) nChannels = 1;
	else if(m_hasAlpha) nChannels = 4;

	for(int idx = 0; idx < renderPasses->extPassesSize(); ++idx)
	{
		imgBuffer.push_back(new imageBuffer_t(width, height, nChannels, TEX_OPTIMIZATION_NONE));
	}
}

void imageHandler_t::clearImgBuffers()
{
	if(!imgBuffer.empty())
	{
		for(size_t idx = 0; idx < imgBuffer.size(); ++idx)
		{
			delete imgBuffer.at(idx);
			imgBuffer.at(idx) = nullptr;
		}
	}
}

__END_YAFRAY
