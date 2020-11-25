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

#include "object_yafaray.h"
#include "geometry/vector.h"
#include "geometry/uv.h"
#include <vector>

BEGIN_YAFARAY

struct Uv;
class FacePrimitive;
class Material;

class MeshObject : public ObjectYafaRay
{
	public:
		static Object *factory(ParamMap &params, const Scene &scene);
		MeshObject(int num_vertices, int num_faces, bool has_uv = false, bool has_orco = false);
		virtual ~MeshObject() override;
		virtual bool isMesh() const override { return true; }
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const override { return faces_.size(); }
		virtual const std::vector<const Primitive *> getPrimitives() const override;
		int lastVertexId() const { return points_.size() - 1; }
		Vec3 getVertexNormal(int index) const { return normals_[index]; }
		Point3 getVertex(int index) const { return points_[index]; }
		Point3 getOrcoVertex(int index) const { return orco_points_[index]; }
		int numVertices() const { return points_.size(); }
		int numNormals() const { return normals_.size(); }
		void addFace(FacePrimitive *face);
		void addFace(const std::vector<int> &vertices, const std::vector<int> &vertices_uv, const Material *mat);
		void calculateNormals();
		const std::vector<FacePrimitive *> getFaces() const { return faces_; }
		const std::vector<Point3> &getPoints() const { return points_; }
		const std::vector<Point3> &getOrcoPoints() const { return orco_points_; }
		const std::vector<Vec3> &getNormals() const { return normals_; }
		const std::vector<Uv> &getUvValues() const { return uv_values_; }
		bool hasOrco() const { return !orco_points_.empty(); }
		bool hasUv() const { return !uv_values_.empty(); }
		bool isSmooth() const { return is_smooth_; }
		bool hasNormalsExported() const { return !normals_.empty(); }
		void addPoint(const Point3 &p) { points_.push_back(p); }
		void addOrcoPoint(const Point3 &p) { orco_points_.push_back(p); }
		void addNormal(const Vec3 &n);
		int addUvValue(const Uv &uv) { uv_values_.push_back(uv); return static_cast<int>(uv_values_.size()) - 1; }
		void setSmooth(bool smooth) { is_smooth_ = smooth; }
		bool smoothNormals(float angle);
		//int convertToBezierControlPoints();
		virtual bool calculateObject(const Material *material) override;
		static MeshObject *getMeshFromObject(Object *object);

	protected:
		std::vector<FacePrimitive *> faces_;
		std::vector<Point3> points_;
		std::vector<Point3> orco_points_;
		std::vector<Vec3> normals_;
		std::vector<Uv> uv_values_;
		bool is_smooth_ = false;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_MESH_H

