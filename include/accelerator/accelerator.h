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

BEGIN_YAFARAY

struct RenderState;
class IntersectData;
class Rgb;
class Bound;
class ParamMap;
class Ray;

template<class T> class Accelerator
{
	public:
		static Accelerator<T> *factory(const T **primitives_list, ParamMap &params);
		virtual ~Accelerator() { };
		virtual bool intersect(const Ray &ray, float dist, T **tr, float &z, IntersectData &data) const = 0;
		virtual bool intersectS(const Ray &ray, float dist, T **tr, float shadow_bias) const = 0;
		virtual bool intersectTs(RenderState &state, const Ray &ray, int max_depth, float dist, T **tr, Rgb &filt, float shadow_bias) const = 0;
		virtual Bound getBound() const = 0;
};

END_YAFARAY
#endif    //YAFARAY_ACCELERATOR_H
