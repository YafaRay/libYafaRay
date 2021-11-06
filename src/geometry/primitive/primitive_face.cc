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

#include "geometry/primitive/primitive_face.h"
#include "geometry/uv.h"
#include "geometry/object/object_mesh.h"
#include "geometry/bound.h"
#include "geometry/matrix4.h"

BEGIN_YAFARAY

FacePrimitive::FacePrimitive(const std::vector<int> &vertices_indices, const std::vector<int> &vertices_uv_indices, const MeshObject &mesh_object) : base_mesh_object_(mesh_object), vertices_(vertices_indices), vertex_normals_(vertices_indices.size(), -1), vertex_uvs_(vertices_uv_indices)
{
}

Point3 FacePrimitive::getVertex(size_t vertex_number, const Matrix4 *obj_to_world) const
{
	const Point3 point = base_mesh_object_.getVertex(vertices_[vertex_number]);
	if(obj_to_world) return (*obj_to_world) * point;
	else return point;
}

Point3 FacePrimitive::getOrcoVertex(size_t vertex_number) const
{
	if(base_mesh_object_.hasOrco()) return base_mesh_object_.getOrcoVertex(vertices_[vertex_number]);
	else return getVertex(vertex_number);
}

Vec3 FacePrimitive::getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, const Matrix4 *obj_to_world) const
{
	if(vertex_normals_[vertex_number] >= 0)
	{
		const Vec3 vertex_normal = base_mesh_object_.getVertexNormal(vertex_normals_[vertex_number]);
		if(obj_to_world) return ((*obj_to_world) * vertex_normal).normalize();
		else return vertex_normal;
	}
	else return surface_normal_world;
}

Uv FacePrimitive::getVertexUv(size_t vertex_number) const
{
	return base_mesh_object_.getUvValues()[vertex_uvs_[vertex_number]];
}

std::vector<Point3> FacePrimitive::getVertices(const Matrix4 *obj_to_world) const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Point3> result(num_vertices);
	//result.reserve(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getVertex(vert_num, obj_to_world);
	}
	return result;
}

std::vector<Point3> FacePrimitive::getOrcoVertices() const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Point3> result(num_vertices);
	//result.reserve(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getOrcoVertex(vert_num);
	}
	return result;
}

std::vector<Vec3> FacePrimitive::getVerticesNormals(const Vec3 &surface_normal, const Matrix4 *obj_to_world) const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Vec3> result(num_vertices);
	//result.reserve(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result[vert_num] = getVertexNormal(vert_num, surface_normal, obj_to_world);
	}
	return result;
}

std::vector<Uv> FacePrimitive::getVerticesUvs() const
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

Bound FacePrimitive::getBound(const Matrix4 *obj_to_world) const
{
	return getBound(getVertices(obj_to_world));
}

Bound FacePrimitive::getBound(const std::vector<Point3> &vertices)
{
	Point3 min_point = vertices[0];
	Point3 max_point = vertices[0];
	const size_t num_vertices = vertices.size();
	for(size_t vert_num = 1; vert_num < num_vertices; ++vert_num)
	{
		if(vertices[vert_num].x_ < min_point.x_) min_point.x_ = vertices[vert_num].x_;
		if(vertices[vert_num].y_ < min_point.y_) min_point.y_ = vertices[vert_num].y_;
		if(vertices[vert_num].z_ < min_point.z_) min_point.z_ = vertices[vert_num].z_;
		if(vertices[vert_num].x_ > max_point.x_) max_point.x_ = vertices[vert_num].x_;
		if(vertices[vert_num].y_ > max_point.y_) max_point.y_ = vertices[vert_num].y_;
		if(vertices[vert_num].z_ > max_point.z_) max_point.z_ = vertices[vert_num].z_;
	}
	return Bound(min_point, max_point);
}

Vec3 FacePrimitive::getGeometricNormal(const Matrix4 *obj_to_world, float, float) const
{
	if(obj_to_world) return ((*obj_to_world) * normal_geometric_).normalize();
	else return normal_geometric_;
}

END_YAFARAY
