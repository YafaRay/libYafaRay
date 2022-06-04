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

#ifndef YAFARAY_OBJECT_MESH_H
#define YAFARAY_OBJECT_MESH_H

#include "object_base.h"
#include "geometry/vector.h"
#include "geometry/uv.h"
#include <vector>
#include "common/logger.h"

BEGIN_YAFARAY

struct Uv;
class FacePrimitive;
class Material;

class MeshObject : public ObjectBase
{
	public:
		static Object *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		MeshObject(int num_vertices, int num_faces, bool has_uv, bool has_orco, bool motion_blur_bezier, float time_range_start, float time_range_end);
		~MeshObject() override;
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		int numPrimitives() const override { return static_cast<int>(faces_.size()); }
		std::vector<const Primitive *> getPrimitives() const override;
		int lastVertexId(size_t time_step) const override { return numVertices(time_step) - 1; }
		Vec3 getVertexNormal(int index, size_t time_step) const { return time_steps_[time_step].vertices_normals_[index]; }
		Point3 getVertex(int index, size_t time_step) const { return time_steps_[time_step].points_[index]; }
		Point3 getOrcoVertex(int index, size_t time_step) const { return time_steps_[time_step].orco_points_[index]; }
		int numVertices(size_t time_step) const override { return static_cast<int>(time_steps_[time_step].points_.size()); }
		int numVerticesNormals(size_t time_step) const override { return static_cast<int>(time_steps_[time_step].vertices_normals_.size()); }
		void addFace(std::unique_ptr<FacePrimitive> &&face);
		void addFace(std::vector<int> &&vertices, std::vector<int> &&vertices_uv, const std::unique_ptr<const Material> *material) override;
		const std::vector<Point3> &getPoints(size_t time_step) const { return time_steps_[time_step].points_; }
		const std::vector<Uv> &getUvValues() const { return uv_values_; }
		bool hasOrco(size_t time_step) const { return !time_steps_[time_step].orco_points_.empty(); }
		bool hasUv() const { return !uv_values_.empty(); }
		bool isSmooth() const { return is_smooth_; }
		bool hasVerticesNormals(size_t time_step) const override { return !time_steps_[time_step].vertices_normals_.empty(); }
		void addPoint(Point3 &&p, size_t time_step) override { time_steps_[time_step].points_.emplace_back(p); }
		void addOrcoPoint(Point3 &&p, size_t time_step) override { time_steps_[time_step].orco_points_.emplace_back(p); }
		void addVertexNormal(Vec3 &&n, size_t time_step) override;
		int addUvValue(Uv &&uv) override { uv_values_.emplace_back(uv); return static_cast<int>(uv_values_.size()) - 1; }
		void setSmooth(bool smooth) override { is_smooth_ = smooth; }
		bool smoothVerticesNormals(Logger &logger, float angle) override;
		bool calculateObject(const std::unique_ptr<const Material> *material) override;
		bool hasMotionBlurBezier() const { return motion_blur_bezier_; }
		float getTimeRangeStart() const { return time_steps_.front().time_; }
		float getTimeRangeEnd() const { return time_steps_.back().time_; }
		size_t numTimeSteps() const { return time_steps_.size(); }
		bool hasMotionBlur() const override { return hasMotionBlurBezier(); }

	private:
		struct TimeStepGeometry final
		{
			float time_ = 0.f;
			std::vector<Point3> points_;
			std::vector<Point3> orco_points_;
			std::vector<Vec3> vertices_normals_;
		};
		void convertToBezierControlPoints();
		static float getAngleSine(const std::array<int, 3> &triangle_indices, const std::vector<Point3> &vertices);
		std::vector<TimeStepGeometry> time_steps_{1};
		std::vector<std::unique_ptr<FacePrimitive>> faces_;
		std::vector<Uv> uv_values_;
		bool is_smooth_ = false;
		bool motion_blur_bezier_ = false;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_MESH_H

