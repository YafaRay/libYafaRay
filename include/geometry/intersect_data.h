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

BEGIN_YAFARAY

struct IntersectData
{
	IntersectData() = default;
	IntersectData(float t_hit, float u = 0.f, float v = 0.f, float time = 0.f) : hit_{true}, t_hit_{t_hit}, u_{u}, v_{v}, time_{time} { }
	IntersectData(const IntersectData &intersect_data);
	IntersectData(IntersectData &&intersect_data) = default;
	IntersectData& operator=(const IntersectData &intersect_data);
	IntersectData& operator=(IntersectData &&intersect_data) = default;
	alignas(8) bool hit_ = false;
	float t_hit_;
	float u_;
	float v_;
	float time_;
};

inline IntersectData::IntersectData(const IntersectData &intersect_data) : hit_{intersect_data.hit_}
{
	if(hit_)
	{
		t_hit_ = intersect_data.t_hit_;
		u_ = intersect_data.u_;
		v_ = intersect_data.v_;
		time_ = intersect_data.time_;
	}
}

inline IntersectData& IntersectData::operator=(const IntersectData &intersect_data)
{
	hit_ = intersect_data.hit_;
	if(hit_)
	{
		t_hit_ = intersect_data.t_hit_;
		u_ = intersect_data.u_;
		v_ = intersect_data.v_;
		time_ = intersect_data.time_;
	}
	return *this;
}


END_YAFARAY

#endif //YAFARAY_INTERSECT_DATA_H
