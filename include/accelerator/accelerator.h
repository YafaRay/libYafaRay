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
		virtual AccelData intersect(const Ray &ray, float t_max) const = 0;
		virtual AccelData intersectShadow(const Ray &ray, float t_max) const = 0;
		virtual AccelTsData intersectTransparentShadow(const Ray &ray, int max_depth, float dist, const Camera *camera) const = 0;
		virtual Bound getBound() const = 0;
		std::pair<std::unique_ptr<const SurfacePoint>, float> intersect(const Ray &ray, const Camera *camera) const;
		std::pair<bool, const Primitive *> isShadowed(const Ray &ray, float shadow_bias) const;
		std::tuple<bool, Rgb, const Primitive *> isShadowedTransparentShadow(const Ray &ray, int max_depth, float shadow_bias, const Camera *camera) const;

	protected:
		static void primitiveIntersection(AccelData &accel_data, const Primitive *primitive, const Ray &ray);
		static bool primitiveIntersectionShadow(AccelData &accel_data, const Primitive *primitive, const Ray &ray, float t_max);
		static bool primitiveIntersectionTransparentShadow(AccelTsData &accel_ts_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Ray &ray, float t_max, const Camera *camera);
		Logger &logger_;
};

inline std::pair<std::unique_ptr<const SurfacePoint>, float> Accelerator::intersect(const Ray &ray, const Camera *camera) const
{
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::infinity();
	// intersect with tree:
	const AccelData accel_data{intersect(ray, t_max)};
	if(accel_data.isHit() && accel_data.primitive())
	{
		const Point3 hit_point{ray.from_ + accel_data.tMax() * ray.dir_};
		auto sp{accel_data.primitive()->getSurface(ray.differentials_.get(), hit_point, ray.time_, accel_data.uv(), camera)};
		return {std::move(sp), accel_data.tMax()};
	}
	else return {nullptr, ray.tmax_};
}

inline std::pair<bool, const Primitive *> Accelerator::isShadowed(const Ray &ray, float shadow_bias) const
{
	Ray sray{ray, Ray::DifferentialsCopy::No};
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = ray.time_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	const AccelData accel_data{intersectShadow(sray, t_max)};
	return {accel_data.isHit(), accel_data.primitive()};
}

inline std::tuple<bool, Rgb, const Primitive *> Accelerator::isShadowedTransparentShadow(const Ray &ray, int max_depth, float shadow_bias, const Camera *camera) const
{
	Ray sray{ray, Ray::DifferentialsCopy::No}; //Should this function use Ray::DifferentialsAssignment::Copy ? If using copy it would be slower but would take into account texture mipmaps, although that's probably irrelevant for transparent shadows?
	sray.from_ += sray.dir_ * sray.tmin_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	AccelTsData accel_ts_data{intersectTransparentShadow(sray, max_depth, t_max, camera)};
	return {accel_ts_data.isHit(), accel_ts_data.transparentColor(), accel_ts_data.isHit() ? accel_ts_data.primitive() : nullptr};
}

inline void Accelerator::primitiveIntersection(AccelData &accel_data, const Primitive *primitive, const Ray &ray)
{
	auto [t_hit, uv]{primitive->intersect(ray.from_, ray.dir_, ray.time_)};
	if(t_hit <= 0.f || t_hit < ray.tmin_ || t_hit >= accel_data.tMax()) return;
	if(const Visibility prim_visibility = primitive->getVisibility(); prim_visibility == Visibility::InvisibleShadowsOnly || prim_visibility == Visibility::Invisible) return;
	else if(const Visibility mat_visibility = primitive->getMaterial()->getVisibility(); mat_visibility == Visibility::InvisibleShadowsOnly || mat_visibility == Visibility::Invisible) return;
	accel_data.setTHit(t_hit);
	accel_data.setUv(std::move(uv));
	accel_data.setPrimitive(primitive);
}

inline bool Accelerator::primitiveIntersectionShadow(AccelData &accel_data, const Primitive *primitive, const Ray &ray, float t_max)
{
	auto [t_hit, uv]{primitive->intersect(ray.from_, ray.dir_, ray.time_)};
	if(t_hit <= 0.f || t_hit >= t_max) return false;
	if(const Visibility prim_visibility = primitive->getVisibility(); prim_visibility == Visibility::VisibleNoShadows || prim_visibility == Visibility::Invisible) return false;
	else if(const Visibility mat_visibility = primitive->getMaterial()->getVisibility(); mat_visibility == Visibility::VisibleNoShadows || mat_visibility == Visibility::Invisible) return false;
	accel_data.setTHit(t_hit);
	accel_data.setUv(std::move(uv));
	accel_data.setPrimitive(primitive);
	return true;
}

inline bool Accelerator::primitiveIntersectionTransparentShadow(AccelTsData &accel_ts_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Ray &ray, float t_max, const Camera *camera)
{
	auto [t_hit, uv]{primitive->intersect(ray.from_, ray.dir_, ray.time_)};
	if(t_hit <= 0.f || t_hit < ray.tmin_ || t_hit >= t_max) return false;
	const Material *mat = nullptr;
	if(const Visibility prim_visibility = primitive->getVisibility(); prim_visibility == Visibility::VisibleNoShadows || prim_visibility == Visibility::Invisible) return false;
	mat = primitive->getMaterial();
	if(mat->getVisibility() == Visibility::VisibleNoShadows || mat->getVisibility() == Visibility::Invisible) return false;
	accel_ts_data.setTHit(t_hit);
	accel_ts_data.setUv(std::move(uv));
	accel_ts_data.setPrimitive(primitive);
	if(!mat || !mat->isTransparent()) return true;
	else if(filtered.insert(primitive).second)
	{
		if(depth >= max_depth) return true;
		const Point3 hit_point{ray.from_ + accel_ts_data.tHit() * ray.dir_};
		const auto sp{primitive->getSurface(ray.differentials_.get(), hit_point, ray.time_, accel_ts_data.uv(), camera)};
		if(sp) accel_ts_data.multiplyTransparentColor(sp->getTransparency(ray.dir_, camera));
		++depth;
	}
	return false;
}

END_YAFARAY
#endif    //YAFARAY_ACCELERATOR_H
