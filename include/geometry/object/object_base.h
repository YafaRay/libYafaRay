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

#ifndef YAFARAY_OBJECT_BASE_H
#define YAFARAY_OBJECT_BASE_H

#include "geometry/object/object.h"

namespace yafaray {

class ObjectBase : public Object
{
	public:
		std::string getName() const override { return name_; }
		void setName(const std::string &name) override { name_ = name; }
		void setVisibility(VisibilityFlags visibility) override { visibility_ = visibility; }
		void useAsBaseObject(bool v) override { is_base_object_ = v; }
		VisibilityFlags getVisibility() const override { return visibility_; }
		bool isBaseObject() const override { return is_base_object_; }
		void setIndex(unsigned int new_obj_index) override { index_ = new_obj_index; }
		void setIndexAuto(unsigned int new_obj_index) override;
		unsigned int getIndex() const override { return index_; }
		Rgb getIndexAutoColor() const override { return index_auto_color_; }
		unsigned int getIndexAuto() const override { return index_auto_; }
		const Light *getLight() const override { return light_; }
		void setLight(const Light *light) override { light_ = light; }

	private:
		std::string name_;
		const Light *light_ = nullptr;
		VisibilityFlags visibility_{VisibilityFlags::Visible | VisibilityFlags::CastsShadows};
		bool is_base_object_ = false;
		unsigned int index_ = 1; //!< Object Index for the object-index render pass
		unsigned int index_auto_ = 1; //!< Object Index automatically generated for the object-index-auto render pass
		Rgb index_auto_color_{0.f}; //!< Object Index color automatically generated for the object-index-auto color render pass
};

} //namespace yafaray

#endif //YAFARAY_OBJECT_BASE_H
