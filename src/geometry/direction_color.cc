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

#include "geometry/direction_color.h"
#include "math/interpolation.h"

namespace yafaray {

std::unique_ptr<DirectionColor> DirectionColor::blend(std::unique_ptr<DirectionColor> direction_color_1, std::unique_ptr<DirectionColor> direction_color_2, float blend_val)
{
	if(blend_val <= 0.f) return direction_color_1;
	else if(blend_val >= 1.f) return direction_color_2;
	if(direction_color_1 && direction_color_2)
	{
		auto direction_color_blend = std::make_unique<DirectionColor>();
		direction_color_blend->col_ = math::lerp(direction_color_1->col_, direction_color_2->col_, blend_val);
		direction_color_blend->dir_ = (direction_color_1->dir_ + direction_color_2->dir_).normalize();
		return direction_color_blend;
	}
	else if(direction_color_1)
	{
		auto direction_color_blend = std::make_unique<DirectionColor>();
		direction_color_blend->col_ = math::lerp(direction_color_1->col_, Rgb{0.f}, blend_val);
		direction_color_blend->dir_ = direction_color_1->dir_.normalize();
		return direction_color_blend;
	}
	else if(direction_color_2)
	{
		auto direction_color_blend = std::make_unique<DirectionColor>();
		direction_color_blend->col_ = math::lerp(Rgb{0.f}, direction_color_2->col_, blend_val);
		direction_color_blend->dir_ = direction_color_2->dir_.normalize();
		return direction_color_blend;
	}
	return nullptr;
}

} //namespace yafaray