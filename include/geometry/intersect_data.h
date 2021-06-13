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

#include "public_api/yafaray_conf.h"

BEGIN_YAFARAY

struct IntersectData
{
	void setIntersectData(const IntersectData &intersect_data);
	bool hit_ = false;
	float t_hit_;
	float barycentric_u_;
	float barycentric_v_;
	float barycentric_w_;
	float time_;
};

inline void IntersectData::setIntersectData(const IntersectData &intersect_data)
{
	hit_ = intersect_data.hit_;
	if(hit_)
	{
		t_hit_ = intersect_data.t_hit_;
		barycentric_u_ = intersect_data.barycentric_u_;
		barycentric_v_ = intersect_data.barycentric_v_;
		barycentric_w_ = intersect_data.barycentric_w_;
		time_ = intersect_data.time_;
	}
}


END_YAFARAY

#endif //YAFARAY_INTERSECT_DATA_H
