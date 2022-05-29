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

#include "common/visibility.h"
#include "geometry/matrix4.h"
#include "geometry/object/object.h"
#include "geometry/object/object_instance.h"
#include "geometry/primitive/primitive_instance.h"
#include "math/interpolation.h"

BEGIN_YAFARAY

void ObjectInstance::addPrimitives(const std::vector<const Primitive *> &base_primitives)
{
	primitive_instances_.reserve(base_primitives.size());
	for(const auto &primitive : base_primitives)
	{
		primitive_instances_.emplace_back(new PrimitiveInstance(primitive, *this));
	}
}

void ObjectInstance::addObjToWorldMatrix(const Matrix4 &obj_to_world, float time)
{
	time_steps_.emplace_back(TimeStepGeometry{obj_to_world, time});
}

void ObjectInstance::addObjToWorldMatrix(Matrix4 &&obj_to_world, float time)
{
	time_steps_.emplace_back(TimeStepGeometry{std::move(obj_to_world), time});
}

Matrix4 ObjectInstance::getObjToWorldMatrixAtTime(float time) const
{
	if(hasMotionBlur())
	{
		const float time_start = time_steps_.front().time_;
		const float time_end = time_steps_.back().time_;
		if(time <= time_start) return getObjToWorldMatrix(0);
		else if(time >= time_end) return getObjToWorldMatrix(2);
		else
		{
			const float time_mapped = math::lerpSegment(time, 0.f, time_start, 1.f, time_end); //time_mapped must be in range [0.f-1.f]
			const auto bezier_factors = math::bezierCalculateFactors(time_mapped);
			const auto m = math::bezierInterpolate<Matrix4>({time_steps_[0].obj_to_world_, time_steps_[1].obj_to_world_, time_steps_[2].obj_to_world_}, bezier_factors);
			return m;
		}
	}
	else return getObjToWorldMatrix(0);
}

std::vector<const Primitive *> ObjectInstance::getPrimitives() const
{
	std::vector<const Primitive *> result;
	for(const auto &primitive_instance : primitive_instances_) result.emplace_back(primitive_instance.get());
	return result;
}

Visibility ObjectInstance::getVisibility() const
{
	return primitive_instances_.front()->getVisibility();
}

unsigned int ObjectInstance::getIndex() const
{
	return primitive_instances_.front()->getObjectIndex();
}

Rgb ObjectInstance::getIndexAutoColor() const
{
	return primitive_instances_.front()->getObjectIndexAutoColor();
}

unsigned int ObjectInstance::getIndexAuto() const
{
	return primitive_instances_.front()->getObjectIndexAuto();
}

const Light *ObjectInstance::getLight() const
{
	return primitive_instances_.front()->getObjectLight();
}

std::vector<const Matrix4 *> ObjectInstance::getObjToWorldMatrices() const
{
	std::vector<const Matrix4 *> result;
	for(const auto &time_step : time_steps_)
	{
		result.emplace_back(&time_step.obj_to_world_);
	}
	return result;
}

END_YAFARAY
