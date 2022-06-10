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

#ifndef LIBYAFARAY_ACCELERATOR_INTERSECT_DATA_H
#define LIBYAFARAY_ACCELERATOR_INTERSECT_DATA_H

#include "geometry/intersect_data.h"
#include "color/color.h"

BEGIN_YAFARAY

class Primitive;

class AcceleratorIntersectData : public IntersectData
{
	public:
		AcceleratorIntersectData() = default;
		explicit AcceleratorIntersectData(bool hit, float time) { hit_ = true; time_ = time; }
		AcceleratorIntersectData(IntersectData &&intersect_data, const Primitive *hit_primitive) : IntersectData(std::move(intersect_data)), t_max_{intersect_data.tHit()}, hit_primitive_{hit_primitive} { }
		float tMax() const { return t_max_; }
		const Primitive *primitive() const { return hit_primitive_; }
		void setTMax(float t_max) { t_max_ = t_max; }

	protected:
		float t_max_ = std::numeric_limits<float>::infinity();
		const Primitive *hit_primitive_ = nullptr;
};

class AcceleratorTsIntersectData final : public AcceleratorIntersectData
{
	public:
		AcceleratorTsIntersectData() = default;
		explicit AcceleratorTsIntersectData(AcceleratorIntersectData &&intersect_data) : AcceleratorIntersectData(std::move(intersect_data)) { }
		const Rgb transparentColor() const { return transparent_color_; }
		void multiplyTransparentColor(const Rgb &color_to_multiply) { transparent_color_ *= color_to_multiply; }

	private:
		Rgb transparent_color_ {1.f};
};

END_YAFARAY

#endif //LIBYAFARAY_ACCELERATOR_INTERSECT_DATA_H
