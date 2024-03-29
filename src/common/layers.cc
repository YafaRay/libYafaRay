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

#include "common/layers.h"
#include "common/param.h"
#include "common/logger.h"
#include <sstream>

BEGIN_YAFARAY

bool Layers::isDefinedAny(const std::vector<Layer::Type> &types) const
{
	for(const auto &type : types)
	{
		if(isDefined(type)) return true;
	}
	return false;
}

const Layers Layers::getLayersWithImages() const
{
	Layers result;
	for(const auto &it : items_)
	{
		if(it.second.hasInternalImage()) result.set(it.first, it.second);
	}
	return result;
}

const Layers Layers::getLayersWithExportedImages() const
{
	Layers result;
	for(const auto &it : items_)
	{
		if(it.second.isExported()) result.set(it.first, it.second);
	}
	return result;
}

std::string Layers::printExportedTable() const
{
	std::stringstream ss;
	for(const auto &it : items_)
	{
		const Layer &layer = it.second;
		if(layer.isExported())
		{
			ss << layer.getExportedImageName() << '\t' << layer.getTypeName() << '\t' << layer.getExportedImageTypeNameShort() << '\t' << layer.getNumExportedChannels() << '\t' << layer.getExportedImageTypeNameLong() << '\n';
		}
	}
	return ss.str();
}

END_YAFARAY
