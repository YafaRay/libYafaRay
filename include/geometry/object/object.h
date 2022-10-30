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

#ifndef LIBYAFARAY_OBJECT_H
#define LIBYAFARAY_OBJECT_H

#include "scene/scene_items.h"
#include "param/class_meta.h"
#include "common/enum.h"
#include "common/visibility.h"

namespace yafaray {

class Scene;
class Material;
class Light;
class Primitive;

class Object
{
		using ThisClassType_t = Object;

	protected: struct Type;
	public:
		inline static std::string getClassName() { return "Object"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<Object>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		Object(ParamResult &param_result, const ParamMap &param_map, const SceneItems<Material> &materials);
		[[nodiscard]] std::string getName() const { return name_; }
		void setName(const std::string &name) { name_ = name; }
		void setVisibility(Visibility visibility) { visibility_ = visibility; }
		/*! Indicates that this object should be used as base object for instances */
		void useAsBaseObject(bool v) { is_base_object_ = v; }
		/*! Returns if this object should be used for rendering and/or shadows. */
		Visibility getVisibility() const { return visibility_; }
		/*! Returns if this object is used as base object for instances. */
		bool isBaseObject() const { return is_base_object_; }
		void setIndexAuto(unsigned int new_obj_index);
		unsigned int getIndex() const { return params_.object_index_; }
		Rgb getIndexAutoColor() const { return index_auto_color_; }
		unsigned int getIndexAuto() const { return index_auto_; }
		const Light *getLight() const { return light_; }
		void setLight(const Light *light) { light_ = light; }
		const Material *getMaterial(size_t material_id) const { return materials_.getById(material_id).first; }
		virtual bool calculateObject(size_t material_id) = 0;
		bool calculateObject() { return calculateObject(0); }
		/*! write the primitive pointers to the given array
			\return number of written primitives */
		virtual std::vector<const Primitive *> getPrimitives() const = 0;
		virtual bool hasMotionBlur() const { return false; }

		/* Mesh-related interface functions below, only for Mesh objects */
		virtual int lastVertexId(int time_step) const { return -1; }
		virtual void addPoint(Point3f &&p, int time_step) { }
		virtual void addOrcoPoint(Point3f &&p, int time_step) { }
		virtual void addVertexNormal(Vec3f &&n, int time_step) { }
		virtual void addFace(std::vector<int> &&vertices, std::vector<int> &&vertices_uv, size_t material_id) { }
		virtual int addUvValue(Uv<float> &&uv) { return -1; }
		virtual bool hasVerticesNormals(int time_step) const { return false; }
		virtual int numVerticesNormals(int time_step) const { return 0; }
		virtual int numVertices(int time_step) const { return 0; }
		virtual void setSmooth(bool smooth) { }
		virtual bool smoothVerticesNormals(Logger &logger, float angle) { return false; }

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

#endif //LIBYAFARAY_OBJECT_H
