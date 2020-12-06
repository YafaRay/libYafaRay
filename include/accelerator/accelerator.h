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

#ifndef YAFARAY_ACCELERATOR_H
#define YAFARAY_ACCELERATOR_H

#include "constants.h"
#include "geometry/intersect_data.h"
#include "color/color.h"
#include <vector>

BEGIN_YAFARAY

class RenderData;
class Bound;
class ParamMap;
class Ray;

template<class T> struct AcceleratorIntersectData : IntersectData
{
	float t_max_ = std::numeric_limits<float>::infinity();
	const T *hit_item_ = nullptr;
};

template<class T> struct AcceleratorTsIntersectData : AcceleratorIntersectData<T>
{
	Rgb transparent_color_ {1.f};
};

template<class T> class Accelerator
{
	public:
		static Accelerator<T> *factory(const std::vector<const T *> &primitives_list, ParamMap &params);
		virtual ~Accelerator() { };
		virtual AcceleratorIntersectData<T> intersect(const Ray &ray, float t_max) const = 0;
		virtual AcceleratorIntersectData<T> intersectS(const Ray &ray, float t_max, float shadow_bias) const = 0;
		virtual AcceleratorTsIntersectData<T> intersectTs(const RenderData &render_data, const Ray &ray, int max_depth, float dist, float shadow_bias) const = 0;
		virtual Bound getBound() const = 0;
};

END_YAFARAY
#endif    //YAFARAY_ACCELERATOR_H
