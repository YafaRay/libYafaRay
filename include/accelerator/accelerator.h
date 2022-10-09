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

#ifndef LIBYAFARAY_ACCELERATOR_H
#define LIBYAFARAY_ACCELERATOR_H

#include "accelerator/intersect_data.h"
#include "param/class_meta.h"
#include "common/enum.h"
#include "geometry/primitive/primitive.h"
#include "geometry/surface.h"
#include <vector>
#include <memory>
#include <limits>
#include <set>

namespace yafaray {

class Camera;

class Accelerator
{
	public:
		inline static std::string getClassName() { return "Accelerator"; }
		static std::pair<std::unique_ptr<Accelerator>, ParamError> factory(Logger &logger, const std::vector<const Primitive *> &primitives_list, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;

		explicit Accelerator(Logger &logger, ParamError &param_error, const ParamMap &param_map) : params_{param_error, param_map}, logger_{logger} { }
		virtual ~Accelerator() = default;
		virtual IntersectData intersect(const Ray &ray, float t_max) const = 0;
		virtual IntersectData intersectShadow(const Ray &ray, float t_max) const = 0;
		virtual IntersectData intersectTransparentShadow(const Ray &ray, int max_depth, float dist, const Camera *camera) const = 0;
		virtual Bound<float> getBound() const = 0;
		std::pair<std::unique_ptr<const SurfacePoint>, float> intersect(const Ray &ray, const Camera *camera) const;
		std::pair<bool, const Primitive *> isShadowed(const Ray &ray) const;
		std::tuple<bool, Rgb, const Primitive *> isShadowedTransparentShadow(const Ray &ray, int max_depth, const Camera *camera) const;

		static constexpr inline float minRayDist() { return min_raydist_; }
		static constexpr inline float shadowBias() { return shadow_bias_; }
		static void primitiveIntersection(IntersectData &intersect_data, const Primitive *primitive, const Point3f &from, const Vec3f &dir, float t_min, float t_max, float time);
		static bool primitiveIntersectionShadow(IntersectData &intersect_data, const Primitive *primitive, const Point3f &from, const Vec3f &dir, float t_min, float t_max, float time);
		static bool primitiveIntersectionTransparentShadow(IntersectData &intersect_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Camera *camera, const Point3f &from, const Vec3f &dir, float t_min, float t_max, float time);

		static float calculateDynamicRayBias(const Bound<float>::Cross &bound_cross) { return 0.1f * minRayDist() * std::abs(bound_cross.leave_ - bound_cross.enter_); } //!< empirical guesstimate for ray bias to avoid self intersections, calculated based on the length segment of the ray crossing the tree bound, to estimate the loss of precision caused by the (very roughly approximate) size of the primitive

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, SimpleTest, KdTreeOriginal, KdTreeMultiThread };
			inline static const EnumMap<ValueType_t> map_{{
					{"yafaray-simpletest", SimpleTest, ""},
					{"yafaray-kdtree-original", KdTreeOriginal, ""},
					{"yafaray-kdtree-multi-thread", KdTreeMultiThread, ""},
				}};
		};
		[[nodiscard]] virtual Type type() const = 0;
		const struct Params
		{
			PARAM_INIT;
		} params_;
		Logger &logger_;
		static constexpr inline float min_raydist_ = 0.00005f;
		static constexpr inline float shadow_bias_ = 0.0005f;
};

inline std::pair<std::unique_ptr<const SurfacePoint>, float> Accelerator::intersect(const Ray &ray, const Camera *camera) const
{
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::max();
	// intersect with tree:
	const IntersectData intersect_data{intersect(ray, t_max)};
	if(intersect_data.isHit() && intersect_data.primitive_)
	{
		const Point3f hit_point{ray.from_ + intersect_data.t_max_ * ray.dir_};
		auto sp{intersect_data.primitive_->getSurface(ray.differentials_.get(), hit_point, ray.time_, intersect_data.uv_, camera)};
		return {std::move(sp), intersect_data.t_max_};
	}
	else return {nullptr, ray.tmax_};
}

inline std::pair<bool, const Primitive *> Accelerator::isShadowed(const Ray &ray) const
{
	Ray sray{ray, Ray::DifferentialsCopy::No};
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = ray.time_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::max();
	const IntersectData intersect_data{intersectShadow(sray, t_max)};
	return {intersect_data.isHit(), intersect_data.primitive_};
}

inline std::tuple<bool, Rgb, const Primitive *> Accelerator::isShadowedTransparentShadow(const Ray &ray, int max_depth, const Camera *camera) const
{
	Ray sray{ray, Ray::DifferentialsCopy::No}; //Should this function use Ray::DifferentialsAssignment::Copy ? If using copy it would be slower but would take into account texture mipmaps, although that's probably irrelevant for transparent shadows?
	sray.from_ += sray.dir_ * sray.tmin_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::max();
	IntersectData intersect_data{intersectTransparentShadow(sray, max_depth, t_max, camera)};
	return {intersect_data.isHit(), intersect_data.color_, intersect_data.primitive_};
}

inline void Accelerator::primitiveIntersection(IntersectData &intersect_data, const Primitive *primitive, const Point3f &from, const Vec3f &dir, float t_min, float t_max, float time)
{
	auto [t_hit, uv]{primitive->intersect(from, dir, time)};
	if(t_hit <= 0.f || t_hit < t_min || t_hit >= t_max) return;
	if(const Visibility prim_visibility = primitive->getVisibility(); !prim_visibility.has(Visibility::Visible)) return;
	else if(const Visibility mat_visibility = primitive->getMaterial()->getVisibility(); !mat_visibility.has(Visibility::Visible)) return;
	intersect_data.t_hit_ = t_hit;
	intersect_data.t_max_ = t_hit;
	intersect_data.uv_ = std::move(uv);
	intersect_data.primitive_ = primitive;
}

inline bool Accelerator::primitiveIntersectionShadow(IntersectData &intersect_data, const Primitive *primitive, const Point3f &from, const Vec3f &dir, float t_min, float t_max, float time)
{
	auto [t_hit, uv]{primitive->intersect(from, dir, time)};
	if(t_hit <= 0.f || t_hit < t_min || t_hit >= t_max) return false;
	if(const Visibility prim_visibility = primitive->getVisibility(); !prim_visibility.has(Visibility::CastsShadows)) return false;
	else if(const Visibility mat_visibility = primitive->getMaterial()->getVisibility(); !mat_visibility.has(Visibility::CastsShadows)) return false;
	intersect_data.t_hit_ = t_hit;
	intersect_data.t_max_ = t_hit;
	intersect_data.uv_ = std::move(uv);
	intersect_data.primitive_ = primitive;
	return true;
}

inline bool Accelerator::primitiveIntersectionTransparentShadow(IntersectData &intersect_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Camera *camera, const Point3f &from, const Vec3f &dir, float t_min, float t_max, float time)
{
	auto [t_hit, uv]{primitive->intersect(from, dir, time)};
	if(t_hit <= 0.f || t_hit < t_min || t_hit >= t_max) return false;
	const Material *mat = nullptr;
	if(const Visibility prim_visibility = primitive->getVisibility(); !prim_visibility.has(Visibility::CastsShadows)) return false;
	mat = primitive->getMaterial();
	if(!mat->getVisibility().has(Visibility::CastsShadows)) return false;
	intersect_data.t_hit_ = t_hit;
	intersect_data.t_max_ = t_hit;
	intersect_data.uv_ = std::move(uv);
	intersect_data.primitive_ = primitive;
	if(!mat || !mat->isTransparent()) return true;
	else if(filtered.insert(primitive).second)
	{
		if(depth >= max_depth) return true;
		const Point3f hit_point{from + intersect_data.t_hit_ * dir};
		const auto sp{primitive->getSurface(nullptr, hit_point, time, intersect_data.uv_, camera)}; //I don't think we need differentials for transparent shadows, no need to blur the texture from a distance for this
		if(sp) intersect_data.color_ *= sp->getTransparency(dir, camera);
		++depth;
	}
	return false;
}

} //namespace yafaray
#endif    //LIBYAFARAY_ACCELERATOR_H
