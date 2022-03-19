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

class ColorLayers final : public Collection<LayerDef::Type, Rgba>  //Actual buffer of colors in the rendering process, one entry for each enabled layer.
{
	public:
		explicit ColorLayers(const Layers &layers);
		void setDefaultColors();
		bool isDefinedAny(const std::vector<LayerDef::Type> &types) const;
		LayerDef::Flags getFlags() const { return flags_; }

	private:
		LayerDef::Flags flags_ = LayerDef::Flags::None;
};

END_YAFARAY

#endif //YAFARAY_COLOR_LAYERS_H
