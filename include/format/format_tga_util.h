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

#ifndef YAFARAY_FORMAT_TGA_UTIL_H
#define YAFARAY_FORMAT_TGA_UTIL_H

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
static constexpr uint8_t bl__ = 0x00;
static constexpr uint8_t br__ = 0x10;
static constexpr uint8_t tl__ = 0x20;
static constexpr uint8_t tr__ = 0x30;

static constexpr uint8_t no_alpha__ = 0x00;
static constexpr uint8_t alpha_8__ = 0x08;

// Mask defines
//15/16 bit color masking for BGRA color order on tga files
// B    |G    |R    |A
// 11111|11111|11111|1
static constexpr uint16_t blue_mask__ = 0xF800;   // 11111|00000|00000|0
static constexpr uint16_t green_mask__ = 0x07C0;  // 00000|11111|00000|0
static constexpr uint16_t red_mask__ = 0x003E;    // 00000|00000|11111|0
static constexpr uint16_t alpha_mask__ = 0x0001;  // 00000|00000|00000|1

// 8Bit gray + 8Bit alpha in 16Bit packets
static constexpr uint16_t alpha_gray_mask_8_bit__ = 0xFF00; // 11111111|00000000
static constexpr uint16_t gray_mask_8_bit__ = 0x00FF;       // 00000000|11111111

// Image description bit masks
static constexpr uint8_t alpha_bit_depth_mask__ = 0x0F;   // 00|00|1111
static constexpr uint8_t top_mask__ = 0x20;               // 00|10|0000
static constexpr uint8_t left_mask__ = 0x10;              // 00|01|0000

static constexpr uint8_t rle_pack_mask__ = 0x80;   // 1|0000000
static constexpr uint8_t rle_rep_mask__ = 0x7F;    // 0|1111111

#pragma pack(push, 1)

struct TgaHeader
{
	// General image info
	uint8_t id_length_ = 0;
	uint8_t color_map_type_ = 0; // 0 or 1 (off or on)
	uint8_t image_type_ = 0; // one of tgaImageDataTypes

	// ColorMap desc
	uint16_t cm_first_entry_index_ = 0; // Used to offset the start of the ColorMap, ie. start at entry 127 out of 256 entries
	uint16_t cm_number_of_entries_ = 0;
	uint8_t cm_entry_bit_depth_ = 0; // 15, 16, 24 or 32

	// Image descriptor
	uint16_t x_origin_ = 0; // used for Truevision TARGA display devices (anybody still has one?)
	uint16_t y_origin_ = 0; // used for Truevision TARGA display devices
	uint16_t width_ = 0; // 0-65535
	uint16_t height_ = 0; // 0-65535
	uint8_t bit_depth_ = 0; // 8, 15, 16, 24 or 32
	uint8_t desc_ = 0; // order of data from most significant bit:
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
	int ext_offset_ = 0;
	int dev_area_offset_ = 0;
	static constexpr char signature_[] = "TRUEVISION-XFILE.";
};

struct TgaPixelRgb
{
	uint8_t b_;
	uint8_t g_;
	uint8_t r_;
	TgaPixelRgb &operator = (const Rgb &c)
	{
		r_ = static_cast<uint8_t>(c.getR() * 255.f);
		g_ = static_cast<uint8_t>(c.getG() * 255.f);
		b_ = static_cast<uint8_t>(c.getB() * 255.f);
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
		r_ = static_cast<uint8_t>(c.getR() * 255.f);
		g_ = static_cast<uint8_t>(c.getG() * 255.f);
		b_ = static_cast<uint8_t>(c.getB() * 255.f);
		a_ = static_cast<uint8_t>(c.getA() * 255.f);
		return *this;
	}
};

#pragma pack(pop)

END_YAFARAY

#endif // YAFARAY_FORMAT_TGA_UTIL_H