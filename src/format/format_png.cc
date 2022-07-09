/****************************************************************************
 *
 *      pngHandler.cc: Portable Network Graphics (PNG) format handler
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

#include "format/format_png.h"
#include "color/color.h"
#include "common/logger.h"
#include "common/param.h"
#include "common/file.h"
#include "scene/scene.h"
#include "image/image_layers.h"

#include <png.h>
#include "format/format_png_util.h"

extern "C"
{
#include <setjmp.h>
}

#include <cstdio>
#include <memory>

namespace yafaray {

struct PngStructs
{
	PngStructs(png_structp &png_ptr, png_infop &info_ptr) : png_ptr_(png_ptr), info_ptr_(info_ptr) { }
	png_structp &png_ptr_;
	png_infop &info_ptr_;
};

bool PngFormat::saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply)
{
	std::FILE *fp = File::open(name, "wb");
	if(!fp)
	{
		logger_.logError(getFormatName(), ": Cannot open file ", name);
		return false;
	}
	const int h = image_layer.getHeight();
	const int w = image_layer.getWidth();
	png_structp png_ptr;
	png_infop info_ptr;
	PngStructs png_structs(png_ptr, info_ptr);
	int channels;
	if(!fillWriteStructs(fp, (image_layer.image_->hasAlpha()) ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB, png_structs, image_layer.image_.get()))
	{
		File::close(fp);
		return false;
	}
	auto row_pointers = std::unique_ptr<png_bytep[]>(new png_bytep[h]);
	channels = 3;
	if(image_layer.image_->hasAlpha()) channels++;
	for(int i = 0; i < h; i++) row_pointers[i] = new uint8_t[w * channels];

	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			Rgba color = Layer::postProcess(image_layer.image_->getColor({{x, y}}), image_layer.layer_.getType(), color_space, gamma, alpha_premultiply);
			color.clampRgba01();
			const int i = x * channels;
			row_pointers[y][i] = (uint8_t)(color.getR() * 255.f);
			row_pointers[y][i + 1] = (uint8_t)(color.getG() * 255.f);
			row_pointers[y][i + 2] = (uint8_t)(color.getB() * 255.f);
			if(image_layer.image_->hasAlpha()) row_pointers[y][i + 3] = (uint8_t)(color.getA() * 255.f);
		}
	}

	png_write_image(png_ptr, row_pointers.get());
	png_write_end(png_ptr, nullptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	File::close(fp);
	// cleanup:
	for(int i = 0; i < h; i++) delete [] row_pointers[i];
	if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	return true;
}

Image * PngFormat::loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	std::FILE *fp = File::open(name, "rb");
	logger_.logInfo(getFormatName(), ": Loading image \"", name, "\"...");
	if(!fp)
	{
		logger_.logError(getFormatName(), ": Cannot open file ", name);
		return nullptr;
	}
	uint8_t signature[8];
	if(fread(signature, 1, 8, fp) != 8)
	{
		logger_.logError(getFormatName(), ": EOF found or error reading image file while reading PNG signature.");
		File::close(fp);
		return nullptr;
	}
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	PngStructs png_structs(png_ptr, info_ptr);
	if(!fillReadStructs(signature, png_structs))
	{
		File::close(fp);
		return nullptr;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	auto image = readFromStructs(png_structs, optimization, color_space, gamma);
	File::close(fp);
	if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	return image;
}

Image * PngFormat::loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	auto reader = std::make_unique<PngDataReader>(data, size);
	uint8_t signature[8];
	if(reader->read(signature, 8) < 8)
	{
		logger_.logError(getFormatName(), ": EOF found on image data while reading PNG signature.");
		return nullptr;
	}
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	PngStructs png_structs(png_ptr, info_ptr);
	if(!fillReadStructs(signature, png_structs))
	{
		return nullptr;
	}
	png_set_read_fn(png_ptr, static_cast<void *>(reader.get()), PngDataReader::readFromMem);
	png_set_sig_bytes(png_ptr, 8);
	auto image = readFromStructs(png_structs, optimization, color_space, gamma);
	return image;
}

bool PngFormat::fillReadStructs(uint8_t *sig, const PngStructs &png_structs)
{
	if(png_sig_cmp(sig, 0, 8))
	{
		logger_.logError(getFormatName(), ": Data is not from a PNG image!");
		return false;
	}
	if(!(png_structs.png_ptr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)))
	{
		logger_.logError(getFormatName(), ": Allocation of png struct failed!");
		return false;
	}
	if(!(png_structs.info_ptr_ = png_create_info_struct(png_structs.png_ptr_)))
	{
		png_destroy_read_struct(&png_structs.png_ptr_, nullptr, nullptr);
		logger_.logError(getFormatName(), ": Allocation of png info failed!");
		return false;
	}
	if(setjmp(png_jmpbuf(png_structs.png_ptr_)))
	{
		png_destroy_read_struct(&png_structs.png_ptr_, &png_structs.info_ptr_, nullptr);
		logger_.logError(getFormatName(), ": Long jump triggered error!");
		return false;
	}
	return true;
}

bool PngFormat::fillWriteStructs(std::FILE *fp, unsigned int color_type, const PngStructs &png_structs, const Image *image)
{
	if(!(png_structs.png_ptr_ = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)))
	{
		logger_.logError(getFormatName(), ": Allocation of png struct failed!");
		return false;
	}
	if(!(png_structs.info_ptr_ = png_create_info_struct(png_structs.png_ptr_)))
	{
		png_destroy_read_struct(&png_structs.png_ptr_, nullptr, nullptr);
		logger_.logError(getFormatName(), ": Allocation of png info failed!");
		return false;
	}
	if(setjmp(png_jmpbuf(png_structs.png_ptr_)))
	{
		png_destroy_read_struct(&png_structs.png_ptr_, &png_structs.info_ptr_, nullptr);
		logger_.logError(getFormatName(), ": Long jump triggered error!");
		return false;
	}
	png_init_io(png_structs.png_ptr_, fp);
	const png_uint_32 h = image->getHeight();
	const png_uint_32 w = image->getWidth();
	png_set_IHDR(png_structs.png_ptr_, png_structs.info_ptr_, w, h,
				 8, color_type, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_structs.png_ptr_, png_structs.info_ptr_);
	return true;
}

Image * PngFormat::readFromStructs(const PngStructs &png_structs, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	png_read_info(png_structs.png_ptr_, png_structs.info_ptr_);
	png_uint_32 w, h;
	int bit_depth, color_type;
	bool has_alpha = false;
	png_get_IHDR(png_structs.png_ptr_, png_structs.info_ptr_, &w, &h, &bit_depth, &color_type, nullptr, nullptr, nullptr);
	int num_chan = png_get_channels(png_structs.png_ptr_, png_structs.info_ptr_);
	switch(color_type)
	{
		case PNG_COLOR_TYPE_RGB:
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			has_alpha = true;
			break;
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(png_structs.png_ptr_);
			if(png_get_valid(png_structs.png_ptr_, png_structs.info_ptr_, PNG_INFO_tRNS)) num_chan = 4;
			else num_chan = 3;
			break;
		case PNG_COLOR_TYPE_GRAY:
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			if(bit_depth < 8)
			{
				png_set_gray_to_rgb(png_structs.png_ptr_);
				bit_depth = 8;
			}
			break;
		default:
			logger_.logError(getFormatName(), ": PNG type is not supported!");
			longjmp(png_jmpbuf(png_structs.png_ptr_), 1);
	}
	// yes i know w and h are unsigned ints and the value can be much bigger, i'll think if a different memeber var is really needed since...
	// an image of 4,294,967,295 (max value of unsigned int) pixels on one side?
	// even 2,147,483,647 (max signed int positive value) pixels on one side is purpostrous
	// at 1 channel, 8 bits per channel and the other side of 1 pixel wide the resulting image uses 2gb of memory
	const Image::Type type = Image::getTypeFromSettings(has_alpha, (num_chan == 1 || grayscale_));
	auto image = Image::factory(logger_, {{static_cast<int>(w), static_cast<int>(h)}}, type, optimization);
	auto row_pointers = std::unique_ptr<png_bytep[]>(new png_bytep[h]);
	int bit_mult = 1;
	if(bit_depth == 16) bit_mult = 2;
	for(size_t i = 0; i < h; i++) row_pointers[i] = new uint8_t[w * num_chan * bit_mult ];
	png_read_image(png_structs.png_ptr_, row_pointers.get());
	float divisor = 1.f;
	if(bit_depth == 8) divisor = inv_max_8_bit_;
	else if(bit_depth == 16) divisor = inv_max_16_bit_;
	for(size_t x = 0; x < w; x++)
	{
		for(size_t y = 0; y < h; y++)
		{
			Rgba color;
			const int i = x * num_chan * bit_mult;
			if(bit_depth < 16)
			{
				switch(num_chan)
				{
					case 4:
						color.set(row_pointers[y][i] * divisor,
								  row_pointers[y][i + 1] * divisor,
								  row_pointers[y][i + 2] * divisor,
								  row_pointers[y][i + 3] * divisor);
						break;
					case 3:
						color.set(row_pointers[y][i] * divisor,
								  row_pointers[y][i + 1] * divisor,
								  row_pointers[y][i + 2] * divisor,
								  1.f);
						break;
					case 2:
					{
						const float c = row_pointers[y][i] * divisor;
						color.set(c, c, c, row_pointers[y][i + 1] * divisor);
						break;
					}
					case 1:
					{
						const float c = row_pointers[y][i] * divisor;
						color.set(c, c, c, 1.f);
						break;
					}
				}
			}
			else
			{
				switch(num_chan)
				{
					case 4:
						color.set(static_cast<uint16_t>((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor,
								  static_cast<uint16_t>((row_pointers[y][i + 2] << 8) | row_pointers[y][i + 3]) * divisor,
								  static_cast<uint16_t>((row_pointers[y][i + 4] << 8) | row_pointers[y][i + 5]) * divisor,
								  static_cast<uint16_t>((row_pointers[y][i + 6] << 8) | row_pointers[y][i + 7]) * divisor);
						break;
					case 3:
						color.set(static_cast<uint16_t>((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor,
								  static_cast<uint16_t>((row_pointers[y][i + 2] << 8) | row_pointers[y][i + 3]) * divisor,
								  static_cast<uint16_t>((row_pointers[y][i + 4] << 8) | row_pointers[y][i + 5]) * divisor,
						          1.f);
						break;
					case 2:
					{
						const float c = static_cast<uint16_t>((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor;
						color.set(c, c, c, static_cast<uint16_t>((row_pointers[y][i + 2] << 8) | row_pointers[y][i + 3]) * divisor);
						break;
					}
					case 1:
					{
						const float c = static_cast<uint16_t>((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor;
						color.set(c, c, c, 1.f);
						break;
					}
				}
			}
			color.linearRgbFromColorSpace(color_space, gamma);
			image->setColor({{static_cast<int>(x), static_cast<int>(y)}}, color);
		}
	}
	png_read_end(png_structs.png_ptr_, png_structs.info_ptr_);
	png_destroy_read_struct(&png_structs.png_ptr_, &png_structs.info_ptr_, nullptr);
	// cleanup:
	for(int i = 0; i < static_cast<int>(h); i++) delete [] row_pointers[i];
	return image;
}

} //namespace yafaray
