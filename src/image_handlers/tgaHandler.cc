/****************************************************************************
 *
 *      tgaHandler.cc: Truevision TGA format handler
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
 
#include <core_api/environment.h>
#include <core_api/imagehandler.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <utilities/math_utils.h>
#include <utilities/fileUtils.h>

#include "tgaUtils.h"

__BEGIN_YAFRAY

class tgaHandler_t;

typedef colorA_t (tgaHandler_t::*colorProcessor)(void *data);

class tgaHandler_t: public imageHandler_t
{
public:
	tgaHandler_t();
	void initForInput();
	~tgaHandler_t();
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name, int imgIndex = 0);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);

private:
	/*! Image data reading template functions */
	template <class ColorType> void readColorMap(FILE *fp, tgaHeader_t &header, colorProcessor cp);
	template <class ColorType> void readRLEImage(FILE *fp, colorProcessor cp);
	template <class ColorType> void readDirectImage(FILE *fp, colorProcessor cp);

	/*! colorProcesors definitions with signature colorA_t (void *)
	to be passed as pointer-to-non-static-member-functions */
	colorA_t processGray8(void *data);
	colorA_t processGray16(void *data);
	colorA_t processColor8(void *data);
	colorA_t processColor15(void *data);
	colorA_t processColor16(void *data);
	colorA_t processColor24(void *data);
	colorA_t processColor32(void *data);
	
	bool precheckFile(tgaHeader_t &header, const std::string &name, bool &isGray, bool &isRLE, bool &hasColorMap, yByte &alphaBitDepth);
	
	rgba2DImage_nw_t *ColorMap;
	size_t totPixels;
	size_t minX, maxX, stepX;
	size_t minY, maxY, stepY;

};

tgaHandler_t::tgaHandler_t()
{
	m_hasAlpha = false;
	m_MultiLayer = false;
	
	handlerName = "TGAHandler";
}

tgaHandler_t::~tgaHandler_t()
{
	clearImgBuffers();
}

bool tgaHandler_t::saveToFile(const std::string &name, int imgIndex)
{
	int h = getHeight(imgIndex);
	int w = getWidth(imgIndex);

	std::string nameWithoutTmp = name;
	nameWithoutTmp.erase(nameWithoutTmp.length()-4);
	if(session.renderInProgress()) Y_INFO << handlerName << ": Autosaving partial render (" << RoundFloatPrecision(session.currentPassPercent(), 0.01) << "% of pass " << session.currentPass() << " of " << session.totalPasses() << ") " << ((m_hasAlpha) ? "RGBA" : "RGB" ) << " file as \"" << nameWithoutTmp << "\"...  " << getDenoiseParams() << yendl;
	else Y_INFO << handlerName << ": Saving " << ((m_hasAlpha) ? "RGBA" : "RGB" ) << " file as \"" << nameWithoutTmp << "\"...  " << getDenoiseParams() << yendl;

	std::string imageId = "Image rendered with YafaRay";
	tgaHeader_t header;
	tgaFooter_t footer;

	header.idLength = imageId.size();
	header.imageType = uncTrueColor;
	header.width = w;
	header.height = h;
	header.bitDepth = ((m_hasAlpha) ? 32 : 24 );
	header.desc = TL | ((m_hasAlpha) ? alpha8 : noAlpha );
		
	FILE * fp = fileUnicodeOpen(name, "wb");

	if (fp == nullptr)
		return false;
	else 
	{
		fwrite(&header, sizeof(tgaHeader_t), 1, fp);
		fwrite(imageId.c_str(), (size_t)header.idLength, 1, fp);

//The denoise functionality will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV

		if(m_Denoise)
		{
			cv::Mat A(h, w, CV_8UC3);
			cv::Mat B(h, w, CV_8UC3);
			cv::Mat_<cv::Vec3b> _A = A;
			cv::Mat_<cv::Vec3b> _B = B;

			for (int y = 0; y < h; y++) 
			{
				for (int x = 0; x < w; x++) 
				{
				colorA_t col = imgBufferRaw.at(imgIndex)->getColor(x, y);
				col.clampRGBA01();
				_A(y, x)[0] = (col.getR() * 255);
				_A(y, x)[1] = (col.getG() * 255);
				_A(y, x)[2] = (col.getB() * 255);
				}
			}

			cv::fastNlMeansDenoisingColored(A, B, m_DenoiseHLum, m_DenoiseHCol, 7, 21);

			for (int y = 0; y < h; y++) 
			{
				for (int x = 0; x < w; x++) 
				{
					if(!m_hasAlpha)
					{
						tgaPixelRGB_t rgb;
						rgb.R = (yByte) (m_DenoiseMix * _B(y, x)[0] + (1.f-m_DenoiseMix) * _A(y, x)[0]);
						rgb.G = (yByte) (m_DenoiseMix * _B(y, x)[1] + (1.f-m_DenoiseMix) * _A(y, x)[1]);
						rgb.B = (yByte) (m_DenoiseMix * _B(y, x)[2] + (1.f-m_DenoiseMix) * _A(y, x)[2]);
						fwrite(&rgb, sizeof(tgaPixelRGB_t), 1, fp);
					}
					else
					{
						colorA_t col = imgBufferRaw.at(imgIndex)->getColor(x, y);
						tgaPixelRGBA_t rgba;
						rgba.R = (yByte) (m_DenoiseMix * _B(y, x)[0] + (1.f-m_DenoiseMix) * _A(y, x)[0]);
						rgba.G = (yByte) (m_DenoiseMix * _B(y, x)[1] + (1.f-m_DenoiseMix) * _A(y, x)[1]);
						rgba.B = (yByte) (m_DenoiseMix * _B(y, x)[2] + (1.f-m_DenoiseMix) * _A(y, x)[2]);
						rgba.A = imgBufferRaw.at(imgIndex)->getColor(x, y).A;
						fwrite(&rgba, sizeof(tgaPixelRGBA_t), 1, fp);
					}
				}
			}
		}
		else
#endif	//If YafaRay is not built with OpenCV, just do normal image processing and skip the denoise process
		{
			for (int y = 0; y < h; y++) 
			{
				for (int x = 0; x < w; x++) 
				{
					colorA_t col = imgBufferRaw.at(imgIndex)->getColor(x, y);
					col.clampRGBA01();
					
					if(!m_hasAlpha)
					{
						tgaPixelRGB_t rgb;
						rgb = (color_t) col;
						fwrite(&rgb, sizeof(tgaPixelRGB_t), 1, fp);
					}
					else
					{
						tgaPixelRGBA_t rgba;
						rgba = col;
						fwrite(&rgba, sizeof(tgaPixelRGBA_t), 1, fp);
					}
				}
			}
		}
		fwrite(&footer, sizeof(tgaFooter_t), 1, fp);
		fileUnicodeClose(fp);
	}
	
	Y_VERBOSE << handlerName << ": Done." << yendl;
	
	return true;
}

template <class ColorType> void tgaHandler_t::readColorMap(FILE *fp, tgaHeader_t &header, colorProcessor cp)
{
	ColorType *color = new ColorType[header.cmNumberOfEntries];
	
	fread(color, sizeof(ColorType), header.cmNumberOfEntries, fp);
	
	for(int x = 0; x < (int)header.cmNumberOfEntries; x++)
	{
		(*ColorMap)(x, 0) = (this->*cp)(&color[x]);
	}
	
	delete [] color;
}

template <class ColorType> void tgaHandler_t::readRLEImage(FILE *fp, colorProcessor cp)
{
	size_t y = minY;
	size_t x = minX;

	while(!feof(fp) && y != maxY)
	{
		yByte packDesc = 0;
		fread(&packDesc, sizeof(yByte), 1, fp);
		
		bool rlePack = (packDesc & rlePackMask);
		int rleRep = (int)(packDesc & rleRepMask) + 1;

		ColorType color;
		
		if(rlePack) fread(&color, sizeof(ColorType), 1, fp);
		
		for(int i = 0; i < rleRep; i++)
		{
			if(!rlePack)  fread(&color, sizeof(ColorType), 1, fp);

			imgBufferRaw.at(0)->setColor(x, y, (this->*cp)(&color));

			x += stepX;

			if(x == maxX)
			{
				x = minX;
				y += stepY;
			}
		}
	}
}

template <class ColorType> void tgaHandler_t::readDirectImage(FILE *fp, colorProcessor cp)
{
	ColorType *color = new ColorType[totPixels];

	fread(color, sizeof(ColorType), totPixels, fp);
	
	size_t i = 0;
	
	for(size_t y = minY; y != maxY; y += stepY)
	{
		for(size_t x = minX; x != maxX; x += stepX)
		{
			imgBufferRaw.at(0)->setColor(x, y, (this->*cp)(&color[i]));
			i++;
		}
	}
	
	delete [] color;
}

colorA_t tgaHandler_t::processGray8(void *data)
{
	return colorA_t(*(yByte *)data * inv255);
}

colorA_t tgaHandler_t::processGray16(void *data)
{
	yWord color = *(yWord *)data;
	return colorA_t( color_t((color & grayMask8Bit) * inv255),
				   ((color & alphaGrayMask8Bit) >> 8) * inv255);
}

colorA_t tgaHandler_t::processColor8(void *data)
{
	yByte color = *(yByte *)data;
	return (*ColorMap)(color, 0);
}

colorA_t tgaHandler_t::processColor15(void *data)
{
	yWord color = *(yWord *)data;
	return colorA_t(((color & RedMask  ) >> 11) * inv31,
					((color & GreenMask) >>  6) * inv31,
					((color & BlueMask ) >>  1) * inv31,
					1.f);
}

colorA_t tgaHandler_t::processColor16(void *data)
{
	yWord color = *(yWord *)data;
	return colorA_t(((color & RedMask  ) >> 11) * inv31,
					((color & GreenMask) >>  6) * inv31,
					((color & BlueMask ) >>  1) * inv31,
					(m_hasAlpha) ? (float)(color & AlphaMask) : 1.f);
}

colorA_t tgaHandler_t::processColor24(void *data)
{
	tgaPixelRGB_t *color = (tgaPixelRGB_t *)data;
	return colorA_t(color->R * inv255,
					color->G * inv255,
					color->B * inv255,
					1.f);
}

colorA_t tgaHandler_t::processColor32(void *data)
{
	tgaPixelRGBA_t *color = (tgaPixelRGBA_t *)data;
	return colorA_t(color->R * inv255,
					color->G * inv255,
					color->B * inv255,
					color->A * inv255);
}

bool tgaHandler_t::precheckFile(tgaHeader_t &header, const std::string &name, bool &isGray, bool &isRLE, bool &hasColorMap, yByte &alphaBitDepth)
{
	switch(header.imageType)
	{
		case noData:
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" has no image data!" << yendl;
			return false;
			break;
			
		case uncColorMap:
			if(!header.ColorMapType)
			{
				Y_ERROR << handlerName << ": TGA file \"" << name << "\" has ColorMap type and no color map embedded!" << yendl;
				return false;
			}
			hasColorMap = true;
			break;
			
		case uncGray:
			isGray = true;
			break;
			
		case rleColorMap:
			if(!header.ColorMapType)
			{
				Y_ERROR << handlerName << ": TGA file \"" << name << "\" has ColorMap type and no color map embedded!" << yendl;
				return false;
			}
			hasColorMap = true;
			isRLE = true;
			break;

		case rleGray:
			isGray = true;
			isRLE = true;
			break;
		
		case rleTrueColor:
			isRLE = true;
			break;
		
		case uncTrueColor:
			break;
	}
	
	if(hasColorMap)
	{
		if(header.cmEntryBitDepth != 15 && header.cmEntryBitDepth != 16 && header.cmEntryBitDepth != 24 && header.cmEntryBitDepth != 32)
		{
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" has a ColorMap bit depth not supported! (BitDepth:" << (int)header.cmEntryBitDepth << ")" << yendl;
			return false;
		}
	}
	
	if(isGray)
	{
		if (header.bitDepth != 8 && header.bitDepth != 16)
		{
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" has an invalid bit depth only 8 bit depth gray images are supported" << yendl;
			return false;
		}
		if(alphaBitDepth != 8 && header.bitDepth == 16)
		{
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" an invalid alpha bit depth for 16 bit gray image" << yendl;
			return false;
		}
	}
	else if(hasColorMap)
	{
		if(header.bitDepth > 16)
		{
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" has an invalid bit depth only 8 and 16 bit depth indexed images are supported" << yendl;
			return false;
		}
	}
	else
	{
		if(header.bitDepth != 15 && header.bitDepth != 16 && header.bitDepth != 24 && header.bitDepth != 32)
		{
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" has an invalid bit depth only 15/16, 24 and 32 bit depth true color images are supported (BitDepth: " << (int)header.bitDepth << ")" << yendl;
			return false;
		}
		if(alphaBitDepth != 1 && header.bitDepth == 16)
		{
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" an invalid alpha bit depth for 16 bit color image" << yendl;
			return false;
		}
		if(alphaBitDepth != 8 && header.bitDepth == 32)
		{
			Y_ERROR << handlerName << ": TGA file \"" << name << "\" an invalid alpha bit depth for 32 bit color image" << yendl;
			return false;
		}
	}
	
	return true;
}

bool tgaHandler_t::loadFromFile(const std::string &name)
{
	FILE *fp = fileUnicodeOpen(name, "rb");

	Y_INFO << handlerName << ": Loading image \"" << name << "\"..." << yendl;
	
	if(!fp)
	{
		Y_ERROR << handlerName << ": Cannot open file " << name << yendl;
		return false;
	}
	
	tgaHeader_t header;
	
	fread(&header, 1, sizeof(tgaHeader_t), fp);

	// Prereading checks

	yByte alphaBitDepth = (yByte)(header.desc & alphaBitDepthMask);
	
	m_width = header.width;
	m_height = header.height;
	m_hasAlpha = (alphaBitDepth != 0 || header.cmEntryBitDepth == 32);

	bool isRLE = false;
	bool hasColorMap = false;
	bool isGray = false;
	bool fromTop = ((header.desc & TopMask) >> 5);
	bool fromLeft = ((header.desc & LeftMask) >> 4);
	
	if(!precheckFile(header, name, isGray, isRLE, hasColorMap, alphaBitDepth))
	{
		fileUnicodeClose(fp);
		return false;
	}

	// Jump over any image Id
	fseek(fp, header.idLength, SEEK_CUR);
	
	clearImgBuffers();

	int nChannels = 3;
	if(header.cmEntryBitDepth == 16 || header.cmEntryBitDepth == 32 || header.bitDepth == 16 || header.bitDepth == 32) nChannels = 4;
	if(m_grayscale) nChannels = 1;

	imgBufferRaw.push_back(new imageBuffer_t(m_width, m_height, nChannels, getTextureOptimization()));
	
	ColorMap = nullptr;
	
	// Read the colormap if needed
	if(hasColorMap)
	{
		ColorMap = new rgba2DImage_nw_t(header.cmNumberOfEntries, 1);
		
		switch(header.cmEntryBitDepth)
		{
			case 15:
				readColorMap<yWord>(fp, header, &tgaHandler_t::processColor15);
				break;
				
			case 16:
				readColorMap<yWord>(fp, header, &tgaHandler_t::processColor16);
				break;
				
			case 24:
				readColorMap<tgaPixelRGB_t>(fp, header, &tgaHandler_t::processColor24);
				break;
			
			case 32:
				readColorMap<tgaPixelRGBA_t>(fp, header, &tgaHandler_t::processColor32);
				break;
		}
	}
	
	totPixels = m_width * m_height;
	
	// Set the reading order to fit yafaray's image coordinates
	
	minX = 0;
	maxX = m_width;
	stepX = 1;

	minY = 0;
	maxY = m_height;
	stepY = 1;
	
	if(!fromTop)
	{
		minY = m_height - 1;
		maxY = -1;
		stepY = -1;
	}
	
	if(fromLeft)
	{
		minX = m_width - 1;
		maxX = -1;
		stepX = -1;
	}
	
	// Read the image data
	
	if(isRLE) // RLE compressed image data
	{	
		switch(header.bitDepth)
		{
			case 8: // Indexed color using ColorMap LUT or grayscale map
				if (isGray)readRLEImage<yByte>(fp, &tgaHandler_t::processGray8);
				else readRLEImage<yByte>(fp, &tgaHandler_t::processColor8);
				break;
			
			case 15:
				readRLEImage<yWord>(fp, &tgaHandler_t::processColor15);
				break;
				
			case 16:
				if(isGray) readRLEImage<yWord>(fp, &tgaHandler_t::processGray16);
				else readRLEImage<yWord>(fp, &tgaHandler_t::processColor16);
				break;
				
			case 24:
				readRLEImage<tgaPixelRGB_t>(fp, &tgaHandler_t::processColor24);
				break;
				
			case 32:
				readRLEImage<tgaPixelRGBA_t>(fp, &tgaHandler_t::processColor32);
				break;
		}
	}
	else // Direct color data (uncompressed)
	{
		switch(header.bitDepth)
		{
			case 8: // Indexed color using ColorMap LUT or grayscale map
				if(isGray) readDirectImage<yByte>(fp, &tgaHandler_t::processGray8);
				else readDirectImage<yByte>(fp, &tgaHandler_t::processColor8);
				break;
			
			case 15:
				readDirectImage<yWord>(fp, &tgaHandler_t::processColor15);
				break;
				
			case 16:
				if(isGray) readDirectImage<yWord>(fp, &tgaHandler_t::processGray16);
				else readDirectImage<yWord>(fp, &tgaHandler_t::processColor16);
				break;
				
			case 24:
				readDirectImage<tgaPixelRGB_t>(fp, &tgaHandler_t::processColor24);
				break;
				
			case 32:
				readDirectImage<tgaPixelRGBA_t>(fp, &tgaHandler_t::processColor32);
				break;
		}
	}
	
	fileUnicodeClose(fp);
	fp = nullptr;
	
	if (ColorMap) delete ColorMap;
	ColorMap = nullptr;

	Y_VERBOSE << handlerName << ": Done." << yendl;

	return true;
}

imageHandler_t *tgaHandler_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	int width = 0;
	int height = 0;
	bool withAlpha = false;
	bool forOutput = true;
	bool img_grayscale = false;
	bool denoiseEnabled = false;
	int denoiseHLum = 3;
	int denoiseHCol = 3;
	float denoiseMix = 0.8f;

	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", withAlpha);
	params.getParam("for_output", forOutput);
	params.getParam("denoiseEnabled", denoiseEnabled);
	params.getParam("denoiseHLum", denoiseHLum);
	params.getParam("denoiseHCol", denoiseHCol);
	params.getParam("denoiseMix", denoiseMix);
	params.getParam("img_grayscale", img_grayscale);

	imageHandler_t *ih = new tgaHandler_t();
	
	if(forOutput)
	{
		if(yafLog.getUseParamsBadge()) height += yafLog.getBadgeHeight();
		ih->initForOutput(width, height, render.getRenderPasses(), denoiseEnabled, denoiseHLum, denoiseHCol, denoiseMix, withAlpha, false, img_grayscale);
	}
	
	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerImageHandler("tga", "tga tpic", "TGA [Truevision TARGA]", tgaHandler_t::factory);
	}

}
__END_YAFRAY
