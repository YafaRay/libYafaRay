/****************************************************************************
 *
 *      pngHandler.cc: Joint Photographic Experts Group (JPEG) format handler
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

#ifdef HAVE_JPEG

#include "format/format_jpg.h"
#include "common/logger.h"
#include "common/param.h"
#include "common/file.h"
#include "scene/scene.h"
#include "color/color.h"

extern "C"
{
#include <setjmp.h>
#include <jpeglib.h>
}

BEGIN_YAFARAY

#define INV_8  0.00392156862745098039f // 1 / 255

// error handlers for libJPEG,

METHODDEF(void) jpgErrorMessage__(j_common_ptr info)
{
	char buffer[JMSG_LENGTH_MAX];
	(*info->err->format_message)(info, buffer);
	Y_ERROR << "JPEG Library Error: " << buffer << YENDL;
}

struct JpgErrorManager
{
	struct ::jpeg_error_mgr pub_;
	jmp_buf setjmp_buffer_;
};

// JPEG error manager pointer
typedef struct JpgErrorManager *ErrorPtr_t;

void jpgExitOnError__(j_common_ptr info)
{
	ErrorPtr_t myerr = (ErrorPtr_t)info->err;
	(*info->err->output_message)(info);
	longjmp(myerr->setjmp_buffer_, 1);
}

bool JpgFormat::saveToFile(const std::string &name, const Image *image)
{
	const int height = image->getHeight();
	const int width = image->getWidth();

	std::string name_without_tmp = name;
	name_without_tmp.erase(name_without_tmp.length() - 4);

	struct ::jpeg_compress_struct info;
	struct JpgErrorManager jerr;
	int x, y, ix;
	uint8_t *scanline = nullptr;

	FILE *fp = File::open(name, "wb");

	if(!fp)
	{
		Y_ERROR << getFormatName() << ": Cannot open file for writing " << name << YENDL;
		return false;
	}

	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage__;
	jerr.pub_.error_exit = jpgExitOnError__;

	jpeg_create_compress(&info);
	jpeg_stdio_dest(&info, fp);

	info.image_width = width;
	info.image_height = height;
	info.in_color_space = JCS_RGB;
	info.input_components = 3;

	jpeg_set_defaults(&info);

	info.dct_method = JDCT_FLOAT;
	jpeg_set_quality(&info, 100, TRUE);

	jpeg_start_compress(&info, TRUE);

	scanline = new uint8_t[width * 3];

	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{
			ix = x * 3;
			Rgba col = image->getColor(x, y);
			col.clampRgba01();
			scanline[ix] = (uint8_t) (col.getR() * 255);
			scanline[ix + 1] = (uint8_t) (col.getG() * 255);
			scanline[ix + 2] = (uint8_t) (col.getB() * 255);
		}

		jpeg_write_scanlines(&info, &scanline, 1);
	}

	delete[] scanline;

	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	File::close(fp);
	Y_VERBOSE << getFormatName() << ": Done." << YENDL;
	return true;
}

bool JpgFormat::saveAlphaChannelOnlyToFile(const std::string &name, const Image *image)
{
	const int height = image->getHeight();
	const int width = image->getWidth();

	struct ::jpeg_compress_struct info;
	struct JpgErrorManager jerr;
	uint8_t *scanline = nullptr;

	FILE *fp = File::open(name, "wb");

	fp = File::open(name, "wb");

	if(!fp)
	{
		Y_ERROR << getFormatName() << ": Cannot open file for writing " << name << YENDL;
		return false;
	}

	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage__;
	jerr.pub_.error_exit = jpgExitOnError__;

	jpeg_create_compress(&info);
	jpeg_stdio_dest(&info, fp);

	info.image_width = width;
	info.image_height = height;
	info.in_color_space = JCS_GRAYSCALE;
	info.input_components = 1;

	jpeg_set_defaults(&info);

	info.dct_method = JDCT_FLOAT;
	jpeg_set_quality(&info, 100, TRUE);

	jpeg_start_compress(&info, TRUE);

	scanline = new uint8_t[ width ];

	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			float col = std::max(0.f, std::min(1.f, image->getColor(x, y).getA()));
			scanline[x] = (uint8_t)(col * 255);
		}

		jpeg_write_scanlines(&info, &scanline, 1);
	}

	delete [] scanline;

	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	File::close(fp);

	Y_VERBOSE << getFormatName() << ": Done." << YENDL;
	return true;
}

Image * JpgFormat::loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	jpeg_decompress_struct info;
	JpgErrorManager jerr;

	FILE *fp = File::open(name, "rb");

	Y_INFO << getFormatName() << ": Loading image \"" << name << "\"..." << YENDL;

	if(!fp)
	{
		Y_ERROR << getFormatName() << ": Cannot open file " << name << YENDL;
		return nullptr;
	}

	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage__;
	jerr.pub_.error_exit = jpgExitOnError__;

	if(setjmp(jerr.setjmp_buffer_))
	{
		jpeg_destroy_decompress(&info);
		File::close(fp);
		return nullptr;
	}

	jpeg_create_decompress(&info);
	jpeg_stdio_src(&info, fp);
	jpeg_read_header(&info, TRUE);
	jpeg_start_decompress(&info);

	bool is_gray = ((info.out_color_space == JCS_GRAYSCALE) & (info.output_components == 1));
	bool is_rgb  = ((info.out_color_space == JCS_RGB) & (info.output_components == 3));
	bool is_cmyk = ((info.out_color_space == JCS_CMYK) & (info.output_components == 4));// TODO: findout if blender's non-standard jpeg + alpha comply with this or not, the code for conversion is below

	if((!is_gray) && (!is_rgb) && (!is_cmyk))
	{
		Y_ERROR << getFormatName() << ": Unsupported color space: " << info.out_color_space << "| Color components: " << info.output_components << YENDL;
		jpeg_finish_decompress(&info);
		jpeg_destroy_decompress(&info);
		File::close(fp);
		return nullptr;
	}

	const int width = info.output_width;
	const int height = info.output_height;
	const Image::Type type = Image::getTypeFromSettings(false, grayscale_);
	Image *image = new Image(width, height, type, optimization);

	uint8_t *scanline = new uint8_t[width * info.output_components];

	int y = 0;
	int ix = 0;

	while(info.output_scanline < info.output_height)
	{
		jpeg_read_scanlines(&info, &scanline, 1);

		for(int x = 0; x < width; x++)
		{
			Rgba color;

			if(is_gray)
			{
				float colscan = scanline[x] * INV_8;
				color.set(colscan, colscan, colscan, 1.f);
			}
			else if(is_rgb)
			{
				ix = x * 3;
				color.set(scanline[ix] * INV_8,
						  scanline[ix + 1] * INV_8,
						  scanline[ix + 2] * INV_8,
				          1.f);
			}
			else if(is_cmyk)
			{
				ix = x * 4;
				float k = scanline[ix + 3] * INV_8;
				float i_k = 1.f - k;

				color.set(1.f - std::max((scanline[ix] * INV_8 * i_k) + k, 1.f),
				          1.f - std::max((scanline[ix + 1] * INV_8 * i_k) + k, 1.f),
				          1.f - std::max((scanline[ix + 2] * INV_8 * i_k) + k, 1.f),
				          1.f);
			}
			else // this is probabbly (surely) never executed, i need to research further; this assumes blender non-standard jpeg + alpha
			{
				ix = x * 4;
				float a = scanline[ix + 3] * INV_8;
				float i_a = 1.f - a;
				color.set(std::max(0.f, std::min((scanline[ix] * INV_8) - i_a, 1.f)),
						  std::max(0.f, std::min((scanline[ix + 1] * INV_8) - i_a, 1.f)),
						  std::max(0.f, std::min((scanline[ix + 2] * INV_8) - i_a, 1.f)),
						  a);
			}
			color.linearRgbFromColorSpace(color_space, gamma);
			image->setColor(x, y, color);
		}
		y++;
	}

	delete [] scanline;

	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);

	File::close(fp);

	Y_VERBOSE << getFormatName() << ": Done." << YENDL;

	return image;
}


Format *JpgFormat::factory(ParamMap &params)
{
	return new JpgFormat();
}

END_YAFARAY

#endif // HAVE_JPEG