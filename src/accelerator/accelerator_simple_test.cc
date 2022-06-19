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

#include "accelerator/accelerator_simple_test.h"
#include "geometry/primitive/primitive.h"
#include "geometry/object/object.h"
#include "material/material.h"
#include "common/logger.h"

namespace yafaray {

const Accelerator * AcceleratorSimpleTest::factory(Logger &logger, const std::vector<const Primitive *> &primitives, const ParamMap &params)
{
	return new AcceleratorSimpleTest(logger, primitives);
}

AcceleratorSimpleTest::AcceleratorSimpleTest(Logger &logger, const std::vector<const Primitive *> &primitives) : Accelerator(logger), primitives_(primitives)
{
	const size_t num_primitives = primitives_.size();
	if(num_primitives > 0) bound_ = primitives.front()->getBound();
	else bound_ = {{0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}};
	for(const auto &primitive : primitives)
	{
		const Bound primitive_bound{primitive->getBound()};
		const Object *object = primitive->getObject();
		const auto &obj_bound_it = objects_data_.find(object);
		if(obj_bound_it == objects_data_.end())
		{
			objects_data_[object].bound_ = primitive_bound;
			objects_data_[object].primitives_.emplace_back(primitive);
		}
		else
		{
			obj_bound_it->second.bound_ = Bound(obj_bound_it->second.bound_, primitive_bound);
			obj_bound_it->second.primitives_.emplace_back(primitive);
		}
		bound_ = Bound(bound_, primitive_bound);
	}
	for(const auto &[object, object_data] : objects_data_)
	{
		if(logger_.isVerbose()) logger_.logVerbose("AcceleratorSimpleTest: Primitives in object '", (object)->getName(), "': ", object_data.primitives_.size(), ", bound: (", object_data.bound_.a_, ", ", object_data.bound_.g_, ")");
	}
	if(logger_.isVerbose()) logger_.logVerbose("AcceleratorSimpleTest: Objects: ", objects_data_.size(), ", primitives in tree: ", num_primitives, ", bound: (", bound_.a_, ", ", bound_.g_, ")");
}

IntersectData AcceleratorSimpleTest::intersect(const Ray &ray, float t_max) const
{
	IntersectData intersect_data;
	for(const auto &[object, object_data] : objects_data_)
	{
		if(const Bound::Cross cross{object_data.bound_.cross(ray, t_max)}; cross.crossed_)
		{
			const float t_min = std::max(ray.tmin_, calculateDynamicRayBias(cross));
			for(const auto &primitive : object_data.primitives_)
			{
				Accelerator::primitiveIntersection(intersect_data, primitive, ray.from_, ray.dir_, t_min, intersect_data.t_max_, ray.time_);
				if(intersect_data.isHit() && intersect_data.t_hit_ >= ray.tmin_  && intersect_data.t_hit_ <= ray.tmax_)
				{
					return intersect_data;
				}
			}
		}
	}
	return intersect_data;
}

IntersectData AcceleratorSimpleTest::intersectShadow(const Ray &ray, float t_max) const
{
	IntersectData intersect_data;
	for(const auto &[object, object_data] : objects_data_)
	{
		if(const Bound::Cross cross{object_data.bound_.cross(ray, t_max)}; cross.crossed_)
		{
			const float t_min = calculateDynamicRayBias(cross);
			for(const auto &primitive : object_data.primitives_)
			{
				if(Accelerator::primitiveIntersectionShadow(intersect_data, primitive, ray.from_, ray.dir_, t_min, t_max, ray.time_)) return intersect_data;
			}
		}
	}
	return intersect_data;
}

IntersectData AcceleratorSimpleTest::intersectTransparentShadow(const Ray &ray, int max_depth, float t_max, const Camera *camera) const
{
	std::set<const Primitive *> filtered;
	int depth = 0;
	IntersectData intersect_data;
	for(const auto &[object, object_data] : objects_data_)
	{
		if(const Bound::Cross cross{object_data.bound_.cross(ray, t_max)}; cross.crossed_)
		{
			const float t_min = std::max(ray.tmin_, calculateDynamicRayBias(cross));
			for(const auto &primitive : object_data.primitives_)
			{
				if(Accelerator::primitiveIntersectionTransparentShadow(intersect_data, filtered, depth, max_depth, primitive, camera, ray.from_, ray.dir_, t_min, t_max, ray.time_)) return intersect_data;
			}
		}
	}
	intersect_data.setNoHit();
	return intersect_data;
}

} //namespace yafaray