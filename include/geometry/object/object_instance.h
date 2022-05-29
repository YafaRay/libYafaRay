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

#ifndef YAFARAY_OBJECT_INSTANCE_H
#define YAFARAY_OBJECT_INSTANCE_H

#include "geometry/object/object.h"
#include "geometry/matrix4.h"
#include <memory>
#include <vector>

BEGIN_YAFARAY

class Primitive;
class Bound;
enum class Visibility : int;

class ObjectInstance final : public Object
{
	public:
		void addPrimitives(const std::vector<const Primitive *> &base_primitives);
		void addObjToWorldMatrix(const Matrix4 &obj_to_world, float time);
		void addObjToWorldMatrix(Matrix4 &&obj_to_world, float time);
		std::vector<const Matrix4 *> getObjToWorldMatrices() const;
		const Matrix4 &getObjToWorldMatrix(size_t time_step) const { return time_steps_[time_step].obj_to_world_; }
		Matrix4 getObjToWorldMatrixAtTime(float time) const;
		float getObjToWorldTime(size_t time_step) const { return time_steps_[time_step].time_; }

		std::string getName() const override { return "instance"; }
		void setName(const std::string &name) override { }
		int numPrimitives() const override { return static_cast<int>(primitive_instances_.size()); }
		std::vector<const Primitive *> getPrimitives() const override;
		void setVisibility(const Visibility &visibility) override { }
		void useAsBaseObject(bool v) override { }
		Visibility getVisibility() const override;
		bool isBaseObject() const override { return false; }
		void setIndex(unsigned int new_obj_index) override { }
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
			TimeStepGeometry(const Matrix4 &obj_to_world, float time) : obj_to_world_{obj_to_world}, time_{time} { }
			TimeStepGeometry(Matrix4 &&obj_to_world, float time) : obj_to_world_{std::move(obj_to_world)}, time_{time} { }
			Matrix4 obj_to_world_;
			float time_ = 0.f;
		};
		std::vector<TimeStepGeometry> time_steps_;
		std::vector<std::unique_ptr<const Primitive>> primitive_instances_;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_INSTANCE_H
