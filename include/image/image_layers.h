#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_IMAGE_LAYERS_H
#define YAFARAY_IMAGE_LAYERS_H

#include <memory>
#include "common/layers.h"
#include "image/image.h"
#include "image_layers.h"

namespace yafaray {

class ImageLayer
{
	public:
		int getWidth() const;
		int getHeight() const;

		std::shared_ptr<Image> image_;
		Layer layer_;
};

class ImageLayers final : public Collection<LayerDef::Type, ImageLayer>  //Actual buffer of images in the rendering process, one entry for each enabled layer.
{
	public:
		void setColor(int x, int y, const Rgba &color, LayerDef::Type layer_type);
		void setColor(int x, int y, Rgba &&color, LayerDef::Type layer_type);
};

} //namespace yafaray

#endif //YAFARAY_IMAGE_LAYERS_H
