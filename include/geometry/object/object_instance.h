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

#include "object.h"

BEGIN_YAFARAY

class Matrix4;

class ObjectInstance : public Object
{
	public:
		ObjectInstance(const Object &base_object, const Matrix4 &obj_to_world);
		int numPrimitives() const override { return primitive_instances_.size(); }
		const std::vector<const Primitive *> getPrimitives() const override;
		std::string getName() const override { return base_object_.getName(); }
		void setName(const std::string &name) override { }
		/*! sample object surface */
		//void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const override;
		/*! Sets the object visibility to the renderer (is added or not to the kdtree) */
		void setVisibility(const Visibility &visibility) override { }
		/*! Indicates that this object should be used as base object for instances */
		void useAsBaseObject(bool v) override { }
		/*! Returns if this object should be used for rendering and/or shadows. */
		Visibility getVisibility() const override { return base_object_.getVisibility(); }
		/*! Returns if this object is used as base object for instances. */
		bool isBaseObject() const override { return false; }
		void resetObjectIndex() override { }
		void setObjectIndex(unsigned int new_obj_index) override { }
		unsigned int getAbsObjectIndex() const override { return base_object_.getAbsObjectIndex(); }
		float getNormObjectIndex() const override { return base_object_.getNormObjectIndex(); }
		Rgb getAbsObjectIndexColor() const override { return base_object_.getAbsObjectIndex(); }
		Rgb getNormObjectIndexColor() const override { return base_object_.getNormObjectIndex(); }
		Rgb getAutoObjectIndexColor() const override { return base_object_.getAutoObjectIndexColor(); }
		Rgb getAutoObjectIndexNumber() const override { return base_object_.getAutoObjectIndexNumber(); }
		const Light *getLight() const override { return base_object_.getLight(); }
		/*! set a light source to be associated with this object */
		void setLight(const Light *light) override { }
		const Matrix4 *getObjToWorldMatrix() const { return obj_to_world_.get(); }
		bool calculateObject(const std::unique_ptr<Material> *material = nullptr) override { return true; }

	protected:
		const Object &base_object_;
		std::unique_ptr<const Matrix4> obj_to_world_;
		std::vector<std::unique_ptr<const Primitive>> primitive_instances_;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_INSTANCE_H
