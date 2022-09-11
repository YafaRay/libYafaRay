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

#include "format/format_jpg.h"
#include "common/logger.h"
#include "common/file.h"
#include "scene/scene.h"
#include "color/color.h"
#include "image/image_layers.h"

extern "C"
{
#include <setjmp.h>
#include <jpeglib.h>
}

namespace yafaray {

// error handlers for libJPEG,

METHODDEF(void) jpgErrorMessage_global(j_common_ptr info)
{
	char buffer[JMSG_LENGTH_MAX];
	(*info->err->format_message)(info, buffer);
	//logger_.logError("JPEG Library Error: ", buffer);
	std::cout << "JPEG Library Error: " << buffer << std::endl;
}

struct JpgErrorManager
{
	struct ::jpeg_error_mgr pub_;
	jmp_buf setjmp_buffer_;
};

// JPEG error manager pointer
typedef struct JpgErrorManager *ErrorPtr_t;

void jpgExitOnError_global(j_common_ptr info)
{
	const auto myerr = reinterpret_cast<ErrorPtr_t>(info->err);
	(*info->err->output_message)(info);
	longjmp(myerr->setjmp_buffer_, 1);
}

bool JpgFormat::saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply)
{
	std::FILE *fp = File::open(name, "wb");
	if(!fp)
	{
		logger_.logError(getFormatName(), ": Cannot open file for writing ", name);
		return false;
	}

	const int height = image_layer.image_->getHeight();
	const int width = image_layer.image_->getWidth();
	struct ::jpeg_compress_struct info;
	struct JpgErrorManager jerr;

	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage_global;
	jerr.pub_.error_exit = jpgExitOnError_global;

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

	auto *scanline = new uint8_t[width * 3];
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			const int ix = x * 3;
			Rgba col = Layer::postProcess(image_layer.image_->getColor({{x, y}}), image_layer.layer_.getType(), color_space, gamma, alpha_premultiply);
			col.clampRgba01();
			scanline[ix] = static_cast<uint8_t>(col.getR() * 255);
			scanline[ix + 1] = static_cast<uint8_t>(col.getG() * 255);
			scanline[ix + 2] = static_cast<uint8_t>(col.getB() * 255);
		}
		jpeg_write_scanlines(&info, &scanline, 1);
	}
	delete[] scanline;
	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	File::close(fp);
	if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	return true;
}

bool JpgFormat::saveAlphaChannelOnlyToFile(const std::string &name, const ImageLayer &image_layer)
{
	std::FILE *fp = File::open(name, "wb");
	if(!fp)
	{
		logger_.logError(getFormatName(), ": Cannot open file for writing ", name);
		return false;
	}
	const int height = image_layer.image_->getHeight();
	const int width = image_layer.image_->getWidth();
	struct ::jpeg_compress_struct info;
	struct JpgErrorManager jerr;
	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage_global;
	jerr.pub_.error_exit = jpgExitOnError_global;
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

	auto *scanline =  new uint8_t[ width ];
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			const float col = std::max(0.f, std::min(1.f, image_layer.image_->getColor({{x, y}}).getA()));
			scanline[x] = (uint8_t)(col * 255);
		}
		jpeg_write_scanlines(&info, &scanline, 1);
	}
	delete [] scanline;
	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	File::close(fp);
	if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	return true;
}

Image * JpgFormat::loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	std::FILE *fp = File::open(name, "rb");
	logger_.logInfo(getFormatName(), ": Loading image \"", name, "\"...");
	if(!fp)
	{
		logger_.logError(getFormatName(), ": Cannot open file ", name);
		return nullptr;
	}
	jpeg_decompress_struct info;
	JpgErrorManager jerr;

	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage_global;
	jerr.pub_.error_exit = jpgExitOnError_global;

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

	const bool is_gray = ((info.out_color_space == JCS_GRAYSCALE) & (info.output_components == 1));
	const bool is_rgb = ((info.out_color_space == JCS_RGB) & (info.output_components == 3));
	const bool is_cmyk = ((info.out_color_space == JCS_CMYK) & (info.output_components == 4));// TODO: findout if blender's non-standard jpeg + alpha comply with this or not, the code for conversion is below
	if((!is_gray) && (!is_rgb) && (!is_cmyk))
	{
		logger_.logError(getFormatName(), ": Unsupported color space: ", info.out_color_space, "| Color components: ", info.output_components);
		jpeg_finish_decompress(&info);
		jpeg_destroy_decompress(&info);
		File::close(fp);
		return nullptr;
	}
	Image::Params image_params;
	image_params.width_ = info.output_width;
	image_params.height_ = info.output_height;
	image_params.type_ = Image::getTypeFromSettings(false, grayscale_);
	image_params.image_optimization_ = optimization;
	image_params.filename_ = name;
	auto image = Image::factory(image_params);
	auto *scanline = new uint8_t[image_params.width_ * info.output_components];
	for(int y = 0; info.output_scanline < info.output_height; ++y)
	{
		jpeg_read_scanlines(&info, &scanline, 1);
		for(int x = 0; x < image_params.width_; x++)
		{
			Rgba color;
			if(is_gray)
			{
				const float colscan = scanline[x] * inv_max_8_bit_;
				color.set(colscan, colscan, colscan, 1.f);
			}
			else if(is_rgb)
			{
				const int ix = x * 3;
				color.set(scanline[ix] * inv_max_8_bit_,
						  scanline[ix + 1] * inv_max_8_bit_,
						  scanline[ix + 2] * inv_max_8_bit_,
				          1.f);
			}
			else if(is_cmyk)
			{
				const int ix = x * 4;
				const float k = scanline[ix + 3] * inv_max_8_bit_;
				const float i_k = 1.f - k;
				color.set(1.f - std::max((scanline[ix] * static_cast<float>(inv_max_8_bit_) * i_k) + k, 1.f),
				          1.f - std::max((scanline[ix + 1] * static_cast<float>(inv_max_8_bit_) * i_k) + k, 1.f),
				          1.f - std::max((scanline[ix + 2] * static_cast<float>(inv_max_8_bit_) * i_k) + k, 1.f),
				          1.f);
			}
			else // this is probabbly (surely) never executed, i need to research further; this assumes blender non-standard jpeg + alpha
			{
				const int ix = x * 4;
				const float a = scanline[ix + 3] * inv_max_8_bit_;
				const float i_a = 1.f - a;
				color.set(std::max(0.f, std::min((scanline[ix] * static_cast<float>(inv_max_8_bit_)) - i_a, 1.f)),
						  std::max(0.f, std::min((scanline[ix + 1] * static_cast<float>(inv_max_8_bit_)) - i_a, 1.f)),
						  std::max(0.f, std::min((scanline[ix + 2] * static_cast<float>(inv_max_8_bit_)) - i_a, 1.f)),
						  a);
			}
			color.linearRgbFromColorSpace(color_space, gamma);
			image->setColor({{x, y}}, color);
		}
	}
	delete [] scanline;
	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);

	File::close(fp);
	if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	return image;
}

} //namespace yafaray
