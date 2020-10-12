#pragma once
/****************************************************************************
 *
 *      tgaUtils.h: Truevision TGA format utilities
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

#ifndef YAFARAY_IMAGEHANDLER_UTIL_TGA_H
#define YAFARAY_IMAGEHANDLER_UTIL_TGA_H

#include "constants.h"
#include "color/color.h"

BEGIN_YAFARAY

enum TgaImageDataType
{
	NoData = 0,
	UncColorMap,
	UncTrueColor,
	UncGray,
	RleColorMap = 9,
	RleTrueColor,
	RleGray
};

// TGA image origin corner descriptions
// B = bottom, T =top, L = left, R = right
#define BL 					0x00
#define BR 					0x10
#define TL 					0x20
#define TR 					0x30

#define NO_ALPHA 			0x00
#define ALPHA_8  			0x08

// Mask defines
//15/16 bit color masking for BGRA color order on tga files
// B    |G    |R    |A
// 11111|11111|11111|1
#define BLUE_MASK  			0xF800 // 11111|00000|00000|0
#define GREEN_MASK 			0x07C0 // 00000|11111|00000|0
#define RED_MASK   			0x003E // 00000|00000|11111|0
#define ALPHA_MASK 			0x0001 // 00000|00000|00000|1

// 8Bit gray + 8Bit alpha in 16Bit packets
#define ALPHA_GRAY_MASK_8_BIT	0xFF00 // 11111111|00000000
#define GRAY_MASK_8_BIT		0x00FF // 00000000|11111111

// Image description bit masks
#define ALPHA_BIT_DEPTH_MASK 	0x0F   // 00|00|1111
#define TOP_MASK 			0x20   // 00|10|0000
#define LEFT_MASK			0x10   // 00|01|0000

#define RLE_PACK_MASK			0x80   // 1|0000000
#define RLE_REP_MASK			0x7F   // 0|1111111

#define INV_31  0.03225806451612903226 // 1 / 31
#define INV_255 0.00392156862745098039 // 1 / 255

const char *tga_signature__ = "TRUEVISION-XFILE.\0";

#pragma pack(push, 1)

class TgaHeader
{
	public:
	TgaHeader()
	{
		id_length_ = 0;
		color_map_type_ = 0;
		image_type_ = 0;
		cm_first_entry_index_ = 0;
		cm_number_of_entries_ = 0;
		cm_entry_bit_depth_ = 0;
		x_origin_ = 0;
		y_origin_ = 0;
		width_ = 0;
		height_ = 0;
		bit_depth_ = 0;
		desc_ = 0;
	}
	// General image info

	uint8_t id_length_;
	uint8_t color_map_type_; // 0 or 1 (off or on)
	uint8_t image_type_; // one of tgaImageDataTypes

	// ColorMap desc

	uint16_t cm_first_entry_index_; // Used to offset the start of the ColorMap, ie. start at entry 127 out of 256 entries
	uint16_t cm_number_of_entries_;
	uint8_t cm_entry_bit_depth_; // 15, 16, 24 or 32

	// Image descriptor

	uint16_t x_origin_; // used for Truevision TARGA display devices (anybody still has one?)
	uint16_t y_origin_; // used for Truevision TARGA display devices
	uint16_t width_; // 0-65535
	uint16_t height_; // 0-65535
	uint8_t bit_depth_; // 8, 15, 16, 24 or 32
	uint8_t desc_; // order of data from most significant bit:
	// |--|--|----| <- 8 bits total
	//  RR BL AlBD
	// RR = 00 <- Reserved
	// BL describe the order of the image data
	// B = 0 or 1 (0 - Bottom, 1 - Top)
	// L = 0 or 1 (0 - Left, 1 - Right)
	// ie. to describe image data ordered from the top left corner use BL = 10
	// AlBD is the bitdepth of the alpha channel, if 0 no alpha channel is defined, valid range 0-8
};

struct TgaFooter
{
	TgaFooter()
	{
		ext_offset_ = 0;
		dev_area_offset_ = 0;
		for(int i = 0; i < 18; i++)
			signature_[i] = tga_signature__[i];
	}
	int ext_offset_;
	int dev_area_offset_;
	char signature_[18];
};

struct TgaPixelRgb
{
	uint8_t b_;
	uint8_t g_;
	uint8_t r_;
	TgaPixelRgb &operator = (const Rgb &c)
	{
		r_ = (uint8_t)(c.getR() * 255.f);
		g_ = (uint8_t)(c.getG() * 255.f);
		b_ = (uint8_t)(c.getB() * 255.f);
		return *this;
	}
};

struct TgaPixelRgba
{
	uint8_t b_;
	uint8_t g_;
	uint8_t r_;
	uint8_t a_;
	TgaPixelRgba &operator = (const Rgba &c)
	{
		r_ = (uint8_t)(c.getR() * 255.f);
		g_ = (uint8_t)(c.getG() * 255.f);
		b_ = (uint8_t)(c.getB() * 255.f);
		a_ = (uint8_t)(c.getA() * 255.f);
		return *this;
	}
};

#pragma pack(pop)

END_YAFARAY

#endif // YAFARAY_IMAGEHANDLER_UTIL_TGA_H