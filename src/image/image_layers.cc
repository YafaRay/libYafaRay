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
 */

#include "image/image_layers.h"

namespace yafaray {

int ImageLayer::getWidth() const
{
	if(!image_) return 0;
	else return image_->getWidth();
}

int ImageLayer::getHeight() const
{
	if(!image_) return 0;
	else return image_->getHeight();
}

void ImageLayers::setColor(const Point2i &point, const Rgba &color, LayerDef::Type layer_type)
{
	ImageLayer *image_layer = find(layer_type);
	if(image_layer) image_layer->image_->setColor(point, color);
}

void ImageLayers::setColor(const Point2i &point, Rgba &&color, LayerDef::Type layer_type)
{
	ImageLayer *image_layer = find(layer_type);
	if(image_layer) image_layer->image_->setColor(point, std::move(color));
}

} //namespace yafaray
