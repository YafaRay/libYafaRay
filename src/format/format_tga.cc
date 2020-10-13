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

#include "format/format_tga.h"
#include "format/format_tga_util.h"
#include "common/logger.h"
#include "common/param.h"
#include "common/file.h"
#include "image/image_buffers.h"
#include "scene/scene.h"

BEGIN_YAFARAY

bool TgaFormat::saveToFile(const std::string &name, const Image *image)
{
	int h = image->getHeight();
	int w = image->getWidth();
	std::string image_id = "Image rendered with YafaRay";
	TgaHeader header;
	TgaFooter footer;

	header.id_length_ = image_id.size();
	header.image_type_ = UncTrueColor;
	header.width_ = w;
	header.height_ = h;
	header.bit_depth_ = ((image->hasAlpha()) ? 32 : 24);
	header.desc_ = TL | ((image->hasAlpha()) ? ALPHA_8 : NO_ALPHA);

	FILE *fp = File::open(name, "wb");

	if(fp == nullptr)
		return false;
	else
	{
		fwrite(&header, sizeof(TgaHeader), 1, fp);
		fwrite(image_id.c_str(), (size_t)header.id_length_, 1, fp);

		for(int y = 0; y < h; y++)
		{
			for(int x = 0; x < w; x++)
			{
				Rgba col = image->getColor(x, y);
				col.clampRgba01();

				if(!image->hasAlpha())
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
		fwrite(&footer, sizeof(TgaFooter), 1, fp);
		File::close(fp);
	}

	Y_VERBOSE << getFormatName() << ": Done." << YENDL;

	return true;
}

template <class ColorType> void TgaFormat::readColorMap(FILE *fp, TgaHeader &header, ColorProcessor_t cp)
{
	ColorType *color = new ColorType[header.cm_number_of_entries_];
	fread(color, sizeof(ColorType), header.cm_number_of_entries_, fp);
	for(int x = 0; x < (int)header.cm_number_of_entries_; x++)
	{
		(*color_map_)(x, 0).setColor( (this->*cp)(&color[x]));
	}
	delete [] color;
}

template <class ColorType> void TgaFormat::readRleImage(FILE *fp, ColorProcessor_t cp, Image *image)
{
	size_t y = min_y_;
	size_t x = min_x_;

	while(!feof(fp) && y != max_y_)
	{
		uint8_t pack_desc = 0;
		fread(&pack_desc, sizeof(uint8_t), 1, fp);

		bool rle_pack = (pack_desc & RLE_PACK_MASK);
		int rle_rep = static_cast<int>(pack_desc & RLE_REP_MASK) + 1;

		ColorType color;

		if(rle_pack) fread(&color, sizeof(ColorType), 1, fp);

		for(int i = 0; i < rle_rep; i++)
		{
			if(!rle_pack)  fread(&color, sizeof(ColorType), 1, fp);

			image->setColor(x, y, (this->*cp)(&color));//FIXME, color_space_, gamma_);

			x += step_x_;

			if(x == max_x_)
			{
				x = min_x_;
				y += step_y_;
			}
		}
	}
}

template <class ColorType> void TgaFormat::readDirectImage(FILE *fp, ColorProcessor_t cp, Image *image)
{
	ColorType *color = new ColorType[tot_pixels_];

	fread(color, sizeof(ColorType), tot_pixels_, fp);

	size_t i = 0;
	for(size_t y = min_y_; y != max_y_; y += step_y_)
	{
		for(size_t x = min_x_; x != max_x_; x += step_x_)
		{
			image->setColor(x, y, (this->*cp)(&color[i]));//FIXME, color_space_, gamma_);
			i++;
		}
	}
	delete [] color;
}

Rgba TgaFormat::processGray8(void *data)
{
	return Rgba(*(uint8_t *)data * INV_255);
}

Rgba TgaFormat::processGray16(void *data)
{
	uint16_t color = *(uint16_t *)data;
	return Rgba(Rgb((color & GRAY_MASK_8_BIT) * INV_255),
				((color & ALPHA_GRAY_MASK_8_BIT) >> 8) * INV_255);
}

Rgba TgaFormat::processColor8(void *data)
{
	uint8_t color = *(uint8_t *)data;
	return (*color_map_)(color, 0).getColor();
}

Rgba TgaFormat::processColor15(void *data)
{
	uint16_t color = *(uint16_t *)data;
	return Rgba(((color & RED_MASK) >> 11) * INV_31,
				((color & GREEN_MASK) >> 6) * INV_31,
				((color & BLUE_MASK) >> 1) * INV_31,
				1.f);
}

Rgba TgaFormat::processColor16(void *data)
{
	uint16_t color = *(uint16_t *)data;
	return Rgba(((color & RED_MASK) >> 11) * INV_31,
				((color & GREEN_MASK) >> 6) * INV_31,
				((color & BLUE_MASK) >> 1) * INV_31,
				(float)(color & ALPHA_MASK));
}

Rgba TgaFormat::processColor24(void *data)
{
	TgaPixelRgb *color = (TgaPixelRgb *)data;
	return Rgba(color->r_ * INV_255,
				color->g_ * INV_255,
				color->b_ * INV_255,
				1.f);
}

Rgba TgaFormat::processColor32(void *data)
{
	TgaPixelRgba *color = (TgaPixelRgba *)data;
	return Rgba(color->r_ * INV_255,
				color->g_ * INV_255,
				color->b_ * INV_255,
				color->a_ * INV_255);
}

bool TgaFormat::precheckFile(TgaHeader &header, const std::string &name, bool &is_gray, bool &is_rle, bool &has_color_map, uint8_t &alpha_bit_depth)
{
	switch(header.image_type_)
	{
		case NoData:
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" has no image data!" << YENDL;
			return false;

		case UncColorMap:
			if(!header.color_map_type_)
			{
				Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" has ColorMap type and no color map embedded!" << YENDL;
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
				Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" has ColorMap type and no color map embedded!" << YENDL;
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
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" has a ColorMap bit depth not supported! (BitDepth:" << (int)header.cm_entry_bit_depth_ << ")" << YENDL;
			return false;
		}
	}

	if(is_gray)
	{
		if(header.bit_depth_ != 8 && header.bit_depth_ != 16)
		{
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" has an invalid bit depth only 8 bit depth gray images are supported" << YENDL;
			return false;
		}
		if(alpha_bit_depth != 8 && header.bit_depth_ == 16)
		{
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" an invalid alpha bit depth for 16 bit gray image" << YENDL;
			return false;
		}
	}
	else if(has_color_map)
	{
		if(header.bit_depth_ > 16)
		{
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" has an invalid bit depth only 8 and 16 bit depth indexed images are supported" << YENDL;
			return false;
		}
	}
	else
	{
		if(header.bit_depth_ != 15 && header.bit_depth_ != 16 && header.bit_depth_ != 24 && header.bit_depth_ != 32)
		{
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" has an invalid bit depth only 15/16, 24 and 32 bit depth true color images are supported (BitDepth: " << (int)header.bit_depth_ << ")" << YENDL;
			return false;
		}
		if(alpha_bit_depth != 1 && header.bit_depth_ == 16)
		{
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" an invalid alpha bit depth for 16 bit color image" << YENDL;
			return false;
		}
		if(alpha_bit_depth != 8 && header.bit_depth_ == 32)
		{
			Y_ERROR << getFormatName() << ": TGA file \"" << name << "\" an invalid alpha bit depth for 32 bit color image" << YENDL;
			return false;
		}
	}

	return true;
}

Image *TgaFormat::loadFromFile(const std::string &name, const Image::Optimization &optimization)
{
	FILE *fp = File::open(name, "rb");

	Y_INFO << getFormatName() << ": Loading image \"" << name << "\"..." << YENDL;

	if(!fp)
	{
		Y_ERROR << getFormatName() << ": Cannot open file " << name << YENDL;
		return nullptr;
	}

	TgaHeader header;

	fread(&header, 1, sizeof(TgaHeader), fp);

	// Prereading checks

	uint8_t alpha_bit_depth = (uint8_t)(header.desc_ & ALPHA_BIT_DEPTH_MASK);

	const bool has_alpha = (alpha_bit_depth != 0 || header.cm_entry_bit_depth_ == 32);

	bool is_rle = false;
	bool has_color_map = false;
	bool is_gray = false;
	bool from_top = ((header.desc_ & TOP_MASK) >> 5);
	bool from_left = ((header.desc_ & LEFT_MASK) >> 4);

	if(!precheckFile(header, name, is_gray, is_rle, has_color_map, alpha_bit_depth))
	{
		File::close(fp);
		return nullptr;
	}

	// Jump over any image Id
	fseek(fp, header.id_length_, SEEK_CUR);

	Image::Type type = Image::getTypeFromSettings(has_alpha, grayscale_);
	if(!has_alpha && !grayscale_ && (header.cm_entry_bit_depth_ == 16 || header.cm_entry_bit_depth_ == 32 || header.bit_depth_ == 16 || header.bit_depth_ == 32)) type = Image::Type::ColorAlpha;

	Image *image = new Image(header.width_, header.height_, type, optimization);

	color_map_ = nullptr;

	// Read the colormap if needed
	if(has_color_map)
	{
		color_map_ = new Rgba2DImage_t(header.cm_number_of_entries_, 1);

		switch(header.cm_entry_bit_depth_)
		{
			case 15:
				readColorMap<uint16_t>(fp, header, &TgaFormat::processColor15);
				break;

			case 16:
				readColorMap<uint16_t>(fp, header, &TgaFormat::processColor16);
				break;

			case 24:
				readColorMap<TgaPixelRgb>(fp, header, &TgaFormat::processColor24);
				break;

			case 32:
				readColorMap<TgaPixelRgba>(fp, header, &TgaFormat::processColor32);
				break;
		}
	}

	tot_pixels_ = header.width_ * header.height_;

	// Set the reading order to fit yafaray's image coordinates

	min_x_ = 0;
	max_x_ = header.width_;
	step_x_ = 1;

	min_y_ = 0;
	max_y_ = header.height_;
	step_y_ = 1;

	if(!from_top)
	{
		min_y_ = header.height_ - 1;
		max_y_ = -1;
		step_y_ = -1;
	}

	if(from_left)
	{
		min_x_ = header.width_ - 1;
		max_x_ = -1;
		step_x_ = -1;
	}

	// Read the image data

	if(is_rle) // RLE compressed image data
	{
		switch(header.bit_depth_)
		{
			case 8: // Indexed color using ColorMap LUT or grayscale map
				if(is_gray)readRleImage<uint8_t>(fp, &TgaFormat::processGray8, image);
				else readRleImage<uint8_t>(fp, &TgaFormat::processColor8, image);
				break;

			case 15:
				readRleImage<uint16_t>(fp, &TgaFormat::processColor15, image);
				break;

			case 16:
				if(is_gray) readRleImage<uint16_t>(fp, &TgaFormat::processGray16, image);
				else readRleImage<uint16_t>(fp, &TgaFormat::processColor16, image);
				break;

			case 24:
				readRleImage<TgaPixelRgb>(fp, &TgaFormat::processColor24, image);
				break;

			case 32:
				readRleImage<TgaPixelRgba>(fp, &TgaFormat::processColor32, image);
				break;
		}
	}
	else // Direct color data (uncompressed)
	{
		switch(header.bit_depth_)
		{
			case 8: // Indexed color using ColorMap LUT or grayscale map
				if(is_gray) readDirectImage<uint8_t>(fp, &TgaFormat::processGray8, image);
				else readDirectImage<uint8_t>(fp, &TgaFormat::processColor8, image);
				break;

			case 15:
				readDirectImage<uint16_t>(fp, &TgaFormat::processColor15, image);
				break;

			case 16:
				if(is_gray) readDirectImage<uint16_t>(fp, &TgaFormat::processGray16, image);
				else readDirectImage<uint16_t>(fp, &TgaFormat::processColor16, image);
				break;

			case 24:
				readDirectImage<TgaPixelRgb>(fp, &TgaFormat::processColor24, image);
				break;

			case 32:
				readDirectImage<TgaPixelRgba>(fp, &TgaFormat::processColor32, image);
				break;
		}
	}

	File::close(fp);

	if(color_map_) delete color_map_;
	color_map_ = nullptr;

	Y_VERBOSE << getFormatName() << ": Done." << YENDL;

	return image;
}

Format *TgaFormat::factory(ParamMap &params)
{
	return new TgaFormat();
}

END_YAFARAY
