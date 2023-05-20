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

#include "common/items.h"
#include "param/class_meta.h"
#include "common/enum.h"
#include "common/visibility.h"

namespace yafaray {

class Scene;
class Material;
class Light;
class Primitive;
template <typename IndexType> class FaceIndices;
template <typename T> class Items;

class Object
{
		using ThisClassType_t = Object;

	protected: struct Type;
	public:
		inline static std::string getClassName() { return "Object"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<Object>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual std::map<std::string, const ParamMeta *> getParamMetaMap() const = 0;
		[[nodiscard]] virtual std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const = 0;
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		Object(ParamResult &param_result, const ParamMap &param_map, const Items <Object> &objects, const Items<Material> &materials, const Items<Light> &lights);
		virtual ~Object() = default;
		[[nodiscard]] std::string getName() const;
		size_t getId() const { return id_; }
		void setId(size_t id);
		/*! Returns if this object should be used for rendering and/or shadows. */
		Visibility getVisibility() const { return params_.visibility_; }
		/*! Returns if this object is used as base object for instances. */
		bool isBaseObject() const { return params_.is_base_object_; }
		int getPassIndex() const { return params_.object_index_; }
		Rgb getIndexAutoColor() const { return index_auto_color_; }
		const Light *getLight() const { return lights_.getById(light_id_).first; }
		void setLight(size_t light_id) { light_id_ = light_id; }
		const Material *getMaterial(size_t material_id) const { return materials_.getById(material_id).first; }
		virtual bool calculateObject(size_t material_id) = 0;
		bool calculateObject() { return calculateObject(0); }
		/*! write the primitive pointers to the given array
			\return number of written primitives */
		virtual std::vector<const Primitive *> getPrimitives() const = 0;
		virtual bool hasMotionBlur() const { return false; }

		/* Mesh-related interface functions below, only for Mesh objects */
		virtual int lastVertexId(unsigned char time_step) const { return -1; }
		virtual void addPoint(Point3f &&p, unsigned char time_step) { }
		virtual void addOrcoPoint(Point3f &&p, unsigned char time_step) { }
		virtual void addVertexNormal(Vec3f &&n, unsigned char time_step) { }
		virtual void addFace(const FaceIndices<int> &face_indices, size_t material_id) { }
		virtual int addUvValue(Uv<float> &&uv) { return -1; }
		virtual bool hasVerticesNormals(unsigned char time_step) const { return false; }
		virtual int numVerticesNormals(unsigned char time_step) const { return 0; }
		virtual int numVertices(unsigned char time_step) const { return 0; }
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
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(std::string, light_name_, "", "light_name", "");
			PARAM_ENUM_DECL(Visibility, visibility_, Visibility::Normal, "visibility", "");
			PARAM_DECL(bool, is_base_object_, false, "is_base_object", "");
			PARAM_DECL(int, object_index_, 0, "object_index", "Object Index for the object-index render pass");
			PARAM_DECL(bool, motion_blur_bezier_, false, "motion_blur_bezier", "");
			PARAM_DECL(float, time_range_start_, 0.f, "time_range_start", "");
			PARAM_DECL(float, time_range_end_, 1.f, "time_range_end", "");
		} params_;

	private:
		size_t id_{0};
		const Items<Object> &objects_;
		const Items<Material> &materials_;
		const Items<Light> &lights_;
		size_t light_id_{math::invalid<size_t>};
		Rgb index_auto_color_{0.f}; //!< Object Index color automatically generated for the object-index-auto color render pass
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_H
