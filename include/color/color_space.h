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
 */

#ifndef LIBYAFARAY_COLOR_SPACE_H
#define LIBYAFARAY_COLOR_SPACE_H

#include "common/enum.h"
#include "common/enum_map.h"

namespace yafaray {

struct ColorSpace : public Enum<ColorSpace>
{
	using Enum::Enum; using Enum::operator=; using Enum::operator==;
	enum : decltype(type())
	{
		RawManualGamma	= 1,
		LinearRgb		= 2,
		Srgb			= 3,
		XyzD65			= 4
	};
	inline static const EnumMap<decltype(type())> map_{{
			{"Raw_Manual_Gamma", RawManualGamma, ""},
			{"LinearRGB", LinearRgb, ""},
			{"sRGB", Srgb, ""},
			{"XYZ", XyzD65, ""},
		}};
};

} //namespace yafaray

#endif //LIBYAFARAY_COLOR_SPACE_H
