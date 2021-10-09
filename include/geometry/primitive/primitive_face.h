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

#ifndef YAFARAY_PRIMITIVE_FACE_H
#define YAFARAY_PRIMITIVE_FACE_H

#include "primitive.h"
#include "geometry/vector.h"
#include "geometry/object/object_mesh.h"
#include <c++/9/vector>

BEGIN_YAFARAY

struct Uv;

class FacePrimitive: public Primitive
{
	public:
		FacePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object);
		//In the following functions "vertex_number" is the vertex number in the face: 0, 1, 2 in triangles, 0, 1, 2, 3 in quads, etc
		virtual Vec3 getGeometricNormal(const Matrix4 *obj_to_world = nullptr, float u = 0.f, float v = 0.f) const override;
		virtual const Material *getMaterial() const override { return material_; }
		virtual Bound getBound(const Matrix4 *obj_to_world) const override;
		virtual void calculateGeometricNormal() = 0;
		Point3 getVertex(size_t vertex_number, const Matrix4 *obj_to_world) const; //!< Get face vertex
		Point3 getOrcoVertex(size_t vertex_number) const; //!< Get face original coordinates (orco) vertex in instance objects
		Vec3 getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, const Matrix4 *obj_to_world) const; //!< Get face vertex normal
		Uv getVertexUv(size_t vertex_number) const; //!< Get face vertex Uv
		void setNormalsIndices(const std::vector<int> &normals_indices) { vertex_normals_ = normals_indices; }
		void setUvIndices(const std::vector<int> &uv_indices) { vertex_uvs_ = uv_indices; }
		const std::vector<int> getVerticesIndices() const { return vertices_; }
		const std::vector<int> getNormalsIndices() const { return vertex_normals_; }
		const std::vector<int> getUvIndices() const { return vertex_uvs_; }
		std::vector<Point3> getVertices(const Matrix4 *obj_to_world = nullptr) const;
		std::vector<Point3> getOrcoVertices() const;
		std::vector<Vec3> getVerticesNormals(const Vec3 &surface_normal, const Matrix4 *obj_to_world = nullptr) const;
		std::vector<Uv> getVerticesUvs() const;
		void setMaterial(const Material *material) { material_ = material; }
		size_t getSelfIndex() const { return self_index_; }
		void setSelfIndex(size_t index) { self_index_ = index; }
		static Bound getBound(const std::vector<Point3> &vertices);
		virtual const Object *getObject() const override { return &base_mesh_object_; }
		virtual Visibility getVisibility() const override { return base_mesh_object_.getVisibility(); }

	protected:
		size_t self_index_ = 0;
		Vec3 normal_geometric_;
		const MeshObject &base_mesh_object_;
		const Material *material_ = nullptr;
		std::vector<int> vertices_; //!< indices in point array, referenced in mesh.
		std::vector<int> vertex_normals_; //!< indices in normal array, if mesh is smoothed.
		std::vector<int> vertex_uvs_; //!< indices in uv array, if mesh has explicit uv.
};

std::ostream &operator<<(std::ostream &out, const FacePrimitive &face);

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_FACE_H
