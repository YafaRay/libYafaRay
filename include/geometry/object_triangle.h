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

#ifndef YAFARAY_OBJECT_TRIANGLE_H
#define YAFARAY_OBJECT_TRIANGLE_H

#include "object_geom.h"
#include "geometry/vector.h"
#include "geometry/uv.h"
#include "geometry/triangle.h"
#include "vector"

BEGIN_YAFARAY

/*!	This is a special version of MeshObject!
	The only difference is that it returns a Triangle instead of VTriangle,
	see declaration if Triangle for more details!
*/

class TriangleObject: public ObjectGeometric
{
	public:
		TriangleObject() = default;
		TriangleObject(int ntris, bool has_uv = false, bool has_orco = false);
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const override { return triangles_.size(); }
		virtual int getPrimitives(const Triangle **prims) const override;
		Triangle *addTriangle(const Triangle &t);
		virtual void finish();
		virtual Vec3 getVertexNormal(int index) const { return normals_[index]; }
		virtual Point3 getVertex(int index) const { return points_[index]; }
		const std::vector<Triangle> &getTriangles() const { return triangles_; }
		const std::vector<Point3> &getPoints() const { return points_; }
		const std::vector<Vec3> &getNormals() const { return normals_; }
		const std::vector<int> &getUvOffsets() const { return uv_offsets_; }
		const std::vector<Uv> &getUvValues() const { return uv_values_; }
		virtual bool hasOrco() const { return has_orco_; }
		virtual bool hasUv() const { return has_uv_; }
		virtual bool isSmooth() const { return is_smooth_; }
		virtual bool hasNormalsExported() const { return normals_exported_; }
		void addPoint(const Point3 &p) { points_.push_back(p); }
		void addNormal(const Vec3 &n, size_t last_vert_id);
		void addUvOffset(int uv_offset) { uv_offsets_.push_back(uv_offset); }
		void addUvValue(const Uv &uv) { uv_values_.push_back(uv); }
		void setSmooth(bool smooth) { is_smooth_ = smooth; }
		bool smoothMesh(float angle);

	private:
		std::vector<Triangle> triangles_;
		std::vector<Point3> points_;
		std::vector<Vec3> normals_;
		std::vector<int> uv_offsets_;
		std::vector<Uv> uv_values_;

		bool has_orco_ = false;
		bool has_uv_ = false;
		bool is_smooth_ = false;
		bool normals_exported_ = false;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_TRIANGLE_H
