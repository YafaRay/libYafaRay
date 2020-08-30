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

#include <core_api/imagehandler.h>
#include <core_api/logging.h>
#include <core_api/session.h>
#include <core_api/environment.h>
#include <core_api/params.h>
#include <utilities/math_utils.h>
#include <core_api/file.h>

#include <png.h>

extern "C"
{
#include <setjmp.h>
}

#include <cstdio>

#include "pngUtils.h"

BEGIN_YAFRAY

class PngHandler: public ImageHandler
{
	public:
		PngHandler();
		~PngHandler();
		bool loadFromFile(const std::string &name);
		bool loadFromMemory(const YByte_t *data, size_t size);
		bool saveToFile(const std::string &name, int img_index = 0);
		static ImageHandler *factory(ParamMap &params, RenderEnvironment &render);
	private:
		void readFromStructs(png_structp png_ptr, png_infop info_ptr);
		bool fillReadStructs(YByte_t *sig, png_structp &png_ptr, png_infop &info_ptr);
		bool fillWriteStructs(FILE *fp, unsigned int rgbype, png_structp &png_ptr, png_infop &info_ptr, int img_index);
};

PngHandler::PngHandler()
{
	has_alpha_ = false;
	multi_layer_ = false;

	handler_name_ = "PNGHandler";
}

PngHandler::~PngHandler()
{
	clearImgBuffers();
}

bool PngHandler::saveToFile(const std::string &name, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	std::string name_without_tmp = name;
	name_without_tmp.erase(name_without_tmp.length() - 4);
	if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") RGB" << (has_alpha_ ? "A" : "") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
	else Y_INFO << handler_name_ << ": Saving RGB" << (has_alpha_ ? "A" : "") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;

	png_structp png_ptr;
	png_infop info_ptr;
	int channels;
	png_bytep *row_pointers = nullptr;

	FILE *fp = File::open(name, "wb");

	if(!fp)
	{
		Y_ERROR << handler_name_ << ": Cannot open file " << name << YENDL;
		return false;
	}

	if(!fillWriteStructs(fp, (has_alpha_) ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB, png_ptr, info_ptr, img_index))
	{
		File::close(fp);
		return false;
	}

	row_pointers = new png_bytep[h];

	channels = 3;

	if(has_alpha_) channels++;

	for(int i = 0; i < h; i++)
	{
		row_pointers[i] = new YByte_t[w * channels ];
	}

	//The denoise functionality will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV
	if(denoise_)
	{
		ImageBuffer denoised_buffer = img_buffer_.at(img_index)->getDenoisedLdrBuffer(denoise_hcol_, denoise_hlum_, denoise_mix_);
		for(int y = 0; y < h; y++)
		{
			for(int x = 0; x < w; x++)
			{
				Rgba color = denoised_buffer.getColor(x, y);
				color.clampRgba01();

				int i = x * channels;

				row_pointers[y][i]   = (YByte_t)(color.getR() * 255.f);
				row_pointers[y][i + 1] = (YByte_t)(color.getG() * 255.f);
				row_pointers[y][i + 2] = (YByte_t)(color.getB() * 255.f);
				if(has_alpha_) row_pointers[y][i + 3] = (YByte_t)(color.getA() * 255.f);
			}
		}
	}
	else
#endif //HAVE_OPENCV
	{
		for(int y = 0; y < h; y++)
		{
			for(int x = 0; x < w; x++)
			{
				Rgba color = img_buffer_.at(img_index)->getColor(x, y);
				color.clampRgba01();

				int i = x * channels;

				row_pointers[y][i] = (YByte_t)(color.getR() * 255.f);
				row_pointers[y][i + 1] = (YByte_t)(color.getG() * 255.f);
				row_pointers[y][i + 2] = (YByte_t)(color.getB() * 255.f);
				if(has_alpha_) row_pointers[y][i + 3] = (YByte_t)(color.getA() * 255.f);
			}
		}
	}

	png_write_image(png_ptr, row_pointers);

	png_write_end(png_ptr, nullptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	File::close(fp);

	// cleanup:
	for(int i = 0; i < h; i++)
	{
		delete [] row_pointers[i];
	}

	delete[] row_pointers;

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

bool PngHandler::loadFromFile(const std::string &name)
{
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	FILE *fp = File::open(name, "rb");

	Y_INFO << handler_name_ << ": Loading image \"" << name << "\"..." << YENDL;

	if(!fp)
	{
		Y_ERROR << handler_name_ << ": Cannot open file " << name << YENDL;
		return false;
	}

	YByte_t signature[8];

	if(fread(signature, 1, 8, fp) != 8)
	{
		Y_ERROR << handler_name_ << ": EOF found or error reading image file while reading PNG signature." << YENDL;
		return false;
	}

	if(!fillReadStructs(signature, png_ptr, info_ptr))
	{
		File::close(fp);
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_sig_bytes(png_ptr, 8);

	readFromStructs(png_ptr, info_ptr);

	File::close(fp);

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}
bool PngHandler::loadFromMemory(const YByte_t *data, size_t size)
{
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	PngDataReader *reader = new PngDataReader(data, size);

	YByte_t signature[8];

	if(reader->read(signature, 8) < 8)
	{
		Y_ERROR << handler_name_ << ": EOF found on image data while reading PNG signature." << YENDL;
		return false;
	}

	if(!fillReadStructs(signature, png_ptr, info_ptr))
	{
		delete reader;
		return false;
	}

	png_set_read_fn(png_ptr, (void *) reader, readFromMem__);

	png_set_sig_bytes(png_ptr, 8);

	readFromStructs(png_ptr, info_ptr);

	delete reader;

	return true;
}

bool PngHandler::fillReadStructs(YByte_t *sig, png_structp &png_ptr, png_infop &info_ptr)
{
	if(png_sig_cmp(sig, 0, 8))
	{
		Y_ERROR << handler_name_ << ": Data is not from a PNG image!" << YENDL;
		return false;
	}

	if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)))
	{
		Y_ERROR << handler_name_ << ": Allocation of png struct failed!" << YENDL;
		return false;
	}

	if(!(info_ptr = png_create_info_struct(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		Y_ERROR << handler_name_ << ": Allocation of png info failed!" << YENDL;
		return false;
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		Y_ERROR << handler_name_ << ": Long jump triggered error!" << YENDL;
		return false;
	}

	return true;
}

bool PngHandler::fillWriteStructs(FILE *fp, unsigned int rgbype, png_structp &png_ptr, png_infop &info_ptr, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	if(!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)))
	{
		Y_ERROR << handler_name_ << ": Allocation of png struct failed!" << YENDL;
		return false;
	}

	if(!(info_ptr = png_create_info_struct(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		Y_ERROR << handler_name_ << ": Allocation of png info failed!" << YENDL;
		return false;
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		Y_ERROR << handler_name_ << ": Long jump triggered error!" << YENDL;
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, (png_uint_32) w, (png_uint_32) h,
				 8, rgbype, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	return true;
}

void PngHandler::readFromStructs(png_structp png_ptr, png_infop info_ptr)
{
	png_uint_32 w, h;

	int bit_depth, rgbype;

	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &rgbype, nullptr, nullptr, nullptr);

	int num_chan = png_get_channels(png_ptr, info_ptr);

	switch(rgbype)
	{
		case PNG_COLOR_TYPE_RGB:
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			has_alpha_ = true;
			break;

		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(png_ptr);
			if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) num_chan = 4;
			else num_chan = 3;
			break;

		case PNG_COLOR_TYPE_GRAY:
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			if(bit_depth < 8)
			{
				png_set_gray_to_rgb(png_ptr);
				bit_depth = 8;
			}
			break;

		default:
			Y_ERROR << handler_name_ << ": PNG type is not supported!" << YENDL;
			longjmp(png_jmpbuf(png_ptr), 1);
	}

	// yes i know w and h are unsigned ints and the value can be much bigger, i'll think if a different memeber var is really needed since...
	// an image of 4,294,967,295 (max value of unsigned int) pixels on one side?
	// even 2,147,483,647 (max signed int positive value) pixels on one side is purpostrous
	// at 1 channel, 8 bits per channel and the other side of 1 pixel wide the resulting image uses 2gb of memory

	width_ = (int)w;
	height_ = (int)h;

	clearImgBuffers();

	int n_channels = num_chan;
	if(grayscale_) n_channels = 1;
	else if(has_alpha_) n_channels = 4;

	img_buffer_.push_back(new ImageBuffer(w, h, n_channels, getTextureOptimization()));

	png_bytepp row_pointers = new png_bytep[height_];

	int bit_mult = 1;
	if(bit_depth == 16) bit_mult = 2;

	for(int i = 0; i < height_; i++)
	{
		row_pointers[i] = new YByte_t[width_ * num_chan * bit_mult ];
	}

	png_read_image(png_ptr, row_pointers);

	float divisor = 1.f;
	if(bit_depth == 8) divisor = INV_8;
	else if(bit_depth == 16) divisor = INV_16;

	for(int x = 0; x < width_; x++)
	{
		for(int y = 0; y < height_; y++)
		{
			Rgba color;

			int i = x * num_chan * bit_mult;
			float c = 0.f;

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
						c = row_pointers[y][i] * divisor;
						color.set(c, c, c, row_pointers[y][i + 1] * divisor);
						break;
					case 1:
						c = row_pointers[y][i] * divisor;
						color.set(c, c, c, 1.f);
						break;
				}
			}
			else
			{
				switch(num_chan)
				{
					case 4:
						color.set((YWord_t)((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor,
								  (YWord_t)((row_pointers[y][i + 2] << 8) | row_pointers[y][i + 3]) * divisor,
								  (YWord_t)((row_pointers[y][i + 4] << 8) | row_pointers[y][i + 5]) * divisor,
								  (YWord_t)((row_pointers[y][i + 6] << 8) | row_pointers[y][i + 7]) * divisor);
						break;
					case 3:
						color.set((YWord_t)((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor,
								  (YWord_t)((row_pointers[y][i + 2] << 8) | row_pointers[y][i + 3]) * divisor,
								  (YWord_t)((row_pointers[y][i + 4] << 8) | row_pointers[y][i + 5]) * divisor,
						          1.f);
						break;
					case 2:
						c = (YWord_t)((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor;
						color.set(c, c, c, (YWord_t)((row_pointers[y][i + 2] << 8) | row_pointers[y][i + 3]) * divisor);
						break;
					case 1:
						c = (YWord_t)((row_pointers[y][i] << 8) | row_pointers[y][i + 1]) * divisor;
						color.set(c, c, c, 1.f);
						break;
				}
			}

			img_buffer_.at(0)->setColor(x, y, color, color_space_, gamma_);
		}
	}

	png_read_end(png_ptr, info_ptr);

	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

	// cleanup:
	for(int i = 0; i < height_; i++)
	{
		delete [] row_pointers[i];
	}

	delete[] row_pointers;
}

ImageHandler *PngHandler::factory(ParamMap &params, RenderEnvironment &render)
{
	int width = 0;
	int height = 0;
	bool with_alpha = false;
	bool for_output = true;
	bool img_grayscale = false;
	bool denoise_enabled = false;
	int denoise_h_lum = 3;
	int denoise_h_col = 3;
	float denoise_mix = 0.8f;

	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", with_alpha);
	params.getParam("for_output", for_output);
	params.getParam("denoiseEnabled", denoise_enabled);
	params.getParam("denoiseHLum", denoise_h_lum);
	params.getParam("denoiseHCol", denoise_h_col);
	params.getParam("denoiseMix", denoise_mix);
	params.getParam("img_grayscale", img_grayscale);

	Y_DEBUG << "denoiseEnabled=" << denoise_enabled << " denoiseHLum=" << denoise_h_lum << " denoiseHCol=" << denoise_h_col << YENDL;

	ImageHandler *ih = new PngHandler();

	if(for_output)
	{
		if(logger__.getUseParamsBadge()) height += logger__.getBadgeHeight();
		ih->initForOutput(width, height, render.getRenderPasses(), denoise_enabled, denoise_h_lum, denoise_h_col, denoise_mix, with_alpha, false, img_grayscale);
	}

	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerImageHandler("png", "png", "PNG [Portable Network Graphics]", PngHandler::factory);
	}

}

END_YAFRAY
