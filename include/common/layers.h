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

#ifndef YAFARAY_LAYERS_H
#define YAFARAY_LAYERS_H

#include "yafaray_common.h"
#include "common/collection.h"
#include "image/image.h"
#include "common/flags.h"
#include "common/layer.h"
#include <set>
#include <map>
#include <string>
#include <vector>
#include <array>

namespace yafaray {

class Layers final : public Collection<LayerDef::Type, Layer>
{
	public:
		bool isDefined(LayerDef::Type type) const;
		bool isDefinedAny(const std::vector<LayerDef::Type> &types) const;
		Layers getLayersWithImages() const;
		Layers getLayersWithExportedImages() const;
		std::string printExportedTable() const;
};

inline bool Layers::isDefined(LayerDef::Type type) const
{
	if(type == LayerDef::Disabled) return false;
	else return find(type);
}

} //namespace yafaray

#endif // YAFARAY_LAYERS_H
