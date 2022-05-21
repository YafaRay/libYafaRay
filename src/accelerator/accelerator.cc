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

#include "accelerator/accelerator.h"
#include "accelerator/accelerator_kdtree.h"
#include "accelerator/accelerator_kdtree_multi_thread.h"
#include "accelerator/accelerator_simple_test.h"
#include "common/logger.h"
#include "common/param.h"
#include "geometry/surface.h"
#include "render/render_data.h"
#include "geometry/primitive/primitive_face.h"
#include "integrator/integrator.h"

BEGIN_YAFARAY

const Accelerator * Accelerator::factory(Logger &logger, const std::vector<const Primitive *> &primitives_list, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Accelerator");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	const Accelerator *accelerator = nullptr;
	if(type == "yafaray-kdtree-original") accelerator = AcceleratorKdTree::factory(logger, primitives_list, params);
	else if(type == "yafaray-kdtree-multi-thread") accelerator = AcceleratorKdTreeMultiThread::factory(logger, primitives_list, params);
	else if(type == "yafaray-simpletest") accelerator = AcceleratorSimpleTest::factory(logger, primitives_list, params);

	if(accelerator) logger.logInfo("Accelerator type '", type, "' created.");
	else
	{
		logger.logWarning("Accelerator type '", type, "' could not be created, using standard single-thread KdTree instead.");
		accelerator = AcceleratorKdTree::factory(logger, primitives_list, params);
	}
	return accelerator;
}

std::pair<std::unique_ptr<const SurfacePoint>, float> Accelerator::intersect(const Ray &ray, const Camera *camera) const
{
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::infinity();
	// intersect with tree:
	const AcceleratorIntersectData accelerator_intersect_data = intersect(ray, t_max);
	if(accelerator_intersect_data.hit_ && accelerator_intersect_data.hit_primitive_)
	{
		const Point3 hit_point{ray.from_ + accelerator_intersect_data.t_max_ * ray.dir_};
		auto sp = accelerator_intersect_data.hit_primitive_->getSurface(ray.differentials_.get(), hit_point, accelerator_intersect_data, nullptr, camera);
		return {std::move(sp), accelerator_intersect_data.t_max_};
	}
	return {nullptr, ray.tmax_};
}

std::pair<bool, const Primitive *> Accelerator::isShadowed(const Ray &ray, float shadow_bias) const
{
	Ray sray(ray, Ray::DifferentialsCopy::No);
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = ray.time_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	const AcceleratorIntersectData accelerator_intersect_data = intersectS(sray, t_max, shadow_bias);
	if(accelerator_intersect_data.hit_) return {true, accelerator_intersect_data.hit_primitive_};
	else return {false, nullptr};
}

std::tuple<bool, Rgb, const Primitive *> Accelerator::isShadowed(const Ray &ray, int max_depth, float shadow_bias, const Camera *camera) const
{
	Ray sray(ray, Ray::DifferentialsCopy::No); //Should this function use Ray::DifferentialsAssignment::Copy ? If using copy it would be slower but would take into account texture mipmaps, although that's probably irrelevant for transparent shadows?
	sray.from_ += sray.dir_ * sray.tmin_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	const AcceleratorTsIntersectData accelerator_intersect_data = intersectTs(sray, max_depth, t_max, shadow_bias, camera);
	std::tuple<bool, Rgb, const Primitive *> result {false, Rgb{0.f}, nullptr};
	std::get<1>(result) = accelerator_intersect_data.transparent_color_;
	if(accelerator_intersect_data.hit_)
	{
		std::get<0>(result) = true;
		std::get<2>(result) = accelerator_intersect_data.hit_primitive_;
	}
	return result;
}

void Accelerator::primitiveIntersection(AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray)
{
	if(IntersectData intersect_data = primitive->intersect(ray);
	   intersect_data.hit_ && intersect_data.t_hit_ < accelerator_intersect_data.t_max_ && intersect_data.t_hit_ >= ray.tmin_)
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

bool Accelerator::primitiveIntersection(AcceleratorIntersectData &accelerator_intersect_data, const Primitive *primitive, const Ray &ray, float t_max)
{
	if(IntersectData intersect_data = primitive->intersect(ray);
	   intersect_data.hit_ && intersect_data.t_hit_ < t_max && intersect_data.t_hit_ >= 0.f)  // '>=' ?
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

bool Accelerator::primitiveIntersection(AcceleratorTsIntersectData &accelerator_intersect_data, std::set<const Primitive *> &filtered, int &depth, int max_depth, const Primitive *primitive, const Ray &ray, float t_max, const Camera *camera)
{
	if(IntersectData intersect_data = primitive->intersect(ray);
	   intersect_data.hit_ && intersect_data.t_hit_ < t_max && intersect_data.t_hit_ >= ray.tmin_)// '>=' ?
	{
		if(const Visibility prim_visibility = primitive->getVisibility();
		   prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::InvisibleShadowsOnly)
		{
			if(const Material *mat = primitive->getMaterial();
			   mat->getVisibility() == Visibility::NormalVisible || mat->getVisibility() == Visibility::InvisibleShadowsOnly)
			{
				accelerator_intersect_data = AcceleratorTsIntersectData{std::move(AcceleratorIntersectData{ std::move(intersect_data), primitive })};
				accelerator_intersect_data.hit_primitive_ = primitive;
				if(!mat->isTransparent()) return true;
				if(filtered.insert(primitive).second)
				{
					if(depth >= max_depth) return true;
					const Point3 hit_point{ray.from_ + accelerator_intersect_data.t_hit_ * ray.dir_};
					const auto sp = primitive->getSurface(ray.differentials_.get(), hit_point, accelerator_intersect_data, nullptr, camera);
					if(sp) accelerator_intersect_data.transparent_color_ *= sp->getTransparency(ray.dir_, camera);
					++depth;
				}
			}
		}
	}
	return false;
}

END_YAFARAY
