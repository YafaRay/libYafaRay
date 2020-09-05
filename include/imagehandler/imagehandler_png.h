/****************************************************************************
 *
 *      imagehandler_png.h: Portable Network Graphics (PNG) format handler
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

#ifndef YAFARAY_IMAGEHANDLER_PNG_H
#define YAFARAY_IMAGEHANDLER_PNG_H

#include "imagehandler/imagehandler.h"

BEGIN_YAFARAY

struct PngStructs;

class PngHandler final : public ImageHandler
{
	public:
		static ImageHandler *factory(ParamMap &params, RenderEnvironment &render);

	private:
		PngHandler();
		virtual ~PngHandler() override;
		virtual bool loadFromFile(const std::string &name) override;
		virtual bool loadFromMemory(const uint8_t *data, size_t size) override;
		virtual bool saveToFile(const std::string &name, int img_index = 0) override;
		void readFromStructs(const PngStructs &png_structs);
		bool fillReadStructs(uint8_t *sig, const PngStructs &png_structs);
		bool fillWriteStructs(FILE *fp, unsigned int rgbype, const PngStructs &png_structs, int img_index);
};

END_YAFARAY

#endif // YAFARAY_IMAGEHANDLER_PNG_H