/****************************************************************************
 *
 *      tifHandler.cc: Tag Image File Format (TIFF) image handler
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

#include <core_api/imagehandler.h>
#include <core_api/logging.h>
#include <core_api/session.h>
#include <core_api/environment.h>
#include <core_api/params.h>
#include <utilities/math_utils.h>
#include <core_api/file.h>

#if defined(_WIN32)
#include <utilities/stringUtils.h>
#endif //defined(_WIN32)

namespace libtiff {
	#include <tiffio.h>
}

__BEGIN_YAFRAY

#define inv8  0.00392156862745098039 // 1 / 255
#define inv16 0.00001525902189669642 // 1 / 65535

class tifHandler_t: public imageHandler_t
{
public:
	tifHandler_t();
	~tifHandler_t();
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name, int imgIndex = 0);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);
};

tifHandler_t::tifHandler_t()
{
	m_hasAlpha = false;
	m_MultiLayer = false;
	
	handlerName = "TIFFHandler";
}

tifHandler_t::~tifHandler_t()
{
	clearImgBuffers();
}

bool tifHandler_t::saveToFile(const std::string &name, int imgIndex)
{
	int h = getHeight(imgIndex);
	int w = getWidth(imgIndex);

	std::string nameWithoutTmp = name;
	nameWithoutTmp.erase(nameWithoutTmp.length()-4);
	if(session.renderInProgress()) Y_INFO << handlerName << ": Autosaving partial render (" << RoundFloatPrecision(session.currentPassPercent(), 0.01) << "% of pass " << session.currentPass() << " of " << session.totalPasses() << ") RGB" << ( m_hasAlpha ? "A" : "" ) << " file as \"" << nameWithoutTmp << "\"...  " << getDenoiseParams() << yendl;
	else Y_INFO << handlerName << ": Saving RGB" << ( m_hasAlpha ? "A" : "" ) << " file as \"" << nameWithoutTmp << "\"...  " << getDenoiseParams() << yendl;

#if defined(_WIN32)
	std::wstring wname = utf8_to_wutf16le(name);    
	libtiff::TIFF *out = libtiff::TIFFOpenW(wname.c_str(), "w");	//Windows needs the path in UTF16LE (unicode, UTF16, little endian) so we have to convert the UTF8 path to UTF16
#else
	libtiff::TIFF *out = libtiff::TIFFOpen(name.c_str(), "w");
#endif

	int channels;
	size_t bytesPerScanline;
	
	if(m_hasAlpha) channels = 4;
	else channels = 3;

	libtiff::TIFFSetField(out, TIFFTAG_IMAGEWIDTH, w);
	libtiff::TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
	libtiff::TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, channels);
	libtiff::TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	libtiff::TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	libtiff::TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	libtiff::TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	bytesPerScanline = channels * w;

    yByte *scanline = (yByte*)libtiff::_TIFFmalloc(bytesPerScanline);
    
    libtiff::TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, libtiff::TIFFDefaultStripSize(out, bytesPerScanline));

	imageBuffer_t *buf = imgBuffer.at(imgIndex);
//The denoise functionality will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV
	if(m_Denoise) {
		imageBuffer_t denoised_buffer = imgBuffer.at(imgIndex)->getDenoisedLDRBuffer(m_DenoiseHCol, m_DenoiseHLum, m_DenoiseMix);
		buf = &denoised_buffer;
	}
#endif //HAVE_OPENCV

	for (int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			int ix = x * channels;
			colorA_t col = buf->getColor(x, y);
			col.clampRGBA01();
			scanline[ix]   = (yByte)(col.getR() * 255.f);
			scanline[ix+1] = (yByte)(col.getG() * 255.f);
			scanline[ix+2] = (yByte)(col.getB() * 255.f);
			if(m_hasAlpha) scanline[ix+3] = (yByte)(col.getA() * 255.f);
		}

		if(TIFFWriteScanline(out, scanline, y, 0) < 0)
		{
			Y_ERROR << handlerName << ": An error occurred while writing TIFF file" << yendl;
			libtiff::TIFFClose(out);
			libtiff::_TIFFfree(scanline);

			return false;
		}
	}

	libtiff::TIFFClose(out);
	libtiff::_TIFFfree(scanline);
	
	Y_VERBOSE << handlerName << ": Done." << yendl;

	return true;
}

bool tifHandler_t::loadFromFile(const std::string &name)
{
	libtiff::uint32 w, h;
	
#if defined(_WIN32)
	std::wstring wname = utf8_to_wutf16le(name);
	libtiff::TIFF *tif = libtiff::TIFFOpenW(wname.c_str(), "r");	//Windows needs the path in UTF16LE (unicode, UTF16, little endian) so we have to convert the UTF8 path to UTF16
#else
	libtiff::TIFF *tif = libtiff::TIFFOpen(name.c_str(), "r");
#endif
	Y_INFO << handlerName << ": Loading image \"" << name << "\"..." << yendl;



	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

	libtiff::uint32 *tiffData = (libtiff::uint32*)libtiff::_TIFFmalloc(w * h * sizeof(libtiff::uint32));
           
	if(!libtiff::TIFFReadRGBAImage(tif, w, h, tiffData, 0))
	{
		Y_ERROR << handlerName << ": Error reading TIFF file" << yendl;
		return false;
	}
	
	m_hasAlpha = true;
	m_width = (int)w;
	m_height = (int)h;

	clearImgBuffers();
	
	int nChannels = 3;
	if(m_grayscale) nChannels = 1;
	else if(m_hasAlpha) nChannels = 4;
	imgBuffer.push_back(new imageBuffer_t(m_width, m_height, nChannels, getTextureOptimization()));
		
	int i = 0;
	
    for( int y = m_height - 1; y >= 0; y-- )
    {
    	for( int x = 0; x < m_width; x++ )
    	{
    		colorA_t color;
    		color.set((float)TIFFGetR(tiffData[i]) * inv8,
					(float)TIFFGetG(tiffData[i]) * inv8,
					(float)TIFFGetB(tiffData[i]) * inv8,
					(float)TIFFGetA(tiffData[i]) * inv8);
			i++;
			
			imgBuffer.at(0)->setColor(x, y, color, m_colorSpace, m_gamma);
    	}
    }

	libtiff::_TIFFfree(tiffData);
	
	libtiff::TIFFClose(tif);

	Y_VERBOSE << handlerName << ": Done." << yendl;

	return true;
}

imageHandler_t *tifHandler_t::factory(paraMap_t &params, renderEnvironment_t &render)
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

	imageHandler_t *ih = new tifHandler_t();
	
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
		render.registerImageHandler("tif", "tif tiff", "TIFF [Tag Image File Format]", tifHandler_t::factory);
	}

}

__END_YAFRAY
