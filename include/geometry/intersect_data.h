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

#ifndef YAFARAY_INTERSECT_DATA_H
#define YAFARAY_INTERSECT_DATA_H

#include "common/yafaray_common.h"
#include "geometry/uv.h"
#include <algorithm>

BEGIN_YAFARAY

class IntersectData final
{
	public:
		IntersectData() = default;
		IntersectData(const IntersectData &intersect_data) = default;
		IntersectData(IntersectData &&intersect_data) = default;
		IntersectData &operator=(const IntersectData &intersect_data) = default;
		IntersectData &operator=(IntersectData &&intersect_data) = default;
		explicit IntersectData(float t_hit) : t_hit_{t_hit}, uv_{Uv<float>{0.f, 0.f}}, time_{0.f} { }
		IntersectData(float t_hit, Uv<float> &&uv, float time) : t_hit_{t_hit}, uv_{std::move(uv)}, time_{time} { }
		bool isHit() const { return t_hit_ >= 0.f; }
		float tHit() const { return t_hit_; }
		Uv<float> uv() const { return uv_; }
		float time() const { return time_; }
		void setNoHit() { t_hit_ = -1.f; }

	private:
		float t_hit_ = -1.f;
		Uv<float> uv_;
		float time_;
};

END_YAFARAY

#endif //YAFARAY_INTERSECT_DATA_H
