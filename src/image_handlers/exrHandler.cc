/****************************************************************************
 *
 * 		exrHandler.cc: EXR format handler
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

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfVersion.h>

#include <cstdio>
#include <locale>
#include <codecvt>

#include <boost/filesystem.hpp>

using namespace Imf;
using namespace Imath;

__BEGIN_YAFRAY

typedef genericScanlineBuffer_t<Rgba> halfRgbaScanlineImage_t;
typedef genericScanlineBuffer_t<float> grayScanlineImage_t;

class exrHandler_t: public imageHandler_t
{
public:
	exrHandler_t();
	void initForOutput(int width, int height, const renderPasses_t *renderPasses, bool withAlpha = false, bool multi_layer = false);
	void initForInput();
	~exrHandler_t();
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name, int imagePassNumber = 0);
    bool saveToFileMultiChannel(const std::string &name, const renderPasses_t *renderPasses);
	void putPixel(int x, int y, const colorA_t &rgba, int imagePassNumber = 0);
	colorA_t getPixel(int x, int y, int imagePassNumber = 0);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);
	bool isHDR() { return true; }

private:
	std::vector<halfRgbaScanlineImage_t*> m_halfrgba;
};

exrHandler_t::exrHandler_t()
{
	handlerName = "EXRHandler";
}

void exrHandler_t::initForOutput(int width, int height, const renderPasses_t *renderPasses, bool withAlpha, bool multi_layer)
{
	m_width = width;
	m_height = height;
	m_hasAlpha = withAlpha;
	m_MultiLayer = multi_layer;
    
	m_halfrgba.resize(renderPasses->extPassesSize());
	
	for(size_t idx = 0; idx < m_halfrgba.size(); ++idx)
	{
		m_halfrgba.at(idx) = new halfRgbaScanlineImage_t(m_height, m_width);
	}
}

exrHandler_t::~exrHandler_t()
{
	for(size_t idx = 0; idx < m_halfrgba.size(); ++idx)
	{
		if(m_halfrgba.at(idx)) delete m_halfrgba.at(idx);
	
		m_halfrgba.at(idx) = nullptr;
	}
}

bool exrHandler_t::saveToFile(const std::string &name, int imagePassNumber)
{
	Y_INFO << handlerName << ": Saving RGB" << ( m_hasAlpha ? "A" : "" ) << " file as \"" << name << "\"..." << yendl;

	int chan_size = sizeof(half);
	const int num_colchan = 4;
	int totchan_size = num_colchan*chan_size;

	Header header(m_width, m_height);

	header.compression() = ZIP_COMPRESSION;

	header.channels().insert("R", Channel(HALF));
	header.channels().insert("G", Channel(HALF));
	header.channels().insert("B", Channel(HALF));
	header.channels().insert("A", Channel(HALF));

	OutputFile file(name.c_str(), header);

	char* data_ptr = (char *)&(*m_halfrgba.at(imagePassNumber))(0, 0);
	

	FrameBuffer fb;
	fb.insert("R", Slice(HALF, data_ptr              , totchan_size, m_width*totchan_size));
	fb.insert("G", Slice(HALF, data_ptr +   chan_size, totchan_size, m_width*totchan_size));
	fb.insert("B", Slice(HALF, data_ptr + 2*chan_size, totchan_size, m_width*totchan_size));
	fb.insert("A", Slice(HALF, data_ptr + 3*chan_size, totchan_size, m_width*totchan_size));

	file.setFrameBuffer(fb);

	try
	{
		file.writePixels(m_height);
		Y_VERBOSE << handlerName << ": Done." << yendl;
		return true;
	}
	catch (const std::exception &exc)
	{
		Y_ERROR << handlerName << ": " << exc.what() << yendl;
		return false;
	}
}

bool exrHandler_t::saveToFileMultiChannel(const std::string &name, const renderPasses_t *renderPasses)
{
    std::string extPassName;
    
    Y_INFO << handlerName << ": Saving Multilayer EXR" << " file as \"" << name << "\"..." << yendl;

	int chan_size = sizeof(half);
	const int num_colchan = 4;
	int totchan_size = num_colchan*chan_size;
	
    Header header(m_width, m_height);
    FrameBuffer fb;
	header.compression() = ZIP_COMPRESSION;
    
    for(int idx = 0; idx < renderPasses->extPassesSize(); ++idx)
    {
		extPassName = "RenderLayer." + renderPasses->extPassTypeStringFromIndex(idx) + ".";        
		Y_VERBOSE << "    Writing EXR Layer: " << renderPasses->extPassTypeStringFromIndex(idx) << yendl;
        
        const std::string channelR_string = extPassName + "R";
        const std::string channelG_string = extPassName + "G";
        const std::string channelB_string = extPassName + "B";
        const std::string channelA_string = extPassName + "A";
        
        const char* channelR = channelR_string.c_str();
        const char* channelG = channelG_string.c_str();
        const char* channelB = channelB_string.c_str();
        const char* channelA = channelA_string.c_str();
        
        header.channels().insert(channelR, Channel(HALF));
        header.channels().insert(channelG, Channel(HALF));
        header.channels().insert(channelB, Channel(HALF));
        header.channels().insert(channelA, Channel(HALF));
 
        char* data_ptr = (char *)&(*m_halfrgba.at(idx))(0, 0);

        fb.insert(channelR, Slice(HALF, data_ptr              , totchan_size, m_width*totchan_size));
        fb.insert(channelG, Slice(HALF, data_ptr +   chan_size, totchan_size, m_width*totchan_size));
        fb.insert(channelB, Slice(HALF, data_ptr + 2*chan_size, totchan_size, m_width*totchan_size));
        fb.insert(channelA, Slice(HALF, data_ptr + 3*chan_size, totchan_size, m_width*totchan_size));
    }
    
    OutputFile file(name.c_str(), header);    
	file.setFrameBuffer(fb);

	try
	{
		file.writePixels(m_height);
		Y_VERBOSE << handlerName << ": Done." << yendl;
		return true;
	}
	catch (const std::exception &exc)
	{
		Y_ERROR << handlerName << ": " << exc.what() << yendl;
		return false;
	}
}

void exrHandler_t::putPixel(int x, int y, const colorA_t &rgba, int imagePassNumber)
{
	Rgba &pixel = (*m_halfrgba.at(imagePassNumber))(y, x);

	pixel.r = rgba.getR();
	pixel.g = rgba.getG();
	pixel.b = rgba.getB();

	if(m_hasAlpha) pixel.a = rgba.getA();
	else pixel.a = 1.f;
}

colorA_t exrHandler_t::getPixel(int x, int y, int imagePassNumber)
{
	Rgba &pixel = (*m_halfrgba.at(imagePassNumber))(x, y);

	return colorA_t(pixel.r, pixel.g, pixel.b, pixel.a);
}

bool exrHandler_t::loadFromFile(const std::string &name)
{
#if defined(_WIN32)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
	std::wstring wname = convert.from_bytes(name);    
	FILE *fp = _wfopen(wname.c_str(), L"rb");	//Windows needs the path in UTF16 (unicode) so we have to convert the UTF8 path to UTF16
	SetConsoleOutputCP(65001);	//set Windows Console to UTF8 so the image path can be displayed correctly
	std::string tempFilePathString = "";	//filename of the temporary exr file that will be generated to deal with the UTF16 ifstream path problems in OpenEXR libraries with MinGW
#else
	FILE *fp = fopen(name.c_str(), "rb");
#endif
	Y_INFO << handlerName << ": Loading image \"" << name << "\"..." << yendl;
	
	if(!fp)
	{
		Y_ERROR << handlerName << ": Cannot open file " << name << yendl;
		return false;
	}
	else
	{
		char bytes[4];
		fread(&bytes, 1, 4, fp);
#if defined(_WIN32)		
		fseek (fp , 0 , SEEK_SET);
		auto tempFolder = boost::filesystem::temp_directory_path();
		auto tempFile = boost::filesystem::unique_path();
		tempFilePathString = tempFolder.string() + tempFile.string() + ".exr";
		Y_VERBOSE << handlerName << ": Creating intermediate temporary file tempstr=" << tempFilePathString << yendl;
		FILE *fpTemp = fopen(tempFilePathString.c_str(), "wb");
		if(!fpTemp)
		{
			Y_ERROR << handlerName << ": Cannot create intermediate temporary file " << tempFilePathString << yendl;
			return false;
		}
		//Copy original EXR texture contents into new temporary file, so we can circumvent the lack of UTF16 support in MinGW ifstream
		unsigned char *copy_buffer = (unsigned char*) malloc(1024);
		int numReadBytes = 0;
		while((numReadBytes = fread(copy_buffer, sizeof(unsigned char), 1024, fp)) == 1024) fwrite(copy_buffer, sizeof(unsigned char), 1024, fpTemp);
		fwrite(copy_buffer, sizeof(unsigned char), numReadBytes, fpTemp);
		fclose(fpTemp);		
		free(copy_buffer);
#endif		
		fclose(fp);
		fp = nullptr;
		if(!isImfMagic(bytes)) return false;
	}

	try
	{
#if defined(_WIN32)
		Y_INFO << handlerName << ": Loading intermediate temporary file tempstr=" << tempFilePathString << yendl;
		RgbaInputFile file(tempFilePathString.c_str());		
#else		
		RgbaInputFile file(name.c_str());		
#endif		
		Box2i dw = file.dataWindow();

		m_width  = dw.max.x - dw.min.x + 1;
		m_height = dw.max.y - dw.min.y + 1;
		m_hasAlpha = true;

		if(!m_halfrgba.empty())
		{
			for(size_t idx = 0; idx < m_halfrgba.size(); ++idx)
			{
				if(m_halfrgba.at(idx)) delete m_halfrgba.at(idx);
			}
			m_halfrgba.clear();
		}
		
		m_halfrgba.push_back(new halfRgbaScanlineImage_t(m_width, m_height));

		file.setFrameBuffer(&(*m_halfrgba.at(0))(0, 0) - dw.min.y - dw.min.x * m_height, m_height, 1);
		file.readPixels(dw.min.y, dw.max.y);
	}
	catch (const std::exception &exc)
	{
		Y_ERROR << handlerName << ": " << exc.what() << yendl;
		return false;
	}

#if defined(_WIN32)
	Y_INFO << handlerName << ": Deleting intermediate temporary file tempstr=" << tempFilePathString << yendl;
	std::remove(tempFilePathString.c_str());		
#endif	

	return true;
}

imageHandler_t *exrHandler_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	int pixtype = HALF;
	int compression = ZIP_COMPRESSION;
	int width = 0;
	int height = 0;
	bool withAlpha = false;
	bool forOutput = true;
	bool multiLayer = false;

	params.getParam("pixel_type", pixtype);
	params.getParam("compression", compression);
	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", withAlpha);
	params.getParam("for_output", forOutput);
	params.getParam("img_multilayer", multiLayer);

	imageHandler_t *ih = new exrHandler_t();

	if(forOutput)
	{
		if(yafLog.getUseParamsBadge()) height += yafLog.getBadgeHeight();
		ih->initForOutput(width, height, render.getRenderPasses(), withAlpha, multiLayer);
	}

	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerImageHandler("exr", "exr", "EXR [IL&M OpenEXR]", exrHandler_t::factory);
	}

}
__END_YAFRAY
