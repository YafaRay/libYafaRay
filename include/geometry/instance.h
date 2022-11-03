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

#ifndef LIBYAFARAY_INSTANCE_H
#define LIBYAFARAY_INSTANCE_H

#include "geometry/matrix.h"
#include "math/interpolation.h"
#include "math/dynamic_array.h"
#include <memory>
#include <vector>

namespace yafaray {

class PrimitiveInstance;
class Object;
class Scene;
template<typename T> class Bound;

class Instance final
{
		using ThisClassType_t = Instance;

	public:
		inline static std::string getClassName() { return "Instance"; }
		void addObject(size_t object_id);
		void addInstance(size_t instance_id);
		void addObjToWorldMatrix(Matrix4f &&obj_to_world, float time);
		std::vector<const Matrix4f *> getObjToWorldMatrices() const;
		const Matrix4f &getObjToWorldMatrix(int time_step) const { return time_steps_[time_step].obj_to_world_; }
		Matrix4f getObjToWorldMatrixAtTime(float time) const;
		void updatePrimitives(const Scene &scene);
		std::vector<const PrimitiveInstance *> getPrimitives() const;
		bool hasMotionBlur() const { return time_steps_.size() > 2; }

	private:
		struct TimeStepGeometry final
		{
			TimeStepGeometry(const Matrix4f &obj_to_world, float time) : obj_to_world_{obj_to_world}, time_{time} { }
			TimeStepGeometry(Matrix4f &&obj_to_world, float time) : obj_to_world_{std::move(obj_to_world)}, time_{time} { }
			Matrix4f obj_to_world_;
			float time_ = 0.f;
		};
		std::vector<TimeStepGeometry> time_steps_;
		DynamicArray<size_t, unsigned short> objects_; //Limited to 65536 objects, should be more than enough
		DynamicArray<size_t, unsigned short> instances_; //Limited to 65536 instances, should be more than enough
		std::vector<std::unique_ptr<const PrimitiveInstance>> primitives_;
};

inline void Instance::addObjToWorldMatrix(Matrix4f &&obj_to_world, float time)
{
	time_steps_.emplace_back(TimeStepGeometry{std::move(obj_to_world), time});
}

inline Matrix4f Instance::getObjToWorldMatrixAtTime(float time) const
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
			const auto m = math::bezierInterpolate<Matrix4f>({time_steps_[0].obj_to_world_, time_steps_[1].obj_to_world_, time_steps_[2].obj_to_world_}, bezier_factors);
			return m;
		}
	}
	else return getObjToWorldMatrix(0);
}

} //namespace yafaray

#endif //LIBYAFARAY_INSTANCE_H
