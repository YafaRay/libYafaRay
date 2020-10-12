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

#include "constants.h"
#include "image/image.h"

BEGIN_YAFARAY

class ImageLayers;
class ParamMap;
class Scene;

class LIBYAFARAY_EXPORT Format
{
	public:
		static Format *factory(ParamMap &params);
		virtual ~Format() { }
		virtual Image *loadFromFile(const std::string &name, const Image::Optimization &optimization) = 0;
		virtual Image *loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization) {return nullptr; }
		virtual bool saveToFile(const std::string &name, const Image *image) = 0;
		virtual bool saveAlphaChannelOnlyToFile(const std::string &name, const Image *image) { return false; }
		virtual bool saveToFileMultiChannel(const std::string &name, const ImageLayers *image_layers) { return false; };
		virtual bool isHdr() const { return false; }
		virtual bool supportsMultiLayer() const { return false; }
		virtual bool supportsAlpha() const { return true; }
		virtual std::string getFormatName() const { return ""; }
		void setGrayScaleSetting(bool grayscale) { grayscale_ = grayscale; }

	protected:
		bool grayscale_ = false; //!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
};

END_YAFARAY

#endif // YAFARAY_IMAGE_H
