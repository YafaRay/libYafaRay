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
#include "geometry/primitive.h"
#include "geometry/object.h"
#include "material/material.h"
#include "common/logger.h"

BEGIN_YAFARAY

std::unique_ptr<Accelerator> AcceleratorSimpleTest::factory(Logger &logger, const std::vector<const Primitive *> &primitives, ParamMap &params)
{
	auto accelerator = std::unique_ptr<Accelerator>(new AcceleratorSimpleTest(logger, primitives));
	return accelerator;
}

AcceleratorSimpleTest::AcceleratorSimpleTest(Logger &logger, const std::vector<const Primitive *> &primitives) : Accelerator(logger), primitives_(primitives), bound_(primitives.front()->getBound())
{
	const size_t num_primitives = primitives_.size();
	for(const auto &primitive : primitives)
	{
		const Bound primitive_bound = primitive->getBound();
		const Object *object = primitive->getObject();
		const auto &obj_bound_it = objects_data_.find(object);
		if(obj_bound_it == objects_data_.end())
		{
			objects_data_[object].bound_ = primitive_bound;
			objects_data_[object].primitives_.push_back(primitive);
		}
		else
		{
			obj_bound_it->second.bound_ = Bound(obj_bound_it->second.bound_, primitive_bound);
			obj_bound_it->second.primitives_.push_back(primitive);
		}
		bound_ = Bound(bound_, primitive_bound);
	}
	for(const auto &object_data : objects_data_)
	{
		if(logger_.isVerbose()) logger_.logVerbose("AcceleratorSimpleTest: Primitives in object '", (object_data.first)->getName(), "': ", object_data.second.primitives_.size(), ", bound: (", object_data.second.bound_.a_, ", ", object_data.second.bound_.g_, ")");
	}
	if(logger_.isVerbose()) logger_.logVerbose("AcceleratorSimpleTest: Objects: ", objects_data_.size(), ", primitives in tree: ", num_primitives, ", bound: (", bound_.a_, ", ", bound_.g_, ")");
}

AcceleratorIntersectData AcceleratorSimpleTest::intersect(const Ray &ray, float t_max) const
{
	AcceleratorIntersectData accelerator_intersect_data;
	accelerator_intersect_data.t_max_ = t_max;
	for(const auto &object_data : objects_data_)
	{
		const Bound::Cross cross = object_data.second.bound_.cross(ray, accelerator_intersect_data.t_max_);
		if(!cross.crossed_) continue;
		for(const auto &primitive : object_data.second.primitives_)
		{
			const IntersectData intersect_data = primitive->intersect(ray);
			if(intersect_data.hit_ && intersect_data.t_hit_ >= ray.tmin_ && intersect_data.t_hit_ < accelerator_intersect_data.t_max_)
			{
				const Visibility prim_visibility = primitive->getVisibility();
				if(prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::VisibleNoShadows)
				{
					const Visibility mat_visibility = primitive->getMaterial()->getVisibility();
					if(mat_visibility == Visibility::NormalVisible || mat_visibility == Visibility::VisibleNoShadows)
					{
						accelerator_intersect_data.setIntersectData(intersect_data);
						accelerator_intersect_data.t_max_ = intersect_data.t_hit_;
						accelerator_intersect_data.hit_primitive_ = primitive;
					}
				}
			}
		}
	}
	return accelerator_intersect_data;
}

AcceleratorIntersectData AcceleratorSimpleTest::intersectS(const Ray &ray, float t_max, float shadow_bias) const
{
	for(const auto &object_data : objects_data_)
	{
		const Bound::Cross cross = object_data.second.bound_.cross(ray, t_max);
		if(!cross.crossed_) continue;
		for(const auto &primitive : object_data.second.primitives_)
		{
			const IntersectData intersect_data = primitive->intersect(ray);
			if(intersect_data.hit_ && intersect_data.t_hit_ >= (ray.tmin_ + shadow_bias) && intersect_data.t_hit_ < t_max)
			{
				const Visibility prim_visibility = primitive->getVisibility();
				if(prim_visibility == Visibility::NormalVisible || prim_visibility == Visibility::InvisibleShadowsOnly)
				{
					const Visibility mat_visibility = primitive->getMaterial()->getVisibility();
					if(mat_visibility == Visibility::NormalVisible || mat_visibility == Visibility::InvisibleShadowsOnly)
					{
						AcceleratorIntersectData accelerator_intersect_data;
						accelerator_intersect_data.hit_ = true;
						return accelerator_intersect_data;
					}
				}
			}
		}
	}
	return {};
}

AcceleratorTsIntersectData AcceleratorSimpleTest::intersectTs(RenderData &render_data, const Ray &ray, int max_depth, float dist, float shadow_bias) const
{
	for(const auto &object_data : objects_data_)
	{
		const Bound::Cross cross = object_data.second.bound_.cross(ray, dist);
		if(!cross.crossed_) continue;
		for(const auto &primitive : object_data.second.primitives_)
		{
			const IntersectData intersect_data = primitive->intersect(ray);
			if(intersect_data.hit_ && intersect_data.t_hit_ >= ray.tmin_ && intersect_data.t_hit_ < dist)
			{
				AcceleratorTsIntersectData accelerator_intersect_data;
				accelerator_intersect_data.setIntersectData(intersect_data);
				accelerator_intersect_data.t_max_ = intersect_data.t_hit_;
				accelerator_intersect_data.hit_primitive_ = primitive;
				return accelerator_intersect_data;
			}
		}
	}
	return {};
}

END_YAFARAY