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

#ifndef YAFARAY_OBJECT_YAFARAY_H
#define YAFARAY_OBJECT_YAFARAY_H

#include "geometry/object.h"

BEGIN_YAFARAY

class ObjectYafaRay : public Object
{
	public:
		virtual ~ObjectYafaRay() override { resetObjectIndex(); }
		virtual std::string getName() const override { return name_; }
		virtual void setName(const std::string &name) override { name_ = name; }
		/*! sample object surface */
		//virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const override { }
		/*! Sets the object visibility to the renderer (is added or not to the kdtree) */
		virtual void setVisibility(const Visibility &visibility) override { visibility_ = visibility; }
		/*! Indicates that this object should be used as base object for instances */
		virtual void useAsBaseObject(bool v) override { is_base_object_ = v; }
		/*! Returns if this object should be used for rendering and/or shadows. */
		virtual Visibility getVisibility() const override { return visibility_; }
		/*! Returns if this object is used as base object for instances. */
		virtual bool isBaseObject() const override { return is_base_object_; }
		virtual void resetObjectIndex() override { highest_object_index_ = 1; object_index_auto_ = 0; }
		virtual void setObjectIndex(unsigned int new_obj_index) override;
		virtual unsigned int getAbsObjectIndex() const override { return object_index_; }
		virtual float getNormObjectIndex() const override { return static_cast<float>(getAbsObjectIndex()) / static_cast<float>(highest_object_index_); }
		virtual Rgb getAbsObjectIndexColor() const override { return getAbsObjectIndex(); }
		virtual Rgb getNormObjectIndexColor() const override { return getNormObjectIndex(); }
		virtual Rgb getAutoObjectIndexColor() const override { return object_index_auto_color_; }
		virtual Rgb getAutoObjectIndexNumber() const override { return object_index_auto_; }
		virtual const Light *getLight() const override { return light_; }
		/*! set a light source to be associated with this object */
		virtual void setLight(const Light *light) override { light_ = light; }

	protected:
		ObjectYafaRay();
		std::string name_;
		const Light *light_ = nullptr;
		Visibility visibility_ = Visibility::NormalVisible;
		bool is_base_object_ = false;
		unsigned int object_index_ = 0;	//!< Object Index for the object-index render pass
		Rgb object_index_auto_color_ = 0.f;	//!< Object Index color automatically generated for the object-index-auto color render pass
		static unsigned int object_index_auto_;	//!< Object Index automatically generated for the object-index-auto render pass
		static unsigned int highest_object_index_;	//!< Class shared variable containing the highest object index used for the Normalized Object Index pass.
};

END_YAFARAY

#endif // YAFARAY_OBJECT_YAFARAY_H
