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

#ifndef LIBYAFARAY_INTERSECT_DATA_H
#define LIBYAFARAY_INTERSECT_DATA_H

#include "color/color.h"
#include "geometry/uv.h"

BEGIN_YAFARAY

class Primitive;

struct IntersectData
{
	bool isHit() const { return t_hit_ > 0.f; }
	void setNoHit() { t_hit_ = 0.f; primitive_ = nullptr; }
	float t_hit_ = 0.f;
	Uv<float> uv_;
	float t_max_ = std::numeric_limits<float>::max();
	const Primitive *primitive_ = nullptr;
};

struct IntersectDataColor : IntersectData
{
	Rgb color_ {1.f};
};

END_YAFARAY

#endif //LIBYAFARAY_INTERSECT_DATA_H
