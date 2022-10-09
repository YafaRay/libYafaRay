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

#include <vector>

namespace yafaray {

class ColorRampItem final
{
	public:
		explicit ColorRampItem(float pos) : position_(pos) { }
		ColorRampItem(const Rgba &col, float pos) : color_(col), position_(pos) {}
		ColorRampItem(Rgba &&col, float pos) : color_(std::move(col)), position_(pos) {}
		bool operator < (const ColorRampItem &item) const { return (position_ < item.position_); }
		bool operator < (float pos) const { return (position_ < pos); }
		bool operator > (const ColorRampItem &item) const { return (position_ > item.position_); }
		Rgba color() const { return color_; }
		float position() const { return position_; }

	private:
		Rgba color_ = Rgba(0.f, 0.f, 0.f, 1.f);
		float position_ = 0.f;
};

class ColorRamp final
{
	public:
		struct Mode : public Enum<Mode>
		{
			using Enum::Enum;
			enum : ValueType_t { Rgb, Hsv, Hsl };
			inline static const EnumMap<ValueType_t> map_{{
					{"HSV", Hsv, ""},
					{"RGB", Rgb, ""},
					{"HSL", Hsl, ""},
				}};
		};
		struct Interpolation : public Enum<Interpolation>
		{
			using Enum::Enum;
			enum : ValueType_t { Constant, Linear, Bspline, Cardinal, Ease }; //Bspline, Cardinal and Ease Not yet supported
			inline static const EnumMap<ValueType_t> map_{{
					{"LINEAR", Linear, ""},
					{"CONSTANT", Constant, ""},
					//{"B_SPLINE", Bspline, "Bspline not yet supported"},
					//{"CARDINAL", Cardinal, "Cardinal not yet supported"},
					//{"EASE", Ease, "Ease not yet supported"},
				}};
		};
		struct HueInterpolation : public Enum<HueInterpolation>
		{
			using Enum::Enum;
			enum : ValueType_t { Near, Far, Clockwise, Counterclockwise };
			inline static const EnumMap<ValueType_t> map_{{
					{"NEAR", Near, ""},
					{"FAR", Far, ""},
					{"CW", Clockwise, ""},
					{"CCW", Counterclockwise, ""},
				}};
		};

		ColorRamp(Mode mode, Interpolation interpolation, HueInterpolation hue_interpolation) : mode_{mode}, interpolation_{interpolation}, hue_interpolation_{hue_interpolation} { }
		void addItem(const Rgba &color, float position);
		Rgba getColorInterpolated(float pos) const;
		const std::vector<ColorRampItem> &getRamp() const { return ramp_; }

	private:
		Mode mode_;
		Interpolation interpolation_;
		HueInterpolation hue_interpolation_;
		std::vector<ColorRampItem> ramp_;
};

} //namespace yafaray

#endif // YAFARAY_COLOR_RAMP_H
