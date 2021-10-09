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

#include "geometry/object.h"

BEGIN_YAFARAY

class Matrix4;

class ObjectInstance : public Object
{
	public:
		ObjectInstance(const Object &base_object, const Matrix4 &obj_to_world);
		virtual int numPrimitives() const override { return base_object_.numPrimitives(); }
		virtual const std::vector<const Primitive *> getPrimitives() const override;
		virtual std::string getName() const override { return base_object_.getName(); }
		virtual void setName(const std::string &name) override { }
		/*! sample object surface */
		//virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const override;
		/*! Sets the object visibility to the renderer (is added or not to the kdtree) */
		virtual void setVisibility(const Visibility &visibility) override { }
		/*! Indicates that this object should be used as base object for instances */
		virtual void useAsBaseObject(bool v) override { }
		/*! Returns if this object should be used for rendering and/or shadows. */
		virtual Visibility getVisibility() const override { return base_object_.getVisibility(); }
		/*! Returns if this object is used as base object for instances. */
		virtual bool isBaseObject() const override { return false; }
		virtual void resetObjectIndex() override { }
		virtual void setObjectIndex(unsigned int new_obj_index) override { }
		virtual unsigned int getAbsObjectIndex() const override { return base_object_.getAbsObjectIndex(); }
		virtual float getNormObjectIndex() const override { return base_object_.getNormObjectIndex(); }
		virtual Rgb getAbsObjectIndexColor() const override { return base_object_.getAbsObjectIndex(); }
		virtual Rgb getNormObjectIndexColor() const override { return base_object_.getNormObjectIndex(); }
		virtual Rgb getAutoObjectIndexColor() const override { return base_object_.getAutoObjectIndexColor(); }
		virtual Rgb getAutoObjectIndexNumber() const override { return base_object_.getAutoObjectIndexNumber(); }
		virtual const Light *getLight() const override { return base_object_.getLight(); }
		/*! set a light source to be associated with this object */
		virtual void setLight(const Light *light) override { }
		const Matrix4 *getObjToWorldMatrix() const { return obj_to_world_.get(); }
		virtual bool calculateObject(const Material *material = nullptr) override { return true; }

	protected:
		const Object &base_object_;
		std::unique_ptr<const Matrix4> obj_to_world_;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_INSTANCE_H
