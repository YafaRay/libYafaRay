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

#include "common/yafaray_common.h"
#include "geometry/intersect_data.h"
#include "color/color.h"
#include <vector>
#include <memory>

BEGIN_YAFARAY

class RenderData;
class Bound;
class ParamMap;
class Ray;
class DiffRay;
class Primitive;
class SurfacePoint;
class Logger;

struct AcceleratorIntersectData : IntersectData
{
	float t_max_ = std::numeric_limits<float>::infinity();
	const Primitive *hit_primitive_ = nullptr;
};

struct AcceleratorTsIntersectData : AcceleratorIntersectData
{
	Rgb transparent_color_ {1.f};
};

class Accelerator
{
	public:
		static std::unique_ptr<Accelerator> factory(Logger &logger, const std::vector<const Primitive *> &primitives_list, ParamMap &params);
		Accelerator(Logger &logger) : logger_(logger) { }
		virtual ~Accelerator() = default;
		virtual AcceleratorIntersectData intersect(const Ray &ray, float t_max) const = 0;
		virtual AcceleratorIntersectData intersectS(const Ray &ray, float t_max, float shadow_bias) const = 0;
		virtual AcceleratorTsIntersectData intersectTs(RenderData &render_data, const Ray &ray, int max_depth, float dist, float shadow_bias) const = 0;
		virtual Bound getBound() const = 0;
		bool intersect(const Ray &ray, SurfacePoint &sp) const;
		bool intersect(const DiffRay &ray, SurfacePoint &sp) const;
		bool isShadowed(const RenderData &render_data, const Ray &ray, float &obj_index, float &mat_index, float shadow_bias) const;
		bool isShadowed(RenderData &render_data, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index, float shadow_bias) const;

	protected:
		Logger &logger_;
};

END_YAFARAY
#endif    //YAFARAY_ACCELERATOR_H
