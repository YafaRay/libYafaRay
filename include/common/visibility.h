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

#include "common/flags.h"
#include <string>

namespace yafaray {

enum class VisibilityFlags : unsigned int
{
	None = 0, Visible = 1 << 0, CastsShadows = 1 << 1,
};

namespace visibility
{

inline VisibilityFlags fromString(const std::string &str)
{
	if(str == "normal") return VisibilityFlags::Visible | VisibilityFlags::CastsShadows; // NOLINT(bugprone-branch-clone)
	else if(str == "invisible") return VisibilityFlags::None;
	else if(str == "shadow_only") return VisibilityFlags::CastsShadows;
	else if(str == "no_shadows") return VisibilityFlags::Visible;
	else return VisibilityFlags::Visible | VisibilityFlags::CastsShadows;
}

inline std::string toString(VisibilityFlags visibility)
{
	if(flags::have(visibility, VisibilityFlags::Visible) && flags::have(visibility, VisibilityFlags::CastsShadows)) return "normal";
	else if(flags::have(visibility, VisibilityFlags::CastsShadows)) return "shadow_only";
	else if(flags::have(visibility, VisibilityFlags::Visible)) return "no_shadows";
	else return "invisible";
}

} //namespace visibility

} //namespace yafaray

#endif //YAFARAY_VISIBILITY_H
