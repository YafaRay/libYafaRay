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

#ifndef LIBYAFARAY_OBJECT_INSTANCE_H
#define LIBYAFARAY_OBJECT_INSTANCE_H

#include "geometry/object/object.h"
#include "geometry/matrix.h"
#include "math/interpolation.h"
#include <memory>
#include <vector>

namespace yafaray {

class Primitive;
template<typename T> class Bound;

class ObjectInstance final : public Object
{
		using ThisClassType_t = ObjectInstance; using ParentClassType_t = Object;

	public:
		inline static std::string getClassName() { return "ObjectInstance"; }
		void addPrimitives(const std::vector<const Primitive *> &base_primitives);
		void addObjToWorldMatrix(const Matrix4f &obj_to_world, float time);
		void addObjToWorldMatrix(Matrix4f &&obj_to_world, float time);
		std::vector<const Matrix4f *> getObjToWorldMatrices() const;
		const Matrix4f &getObjToWorldMatrix(int time_step) const { return time_steps_[time_step].obj_to_world_; }
		Matrix4f getObjToWorldMatrixAtTime(float time) const;
		float getObjToWorldTime(int time_step) const { return time_steps_[time_step].time_; }

		[[nodiscard]] std::string getName() const override { return "instance"; }
		void setName(const std::string &name) override { }
		int numPrimitives() const override { return static_cast<int>(primitive_instances_.size()); }
		std::vector<const Primitive *> getPrimitives() const override;
		void setVisibility(Visibility visibility) override { }
		void useAsBaseObject(bool v) override { }
		Visibility getVisibility() const override;
		bool isBaseObject() const override { return false; }
		void setIndexAuto(unsigned int new_obj_index) override { }
		unsigned int getIndex() const override;
		Rgb getIndexAutoColor() const override;
		unsigned int getIndexAuto() const override;
		const Light *getLight() const override;
		void setLight(const Light *light) override { }
		bool calculateObject(const std::unique_ptr<const Material> *material) override { return false; }
		bool hasMotionBlur() const override { return time_steps_.size() > 2; }

	private:
		struct TimeStepGeometry final
		{
			TimeStepGeometry(const Matrix4f &obj_to_world, float time) : obj_to_world_{obj_to_world}, time_{time} { }
			TimeStepGeometry(Matrix4f &&obj_to_world, float time) : obj_to_world_{std::move(obj_to_world)}, time_{time} { }
			Matrix4f obj_to_world_;
			float time_ = 0.f;
		};
		std::vector<TimeStepGeometry> time_steps_;
		std::vector<std::unique_ptr<const Primitive>> primitive_instances_;
};

inline void ObjectInstance::addObjToWorldMatrix(const Matrix4f &obj_to_world, float time)
{
	time_steps_.emplace_back(TimeStepGeometry{obj_to_world, time});
}

inline void ObjectInstance::addObjToWorldMatrix(Matrix4f &&obj_to_world, float time)
{
	time_steps_.emplace_back(TimeStepGeometry{std::move(obj_to_world), time});
}

inline Matrix4f ObjectInstance::getObjToWorldMatrixAtTime(float time) const
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

#endif //LIBYAFARAY_OBJECT_INSTANCE_H
