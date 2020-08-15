#pragma once
/****************************************************************************
 *
 * 			color_ramp.h: Color Ramps api
 *      This is part of the yafray package
 *		Copyright (C) 2016  David Bluecame
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
#ifndef Y_COLOR_RAMP_H
#define Y_COLOR_RAMP_H

#include <yafray_constants.h>
#include <vector>

__BEGIN_YAFRAY

enum color_ramp_mode_t
{
	C_RAMP_RGB,
	C_RAMP_HSV,
	C_RAMP_HSL	//Not yet supported, using HSV instead
};

enum color_ramp_interpolation_t
{
	C_RAMP_CONSTANT,
	C_RAMP_LINEAR,
	C_RAMP_BSPLINE,	//Not yet supported
	C_RAMP_CARDINAL,	//Not yet supported
	C_RAMP_EASE	//Not yet supported
};

enum color_ramp_hue_interpolation_t
{
	C_RAMP_HUE_NEAR,
	C_RAMP_HUE_FAR,
	C_RAMP_HUE_CLOCKWISE,
	C_RAMP_HUE_COUNTERCLOCKWISE
};

class YAFRAYCORE_EXPORT color_ramp_item_t
{
		friend class color_ramp_t;

	public:
		color_ramp_item_t(float pos) : position(pos) {}
		color_ramp_item_t(const colorA_t &col, float pos) : color(col), position(pos) {}
		bool operator < (const color_ramp_item_t &item) const {	return (position < item.position); }
		bool operator > (const color_ramp_item_t &item) const {	return (position > item.position); }

	protected:
		colorA_t color = colorA_t(0.f, 0.f, 0.f, 1.f);
		float position = 0.f;
};

class YAFRAYCORE_EXPORT color_ramp_t
{
	public:
		color_ramp_t(int mode, int interpolation, int hue_interpolation);
		color_ramp_t(std::string modeStr, std::string interpolationStr, std::string hue_interpolationStr);
		void add_item(const colorA_t &color, float position);
		colorA_t get_color_interpolated(float pos) const;

	protected:
		int ramp_mode = C_RAMP_RGB;
		int ramp_interpolation = C_RAMP_LINEAR;
		int ramp_hue_interpolation = C_RAMP_HUE_NEAR;
		std::vector<color_ramp_item_t> ramp;
};

__END_YAFRAY

#endif // Y_COLOR_RAMP_H
