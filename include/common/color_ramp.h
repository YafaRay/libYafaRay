#pragma once
/****************************************************************************
 *
 *      color_ramp.h: Color Ramps api
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
#ifndef YAFARAY_COLOR_RAMP_H
#define YAFARAY_COLOR_RAMP_H

#include "constants.h"
#include <vector>

BEGIN_YAFARAY

class ColorRampItem final
{
	public:
		ColorRampItem(float pos) : position_(pos) { }
		ColorRampItem(const Rgba &col, float pos) : color_(col), position_(pos) {}
		bool operator < (const ColorRampItem &item) const { return (position_ < item.position_); }
		bool operator > (const ColorRampItem &item) const { return (position_ > item.position_); }
		const Rgba color() const { return color_; }
		const float position() const { return position_; }

	private:
		Rgba color_ = Rgba(0.f, 0.f, 0.f, 1.f);
		float position_ = 0.f;
};

class ColorRamp final
{
	public:
		enum Mode { Rgb, Hsv, Hsl }; //Hsl not yet supported, using Hsv instead
		enum Interpolation { Constant, Linear, Bspline, Cardinal, Ease }; //Bspline, Cardinal and Ease Not yet supported
		enum HueInterpolation {	Near, Far, Clockwise, Counterclockwise };

		ColorRamp(Mode mode, Interpolation interpolation, HueInterpolation hue_interpolation);
		ColorRamp(const std::string &mode_str, const std::string &interpolation_str, const std::string &hue_interpolation_str);
		void addItem(const Rgba &color, float position);
		Rgba getColorInterpolated(float pos) const;

	private:
		Mode mode_ = Rgb;
		Interpolation interpolation_ = Linear;
		HueInterpolation hue_interpolation_ = Near;
		std::vector<ColorRampItem> ramp_;
};

END_YAFARAY

#endif // YAFARAY_COLOR_RAMP_H
