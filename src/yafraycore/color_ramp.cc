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
	Y_DEBUG << "modeStr='"<<modeStr<<"' interpolationStr='"<<interpolationStr<<"' hue_interpolationStr='"<<hue_interpolationStr<<"'"<<yendl;
	if(modeStr == "RGB" || modeStr == "rgb") ramp_mode = C_RAMP_RGB;
	else if(modeStr == "HSV" || modeStr == "hsv") ramp_mode = C_RAMP_HSV;
	else if(modeStr == "HSL" || modeStr == "hsl") ramp_mode = C_RAMP_HSV; //TODO: HSL not supported yet, using hsl instead
	else ramp_mode = C_RAMP_RGB;
	
	if(interpolationStr == "CONSTANT" || interpolationStr == "constant" ) ramp_interpolation = C_RAMP_CONSTANT;
	else ramp_interpolation = C_RAMP_LINEAR;	//Other modes not supported yet

	if(hue_interpolationStr == "NEAR" || hue_interpolationStr == "near") ramp_hue_interpolation = C_RAMP_HUE_NEAR;
	else if(hue_interpolationStr == "FAR" || hue_interpolationStr == "far") ramp_hue_interpolation = C_RAMP_HUE_FAR;
	else if(hue_interpolationStr == "CW" || hue_interpolationStr == "cw") ramp_hue_interpolation = C_RAMP_HUE_CLOCKWISE;
	else if(hue_interpolationStr == "CCW" || hue_interpolationStr == "ccw") ramp_hue_interpolation = C_RAMP_HUE_COUNTERCLOCKWISE;
	else ramp_hue_interpolation = C_RAMP_HUE_NEAR;
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

float interpolation_linear(float pos, float val1, float pos1, float val2, float pos2)
{
	if(pos == pos1 || pos1 == pos2) return val1;
	else if(pos == pos2) return val2;
	
	float diffVal21 = val2 - val1;
	float diffPos21 = pos2 - pos1;
	float diffPos = pos - pos1;
	
	return val1 + ((diffPos / diffPos21) * diffVal21);
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
		
		if(ramp_mode == C_RAMP_RGB)
		{
			if(ramp_interpolation == C_RAMP_CONSTANT) result = item_current->color;
			else result = interpolation_linear(pos, item_current->color, item_current->position, item_previous->color, item_previous->position);
		}
		else if(ramp_mode == C_RAMP_HSV || ramp_mode == C_RAMP_HSL) //HSL not yet supported, using HSV instead
		{
			float pos1 = item_current->position;
			float pos2 = item_previous->position;
			float h1=0.f, s1=0.f, v1=0.f, a1=0.f;
			float h2=0.f, s2=0.f, v2=0.f, a2=0.f;
			float h=0.f, s=0.f, v=0.f, a=0.f;
			
			item_current->color.rgb_to_hsv(h1, s1, v1);
			a1 = item_current->color.A;
			
			item_previous->color.rgb_to_hsv(h2, s2, v2);
			a2 = item_previous->color.A;
			
			s = interpolation_linear(pos, s1, pos1, s2, pos2);
			v = interpolation_linear(pos, v1, pos1, v2, pos2);
			a = interpolation_linear(pos, a1, pos1, a2, pos2);

			if(ramp_hue_interpolation == C_RAMP_HUE_CLOCKWISE && h1 < h2) h1 += 6.f;
			else if(ramp_hue_interpolation == C_RAMP_HUE_COUNTERCLOCKWISE && h1 > h2) h2 += 6.f;
			else if(ramp_hue_interpolation == C_RAMP_HUE_NEAR && h1 < h2 && (h2 - h1) > 3.f) h1 += 6.f;
			else if(ramp_hue_interpolation == C_RAMP_HUE_NEAR && h1 > h2 && (h2 - h1) < -3.f) h2 += 6.f;
			else if(ramp_hue_interpolation == C_RAMP_HUE_FAR && h1 < h2 && (h2 - h1) < 3.f) h1 += 6.f;
			else if(ramp_hue_interpolation == C_RAMP_HUE_FAR && h1 > h2 && (h2 - h1) > -3.f) h2 += 6.f;

			h = interpolation_linear(pos, h1, pos1, h2, pos2);

			if(h < 0.f) h += 6.f;
			else if(h > 6.f) h -= 6.f;
			result.hsv_to_rgb(h, s, v);
			result.A = a;
		}
	}
	return result;
}


__END_YAFRAY
