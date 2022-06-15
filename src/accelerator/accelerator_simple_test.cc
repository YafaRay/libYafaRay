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

BEGIN_YAFARAY

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

AccelData AcceleratorSimpleTest::intersect(const Ray &ray, float t_max) const
{
	AccelData accel_data;
	for(const auto &[object, object_data] : objects_data_)
	{
		if(const Bound::Cross cross{object_data.bound_.cross(ray, t_max)}; cross.crossed_)
		{
			for(const auto &primitive : object_data.primitives_)
			{
				Accelerator::primitiveIntersection(accel_data, primitive, ray);
				if(accel_data.isHit() && accel_data.tHit() >= ray.tmin_  && accel_data.tHit() <= ray.tmax_)
				{
					return accel_data;
				}
			}
		}
	}
	return accel_data;
}

AccelData AcceleratorSimpleTest::intersectS(const Ray &ray, float t_max) const
{
	AccelData accel_data;
	for(const auto &[object, object_data] : objects_data_)
	{
		if(const Bound::Cross cross{object_data.bound_.cross(ray, t_max)}; cross.crossed_)
		{
			for(const auto &primitive : object_data.primitives_)
			{
				if(Accelerator::primitiveIntersection(accel_data, primitive, ray, t_max)) return accel_data;
			}
		}
	}
	return accel_data;
}

AccelTsData AcceleratorSimpleTest::intersectTs(const Ray &ray, int max_depth, float t_max, const Camera *camera) const
{
	std::set<const Primitive *> filtered;
	int depth = 0;
	AccelTsData accel_ts_data;
	for(const auto &[object, object_data] : objects_data_)
	{
		if(const Bound::Cross cross{object_data.bound_.cross(ray, t_max)}; cross.crossed_)
		{
			for(const auto &primitive : object_data.primitives_)
			{
				if(Accelerator::primitiveIntersection(accel_ts_data, filtered, depth, max_depth, primitive, ray, t_max, camera)) return accel_ts_data;
			}
		}
	}
	return accel_ts_data;
}

END_YAFARAY