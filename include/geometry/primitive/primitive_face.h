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
#include "math/interpolation.h"
#include <vector>

BEGIN_YAFARAY

template <typename T> struct Uv;

class FacePrimitive: public Primitive
{
	public:
		//Note: some methods have an unused last bool argument which is used as a dummy argument for later template specialization to be used (or not) for Matrix4 obj_to_world operations
		FacePrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object);
		//In the following functions "vertex_number" is the vertex number in the face: 0, 1, 2 in triangles, 0, 1, 2, 3 in quads, etc
		Point3 getVertex(size_t vertex_number, int time_step, const Matrix4 &obj_to_world) const;
		Point3 getVertex(size_t vertex_number, int time_step, bool = false) const;
		Vec3 getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, int time_step, bool = false) const;
		Vec3 getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, int time_step, const Matrix4 &obj_to_world) const;
		Point3 getOrcoVertex(size_t vertex_number, int time_step) const; //!< Get face original coordinates (orco) vertex in instance objects
		Uv<float> getVertexUv(size_t vertex_number) const; //!< Get face vertex Uv<float>
		size_t numVertices() const { return vertices_.size(); }
		const std::vector<int> &getVerticesIndices() const { return vertices_; }
		const std::vector<int> &getVerticesNormalsIndices() const { return vertices_normals_; }
		void generateInitialVerticesNormalsIndices() { vertices_normals_ = vertices_; }
		void setVerticesNormalsIndices(std::vector<int> &&vertices_normals_indices) { vertices_normals_ = std::move(vertices_normals_indices); }
		template<typename T=bool> std::vector<Point3> getVerticesAsVector(int time_step, const T &obj_to_world = {}) const;
		template<typename T=bool> Point3 getVertex(size_t vertex_number, const std::array<float, 3> &bezier_factors, const T &obj_to_world = {}) const;
		template<typename T=bool> Point3 getVertexAtTime(size_t vertex_number, float time, const T &obj_to_world = {}) const;
		static Bound getBound(const std::vector<Point3> &vertices);
		template<typename T=bool> Bound getBoundTimeSteps(const T &obj_to_world = {}) const;
		const Material *getMaterial() const override { return material_->get(); }
		void setMaterial(const std::unique_ptr<const Material> *material) { material_ = material; }
		const Object *getObject() const override { return &base_mesh_object_; }
		Visibility getVisibility() const override { return base_mesh_object_.getVisibility(); }
		unsigned int getObjectIndex() const override { return base_mesh_object_.getIndex(); }
		unsigned int getObjectIndexAuto() const override { return base_mesh_object_.getIndexAuto(); }
		Rgb getObjectIndexAutoColor() const override { return base_mesh_object_.getIndexAutoColor(); }
		const Light *getObjectLight() const override { return base_mesh_object_.getLight(); }
		bool hasObjectMotionBlur() const override { return base_mesh_object_.hasMotionBlur(); }

	protected:
		alignas(8) const MeshObject &base_mesh_object_;
		const std::unique_ptr<const Material> *material_ = nullptr;
		alignas(8) std::vector<int> vertices_; //!< indices in point array, referenced in mesh.
		std::vector<int> vertices_normals_; //!< indices in normal array, if mesh is smoothed.
		std::vector<int> vertex_uvs_; //!< indices in uv array, if mesh has explicit uv.
};

inline FacePrimitive::FacePrimitive(std::vector<int> &&vertices_indices, std::vector<int> &&vertices_uv_indices, const MeshObject &mesh_object) : base_mesh_object_(mesh_object), vertices_(std::move(vertices_indices)), vertices_normals_(vertices_indices.size(), -1), vertex_uvs_(std::move(vertices_uv_indices))
{
}

inline Point3 FacePrimitive::getVertex(size_t vertex_number, int time_step, bool) const
{
	return base_mesh_object_.getVertex(vertices_[vertex_number], time_step);
}

inline Point3 FacePrimitive::getVertex(size_t vertex_number, int time_step, const Matrix4 &obj_to_world) const
{
	return obj_to_world * base_mesh_object_.getVertex(vertices_[vertex_number], time_step);
}

template<typename T>
inline Point3 FacePrimitive::getVertex(size_t vertex_number, const std::array<float, 3> &bezier_factors, const T &obj_to_world) const
{
	return math::bezierInterpolate<Point3>({getVertex(vertex_number, 0, obj_to_world), getVertex(vertex_number, 1, obj_to_world), getVertex(vertex_number, 2, obj_to_world)}, bezier_factors);
}

template<typename T>
inline Point3 FacePrimitive::getVertexAtTime(size_t vertex_number, float time, const T &obj_to_world) const
{
	const float time_mapped = math::lerpSegment(time, 0.f, base_mesh_object_.getTimeRangeStart(), 1.f, base_mesh_object_.getTimeRangeEnd()); //time_mapped must be in range [0.f-1.f]
	const auto bezier_factors = math::bezierCalculateFactors(time_mapped);
	return getVertex(vertex_number, bezier_factors, obj_to_world);
}

inline Point3 FacePrimitive::getOrcoVertex(size_t vertex_number, int time_step) const
{
	if(base_mesh_object_.hasOrco(time_step)) return base_mesh_object_.getOrcoVertex(vertices_[vertex_number], time_step);
	else return getVertex(vertex_number, time_step);
}

inline Vec3 FacePrimitive::getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, int time_step, bool) const
{
	if(vertices_normals_[vertex_number] != -1) return base_mesh_object_.getVertexNormal(vertices_normals_[vertex_number], time_step);
	else return surface_normal_world;
}

inline Vec3 FacePrimitive::getVertexNormal(size_t vertex_number, const Vec3 &surface_normal_world, int time_step, const Matrix4 &obj_to_world) const
{
	if(vertices_normals_[vertex_number] != -1)
	{
		return (obj_to_world * base_mesh_object_.getVertexNormal(vertices_normals_[vertex_number], time_step)).normalize();
	}
	else return surface_normal_world;
}

inline Uv<float> FacePrimitive::getVertexUv(size_t vertex_number) const
{
	return base_mesh_object_.getUvValues()[vertex_uvs_[vertex_number]];
}

template <typename T>
inline std::vector<Point3> FacePrimitive::getVerticesAsVector(int time_step, const T &obj_to_world) const
{
	const size_t num_vertices = vertices_.size();
	std::vector<Point3> result;
	result.reserve(num_vertices);
	for(size_t vert_num = 0; vert_num < num_vertices; ++vert_num)
	{
		result.emplace_back(getVertex(vert_num, time_step, obj_to_world));
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
	return {std::move(min_point), std::move(max_point)};
}

template <typename T>
inline Bound FacePrimitive::getBoundTimeSteps(const T &obj_to_world) const
{
	std::vector<Point3> vertices;
	vertices.reserve(numVertices() * base_mesh_object_.numTimeSteps());
	for(size_t vertex_num = 0; vertex_num < numVertices(); ++vertex_num)
	{
		for(int time_step = 0; time_step < base_mesh_object_.numTimeSteps(); ++time_step)
		{
			vertices.emplace_back(getVertex(vertex_num, time_step, obj_to_world));
		}
	}
	return FacePrimitive::getBound(vertices);
}

std::ostream &operator<<(std::ostream &out, const FacePrimitive &face);

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_FACE_H
