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

#ifndef LIBYAFARAY_OBJECT_BASE_H
#define LIBYAFARAY_OBJECT_BASE_H

#include "geometry/object/object.h"
#include "scene/scene_items.h"
#include "param/class_meta.h"
#include "common/enum.h"

namespace yafaray {

class ObjectBase : public Object
{
		using ThisClassType_t = ObjectBase; using ParentClassType_t = Object;

	protected: struct Type;
	public:
		inline static std::string getClassName() { return "ObjectBase"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<Object>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		ObjectBase(ParamResult &param_result, const ParamMap &param_map, const SceneItems<Material> &materials);
		[[nodiscard]] std::string getName() const override { return name_; }
		void setName(const std::string &name) override { name_ = name; }
		void setVisibility(Visibility visibility) override { visibility_ = visibility; }
		void useAsBaseObject(bool v) override { is_base_object_ = v; }
		Visibility getVisibility() const override { return visibility_; }
		bool isBaseObject() const override { return is_base_object_; }
		void setIndexAuto(unsigned int new_obj_index) override;
		unsigned int getIndex() const override { return params_.object_index_; }
		Rgb getIndexAutoColor() const override { return index_auto_color_; }
		unsigned int getIndexAuto() const override { return index_auto_; }
		const Light *getLight() const override { return light_; }
		void setLight(const Light *light) override { light_ = light; }
		const Material *getMaterial(size_t material_id) const { return materials_.getById(material_id).first; }

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Mesh, Curve, Sphere };
			inline static const EnumMap<ValueType_t> map_{{
					{"mesh", Mesh, ""},
					{"curve", Curve, ""},
					{"sphere", Sphere, ""},
				}};
		};
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(std::string, light_name_, "", "light_name", "");
			PARAM_ENUM_DECL(Visibility, visibility_, Visibility::Normal, "visibility", "");
			PARAM_DECL(bool, is_base_object_, false, "is_base_object", "");
			PARAM_DECL(int, object_index_, 0, "object_index", "Object Index for the object-index render pass");
			PARAM_DECL(bool, motion_blur_bezier_, false, "motion_blur_bezier", "");
			PARAM_DECL(float, time_range_start_, 0.f, "time_range_start", "");
			PARAM_DECL(float, time_range_end_, 1.f, "time_range_end", "");
		} params_;

	private:
		std::string name_;
		const Light *light_ = nullptr;
		const SceneItems<Material> &materials_;
		Visibility visibility_{params_.visibility_};
		bool is_base_object_{params_.is_base_object_};
		unsigned int index_auto_ = 1; //!< Object Index automatically generated for the object-index-auto render pass
		Rgb index_auto_color_{0.f}; //!< Object Index color automatically generated for the object-index-auto color render pass
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_BASE_H
