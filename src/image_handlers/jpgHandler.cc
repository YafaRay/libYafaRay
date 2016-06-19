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
#include <core_api/scene.h>
#include <utilities/math_utils.h>
#include <utilities/fileUtils.h>

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
	Y_ERROR << "JPEG Library Error: " << buffer << yendl;
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
	void initForOutput(int width, int height, const renderPasses_t *renderPasses, bool withAlpha = false, bool multi_layer = false);
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name, int imagePassNumber = 0);
	void putPixel(int x, int y, const colorA_t &rgba, int imagePassNumber = 0);
	colorA_t getPixel(int x, int y, int imagePassNumber = 0);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);
};

jpgHandler_t::jpgHandler_t()
{
	m_width = 0;
	m_height = 0;
	m_hasAlpha = false;
	m_MultiLayer = false;
	
	handlerName = "JPEGHandler";
	
	rgbOptimizedBuffer = nullptr;
	rgbCompressedBuffer = nullptr;
}

void jpgHandler_t::initForOutput(int width, int height, const renderPasses_t *renderPasses, bool withAlpha, bool multi_layer)
{
	m_width = width;
	m_height = height;
	m_hasAlpha = withAlpha;
    m_MultiLayer = multi_layer;

	imagePasses.resize(renderPasses->extPassesSize());
	
	for(size_t idx = 0; idx < imagePasses.size(); ++idx)
	{
		imagePasses.at(idx) = new rgba2DImage_nw_t(m_width, m_height);
	}
}

jpgHandler_t::~jpgHandler_t()
{
	if(!imagePasses.empty())
	{
		for(size_t idx = 0; idx < imagePasses.size(); ++idx)
		{
			if(imagePasses.at(idx)) delete imagePasses.at(idx);
			imagePasses.at(idx) = nullptr;
		}
	}

	if(rgbOptimizedBuffer) delete rgbOptimizedBuffer;
	if(rgbCompressedBuffer) delete rgbCompressedBuffer;

	rgbOptimizedBuffer = nullptr;
	rgbCompressedBuffer = nullptr;
}

void jpgHandler_t::putPixel(int x, int y, const colorA_t &rgba, int imagePassNumber)
{
	(*imagePasses.at(imagePassNumber))(x, y) = rgba;
}

colorA_t jpgHandler_t::getPixel(int x, int y, int imagePassNumber)
{
	if(rgbOptimizedBuffer) return (*rgbOptimizedBuffer)(x, y).getColor();
	else if(rgbCompressedBuffer) return (*rgbCompressedBuffer)(x, y).getColor();
	else if(!imagePasses.empty() && imagePasses.at(0)) return (*imagePasses.at(0))(x, y);
	else return colorA_t(0.f);	//This should not happen, but just in case
}

bool jpgHandler_t::saveToFile(const std::string &name, int imagePassNumber)
{
	std::string nameWithoutTmp = name;
	nameWithoutTmp.erase(nameWithoutTmp.length()-4);
	if(session.renderInProgress()) Y_INFO << handlerName << ": Autosaving partial render (" << RoundFloatPrecision(session.currentPassPercent(), 0.01) << "% of pass " << session.currentPass() << " of " << session.totalPasses() << ") RGB" << " file as \"" << nameWithoutTmp << "\"..." << yendl;
	else Y_INFO << handlerName << ": Saving RGB" << " file as \"" << nameWithoutTmp << "\"..." << yendl;

	struct jpeg_compress_struct info;
	struct jpgErrorManager jerr;
	int x, y, ix;
	yByte *scanline = nullptr;
	
	FILE * fp = fileUnicodeOpen(name, "wb");
	
	if (!fp)
	{
		Y_ERROR << handlerName << ": Cannot open file for writing " << name << yendl;
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

	cv::Mat A(m_height, m_width, CV_8UC3);
	cv::Mat B(m_height, m_width, CV_8UC3);
	cv::Mat_<cv::Vec3b> _A = A;
	cv::Mat_<cv::Vec3b> _B = B;

	for(y = 0; y < m_height; y++)
	{
		for (x = 0; x < m_width; x++)
		{
			ix = x * 3;
			colorA_t &col = (*imagePasses.at(imagePassNumber))(x, y);
			col.clampRGBA01();
			_A(y, x)[2]   = (col.getR() * 255);
			_A(y, x)[0] = (col.getG() * 255);
			_A(y, x)[1] = (col.getB() * 255);
		}
	}
	
	//cv::blur(A, B, cv::Size(10,10));
	//cv::imwrite("/home/ARCHIVOS/VirtualBox/compartir/opencv_render/opencv - 001blur.png", B);

	cv::fastNlMeansDenoisingColored(A, B, 3, 60);
	//cv::imwrite("/home/ARCHIVOS/VirtualBox/compartir/opencv_render/opencv - 001denoise.png", B);
	
	for(y = 0; y < m_height; y++)
	{
		for (x = 0; x < m_width; x++)
		{
			ix = x * 3;
			colorA_t &col = (*imagePasses.at(imagePassNumber))(x, y);
			col.clampRGBA01();
			scanline[ix]   = (yByte) _B(y, x)[2];
			scanline[ix+1] = (yByte) _B(y, x)[0];
			scanline[ix+2] = (yByte) _B(y, x)[1];
		}

		jpeg_write_scanlines(&info, &scanline, 1);
	}

	delete [] scanline;

	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	fileUnicodeClose(fp);

	if(m_hasAlpha)
	{
		std::string alphaname = name.substr(0, name.size() - 4) + "_alpha.jpg";
		if(session.renderInProgress()) Y_INFO << handlerName << ": Autosaving partial render (" << RoundFloatPrecision(session.currentPassPercent(), 0.01) << "% of pass " << session.currentPass() << " of " << session.totalPasses() << ") Alpha channel as \"" << alphaname << "\"..." << yendl;
		else Y_INFO << handlerName << ": Saving Alpha channel as \"" << alphaname << "\"..." << yendl;

		fp = fileUnicodeOpen(alphaname, "wb");
		
		if (!fp)
		{
			Y_ERROR << handlerName << ": Cannot open file for writing " << alphaname << yendl;
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
				float col = std::max(0.f, std::min(1.f, (*imagePasses.at(imagePassNumber))(x, y).getA()));

				scanline[x] = (yByte)(col * 255);
			}

			jpeg_write_scanlines(&info, &scanline, 1);
		}

		delete [] scanline;

		jpeg_finish_compress(&info);
		jpeg_destroy_compress(&info);

		fileUnicodeClose(fp);
	}

	Y_VERBOSE << handlerName << ": Done." << yendl;

	return true;
}

bool jpgHandler_t::loadFromFile(const std::string &name)
{
	jpeg_decompress_struct info;
	jpgErrorManager jerr;

	FILE *fp = fileUnicodeOpen(name, "rb");

	Y_INFO << handlerName << ": Loading image \"" << name << "\"..." << yendl;

	if(!fp)
	{
		Y_ERROR << handlerName << ": Cannot open file " << name << yendl;
		return false;
	}

	info.err = jpeg_std_error(&jerr.pub);
	info.err->output_message = jpgErrorMessage;
	jerr.pub.error_exit = jpgExitOnError;

	if (setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&info);
		
		fileUnicodeClose(fp);
		
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
		Y_ERROR << handlerName << ": Unsupported color space: " << info.out_color_space << "| Color components: " << info.output_components << yendl;
		
		jpeg_finish_decompress(&info);
		jpeg_destroy_decompress(&info);
		
		fileUnicodeClose(fp);
		
		return false;
	}
	
	m_hasAlpha = false;
	m_width = info.output_width;
	m_height = info.output_height;

	if(!imagePasses.empty())
	{
		for(size_t idx = 0; idx < imagePasses.size(); ++idx)
		{
			if(imagePasses.at(idx)) delete imagePasses.at(idx);
		}
		imagePasses.clear();
	}
	
	if(getTextureOptimization() == TEX_OPTIMIZATION_OPTIMIZED) rgbOptimizedBuffer = new rgbOptimizedImage_nw_t(m_width, m_height);	//JPG does not have alpha, so we can save 8 bits in the optimized buffer
	else if(getTextureOptimization() == TEX_OPTIMIZATION_COMPRESSED) rgbCompressedBuffer = new rgbCompressedImage_nw_t(m_width, m_height);
	else imagePasses.push_back(new rgba2DImage_nw_t(m_width, m_height));

	yByte* scanline = new yByte[m_width * info.output_components];
	
	int y = 0;
	int ix = 0;
	
	while ( info.output_scanline < info.output_height )
	{
		jpeg_read_scanlines(&info, &scanline, 1);
		
		for (int x = 0; x < m_width; x++)
		{
			colorA_t color;
			
			if (isGray)
			{
				float colscan = scanline[x] * inv8;
				color.set(colscan, colscan, colscan, 1.f);
			}
			else if(isRGB)
			{
				ix = x * 3;
				color.set( scanline[ix] * inv8,
									 scanline[ix+1] * inv8,
									 scanline[ix+2] * inv8,
									 1.f);
			}
			else if(isCMYK)
			{
				ix = x * 4;
				float K = scanline[ix+3] * inv8;
				float iK = 1.f - K;
				
				color.set( 1.f - std::max((scanline[ix]   * inv8 * iK) + K, 1.f), 
									 1.f - std::max((scanline[ix+1] * inv8 * iK) + K, 1.f),
									 1.f - std::max((scanline[ix+2] * inv8 * iK) + K, 1.f),
									 1.f);
			}
			else // this is probabbly (surely) never executed, i need to research further; this assumes blender non-standard jpeg + alpha
			{
				ix = x * 4;
				float A = scanline[ix+3] * inv8;
				float iA = 1.f - A;
				color.set( std::max(0.f, std::min((scanline[ix]   * inv8) - iA, 1.f)),
									 std::max(0.f, std::min((scanline[ix+1] * inv8) - iA, 1.f)),
									 std::max(0.f, std::min((scanline[ix+2] * inv8) - iA, 1.f)),
									 A);
			}
			
			if(rgbOptimizedBuffer) (*rgbOptimizedBuffer)(x, y).setColor(color);
			else if(rgbCompressedBuffer) (*rgbCompressedBuffer)(x, y).setColor(color);
			else if(!imagePasses.empty() && imagePasses.at(0)) (*imagePasses.at(0))(x, y) = color;	
		}
		y++;
	}
	
	delete [] scanline;

	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	
	fileUnicodeClose(fp);

	Y_VERBOSE << handlerName << ": Done." << yendl;

	return true;
}


imageHandler_t *jpgHandler_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	int width = 0;
	int height = 0;
	bool withAlpha = false;
	bool forOutput = true;

	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", withAlpha);
	params.getParam("for_output", forOutput);

	imageHandler_t *ih = new jpgHandler_t();
	
	if(forOutput)
	{
		if(yafLog.getUseParamsBadge()) height += yafLog.getBadgeHeight();
		ih->initForOutput(width, height, render.getRenderPasses(), withAlpha, false);
	}
	
	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerImageHandler("jpg", "jpg jpeg", "JPEG [Joint Photographic Experts Group]", jpgHandler_t::factory);
	}

}

__END_YAFRAY
