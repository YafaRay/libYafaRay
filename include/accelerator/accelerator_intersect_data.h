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

class AccelData
{
	public:
		AccelData() = default;
		AccelData(const AccelData &accel_data) = default;
		AccelData(AccelData &&accel_data) = default;
		AccelData &operator=(const AccelData &accel_data) = default;
		AccelData &operator=(AccelData &&accel_data) = default;
		float tMax() const { return t_max_; }
		const Primitive *primitive() const { return hit_primitive_; }
		void setPrimitive(const Primitive *primitive) { hit_primitive_ = primitive; }
		void setTMax(float t_max) { t_max_ = t_max; }
		bool isHit() const { return intersect_data_.isHit(); }
		float tHit() const { return intersect_data_.tHit(); }
		Uv<float> uv() const { return intersect_data_.uv(); }
		void setNoHit() { intersect_data_.setNoHit(); }
		IntersectData intersectData() const { return intersect_data_; }
		void setIntersectData(IntersectData &&intersect_data) { setTMax(intersect_data.tHit()); intersect_data_ = std::move(intersect_data); }

	protected:
		IntersectData intersect_data_;
		float t_max_ = std::numeric_limits<float>::infinity();
		const Primitive *hit_primitive_ = nullptr;
};

class AccelTsData
{
	public:
		AccelTsData() = default;
		AccelTsData(const AccelTsData &accel_ts_data) = default;
		AccelTsData(AccelTsData &&accel_ts_data) = default;
		AccelTsData &operator=(const AccelTsData &accel_ts_data) = default;
		AccelTsData &operator=(AccelTsData &&accel_ts_data) = default;
		AccelTsData(AccelData &&accel_data, Rgb &&transparent_color) : accel_data_{std::move(accel_data)}, transparent_color_{std::move(transparent_color)} { }
		Rgb transparentColor() const { return transparent_color_; }
		void multiplyTransparentColor(const Rgb &color_to_multiply) { transparent_color_ *= color_to_multiply; }
		float tMax() const { return accel_data_.tMax(); }
		void setTMax(float t_max) { accel_data_.setTMax(t_max); }
		const Primitive *primitive() const { return accel_data_.primitive(); }
		void setPrimitive(const Primitive *primitive) { accel_data_.setPrimitive(primitive); }
		bool isHit() const { return accel_data_.isHit(); }
		float tHit() const { return accel_data_.tHit(); }
		Uv<float> uv() const { return accel_data_.uv(); }
		void setNoHit() { accel_data_.setNoHit(); }
		IntersectData intersectData() const { return accel_data_.intersectData(); }
		void setIntersectData(IntersectData &&intersect_data) { accel_data_.setIntersectData(std::move(intersect_data)); }

	private:
		AccelData accel_data_;
		Rgb transparent_color_ {1.f};
};

END_YAFARAY

#endif //LIBYAFARAY_ACCELERATOR_INTERSECT_DATA_H
