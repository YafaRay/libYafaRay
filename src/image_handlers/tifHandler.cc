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
 
#include <core_api/environment.h>
#include <core_api/imagehandler.h>
#include <core_api/params.h>

#include <tiffio.h>

__BEGIN_YAFRAY

#define inv8  0.00392156862745098039 // 1 / 255
#define inv16 0.00001525902189669642 // 1 / 65535

class tifHandler_t: public imageHandler_t
{
public:
	tifHandler_t();
	~tifHandler_t();
	void initForOutput(int width, int height, bool withAlpha = false, bool withDepth = false);
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name);
	void putPixel(int x, int y, const colorA_t &rgba, float depth = 0.f);
	colorA_t getPixel(int x, int y);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);
};

tifHandler_t::tifHandler_t()
{
	m_width = 0;
	m_height = 0;
	m_hasAlpha = false;
	m_hasDepth = false;
	
	m_rgba = NULL;
	m_depth = NULL;
	
	handlerName = "TIFFHandler";
}

void tifHandler_t::initForOutput(int width, int height, bool withAlpha, bool withDepth)
{
	m_width = width;
	m_height = height;
	m_hasAlpha = withAlpha;
	m_hasDepth = withDepth;
	
	m_rgba = new rgba2DImage_nw_t(m_width, m_height);
	
	if(m_hasDepth)
	{
		m_depth = new gray2DImage_nw_t(m_width, m_height);
	}
}

tifHandler_t::~tifHandler_t()
{
	if(m_rgba) delete m_rgba;
	if(m_depth) delete m_depth;
	m_rgba = NULL;
	m_depth = NULL;

}

void tifHandler_t::putPixel(int x, int y, const colorA_t &rgba, float depth)
{
	(*m_rgba)(x, y) = rgba;
	if(m_hasDepth) (*m_depth)(x, y) = depth;
}

colorA_t tifHandler_t::getPixel(int x, int y)
{
	return (*m_rgba)(x, y);
}

bool tifHandler_t::saveToFile(const std::string &name)
{
	Y_INFO << handlerName << ": Saving RGB" << ( m_hasAlpha ? "A" : "" ) << " file as \"" << name << "\"..." << yendl;

	TIFF *out = TIFFOpen(name.c_str(), "w");
	int channels;
	size_t bytesPerScanline;
	
	if(m_hasAlpha) channels = 4;
	else channels = 3;

	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, m_width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, m_height);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, channels);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	bytesPerScanline = channels * m_width;

    yByte *scanline = (yByte*)_TIFFmalloc(bytesPerScanline);
    
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, bytesPerScanline));

    for (int y = 0; y < m_height; y++)
    {
    	for(int x = 0; x < m_width; x++)
    	{
    		int ix = x * channels;
    		colorA_t &col = (*m_rgba)(x, y);
    		col.clampRGBA01();
    		scanline[ix]   = (yByte)(col.getR() * 255.f);
    		scanline[ix+1] = (yByte)(col.getG() * 255.f);
    		scanline[ix+2] = (yByte)(col.getB() * 255.f);
    		if(m_hasAlpha) scanline[ix+3] = (yByte)(col.getA() * 255.f);
    	}
    	
		if(TIFFWriteScanline(out, scanline, y, 0) < 0)
		{
			Y_ERROR << handlerName << ": An error occurred while writing TIFF file" << yendl;
			TIFFClose(out);
			_TIFFfree(scanline);

			return false;
		}
    }
	
	TIFFClose(out);
	_TIFFfree(scanline);
	
	if(m_hasDepth)
	{
		std::string zbufname = name.substr(0, name.size() - 4) + "_zbuffer.tif";
		Y_INFO << handlerName << ": Saving Z-Buffer as \"" << zbufname << "\" (16Bits grayscale)..." << yendl;

		out = TIFFOpen(zbufname.c_str(), "w");
		
		TIFFSetField(out, TIFFTAG_IMAGEWIDTH, m_width);
		TIFFSetField(out, TIFFTAG_IMAGELENGTH, m_height);
		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 16);
		TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

		bytesPerScanline = m_width * 2; // 16 bits wide = 2 bytes per entry

		yWord *scanline16 = (yWord*)_TIFFmalloc(bytesPerScanline);
		
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, bytesPerScanline));

		for (int y = 0; y < m_height; y++)
		{
			for(int x = 0; x < m_width; x++)
			{
				float col = std::max(0.f, std::min(1.f, (*m_depth)(x, y)));
				scanline16[x] = (yWord)(col * 65535.f);
			}
			
			if(TIFFWriteScanline(out, scanline16, y, 0) < 0)
			{
				Y_ERROR << handlerName << ": An error occurred while writing TIFF file" << yendl;
				TIFFClose(out);
				_TIFFfree(scanline16);

				return false;
			}
		}
		
		TIFFClose(out);
		_TIFFfree(scanline16);

	}

	Y_INFO << handlerName << ": Done." << yendl;

	return true;
}

bool tifHandler_t::loadFromFile(const std::string &name)
{
	Y_INFO << handlerName << ": Loading image \"" << name << "\"..." << yendl;

	uint32 w, h;
	
	TIFF *tif = TIFFOpen(name.c_str(), "r");

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

	uint32 *tiffData = (uint32*)_TIFFmalloc(w * h * sizeof(uint32));
           
	if(!TIFFReadRGBAImage(tif, w, h, tiffData, 0))
	{
		Y_ERROR << handlerName << ": Error reading TIFF file" << yendl;
		return false;
	}
	
	m_hasAlpha = true;
	m_hasDepth = false;
	m_width = (int)w;
	m_height = (int)h;

	if(m_rgba) delete m_rgba;
	m_rgba = new rgba2DImage_nw_t(m_width, m_height);
	
	int i = 0;
	
    for( int y = m_height - 1; y >= 0; y-- )
    {
    	for( int x = 0; x < m_width; x++ )
    	{
    		colorA_t &col = (*m_rgba)(x, y);
    		col.set((float)TIFFGetR(tiffData[i]) * inv8,
					(float)TIFFGetG(tiffData[i]) * inv8,
					(float)TIFFGetB(tiffData[i]) * inv8,
					(float)TIFFGetA(tiffData[i]) * inv8);
			i++;
    	}
    }

	_TIFFfree(tiffData);
	
	TIFFClose(tif);

	Y_INFO << handlerName << ": Done." << yendl;

	return true;
}

imageHandler_t *tifHandler_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	int width = 0;
	int height = 0;
	bool withAlpha = false;
	bool withDepth = false;
	bool forOutput = true;

	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", withAlpha);
	params.getParam("z_channel", withDepth);
	params.getParam("for_output", forOutput);
	
	imageHandler_t *ih = new tifHandler_t();
	
	if(forOutput) ih->initForOutput(width, height, withAlpha, withDepth);
	
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
