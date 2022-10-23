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

#ifndef LIBYAFARAY_FORMAT_PNG_H
#define LIBYAFARAY_FORMAT_PNG_H

#include "format/format.h"

namespace yafaray {

struct PngStructs;

class PngFormat final : public Format
{
	public:
		inline static std::string getClassName() { return "PngFormat"; }
		explicit PngFormat(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : Format(logger, param_result, param_map) { }

	private:
		[[nodiscard]] Type type() const override { return Type::Png; }
		std::string getFormatName() const override { return "PngFormat"; }
		std::unique_ptr<Image> loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) override;
		std::unique_ptr<Image> loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) override;
		bool saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply) override;
		std::unique_ptr<Image> readFromStructs(const PngStructs &png_structs, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma, const std::string &filename);
		bool fillReadStructs(uint8_t *sig, const PngStructs &png_structs);
		bool fillWriteStructs(std::FILE *fp, unsigned int color_type, const PngStructs &png_structs, const Image *image);
};

} //namespace yafaray

#endif // LIBYAFARAY_FORMAT_PNG_H