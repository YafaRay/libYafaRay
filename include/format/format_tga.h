#pragma once
/****************************************************************************
 *
 *      imagehandler_tga.h: Truevision TGA format handler
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

#ifndef YAFARAY_FORMAT_TGA_H
#define YAFARAY_FORMAT_TGA_H

#include "format.h"

BEGIN_YAFARAY

class TgaFormat;
struct TgaHeader;
class RgbAlpha;

typedef Rgba (TgaFormat::*ColorProcessor_t)(void *data);

class TgaFormat final : public Format
{
	public:
		TgaFormat(Logger &logger) : Format(logger) { }

	private:
		std::string getFormatName() const override { return "TgaFormat"; }
		Image * loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) override;
		bool saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply) override;

		/*! Image data reading template functions */
		template <class ColorType> void readColorMap(std::FILE *fp, TgaHeader &header, ColorProcessor_t cp);
		template <class ColorType> void readRleImage(std::FILE *fp, ColorProcessor_t cp, Image *image, const ColorSpace &color_space, float gamma);
		template <class ColorType> void readDirectImage(std::FILE *fp, ColorProcessor_t cp, Image *image, const ColorSpace &color_space, float gamma);

		/*! colorProcesors definitions with signature Rgba (void *)
		to be passed as pointer-to-non-static-member-functions */
		Rgba processGray8(void *data);
		Rgba processGray16(void *data);
		Rgba processColor8(void *data);
		Rgba processColor15(void *data);
		Rgba processColor16(void *data);
		Rgba processColor24(void *data);
		Rgba processColor32(void *data);

		bool precheckFile(TgaHeader &header, const std::string &name, bool &is_gray, bool &is_rle, bool &has_color_map, uint8_t &alpha_bit_depth);

		std::unique_ptr<ImageBuffer2D<RgbAlpha>> color_map_;
		size_t tot_pixels_;
		size_t min_x_, max_x_, step_x_;
		size_t min_y_, max_y_, step_y_;

};

END_YAFARAY

#endif // YAFARAY_FORMAT_TGA_H