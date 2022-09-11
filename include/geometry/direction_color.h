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

#ifndef LIBYAFARAY_DIRECTION_COLOR_H
#define LIBYAFARAY_DIRECTION_COLOR_H

#include "geometry/vector.h"
#include "color/color.h"
#include <memory>

namespace yafaray {

struct DirectionColor
{
	static std::unique_ptr<DirectionColor> blend(std::unique_ptr<DirectionColor> direction_color_1, std::unique_ptr<DirectionColor> direction_color_2, float blend_val);
	Vec3f dir_;
	Rgb col_;
};

} //namespace yafaray

#endif //LIBYAFARAY_DIRECTION_COLOR_H
