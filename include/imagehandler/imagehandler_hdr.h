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

#ifndef YAFARAY_IMAGEHANDLER_HDR_H
#define YAFARAY_IMAGEHANDLER_HDR_H

#include "imagehandler/imagehandler.h"
#include "imagehandler/imagehandler_util_hdr.h"

BEGIN_YAFARAY

class HdrHandler final : public ImageHandler
{
	public:
		static ImageHandler *factory(ParamMap &params, RenderEnvironment &render);

	private:
		HdrHandler();
		virtual ~HdrHandler() override;
		virtual bool loadFromFile(const std::string &name) override;
		virtual bool saveToFile(const std::string &name, int img_index = 0) override;
		virtual bool isHdr() const override { return true; }
		bool writeHeader(std::ofstream &file, int img_index);
		bool writeScanline(std::ofstream &file, RgbePixel *scanline, int img_index);
		bool readHeader(FILE *fp); //!< Reads file header and detects if the file is valid
		bool readOrle(FILE *fp, int y, int scan_width); //!< Reads the scanline with the original Radiance RLE schema or without compression
		bool readArle(FILE *fp, int y, int scan_width); //!< Reads a scanline with Adaptative RLE schema

		RgbeHeader header_;
};

END_YAFARAY

#endif // YAFARAY_IMAGEHANDLER_HDR_H