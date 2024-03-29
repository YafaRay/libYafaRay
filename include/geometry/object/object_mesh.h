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

#include "object.h"
#include "geometry/vector.h"
#include "geometry/uv.h"
#include <vector>
#include "common/logger.h"

namespace yafaray {

template <typename T> struct Uv;
class FacePrimitive;
class Material;

class MeshObject : public Object
{
		using ThisClassType_t = MeshObject; using ParentClassType_t = Object;

	public:
		inline static std::string getClassName() { return "MeshObject"; }
		static std::pair<std::unique_ptr<MeshObject>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const override;
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		MeshObject(ParamResult &param_result, const ParamMap &param_map, const Items <Object> &objects, const Items<Material> &materials, const Items<Light> &lights);
		~MeshObject() override;
		std::vector<const Primitive *> getPrimitives() const override;
		int lastVertexId(unsigned char time_step) const override { return numVertices(time_step) - 1; }
		Vec3f getVertexNormal(int index, unsigned char time_step) const { return time_steps_[time_step].vertices_normals_[index]; }
		Point3f getVertex(int index, unsigned char time_step) const { return time_steps_[time_step].points_[index]; }
		Point3f getOrcoVertex(int index, unsigned char time_step) const { return time_steps_[time_step].orco_points_[index]; }
		int numVertices(unsigned char time_step) const override { return static_cast<int>(time_steps_[time_step].points_.size()); }
		int numVerticesNormals(unsigned char time_step) const override { return static_cast<int>(time_steps_[time_step].vertices_normals_.size()); }
		void addFace(std::unique_ptr<FacePrimitive> &&face);
		void addFace(const FaceIndices<int> &face_indices, size_t material_id) override;
		const std::vector<Point3f> &getPoints(unsigned char time_step) const { return time_steps_[time_step].points_; }
		const std::vector<Uv<float>> &getUvValues() const { return uv_values_; }
		bool hasOrco(unsigned char time_step) const { return !time_steps_[time_step].orco_points_.empty(); }
		bool hasUv() const { return !uv_values_.empty(); }
		bool isSmooth() const { return is_smooth_; }
		bool isAutoSmooth() const { return is_auto_smooth_; }
		float getSmoothAngle() const { return smooth_angle_; }
		bool hasVerticesNormals(unsigned char time_step) const override { return !time_steps_[time_step].vertices_normals_.empty(); }
		void addPoint(Point3f &&p, unsigned char time_step) override { time_steps_[time_step].points_.emplace_back(p); }
		void addOrcoPoint(Point3f &&p, unsigned char time_step) override { time_steps_[time_step].orco_points_.emplace_back(p); }
		void addVertexNormal(Vec3f &&n, unsigned char time_step) override;
		int addUvValue(Uv<float> &&uv) override { uv_values_.emplace_back(uv); return static_cast<int>(uv_values_.size()) - 1; }
		void setSmooth(bool smooth) override { is_smooth_ = smooth; }
		void setAutoSmooth(float smooth_angle) override { setSmooth(true); smooth_angle_ = smooth_angle; is_auto_smooth_ = true; }
		bool smoothVerticesNormals(Logger &logger, float angle) override;
		bool calculateObject(size_t material_id) override;
		bool hasMotionBlurBezier() const { return Object::params_.motion_blur_bezier_; }
		float getTimeRangeStart() const { return time_steps_.front().time_; }
		float getTimeRangeEnd() const { return time_steps_.back().time_; }
		int numTimeSteps() const { return static_cast<int>(time_steps_.size()); }
		bool hasMotionBlur() const override { return hasMotionBlurBezier(); }

	protected:
		[[nodiscard]] Type type() const override { return Type::Mesh; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
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
		bool is_auto_smooth_ = false;
		float smooth_angle_ = 0.f;
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_MESH_H

