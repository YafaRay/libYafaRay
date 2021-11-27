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
#include "color/color.h"
#include "common/param.h"
#include "common/logger.h"
#include <sstream>

BEGIN_YAFARAY

Rgba Layer::postProcess(const Rgba &color, LayerDef::Type layer_type, ColorSpace color_space, float gamma, bool alpha_premultiply)
{
	Rgba result{color};
	result.clampRgb0();
	if(LayerDef::applyColorSpace(layer_type)) result.colorSpaceFromLinearRgb(color_space, gamma);
	if(alpha_premultiply) result.alphaPremultiply();

	//To make sure we don't have any weird Alpha values outside the range [0.f, +1.f]
	if(result.a_ < 0.f) result.a_ = 0.f;
	else if(result.a_ > 1.f) result.a_ = 1.f;

	return result;
}

std::string Layer::print() const
{
	std::stringstream ss;
	ss << "Layer '" << getTypeName() << (isExported() ? "' (exported)" : "' (internal)");
	if(hasInternalImage()) ss << " with internal image type: '" << getImageTypeName() << "'";
	if(isExported())
	{
		ss << " with exported image type: '" << getExportedImageTypeNameLong() << "'";
		if(!getExportedImageName().empty()) ss << " and name: '" << getExportedImageName() << "'";
	}
	return ss.str();
}

END_YAFARAY
