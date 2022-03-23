#pragma once
/****************************************************************************
 *
 *      imagehandler_hdr.h: Radiance hdr format handler
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 George Laskowsky Ziguilinsky (Glaskows)
 *      			   Rodrigo Placencia Vazquez (DarkTide)
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

#ifndef YAFARAY_FORMAT_HDR_H
#define YAFARAY_FORMAT_HDR_H

#include "format/format.h"
#include "format/format_hdr_util.h"

BEGIN_YAFARAY

class HdrFormat final : public Format
{
	public:
		explicit HdrFormat(Logger &logger) : Format(logger) { }

	private:
		std::string getFormatName() const override { return "HdrFormat"; }
		Image * loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) override;
		bool saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply) override;
		bool isHdr() const override { return true; }
		bool writeHeader(std::ofstream &file, const Image *image);
		bool readHeader(std::FILE *fp, int &width, int &height); //!< Reads file header and detects if the file is valid
		bool readOrle(std::FILE *fp, int y, int scan_width, Image *image, const ColorSpace &color_space, float gamma); //!< Reads the scanline with the original Radiance RLE schema or without compression
		bool readArle(std::FILE *fp, int y, int scan_width, Image *image, const ColorSpace &color_space, float gamma); //!< Reads a scanline with Adaptative RLE schema
		static bool writeScanline(std::ofstream &file, RgbePixel *scanline, const Image *image);

		RgbeHeader header_;
};

END_YAFARAY

#endif // YAFARAY_FORMAT_HDR_H