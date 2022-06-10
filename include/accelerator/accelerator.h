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
#include "accelerator/accelerator_intersect_data.h"
#include "camera/camera.h"
#include "geometry/primitive/primitive.h"
#include "geometry/surface.h"
#include <vector>
#include <memory>
#include <limits>
#include <set>

BEGIN_YAFARAY

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

inline std::pair<std::unique_ptr<const SurfacePoint>, float> Accelerator::intersect(const Ray &ray, const Camera *camera) const
{
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::infinity();
	// intersect with tree:
	const AcceleratorIntersectData accelerator_intersect_data = intersect(ray, t_max);
	if(accelerator_intersect_data.isHit() && accelerator_intersect_data.primitive())
	{
		const Point3 hit_point{ray.from_ + accelerator_intersect_data.tMax() * ray.dir_};
		auto sp = accelerator_intersect_data.primitive()->getSurface(ray.differentials_.get(), hit_point, accelerator_intersect_data, camera);
		return {std::move(sp), accelerator_intersect_data.tMax()};
	}
	else return {nullptr, ray.tmax_};
}

inline std::pair<bool, const Primitive *> Accelerator::isShadowed(const Ray &ray, float shadow_bias) const
{
	Ray sray(ray, Ray::DifferentialsCopy::No);
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = ray.time_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	const AcceleratorIntersectData accelerator_intersect_data = intersectS(sray, t_max, shadow_bias);
	return {accelerator_intersect_data.isHit(), accelerator_intersect_data.primitive()};
}

inline std::tuple<bool, Rgb, const Primitive *> Accelerator::isShadowed(const Ray &ray, int max_depth, float shadow_bias, const Camera *camera) const
{
	Ray sray(ray, Ray::DifferentialsCopy::No); //Should this function use Ray::DifferentialsAssignment::Copy ? If using copy it would be slower but would take into account texture mipmaps, although that's probably irrelevant for transparent shadows?
	sray.from_ += sray.dir_ * sray.tmin_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	AcceleratorTsIntersectData accelerator_intersect_data = intersectTs(sray, max_depth, t_max, shadow_bias, camera);
	return {accelerator_intersect_data.isHit(), std::move(accelerator_intersect_data.transparentColor()), accelerator_intersect_data.isHit() ? accelerator_intersect_data.primitive() : nullptr};
}

inline void Accelerator::primitiveIntersection(AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray)
{
	if(IntersectData intersect_data = primitive->intersect(ray);
	   intersect_data.isHit() && intersect_data.tHit() < accelerator_intersect_data.tMax() && intersect_data.tHit() >= ray.tmin_)
	{
		if(const Visibility prim_visibility = primitive->getVisibility();
		   prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::VisibleNoShadows)
		{
			if(const Visibility mat_visibility = primitive->getMaterial()->getVisibility();
			   mat_visibility == Visibility::NormalVisible || mat_visibility == Visibility::VisibleNoShadows)
			{
				accelerator_intersect_data = { std::move(intersect_data), primitive };
				return;
			}
		}
	}
}

inline bool Accelerator::primitiveIntersection(AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray, float t_max)
{
	if(IntersectData intersect_data = primitive->intersect(ray);
	   intersect_data.isHit() && intersect_data.tHit() < t_max && intersect_data.tHit() >= 0.f)  // '>=' ?
	{
		if(const Visibility prim_visibility = primitive->getVisibility();
		   prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::InvisibleShadowsOnly)
		{
			if(const Visibility mat_visibility = primitive->getMaterial()->getVisibility();
			   mat_visibility == Visibility::NormalVisible || mat_visibility == Visibility::InvisibleShadowsOnly)
			{
				accelerator_intersect_data = { std::move(intersect_data), primitive };
				return true;
			}
		}
	}
	return false;
}

inline bool Accelerator::primitiveIntersection(AcceleratorTsIntersectData &accelerator_intersect_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Ray &ray, float t_max, const Camera *camera)
{
	if(IntersectData intersect_data = primitive->intersect(ray);
	   intersect_data.isHit() && intersect_data.tHit() < t_max && intersect_data.tHit() >= ray.tmin_)// '>=' ?
	{
		if(const Visibility prim_visibility = primitive->getVisibility();
		   prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::InvisibleShadowsOnly)
		{
			if(const Material *mat = primitive->getMaterial();
			   mat->getVisibility() == Visibility::NormalVisible || mat->getVisibility() == Visibility::InvisibleShadowsOnly)
			{
				accelerator_intersect_data = AcceleratorTsIntersectData{AcceleratorIntersectData{ std::move(intersect_data), primitive}};
				if(!mat->isTransparent()) return true;
				if(filtered.insert(primitive).second)
				{
					if(depth >= max_depth) return true;
					const Point3 hit_point{ray.from_ + accelerator_intersect_data.tHit() * ray.dir_};
					const auto sp = primitive->getSurface(ray.differentials_.get(), hit_point, accelerator_intersect_data, camera);
					if(sp) accelerator_intersect_data.multiplyTransparentColor(sp->getTransparency(ray.dir_, camera));
					++depth;
				}
			}
		}
	}
	return false;
}

END_YAFARAY
#endif    //YAFARAY_ACCELERATOR_H
