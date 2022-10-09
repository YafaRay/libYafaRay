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

#ifndef YAFARAY_VISIBILITY_H
#define YAFARAY_VISIBILITY_H

#include "common/enum.h"
#include "common/enum_map.h"
#include <string>

namespace yafaray {

struct Visibility : public Enum<Visibility>
{
	using Enum::Enum;
	enum : ValueType_t { None = 0, Visible = 1 << 0, CastsShadows = 1 << 1, Normal = Visible | CastsShadows };
	inline static const EnumMap<ValueType_t> map_{{
			{"normal", Normal, ""},
			{"invisible", None, ""},
			{"shadow_only", CastsShadows, ""},
			{"no_shadows", Visible, ""},
		}};
};

} //namespace yafaray

#endif //YAFARAY_VISIBILITY_H
