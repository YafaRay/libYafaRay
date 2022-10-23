#pragma once
/****************************************************************************
 *
 *      imagehandler_jpg.h: Joint Photographic Experts Group (JPEG) format handler
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

#ifndef LIBYAFARAY_FORMAT_JPG_H
#define LIBYAFARAY_FORMAT_JPG_H

#include "format/format.h"

namespace yafaray {

class JpgFormat final : public Format
{
	public:
		inline static std::string getClassName() { return "JpgFormat"; }
		explicit JpgFormat(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : Format(logger, param_result, param_map) { }

	private:
		[[nodiscard]] Type type() const override { return Type::Jpg; }
		std::string getFormatName() const override { return "JpgFormat"; }
		std::unique_ptr<Image> loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) override;
		bool saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply) override;
		bool supportsAlpha() const override { return false; }
		bool saveAlphaChannelOnlyToFile(const std::string &name, const ImageLayer &image_layer) override;
};

} //namespace yafaray

#endif // LIBYAFARAY_FORMAT_JPG_H