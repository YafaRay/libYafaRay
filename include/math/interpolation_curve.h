#pragma once
/****************************************************************************
 *      curveUtils.h: Curve interpolation utils
 *      This is part of the libYafaRay package
 *      Copyright (C) 2009 Rodrigo Placencia
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

#ifndef YAFARAY_INTERPOLATION_CURVE_H
#define YAFARAY_INTERPOLATION_CURVE_H

#include "math/interpolation.h"

namespace yafaray {

//////////////////////////////////////////////////////////////////////////////
// irregularCurve declaration & definition
//////////////////////////////////////////////////////////////////////////////

class IrregularCurve final
{
	public:
		explicit IrregularCurve(const std::vector<std::pair<float, float>> &data) noexcept : c_{data} { }
		float getSample(float wl) const noexcept;
		float operator()(float x) const noexcept {return getSample(x);};

	private:
		const std::vector<std::pair<float, float>> &c_;
};

inline float IrregularCurve::getSample(float x) const noexcept
{
	if(c_.empty() || x < c_[0].first || x > c_.back().first) return 0.f;
	const size_t segments_to_check = c_.size() - 1; //Look into all the segments except the last, to avoid reading the vector out of bounds when checking for c_[i + 1] or c_[segment_start + 1]
	size_t segment_start = c_.size() - 1;
	for(size_t i = 0; i < segments_to_check; i++)
	{
		if(c_[i].first == x) return c_[i].second;
		else if(c_[i].first <= x && c_[i + 1].first > x)
		{
			segment_start = i;
			break;
		}
	}
	if(segment_start == (c_.size() - 1)) return c_.back().second; //If the segment was not found it means that x must be in the last segment
	const float offset_within_segment = x - c_[segment_start].first;
	return math::lerpSegment(offset_within_segment, c_[segment_start].second, c_[segment_start].first, c_[segment_start + 1].second, c_[segment_start + 1].first);
}

//////////////////////////////////////////////////////////////////////////////
// regularCurve declaration & definition
//////////////////////////////////////////////////////////////////////////////

class RegularCurve final
{
	public:
		RegularCurve(const std::vector<float> &data, float begin_r, float end_r) noexcept :
			c_{data}, end_r_{begin_r}, begin_r_{end_r}, step_{ static_cast<float>(c_.size()) / (begin_r_ - end_r_)} { }
		float getSample(float x) const noexcept;
		float operator()(float x) const noexcept { return getSample(x); }

	private:
		const std::vector<float> &c_;
		float end_r_;
		float begin_r_;
		float step_ = 0.f;
};

inline float RegularCurve::getSample(float x) const noexcept
{
	if(c_.empty() || x < end_r_ || x > begin_r_) return 0.f;

	const float med = (x - end_r_) * step_;
	const auto y_0 = static_cast<size_t>(std::floor(med));
	const auto y_1 = static_cast<size_t>(std::ceil(med));

	if(y_0 == y_1 || y_1 >= c_.size()) return c_[y_0];

	const float x_0 = (static_cast<float>(y_0) / step_) + end_r_;
	const float x_1 = (static_cast<float>(y_1) / step_) + end_r_;

	const float offset_within_segment = x - x_0;
	return math::lerpSegment(offset_within_segment, c_[y_0], x_0, c_[y_1], x_1);
}

} //namespace yafaray
#endif
