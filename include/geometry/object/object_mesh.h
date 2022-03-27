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

#include "object_basic.h"
#include "geometry/vector.h"
#include "geometry/uv.h"
#include <vector>
#include "common/logger.h"

BEGIN_YAFARAY

struct Uv;
class FacePrimitive;
class Material;

class MeshObject : public ObjectBasic
{
	public:
		static Object *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		MeshObject(int num_vertices, int num_faces, bool has_uv = false, bool has_orco = false);
		~MeshObject() override;
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		int numPrimitives() const override { return faces_.size(); }
		const std::vector<const Primitive *> getPrimitives() const override;
		int lastVertexId() const override { return points_.size() - 1; }
		Vec3 getVertexNormal(int index) const { return normals_[index]; }
		Point3 getVertex(int index) const { return points_[index]; }
		Point3 getOrcoVertex(int index) const { return orco_points_[index]; }
		int numVertices() const override { return points_.size(); }
		int numNormals() const override { return normals_.size(); }
		void addFace(std::unique_ptr<FacePrimitive> face);
		void addFace(const std::vector<int> &vertices, const std::vector<int> &vertices_uv, const std::unique_ptr<const Material> *material) override;
		void calculateNormals();
		const std::vector<Point3> &getPoints() const { return points_; }
		const std::vector<Uv> &getUvValues() const { return uv_values_; }
		bool hasOrco() const { return !orco_points_.empty(); }
		bool hasUv() const { return !uv_values_.empty(); }
		bool isSmooth() const { return is_smooth_; }
		bool hasNormalsExported() const override { return !normals_.empty(); }
		void addPoint(const Point3 &p) override { points_.push_back(p); }
		void addOrcoPoint(const Point3 &p) override { orco_points_.push_back(p); }
		void addNormal(const Vec3 &n) override;
		int addUvValue(const Uv &uv) override { uv_values_.push_back(uv); return static_cast<int>(uv_values_.size()) - 1; }
		void setSmooth(bool smooth) override { is_smooth_ = smooth; }
		bool smoothNormals(Logger &logger, float angle) override;
		//int convertToBezierControlPoints();
		bool calculateObject(const std::unique_ptr<const Material> *material) override;

	protected:
		static float getAngleSine(const std::array<int, 3> &triangle_indices, const std::vector<Point3> &vertices);
		std::vector<std::unique_ptr<FacePrimitive>> faces_;
		std::vector<Point3> points_;
		std::vector<Point3> orco_points_;
		std::vector<Vec3> normals_;
		std::vector<Uv> uv_values_;
		bool is_smooth_ = false;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_MESH_H

