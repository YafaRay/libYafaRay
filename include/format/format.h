#pragma once
/****************************************************************************
 *
 *      imagehandler.h: image load and save abstract class
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

#ifndef YAFARAY_FORMAT_H
#define YAFARAY_FORMAT_H

#include "image/image.h"
#include "image/image_buffers.h"
#include <limits>

namespace yafaray {

class ImageLayers;
class ImageLayer;
class ParamMap;
class Scene;
enum class ColorSpace : unsigned char;

class Format
{
	public:
		static Format *factory(Logger &logger, const ParamMap &params);
		explicit Format(Logger &logger) : logger_(logger) { }
		virtual ~Format() = default;
		virtual Image *loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma) = 0;
		virtual Image *loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma);
		virtual bool saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply) = 0;
		virtual bool saveAlphaChannelOnlyToFile(const std::string &name, const ImageLayer &image_layer) { return false; }
		virtual bool saveToFileMultiChannel(const std::string &name, const ImageLayers &image_layers, ColorSpace color_space, float gamma, bool alpha_premultiply) { return false; };
		virtual bool isHdr() const { return false; }
		virtual bool supportsMultiLayer() const { return false; }
		virtual bool supportsAlpha() const { return true; }
		virtual std::string getFormatName() const { return ""; }
		void setGrayScaleSetting(bool grayscale) { grayscale_ = grayscale; }

	protected:
		bool grayscale_ = false; //!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
		Logger &logger_;
		static constexpr inline double inv_31_ = 1.0 / 31.0;
		static constexpr inline double inv_max_8_bit_ = 1.0 / static_cast<double>(std::numeric_limits<uint8_t>::max());
		static constexpr inline double inv_max_16_bit_ = 1.0 / static_cast<double>(std::numeric_limits<uint16_t>::max());
};

} //namespace yafaray

#endif // YAFARAY_IMAGE_H
