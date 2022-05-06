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
#include "geometry/matrix4.h"
#include <vector>

BEGIN_YAFARAY

struct Uv;

class FacePrimitive: public Primitive
{
	public:
		FacePrimitive(const std::vector<int> &vertices_indices, std::vector<int> vertices_uv_indices, const MeshObject &mesh_object);
		//In the following functions "vertex_number" is the vertex number in the face: 0, 1, 2 in triangles, 0, 1, 2, 3 in quads, etc
		Vec3 getGeometricFaceNormal(const Matrix4 *obj_to_world, float u, float v) const override;
		const Material *getMaterial() const override { return material_->get(); }
		Bound getBound() const override { return getBound(getVertices()); }
		Bound getBound(const Matrix4 *obj_to_world) const override { return getBound(getVertices(obj_to_world)); }
		virtual void calculateGeometricFaceNormal() = 0;
		Point3 getVertex(size_t vertex_number) const { return base_mesh_object_.getVertex(vertices_[vertex_number]); } //!< Get face vertex
		Point3 getVertex(size_t vertex_number, const Matrix4 *obj_to_world) const { if(obj_to_world) return (*obj_to_world) * getVertex(vertex_number); else return getVertex(vertex_number); } //!< Get face vertex
		Point3 getOrcoVertex(size_t vertex_number) const; //!< Get face original coordinates (orco) vertex in instance objects
		Vec3 getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world) const; //!< Get face vertex normal
		Vec3 getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, const Matrix4 *obj_to_world) const; //!< Get face vertex normal
		Uv getVertexUv(size_t vertex_number) const; //!< Get face vertex Uv
		void setVerticesNormalsIndices(const std::vector<int> &vertices_normals_indices) { vertices_normals_indices_ = vertices_normals_indices; }
		void setUvIndices(const std::vector<int> &uv_indices) { vertex_uvs_ = uv_indices; }
		std::vector<int> getVerticesIndices() const { return vertices_; }
		std::vector<int> getVerticesNormalsIndices() const { return vertices_normals_indices_; }
		std::vector<int> getUvIndices() const { return vertex_uvs_; }
		std::vector<Point3> getVertices() const;
		std::vector<Point3> getVertices(const Matrix4 *obj_to_world) const;
		std::vector<Point3> getOrcoVertices() const;
		std::vector<Vec3> getVerticesNormals(const Vec3 &surface_normal) const;
		std::vector<Vec3> getVerticesNormals(const Vec3 &surface_normal, const Matrix4 *obj_to_world) const;
		std::vector<Uv> getVerticesUvs() const;
		void setMaterial(const std::unique_ptr<const Material> *material) { material_ = material; }
		size_t getSelfIndex() const { return self_index_; }
		void setSelfIndex(size_t index) { self_index_ = index; }
		static Bound getBound(const std::vector<Point3> &vertices);
		const Object *getObject() const override { return &base_mesh_object_; }
		Visibility getVisibility() const override { return base_mesh_object_.getVisibility(); }

	protected:
		size_t self_index_ = 0;
		Vec3 face_normal_geometric_;
		const MeshObject &base_mesh_object_;
		const std::unique_ptr<const Material> *material_ = nullptr;
		std::vector<int> vertices_; //!< indices in point array, referenced in mesh.
		std::vector<int> vertices_normals_indices_; //!< indices in normal array, if mesh is smoothed.
		std::vector<int> vertex_uvs_; //!< indices in uv array, if mesh has explicit uv.
};

inline FacePrimitive::FacePrimitive(const std::vector<int> &vertices_indices, std::vector<int> vertices_uv_indices, const MeshObject &mesh_object) : base_mesh_object_(mesh_object), vertices_(vertices_indices), vertices_normals_indices_(vertices_indices.size(), -1), vertex_uvs_(std::move(vertices_uv_indices))
{
}

inline Point3 FacePrimitive::getOrcoVertex(size_t vertex_number) const
{
	if(base_mesh_object_.hasOrco()) return base_mesh_object_.getOrcoVertex(vertices_[vertex_number]);
	else return getVertex(vertex_number);
}

inline Vec3 FacePrimitive::getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world) const
{
	if(vertices_normals_indices_[vertex_number] >= 0) return base_mesh_object_.getVertexNormal(vertices_normals_indices_[vertex_number]);
	else return surface_normal_world;
}

inline Vec3 FacePrimitive::getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, const Matrix4 *obj_to_world) const
{
	if(vertices_normals_indices_[vertex_number] >= 0)
	{
		if(obj_to_world) return ((*obj_to_world) * getVertexNormal(vertex_number, surface_normal_world)).normalize();
		else return getVertexNormal(vertex_number, surface_normal_world);
	}
	else return surface_normal_world;
}

inline Uv FacePrimitive::getVertexUv(size_t vertex_number) const
{
	return base_mesh_object_.getUvValues()[vertex_uvs_[vertex_number]];
}

inline std::vector<Point3> FacePrimitive::getVertices() const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Point3> result(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getVertex(vert_num);
	}
	return result;
}

inline std::vector<Point3> FacePrimitive::getVertices(const Matrix4 *obj_to_world) const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Point3> result(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getVertex(vert_num, obj_to_world);
	}
	return result;
}

inline std::vector<Point3> FacePrimitive::getOrcoVertices() const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Point3> result(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getOrcoVertex(vert_num);
	}
	return result;
}

inline std::vector<Vec3> FacePrimitive::getVerticesNormals(const Vec3 &surface_normal) const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Vec3> result(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getVertexNormal(vert_num, surface_normal);
	}
	return result;
}

inline std::vector<Vec3> FacePrimitive::getVerticesNormals(const Vec3 &surface_normal, const Matrix4 *obj_to_world) const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Vec3> result(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getVertexNormal(vert_num, surface_normal, obj_to_world);
	}
	return result;
}

inline std::vector<Uv> FacePrimitive::getVerticesUvs() const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Uv> result(num_vertices);
	//result.reserve(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getVertexUv(vert_num);
	}
	return result;
}

inline Bound FacePrimitive::getBound(const std::vector<Point3> &vertices)
{
	Point3 min_point{vertices[0]};
	Point3 max_point{vertices[0]};
	const size_t num_vertices = vertices.size();
	for(size_t vert_num = 1; vert_num < num_vertices; ++vert_num)
	{
		if(vertices[vert_num].x() < min_point.x()) min_point.x() = vertices[vert_num].x();
		if(vertices[vert_num].y() < min_point.y()) min_point.y() = vertices[vert_num].y();
		if(vertices[vert_num].z() < min_point.z()) min_point.z() = vertices[vert_num].z();
		if(vertices[vert_num].x() > max_point.x()) max_point.x() = vertices[vert_num].x();
		if(vertices[vert_num].y() > max_point.y()) max_point.y() = vertices[vert_num].y();
		if(vertices[vert_num].z() > max_point.z()) max_point.z() = vertices[vert_num].z();
	}
	return {min_point, max_point};
}

inline Vec3 FacePrimitive::getGeometricFaceNormal(const Matrix4 *obj_to_world, float u, float v) const
{
	if(obj_to_world) return ((*obj_to_world) * face_normal_geometric_).normalize();
	else return face_normal_geometric_;
}

std::ostream &operator<<(std::ostream &out, const FacePrimitive &face);

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_FACE_H
