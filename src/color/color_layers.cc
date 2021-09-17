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

#include "color/color_layers.h"

BEGIN_YAFARAY

Rgba ColorLayer::postProcess(const Rgba &color, const Layer::Type &layer_type, ColorSpace color_space, float gamma, bool alpha_premultiply)
{
	Rgba result{color};
	result.clampRgb0();
	if(Layer::applyColorSpace(layer_type)) result.colorSpaceFromLinearRgb(color_space, gamma);
	if(alpha_premultiply) result.alphaPremultiply();

	//To make sure we don't have any weird Alpha values outside the range [0.f, +1.f]
	if(result.a_ < 0.f) result.a_ = 0.f;
	else if(result.a_ > 1.f) result.a_ = 1.f;

	return result;
}

ColorLayers::ColorLayers(const Layers &layers)
{
	for(const auto &layer : layers)
	{
		set(layer.first, layer.first);
		flags_ |= layer.second.getFlags();
	}
}

void ColorLayers::setDefaultColors()
{
	for(auto &item : items_)
	{
		item.second.color_ = Layer::getDefaultColor(item.first);
	}
}

bool ColorLayers::isDefinedAny(const std::vector<Layer::Type> &types) const
{
	for(const auto &it : types)
	{
		if(find(it)) return true;
	}
	return false;
}


END_YAFARAY