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

#ifndef LIBYAFARAY_OBJECT_MESH_H
#define LIBYAFARAY_OBJECT_MESH_H

#include "object_base.h"
#include "geometry/vector.h"
#include "geometry/uv.h"
#include <vector>
#include "common/logger.h"

namespace yafaray {

template <typename T> struct Uv;
class FacePrimitive;
class Material;

class MeshObject : public ObjectBase
{
		using ThisClassType_t = MeshObject; using ParentClassType_t = ObjectBase;

	public:
		inline static std::string getClassName() { return "MeshObject"; }
		static std::pair<std::unique_ptr<Object>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		MeshObject(ParamError &param_error, const ParamMap &param_map, const std::vector<std::unique_ptr<Material>> &materials);
		~MeshObject() override;
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		int numPrimitives() const override { return static_cast<int>(faces_.size()); }
		std::vector<const Primitive *> getPrimitives() const override;
		int lastVertexId(int time_step) const override { return numVertices(time_step) - 1; }
		Vec3f getVertexNormal(int index, int time_step) const { return time_steps_[time_step].vertices_normals_[index]; }
		Point3f getVertex(int index, int time_step) const { return time_steps_[time_step].points_[index]; }
		Point3f getOrcoVertex(int index, int time_step) const { return time_steps_[time_step].orco_points_[index]; }
		int numVertices(int time_step) const override { return static_cast<int>(time_steps_[time_step].points_.size()); }
		int numVerticesNormals(int time_step) const override { return static_cast<int>(time_steps_[time_step].vertices_normals_.size()); }
		void addFace(std::unique_ptr<FacePrimitive> &&face);
		void addFace(std::vector<int> &&vertices, std::vector<int> &&vertices_uv, size_t material_id) override;
		const std::vector<Point3f> &getPoints(int time_step) const { return time_steps_[time_step].points_; }
		const std::vector<Uv<float>> &getUvValues() const { return uv_values_; }
		bool hasOrco(int time_step) const { return !time_steps_[time_step].orco_points_.empty(); }
		bool hasUv() const { return !uv_values_.empty(); }
		bool isSmooth() const { return is_smooth_; }
		bool hasVerticesNormals(int time_step) const override { return !time_steps_[time_step].vertices_normals_.empty(); }
		void addPoint(Point3f &&p, int time_step) override { time_steps_[time_step].points_.emplace_back(p); }
		void addOrcoPoint(Point3f &&p, int time_step) override { time_steps_[time_step].orco_points_.emplace_back(p); }
		void addVertexNormal(Vec3f &&n, int time_step) override;
		int addUvValue(Uv<float> &&uv) override { uv_values_.emplace_back(uv); return static_cast<int>(uv_values_.size()) - 1; }
		void setSmooth(bool smooth) override { is_smooth_ = smooth; }
		bool smoothVerticesNormals(Logger &logger, float angle) override;
		bool calculateObject(size_t material_id) override;
		bool hasMotionBlurBezier() const { return ObjectBase::params_.motion_blur_bezier_; }
		float getTimeRangeStart() const { return time_steps_.front().time_; }
		float getTimeRangeEnd() const { return time_steps_.back().time_; }
		int numTimeSteps() const { return static_cast<int>(time_steps_.size()); }
		bool hasMotionBlur() const override { return hasMotionBlurBezier(); }

	protected:
		[[nodiscard]] Type type() const override { return Type::Mesh; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(int , num_faces_, 0, "num_faces", "");
			PARAM_DECL(int , num_vertices_, 0, "num_vertices", "");
			PARAM_DECL(bool, has_uv_, false, "has_uv", "");
			PARAM_DECL(bool, has_orco_, false, "has_orco", "");
		} params_;

	private:
		struct TimeStepGeometry final
		{
			float time_ = 0.f;
			std::vector<Point3f> points_;
			std::vector<Point3f> orco_points_;
			std::vector<Vec3f> vertices_normals_;
		};
		virtual int calculateNumFaces() const { return params_.num_faces_; }
		void convertToBezierControlPoints();
		static float getAngleSine(const std::array<int, 3> &triangle_indices, const std::vector<Point3f> &vertices);
		std::vector<TimeStepGeometry> time_steps_{1};
		std::vector<std::unique_ptr<FacePrimitive>> faces_;
		std::vector<Uv<float>> uv_values_;
		bool is_smooth_ = false;
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_MESH_H

