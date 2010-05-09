/****************************************************************************
 *
 *      pngHandler.cc: Joint Photographic Experts Group (JPEG) format handler
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

#include <cstdio>

extern "C"
{
	#include <setjmp.h>
	#include <jpeglib.h>
}

__BEGIN_YAFRAY

#define inv8  0.00392156862745098039f // 1 / 255

// error handlers for libJPEG,

METHODDEF(void) jpgErrorMessage(j_common_ptr info)
{
	char buffer[JMSG_LENGTH_MAX];
	(*info->err->format_message)(info, buffer);
	Y_ERROR << "JPEG Library Error: " << buffer << std::endl;
}

struct jpgErrorManager
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

// JPEG error manager pointer
typedef struct jpgErrorManager *error_ptr;

void jpgExitOnError(j_common_ptr info)
{
	error_ptr myerr = (error_ptr)info->err;
	(*info->err->output_message)(info);
	longjmp(myerr->setjmp_buffer, 1);
}

// The actual class

class jpgHandler_t: public imageHandler_t
{
public:
	jpgHandler_t();
	~jpgHandler_t();
	void initForOutput(int width, int height, bool withAlpha = false, bool withDepth = true);
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name);
	void putPixel(int x, int y, const colorA_t &rgba, float depth = 0.f);
	colorA_t getPixel(int x, int y);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);
};

jpgHandler_t::jpgHandler_t()
{
	m_width = 0;
	m_height = 0;
	m_hasAlpha = false;
	m_hasDepth = false;
	
	m_rgba = NULL;
	m_depth = NULL;
	
	handlerName = "JPEGHandler";
}

void jpgHandler_t::initForOutput(int width, int height, bool withAlpha, bool withDepth)
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

jpgHandler_t::~jpgHandler_t()
{
	if(m_rgba) delete m_rgba;
	if(m_depth) delete m_depth;
	m_rgba = NULL;
	m_depth = NULL;

}

void jpgHandler_t::putPixel(int x, int y, const colorA_t &rgba, float depth)
{
	(*m_rgba)(x, y) = rgba;
	if(m_hasDepth) (*m_depth)(x, y) = depth;
}

colorA_t jpgHandler_t::getPixel(int x, int y)
{
	return (*m_rgba)(x, y);
}

bool jpgHandler_t::saveToFile(const std::string &name)
{
	Y_INFO << handlerName << ": Saving RGB" << " file as \"" << name << "\"..." << std::endl;

	FILE * fp;
	struct jpeg_compress_struct info;
	struct jpgErrorManager jerr;
	int x, y, ix;
	yByte *scanline = NULL;
	
	fp = fopen(name.c_str(), "wb");
	
	if (!fp)
	{
		Y_ERROR << handlerName << ": Cannot open file for writing " << name << std::endl;
		return false;
	}
	
	info.err = jpeg_std_error(&jerr.pub);
	info.err->output_message = jpgErrorMessage;
	jerr.pub.error_exit = jpgExitOnError;

	jpeg_create_compress(&info);
	jpeg_stdio_dest(&info, fp);

	info.image_width = m_width;
	info.image_height = m_height;
	info.in_color_space = JCS_RGB;
	info.input_components = 3;
	
	jpeg_set_defaults(&info);
	
	info.dct_method = JDCT_FLOAT;
	jpeg_set_quality(&info, 100, TRUE);
	
	jpeg_start_compress(&info, TRUE);

	scanline = new yByte[ m_width * 3 ];

	for(y = 0; y < m_height; y++)
	{
		for (x = 0; x < m_width; x++)
		{
			ix = x * 3;
			colorA_t &col = (*m_rgba)(x, y);
			col.clampRGBA01();
			scanline[ix]   = (yByte)(col.getR() * 255);
			scanline[ix+1] = (yByte)(col.getG() * 255);
			scanline[ix+2] = (yByte)(col.getB() * 255);
		}

		jpeg_write_scanlines(&info, &scanline, 1);
	}

	delete [] scanline;

	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	fclose(fp);

	if(m_hasAlpha)
	{
		std::string alphaname = name.substr(0, name.size() - 4) + "_alpha.jpg";
		Y_INFO << handlerName << ": Saving Alpha channel as \"" << alphaname << "\"..." << std::endl;

		fp = fopen(alphaname.c_str(), "wb");
		
		if (!fp)
		{
			Y_ERROR << handlerName << ": Cannot open file for writing " << alphaname << std::endl;
			return false;
		}
		
		info.err = jpeg_std_error(&jerr.pub);
		info.err->output_message = jpgErrorMessage;
		jerr.pub.error_exit = jpgExitOnError;

		jpeg_create_compress(&info);
		jpeg_stdio_dest(&info, fp);

		info.image_width = m_width;
		info.image_height = m_height;
		info.in_color_space = JCS_GRAYSCALE;
		info.input_components = 1;
		
		jpeg_set_defaults(&info);
		
		info.dct_method = JDCT_FLOAT;
		jpeg_set_quality(&info, 100, TRUE);
		
		jpeg_start_compress(&info, TRUE);

		scanline = new yByte[ m_width ];

		for(y = 0; y < m_height; y++)
		{
			for (x = 0; x < m_width; x++)
			{
				float col = std::max(0.f, std::min(1.f, (*m_rgba)(x, y).getA()));

				scanline[x] = (yByte)(col * 255);
			}

			jpeg_write_scanlines(&info, &scanline, 1);
		}

		delete [] scanline;

		jpeg_finish_compress(&info);
		jpeg_destroy_compress(&info);

		fclose(fp);
	}

	if(m_hasDepth)
	{
		std::string zbufname = name.substr(0, name.size() - 4) + "_zbuffer.jpg";
		Y_INFO << handlerName << ": Saving Z-Buffer as \"" << zbufname << "\"..." << std::endl;

		fp = fopen(zbufname.c_str(), "wb");
		
		if (!fp)
		{
			Y_ERROR << handlerName << ": Cannot open file for writing " << zbufname << std::endl;
			return false;
		}
		
		info.err = jpeg_std_error(&jerr.pub);
		info.err->output_message = jpgErrorMessage;
		jerr.pub.error_exit = jpgExitOnError;

		jpeg_create_compress(&info);
		jpeg_stdio_dest(&info, fp);

		info.image_width = m_width;
		info.image_height = m_height;
		info.in_color_space = JCS_GRAYSCALE;
		info.input_components = 1;
		
		jpeg_set_defaults(&info);
		
		info.dct_method = JDCT_FLOAT;
		jpeg_set_quality(&info, 100, TRUE);
		
		jpeg_start_compress(&info, TRUE);

		scanline = new yByte[ m_width ];

		for(y = 0; y < m_height; y++)
		{
			for (x = 0; x < m_width; x++)
			{
				float col = std::max(0.f, std::min(1.f, (*m_depth)(x, y)));

				scanline[x] = (yByte)(col * 255);
			}

			jpeg_write_scanlines(&info, &scanline, 1);
		}

		delete [] scanline;

		jpeg_finish_compress(&info);
		jpeg_destroy_compress(&info);

		fclose(fp);
	}

	Y_INFO << handlerName << ": Done." << std::endl;

	return true;
}

bool jpgHandler_t::loadFromFile(const std::string &name)
{
	Y_INFO << handlerName << ": Loading image \"" << name << "\"..." << std::endl;

	FILE *fp;
	jpeg_decompress_struct info;
	jpgErrorManager jerr;
	
	fp = fopen(name.c_str(),"rb");

	if(!fp)
	{
		Y_ERROR << handlerName << ": Cannot open file " << name << std::endl;
		return false;
	}

	info.err = jpeg_std_error(&jerr.pub);
	info.err->output_message = jpgErrorMessage;
	jerr.pub.error_exit = jpgExitOnError;

	if (setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&info);
		
		fclose(fp);
		
		return false;
	}

	jpeg_create_decompress(&info);
	jpeg_stdio_src(&info, fp);
	jpeg_read_header(&info, TRUE);

	jpeg_start_decompress(&info);

	bool isGray = ((info.out_color_space == JCS_GRAYSCALE) & (info.output_components == 1));
	bool isRGB  = ((info.out_color_space == JCS_RGB) & (info.output_components == 3));
	bool isCMYK = ((info.out_color_space == JCS_CMYK) & (info.output_components == 4));// TODO: findout if blender's non-standard jpeg + alpha comply with this or not, the code for conversion is below

	if ((!isGray) && (!isRGB) && (!isCMYK))
	{
		Y_ERROR << handlerName << ": Unsupported color space: " << info.out_color_space << "| Color components: " << info.output_components << std::endl;
		
		jpeg_finish_decompress(&info);
		jpeg_destroy_decompress(&info);
		
		fclose(fp);
		
		return false;
	}
	
	m_hasAlpha = false;
	m_hasDepth = false;
	m_width = info.output_width;
	m_height = info.output_height;
	
	if(m_rgba) delete m_rgba;
	m_rgba = new rgba2DImage_nw_t(m_width, m_height);

	yByte* scanline = new yByte[m_width * info.output_components];
	
	int y = 0;
	int ix = 0;
	
	while ( info.output_scanline < info.output_height )
	{
		jpeg_read_scanlines(&info, &scanline, 1);
		
		for (int x = 0; x < m_width; x++)
		{
			if (isGray)
			{
				float color = scanline[x] * inv8;
				(*m_rgba)(x, y).set(color, color, color, 1.f);
			}
			else if(isRGB)
			{
				ix = x * 3;
				(*m_rgba)(x, y).set( scanline[ix] * inv8,
									 scanline[ix+1] * inv8,
									 scanline[ix+2] * inv8,
									 1.f);
			}
			else if(isCMYK)
			{
				ix = x * 4;
				float K = scanline[ix+3] * inv8;
				float iK = 1.f - K;
				
				(*m_rgba)(x, y).set( 1.f - std::max((scanline[ix]   * inv8 * iK) + K, 1.f),
									 1.f - std::max((scanline[ix+1] * inv8 * iK) + K, 1.f),
									 1.f - std::max((scanline[ix+2] * inv8 * iK) + K, 1.f),
									 1.f);
			}
			else // this is probabbly (surely) never executed, i need to research further; this assumes blender non-standard jpeg + alpha
			{
				ix = x * 4;
				float A = scanline[ix+3] * inv8;
				float iA = 1.f - A;
				(*m_rgba)(x, y).set( std::max(0.f, std::min((scanline[ix]   * inv8) - iA, 1.f)),
									 std::max(0.f, std::min((scanline[ix+1] * inv8) - iA, 1.f)),
									 std::max(0.f, std::min((scanline[ix+2] * inv8) - iA, 1.f)),
									 A);
			}
		}
		y++;
	}
	
	delete [] scanline;

	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	
	fclose(fp);

	Y_INFO << handlerName << ": Done." << std::endl;

	return true;
}

imageHandler_t *jpgHandler_t::factory(paraMap_t &params, renderEnvironment_t &render)
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
	
	imageHandler_t *ih = new jpgHandler_t();
	
	if(forOutput) ih->initForOutput(width, height, withAlpha, withDepth);
	
	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerImageHandler("jpg", "jpg jpeg", "JPEG (Joint Photographic Experts Group)", jpgHandler_t::factory);
	}

}

__END_YAFRAY
