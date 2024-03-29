#pragma once
/****************************************************************************
 *
 *      imagehandler_exr.h: EXR format handler
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

#ifndef LIBYAFARAY_FORMAT_EXR_H
#define LIBYAFARAY_FORMAT_EXR_H

#include "format/format.h"

namespace yafaray {

class ExrFormat final : public Format
{
	public:
		inline static std::string getClassName() { return "ExrFormat"; }
		explicit ExrFormat(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : Format(logger, param_result, param_map) { }

	private:
		[[nodiscard]] Type type() const override { return Type::Exr; }
		std::string getFormatName() const override { return "ExrFormat"; }
		std::unique_ptr<Image> loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) override;
		bool saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply) override;
		bool saveToFileMultiChannel(const std::string &name, const ImageLayers &image_layers, ColorSpace color_space, float gamma, bool alpha_premultiply) override;
		bool isHdr() const override { return true; }
		bool supportsMultiLayer() const override { return true; }
};

} //namespace yafaray

#endif // LIBYAFARAY_FORMAT_EXR_H