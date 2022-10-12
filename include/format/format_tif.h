/****************************************************************************
 *
 *      imagehandler_tiff.h: Tag Image File Format (TIFF) image handler
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

#ifndef YAFARAY_FORMAT_TIF_H
#define YAFARAY_FORMAT_TIF_H

#include "format.h"

namespace yafaray {

class TifFormat final : public Format
{
	public:
		inline static std::string getClassName() { return "TifFormat"; }
		explicit TifFormat(Logger &logger, ParamError &param_error, const ParamMap &param_map) : Format(logger, param_error, param_map) { }

	private:
		[[nodiscard]] Type type() const override { return Type::Tif; }
		std::string getFormatName() const override { return "TifFormat"; }
		std::unique_ptr<Image> loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) override;
		bool saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply) override;
};

} //namespace yafaray

#endif // YAFARAY_FORMAT_TIF_H