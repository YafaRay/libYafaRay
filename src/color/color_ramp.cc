/****************************************************************************
 *
 *      color()ramp.cc: Color ramp type and operators implementation
 *      This is part of the libYafaRay package
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
#include "color/color.h"
#include "color/color_ramp.h"
#include "math/interpolation.h"

BEGIN_YAFARAY

ColorRamp::ColorRamp(const std::string &mode_str, const std::string &interpolation_str, const std::string &hue_interpolation_str)
{
	if(mode_str == "RGB" || mode_str == "rgb") mode_ = Rgb;
	else if(mode_str == "HSV" || mode_str == "hsv") mode_ = Hsv;
	else if(mode_str == "HSL" || mode_str == "hsl") mode_ = Hsl;
	else mode_ = Rgb;

	if(interpolation_str == "CONSTANT" || interpolation_str == "constant") interpolation_ = Constant;
	else interpolation_ = Linear;	//Other modes not supported yet

	if(hue_interpolation_str == "NEAR" || hue_interpolation_str == "near") hue_interpolation_ = Near;
	else if(hue_interpolation_str == "FAR" || hue_interpolation_str == "far") hue_interpolation_ = Far;
	else if(hue_interpolation_str == "CW" || hue_interpolation_str == "cw") hue_interpolation_ = Clockwise;
	else if(hue_interpolation_str == "CCW" || hue_interpolation_str == "ccw") hue_interpolation_ = Counterclockwise;
	else hue_interpolation_ = Near;
}

void ColorRamp::addItem(const Rgba &color, float position)
{
	ramp_.emplace_back(color, position);
	std::sort(ramp_.begin(), ramp_.end());
}

Rgba ColorRamp::getColorInterpolated(float pos) const
{
	Rgba result;
	if(pos < ramp_.front().position()) result = ramp_.front().color();
	else if(pos > ramp_.back().position()) result = ramp_.back().color();
	else
	{
		const auto item_current = std::lower_bound(ramp_.begin(), ramp_.end(), pos);
		const auto item_previous = std::prev(item_current);

		if(mode_ == Rgb)
		{
			if(interpolation_ == Constant) result = item_current->color();
			else result = math::lerpSegment(pos, item_current->color(), item_current->position(), item_previous->color(), item_previous->position());
		}
		else if(mode_ == Hsv)
		{
			const float pos_1 = item_current->position();
			const float pos_2 = item_previous->position();
			float h_1 = 0.f, s_1 = 0.f, v_1 = 0.f;
			float h_2 = 0.f, s_2 = 0.f, v_2 = 0.f;

			item_current->color().rgbToHsv(h_1, s_1, v_1);
			const float a_1 = item_current->color().a_;

			item_previous->color().rgbToHsv(h_2, s_2, v_2);
			const float a_2 = item_previous->color().a_;

			const float s = math::lerpSegment(pos, s_1, pos_1, s_2, pos_2);
			const float v = math::lerpSegment(pos, v_1, pos_1, v_2, pos_2);
			const float a = math::lerpSegment(pos, a_1, pos_1, a_2, pos_2);

			if(hue_interpolation_ == Clockwise && h_1 < h_2) h_1 += 6.f;
			else if(hue_interpolation_ == Counterclockwise && h_1 > h_2) h_2 += 6.f;
			else if(hue_interpolation_ == Near && h_1 < h_2 && (h_2 - h_1) > 3.f) h_1 += 6.f;
			else if(hue_interpolation_ == Near && h_1 > h_2 && (h_2 - h_1) < -3.f) h_2 += 6.f;
			else if(hue_interpolation_ == Far && h_1 < h_2 && (h_2 - h_1) < 3.f) h_1 += 6.f;
			else if(hue_interpolation_ == Far && h_1 > h_2 && (h_2 - h_1) > -3.f) h_2 += 6.f;

			float h = math::lerpSegment(pos, h_1, pos_1, h_2, pos_2);

			if(h < 0.f) h += 6.f;
			else if(h > 6.f) h -= 6.f;
			result.hsvToRgb(h, s, v);
			result.a_ = a;
		}
		else if(mode_ == Hsl)
		{
			const float pos_1 = item_current->position();
			const float pos_2 = item_previous->position();
			float h_1 = 0.f, s_1 = 0.f, l_1 = 0.f;
			float h_2 = 0.f, s_2 = 0.f, l_2 = 0.f;

			item_current->color().rgbToHsl(h_1, s_1, l_1);
			const float a_1 = item_current->color().a_;

			item_previous->color().rgbToHsl(h_2, s_2, l_2);
			const float a_2 = item_previous->color().a_;

			const float s = math::lerpSegment(pos, s_1, pos_1, s_2, pos_2);
			const float l = math::lerpSegment(pos, l_1, pos_1, l_2, pos_2);
			const float a = math::lerpSegment(pos, a_1, pos_1, a_2, pos_2);

			if(hue_interpolation_ == Clockwise && h_1 < h_2) h_1 += 6.f;
			else if(hue_interpolation_ == Counterclockwise && h_1 > h_2) h_2 += 6.f;
			else if(hue_interpolation_ == Near && h_1 < h_2 && (h_2 - h_1) > 3.f) h_1 += 6.f;
			else if(hue_interpolation_ == Near && h_1 > h_2 && (h_2 - h_1) < -3.f) h_2 += 6.f;
			else if(hue_interpolation_ == Far && h_1 < h_2 && (h_2 - h_1) < 3.f) h_1 += 6.f;
			else if(hue_interpolation_ == Far && h_1 > h_2 && (h_2 - h_1) > -3.f) h_2 += 6.f;

			float h = math::lerpSegment(pos, h_1, pos_1, h_2, pos_2);

			if(h < 0.f) h += 6.f;
			else if(h > 6.f) h -= 6.f;
			result.hsvToRgb(h, s, l);
			result.a_ = a;
		}
	}
	return result;
}


END_YAFARAY
