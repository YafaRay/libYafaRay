/****************************************************************************
 *
 * 			color_ramp.cc: Color ramp type and operators implementation 
 *      This is part of the yafray package
 *      Copyright (C) 2016  David Bluecame
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
#include <core_api/color.h>
#include <core_api/color_ramp.h>
#include <iterator>

#include<iostream>

__BEGIN_YAFRAY

color_ramp_t::color_ramp_t(int mode, int interpolation, int hue_interpolation):ramp_mode(mode), ramp_interpolation(interpolation), ramp_hue_interpolation(hue_interpolation) {}

color_ramp_t::color_ramp_t(std::string modeStr, std::string interpolationStr, std::string hue_interpolationStr)
{
	ramp_mode = C_RAMP_RGB;	//Other modes not supported yet
	ramp_hue_interpolation = C_RAMP_HUE_NEAR;	//Other modes not supported yet

	if(interpolationStr == "CONSTANT") ramp_interpolation = C_RAMP_CONSTANT;
	else ramp_interpolation = C_RAMP_LINEAR;	//Other modes not supported yet
}

void color_ramp_t::add_item(const colorA_t & color, float position)
{
	ramp.push_back(color_ramp_item_t(color, position));
	std::sort(ramp.begin(), ramp.end());
}

colorA_t interpolation_linear(float pos, const colorA_t & col1, float pos1, const colorA_t & col2, float pos2)
{
	if(pos == pos1 || pos1 == pos2) return col1;
	else if(pos == pos2) return col2;
	
	colorA_t diffCol21 = col2 - col1;
	float diffPos21 = pos2 - pos1;
	float diffPos = pos - pos1;
	
	return col1 + ((diffPos / diffPos21) * diffCol21);
}

colorA_t color_ramp_t::get_color_interpolated(float pos) const
{
	colorA_t result;
	if(pos < 0.f) result = ramp.front().color;
	else if(pos > 1.f) result = ramp.back().color;
	else
	{
		auto item_current = std::lower_bound(ramp.begin(), ramp.end(), pos);
		auto item_previous = std::prev(item_current);
		
		if(ramp_interpolation == C_RAMP_CONSTANT) result = item_current->color;
		else result = interpolation_linear(pos, item_current->color, item_current->position, item_previous->color, item_previous->position);
	}
	return result;
}


__END_YAFRAY
