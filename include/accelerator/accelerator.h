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
#include "camera/camera.h"
#include <vector>
#include <memory>
#include <limits>
#include <set>

BEGIN_YAFARAY

class Bound;
class ParamMap;
class Ray;
class Ray;
class Primitive;
class SurfacePoint;
class Logger;
class MaterialData;

struct AcceleratorIntersectData : IntersectData
{
	AcceleratorIntersectData() = default;
	explicit AcceleratorIntersectData(bool hit) { hit_ = true; }
	AcceleratorIntersectData(IntersectData &&intersect_data, const Primitive *hit_primitive) : IntersectData(std::move(intersect_data)), t_max_{intersect_data.t_hit_}, hit_primitive_{hit_primitive} { }
	float t_max_ = std::numeric_limits<float>::infinity();
	const Primitive *hit_primitive_ = nullptr;
};

struct AcceleratorTsIntersectData : AcceleratorIntersectData
{
	AcceleratorTsIntersectData() = default;
	explicit AcceleratorTsIntersectData(AcceleratorIntersectData &&intersect_data, const Rgb &transparent_color = Rgb{1.f}) : AcceleratorIntersectData(std::move(intersect_data)), transparent_color_{transparent_color} { }
	Rgb transparent_color_ {1.f};
};

class Accelerator
{
	public:
		static const Accelerator * factory(Logger &logger, const std::vector<const Primitive *> &primitives_list, const ParamMap &params);
		explicit Accelerator(Logger &logger) : logger_(logger) { }
		virtual ~Accelerator() = default;
		virtual AcceleratorIntersectData intersect(const Ray &ray, float t_max) const = 0;
		virtual AcceleratorIntersectData intersectS(const Ray &ray, float t_max, float shadow_bias) const = 0;
		virtual AcceleratorTsIntersectData intersectTs(const Ray &ray, int max_depth, float dist, float shadow_bias, const Camera *camera) const = 0;
		virtual Bound getBound() const = 0;
		std::pair<std::unique_ptr<const SurfacePoint>, float> intersect(const Ray &ray, const Camera *camera) const;
		std::pair<bool, const Primitive *> isShadowed(const Ray &ray, float shadow_bias) const;
		std::tuple<bool, Rgb, const Primitive *> isShadowed(const Ray &ray, int max_depth, float shadow_bias, const Camera *camera) const;

	protected:
		static void primitiveIntersection(AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray);
		static bool primitiveIntersection(AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray, float t_max);
		static bool primitiveIntersection(AcceleratorTsIntersectData &accelerator_intersect_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Ray &ray, float t_max, const Camera *camera);
		Logger &logger_;
};

END_YAFARAY
#endif    //YAFARAY_ACCELERATOR_H
