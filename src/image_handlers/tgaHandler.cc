/****************************************************************************
 *
 *      tgaHandler.cc: Truevision TGA format handler
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

#include "tgaUtils.h"

BEGIN_YAFRAY

class TgaHandler;

typedef Rgba (TgaHandler::*ColorProcessor_t)(void *data);

class TgaHandler: public ImageHandler
{
	public:
		TgaHandler();
		void initForInput();
		~TgaHandler();
		bool loadFromFile(const std::string &name);
		bool saveToFile(const std::string &name, int img_index = 0);
		static ImageHandler *factory(ParamMap &params, RenderEnvironment &render);

	private:
		/*! Image data reading template functions */
		template <class ColorType> void readColorMap(FILE *fp, TgaHeader &header, ColorProcessor_t cp);
		template <class ColorType> void readRleImage(FILE *fp, ColorProcessor_t cp);
		template <class ColorType> void readDirectImage(FILE *fp, ColorProcessor_t cp);

		/*! colorProcesors definitions with signature Rgba (void *)
		to be passed as pointer-to-non-static-member-functions */
		Rgba processGray8(void *data);
		Rgba processGray16(void *data);
		Rgba processColor8(void *data);
		Rgba processColor15(void *data);
		Rgba processColor16(void *data);
		Rgba processColor24(void *data);
		Rgba processColor32(void *data);

		bool precheckFile(TgaHeader &header, const std::string &name, bool &is_gray, bool &is_rle, bool &has_color_map, YByte_t &alpha_bit_depth);

		Rgba2DImage_t *color_map_;
		size_t tot_pixels_;
		size_t min_x_, max_x_, step_x_;
		size_t min_y_, max_y_, step_y_;

};

TgaHandler::TgaHandler()
{
	has_alpha_ = false;
	multi_layer_ = false;

	handler_name_ = "TGAHandler";
}

TgaHandler::~TgaHandler()
{
	clearImgBuffers();
}

bool TgaHandler::saveToFile(const std::string &name, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	std::string name_without_tmp = name;
	name_without_tmp.erase(name_without_tmp.length() - 4);
	if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") " << ((has_alpha_) ? "RGBA" : "RGB") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
	else Y_INFO << handler_name_ << ": Saving " << ((has_alpha_) ? "RGBA" : "RGB") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;

	std::string image_id = "Image rendered with YafaRay";
	TgaHeader header;
	TgaFooter footer;

	header.id_length_ = image_id.size();
	header.image_type_ = UncTrueColor;
	header.width_ = w;
	header.height_ = h;
	header.bit_depth_ = ((has_alpha_) ? 32 : 24);
	header.desc_ = TL | ((has_alpha_) ? ALPHA_8 : NO_ALPHA);

	FILE *fp = File::open(name, "wb");

	if(fp == nullptr)
		return false;
	else
	{
		fwrite(&header, sizeof(TgaHeader), 1, fp);
		fwrite(image_id.c_str(), (size_t)header.id_length_, 1, fp);

		//The denoise functionality will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV
		if(denoise_)
		{
			ImageBuffer denoised_buffer = img_buffer_.at(img_index)->getDenoisedLdrBuffer(denoise_hcol_, denoise_hlum_, denoise_mix_);
			for(int y = 0; y < h; y++)
			{
				for(int x = 0; x < w; x++)
				{
					Rgba col = denoised_buffer.getColor(x, y);
					col.clampRgba01();

					if(!has_alpha_)
					{
						TgaPixelRgb rgb;
						rgb = (Rgb) col;
						fwrite(&rgb, sizeof(TgaPixelRgb), 1, fp);
					}
					else
					{
						TgaPixelRgba rgba;
						rgba = col;
						fwrite(&rgba, sizeof(TgaPixelRgba), 1, fp);
					}
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
					Rgba col = img_buffer_.at(img_index)->getColor(x, y);
					col.clampRgba01();

					if(!has_alpha_)
					{
						TgaPixelRgb rgb;
						rgb = (Rgb) col;
						fwrite(&rgb, sizeof(TgaPixelRgb), 1, fp);
					}
					else
					{
						TgaPixelRgba rgba;
						rgba = col;
						fwrite(&rgba, sizeof(TgaPixelRgba), 1, fp);
					}
				}
			}
		}
		fwrite(&footer, sizeof(TgaFooter), 1, fp);
		File::close(fp);
	}

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

template <class ColorType> void TgaHandler::readColorMap(FILE *fp, TgaHeader &header, ColorProcessor_t cp)
{
	ColorType *color = new ColorType[header.cm_number_of_entries_];

	fread(color, sizeof(ColorType), header.cm_number_of_entries_, fp);

	for(int x = 0; x < (int)header.cm_number_of_entries_; x++)
	{
		(*color_map_)(x, 0) = (this->*cp)(&color[x]);
	}

	delete [] color;
}

template <class ColorType> void TgaHandler::readRleImage(FILE *fp, ColorProcessor_t cp)
{
	size_t y = min_y_;
	size_t x = min_x_;

	while(!feof(fp) && y != max_y_)
	{
		YByte_t pack_desc = 0;
		fread(&pack_desc, sizeof(YByte_t), 1, fp);

		bool rle_pack = (pack_desc & RLE_PACK_MASK);
		int rle_rep = (int)(pack_desc & RLE_REP_MASK) + 1;

		ColorType color;

		if(rle_pack) fread(&color, sizeof(ColorType), 1, fp);

		for(int i = 0; i < rle_rep; i++)
		{
			if(!rle_pack)  fread(&color, sizeof(ColorType), 1, fp);

			img_buffer_.at(0)->setColor(x, y, (this->*cp)(&color), color_space_, gamma_);

			x += step_x_;

			if(x == max_x_)
			{
				x = min_x_;
				y += step_y_;
			}
		}
	}
}

template <class ColorType> void TgaHandler::readDirectImage(FILE *fp, ColorProcessor_t cp)
{
	ColorType *color = new ColorType[tot_pixels_];

	fread(color, sizeof(ColorType), tot_pixels_, fp);

	size_t i = 0;

	for(size_t y = min_y_; y != max_y_; y += step_y_)
	{
		for(size_t x = min_x_; x != max_x_; x += step_x_)
		{
			img_buffer_.at(0)->setColor(x, y, (this->*cp)(&color[i]), color_space_, gamma_);
			i++;
		}
	}

	delete [] color;
}

Rgba TgaHandler::processGray8(void *data)
{
	return Rgba(*(YByte_t *)data * INV_255);
}

Rgba TgaHandler::processGray16(void *data)
{
	YWord_t color = *(YWord_t *)data;
	return Rgba(Rgb((color & GRAY_MASK_8_BIT) * INV_255),
				((color & ALPHA_GRAY_MASK_8_BIT) >> 8) * INV_255);
}

Rgba TgaHandler::processColor8(void *data)
{
	YByte_t color = *(YByte_t *)data;
	return (*color_map_)(color, 0);
}

Rgba TgaHandler::processColor15(void *data)
{
	YWord_t color = *(YWord_t *)data;
	return Rgba(((color & RED_MASK) >> 11) * INV_31,
				((color & GREEN_MASK) >> 6) * INV_31,
				((color & BLUE_MASK) >> 1) * INV_31,
				1.f);
}

Rgba TgaHandler::processColor16(void *data)
{
	YWord_t color = *(YWord_t *)data;
	return Rgba(((color & RED_MASK) >> 11) * INV_31,
				((color & GREEN_MASK) >> 6) * INV_31,
				((color & BLUE_MASK) >> 1) * INV_31,
				(has_alpha_) ? (float)(color & ALPHA_MASK) : 1.f);
}

Rgba TgaHandler::processColor24(void *data)
{
	TgaPixelRgb *color = (TgaPixelRgb *)data;
	return Rgba(color->r_ * INV_255,
				color->g_ * INV_255,
				color->b_ * INV_255,
				1.f);
}

Rgba TgaHandler::processColor32(void *data)
{
	TgaPixelRgba *color = (TgaPixelRgba *)data;
	return Rgba(color->r_ * INV_255,
				color->g_ * INV_255,
				color->b_ * INV_255,
				color->a_ * INV_255);
}

bool TgaHandler::precheckFile(TgaHeader &header, const std::string &name, bool &is_gray, bool &is_rle, bool &has_color_map, YByte_t &alpha_bit_depth)
{
	switch(header.image_type_)
	{
		case NoData:
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" has no image data!" << YENDL;
			return false;
			break;

		case UncColorMap:
			if(!header.color_map_type_)
			{
				Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" has ColorMap type and no color map embedded!" << YENDL;
				return false;
			}
			has_color_map = true;
			break;

		case UncGray:
			is_gray = true;
			break;

		case RleColorMap:
			if(!header.color_map_type_)
			{
				Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" has ColorMap type and no color map embedded!" << YENDL;
				return false;
			}
			has_color_map = true;
			is_rle = true;
			break;

		case RleGray:
			is_gray = true;
			is_rle = true;
			break;

		case RleTrueColor:
			is_rle = true;
			break;

		case UncTrueColor:
			break;
	}

	if(has_color_map)
	{
		if(header.cm_entry_bit_depth_ != 15 && header.cm_entry_bit_depth_ != 16 && header.cm_entry_bit_depth_ != 24 && header.cm_entry_bit_depth_ != 32)
		{
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" has a ColorMap bit depth not supported! (BitDepth:" << (int)header.cm_entry_bit_depth_ << ")" << YENDL;
			return false;
		}
	}

	if(is_gray)
	{
		if(header.bit_depth_ != 8 && header.bit_depth_ != 16)
		{
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" has an invalid bit depth only 8 bit depth gray images are supported" << YENDL;
			return false;
		}
		if(alpha_bit_depth != 8 && header.bit_depth_ == 16)
		{
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" an invalid alpha bit depth for 16 bit gray image" << YENDL;
			return false;
		}
	}
	else if(has_color_map)
	{
		if(header.bit_depth_ > 16)
		{
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" has an invalid bit depth only 8 and 16 bit depth indexed images are supported" << YENDL;
			return false;
		}
	}
	else
	{
		if(header.bit_depth_ != 15 && header.bit_depth_ != 16 && header.bit_depth_ != 24 && header.bit_depth_ != 32)
		{
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" has an invalid bit depth only 15/16, 24 and 32 bit depth true color images are supported (BitDepth: " << (int)header.bit_depth_ << ")" << YENDL;
			return false;
		}
		if(alpha_bit_depth != 1 && header.bit_depth_ == 16)
		{
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" an invalid alpha bit depth for 16 bit color image" << YENDL;
			return false;
		}
		if(alpha_bit_depth != 8 && header.bit_depth_ == 32)
		{
			Y_ERROR << handler_name_ << ": TGA file \"" << name << "\" an invalid alpha bit depth for 32 bit color image" << YENDL;
			return false;
		}
	}

	return true;
}

bool TgaHandler::loadFromFile(const std::string &name)
{
	FILE *fp = File::open(name, "rb");

	Y_INFO << handler_name_ << ": Loading image \"" << name << "\"..." << YENDL;

	if(!fp)
	{
		Y_ERROR << handler_name_ << ": Cannot open file " << name << YENDL;
		return false;
	}

	TgaHeader header;

	fread(&header, 1, sizeof(TgaHeader), fp);

	// Prereading checks

	YByte_t alpha_bit_depth = (YByte_t)(header.desc_ & ALPHA_BIT_DEPTH_MASK);

	width_ = header.width_;
	height_ = header.height_;
	has_alpha_ = (alpha_bit_depth != 0 || header.cm_entry_bit_depth_ == 32);

	bool is_rle = false;
	bool has_color_map = false;
	bool is_gray = false;
	bool from_top = ((header.desc_ & TOP_MASK) >> 5);
	bool from_left = ((header.desc_ & LEFT_MASK) >> 4);

	if(!precheckFile(header, name, is_gray, is_rle, has_color_map, alpha_bit_depth))
	{
		File::close(fp);
		return false;
	}

	// Jump over any image Id
	fseek(fp, header.id_length_, SEEK_CUR);

	clearImgBuffers();

	int n_channels = 3;
	if(header.cm_entry_bit_depth_ == 16 || header.cm_entry_bit_depth_ == 32 || header.bit_depth_ == 16 || header.bit_depth_ == 32) n_channels = 4;
	if(grayscale_) n_channels = 1;

	img_buffer_.push_back(new ImageBuffer(width_, height_, n_channels, getTextureOptimization()));

	color_map_ = nullptr;

	// Read the colormap if needed
	if(has_color_map)
	{
		color_map_ = new Rgba2DImage_t(header.cm_number_of_entries_, 1);

		switch(header.cm_entry_bit_depth_)
		{
			case 15:
				readColorMap<YWord_t>(fp, header, &TgaHandler::processColor15);
				break;

			case 16:
				readColorMap<YWord_t>(fp, header, &TgaHandler::processColor16);
				break;

			case 24:
				readColorMap<TgaPixelRgb>(fp, header, &TgaHandler::processColor24);
				break;

			case 32:
				readColorMap<TgaPixelRgba>(fp, header, &TgaHandler::processColor32);
				break;
		}
	}

	tot_pixels_ = width_ * height_;

	// Set the reading order to fit yafaray's image coordinates

	min_x_ = 0;
	max_x_ = width_;
	step_x_ = 1;

	min_y_ = 0;
	max_y_ = height_;
	step_y_ = 1;

	if(!from_top)
	{
		min_y_ = height_ - 1;
		max_y_ = -1;
		step_y_ = -1;
	}

	if(from_left)
	{
		min_x_ = width_ - 1;
		max_x_ = -1;
		step_x_ = -1;
	}

	// Read the image data

	if(is_rle) // RLE compressed image data
	{
		switch(header.bit_depth_)
		{
			case 8: // Indexed color using ColorMap LUT or grayscale map
				if(is_gray)readRleImage<YByte_t>(fp, &TgaHandler::processGray8);
				else readRleImage<YByte_t>(fp, &TgaHandler::processColor8);
				break;

			case 15:
				readRleImage<YWord_t>(fp, &TgaHandler::processColor15);
				break;

			case 16:
				if(is_gray) readRleImage<YWord_t>(fp, &TgaHandler::processGray16);
				else readRleImage<YWord_t>(fp, &TgaHandler::processColor16);
				break;

			case 24:
				readRleImage<TgaPixelRgb>(fp, &TgaHandler::processColor24);
				break;

			case 32:
				readRleImage<TgaPixelRgba>(fp, &TgaHandler::processColor32);
				break;
		}
	}
	else // Direct color data (uncompressed)
	{
		switch(header.bit_depth_)
		{
			case 8: // Indexed color using ColorMap LUT or grayscale map
				if(is_gray) readDirectImage<YByte_t>(fp, &TgaHandler::processGray8);
				else readDirectImage<YByte_t>(fp, &TgaHandler::processColor8);
				break;

			case 15:
				readDirectImage<YWord_t>(fp, &TgaHandler::processColor15);
				break;

			case 16:
				if(is_gray) readDirectImage<YWord_t>(fp, &TgaHandler::processGray16);
				else readDirectImage<YWord_t>(fp, &TgaHandler::processColor16);
				break;

			case 24:
				readDirectImage<TgaPixelRgb>(fp, &TgaHandler::processColor24);
				break;

			case 32:
				readDirectImage<TgaPixelRgba>(fp, &TgaHandler::processColor32);
				break;
		}
	}

	File::close(fp);
	fp = nullptr;

	if(color_map_) delete color_map_;
	color_map_ = nullptr;

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

ImageHandler *TgaHandler::factory(ParamMap &params, RenderEnvironment &render)
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

	ImageHandler *ih = new TgaHandler();

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
		render.registerImageHandler("tga", "tga tpic", "TGA [Truevision TARGA]", TgaHandler::factory);
	}

}
END_YAFRAY
