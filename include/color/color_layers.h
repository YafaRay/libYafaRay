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

#ifndef YAFARAY_COLOR_LAYERS_H
#define YAFARAY_COLOR_LAYERS_H

#include "common/layers.h"
#include "color/color.h"
#include "common/flags.h"

BEGIN_YAFARAY

struct ColorLayer final
{
	ColorLayer() = default;
	ColorLayer(const Layer::Type &layer_type) : color_(Layer::getDefaultColor(layer_type)), layer_type_(layer_type) { }
	Rgba color_;
	Layer::Type layer_type_;
};

class ColorLayers final : public Collection<Layer::Type, ColorLayer>  //Actual buffer of colors in the rendering process, one entry for each enabled layer.
{
	public:
		ColorLayers(const Layers &layers);
		void setDefaultColors();
		void setLayer(const Layer::Type key, const ColorLayer &layer) { flags_ |= Layer::getFlags(key); set(key, layer); };
		bool isDefinedAny(const std::vector<Layer::Type> &types) const;
		MaskParams getMaskParams() const { return mask_params_; }
		Layer::Flags getFlags() const { return flags_; }

	private:
		Layer::Flags flags_;
		const MaskParams &mask_params_;
};

END_YAFARAY

#endif //YAFARAY_COLOR_LAYERS_H
