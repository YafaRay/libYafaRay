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

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfVersion.h>

#include <cstdio>

using namespace Imf;
using namespace Imath;

__BEGIN_YAFRAY

typedef genericScanlineBuffer_t<Rgba> halfRgbaScanlineImage_t;
typedef genericScanlineBuffer_t<float> grayScanlineImage_t;

class exrHandler_t: public imageHandler_t
{
public:
	exrHandler_t();
	void initForOutput(int width, int height, bool withAlpha = false, bool withDepth = true);
	void initForInput();
	~exrHandler_t();
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name);
	void putPixel(int x, int y, const colorA_t &rgba, float depth = 0.f);
	colorA_t getPixel(int x, int y);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);
	bool isHDR() { return true; }

private:
	halfRgbaScanlineImage_t *m_halfrgba;
	grayScanlineImage_t *m_depthSL;
};

exrHandler_t::exrHandler_t()
{
	m_width = 0;
	m_height = 0;
	m_hasAlpha = false;
	m_hasDepth = false;

	m_rgba = NULL;
	m_depth = NULL;
	m_halfrgba = NULL;
	m_depthSL = NULL;

	handlerName = "EXRHandler";
}

void exrHandler_t::initForOutput(int width, int height, bool withAlpha, bool withDepth)
{
	m_width = width;
	m_height = height;
	m_hasAlpha = withAlpha;
	m_hasDepth = withDepth;

	m_halfrgba = new halfRgbaScanlineImage_t(m_height, m_width);

	if(m_hasDepth)
	{
		m_depthSL = new grayScanlineImage_t(m_height, m_width);
	}
}

exrHandler_t::~exrHandler_t()
{
	if(m_halfrgba) delete m_halfrgba;
	if(m_depthSL) delete m_depthSL;
	m_halfrgba = NULL;
	m_depthSL = NULL;
}

bool exrHandler_t::saveToFile(const std::string &name)
{
	Y_INFO << handlerName << ": Saving RGB" << ( m_hasAlpha ? "A" : "" ) << ( m_hasDepth ? "Z" : "" ) << " file as \"" << name << "\"..." << std::endl;

	int chan_size = sizeof(half);
	const int num_colchan = 4;
	int totchan_size = num_colchan*chan_size;

	Header header(m_width, m_height);

	header.compression() = ZIP_COMPRESSION;

	header.channels().insert("R", Channel(HALF));
	header.channels().insert("G", Channel(HALF));
	header.channels().insert("B", Channel(HALF));
	header.channels().insert("A", Channel(HALF));
	if(m_hasDepth) header.channels().insert("Z", Channel(Imf::FLOAT));

	OutputFile file(name.c_str(), header);

	char* data_ptr = (char *)&(*m_halfrgba)(0, 0);

	FrameBuffer fb;
	fb.insert("R", Slice(HALF, data_ptr              , totchan_size, m_width*totchan_size));
	fb.insert("G", Slice(HALF, data_ptr +   chan_size, totchan_size, m_width*totchan_size));
	fb.insert("B", Slice(HALF, data_ptr + 2*chan_size, totchan_size, m_width*totchan_size));
	fb.insert("A", Slice(HALF, data_ptr + 3*chan_size, totchan_size, m_width*totchan_size));
	if (m_hasDepth) fb.insert("Z", Slice(Imf::FLOAT, (char *)&(*m_depthSL)(0, 0), sizeof(float), m_width*sizeof(float)));

	file.setFrameBuffer(fb);

	try
	{
		file.writePixels(m_height);
		Y_INFO << handlerName << ": Done." << std::endl;
		return true;
	}
	catch (const std::exception &exc)
	{
		Y_ERROR << handlerName << ": " << exc.what() << std::endl;
		return false;
	}
}

void exrHandler_t::putPixel(int x, int y, const colorA_t &rgba, float depth)
{
	Rgba &pixel = (*m_halfrgba)(y, x);

	pixel.r = rgba.getR();
	pixel.g = rgba.getG();
	pixel.b = rgba.getB();

	if(m_hasAlpha) pixel.a = rgba.getA();
	else pixel.a = 1.f;

	if(m_hasDepth) (*m_depthSL)(y, x) = depth;
}

colorA_t exrHandler_t::getPixel(int x, int y)
{
	Rgba &pixel = (*m_halfrgba)(x, y);

	return colorA_t(pixel.r, pixel.g, pixel.b, pixel.a);
}

bool exrHandler_t::loadFromFile(const std::string &name)
{
	FILE* fp = std::fopen(name.c_str(), "rb");
	if (fp)
	{
		char bytes[4];
		fread(&bytes, 1, 4, fp);
		fclose(fp);
		fp = NULL;
		if(!isImfMagic(bytes)) return false;
	}

	try
	{
		RgbaInputFile file(name.c_str());
		Box2i dw = file.dataWindow();

		m_width  = dw.max.x - dw.min.x + 1;
		m_height = dw.max.y - dw.min.y + 1;
		m_hasAlpha = true;
		m_hasDepth = false;

		if(m_halfrgba) delete m_halfrgba;
		m_halfrgba = new halfRgbaScanlineImage_t(m_width, m_height);

		file.setFrameBuffer(&(*m_halfrgba)(0, 0) - dw.min.y - dw.min.x * m_height, m_height, 1);
		file.readPixels(dw.min.y, dw.max.y);

		return true;
	}
	catch (const std::exception &exc)
	{
		Y_ERROR << handlerName << ": " << exc.what() << std::endl;
		return false;
	}
}

imageHandler_t *exrHandler_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	int pixtype = HALF;
	int compression = ZIP_COMPRESSION;
	int width = 0;
	int height = 0;
	bool withAlpha = false;
	bool withDepth = false;
	bool forOutput = true;

	params.getParam("pixel_type", pixtype);
	params.getParam("compression", compression);
	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", withAlpha);
	params.getParam("z_channel", withDepth);
	params.getParam("for_output", forOutput);

	imageHandler_t *ih = new exrHandler_t();

	if(forOutput) ih->initForOutput(width, height, withAlpha, withDepth);

	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerImageHandler("exr", "exr", "EXR (IL&M OpenEXR)", exrHandler_t::factory);
	}

}
__END_YAFRAY
