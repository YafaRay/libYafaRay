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

#include "scene/yafaray/object_mesh.h"
#include "scene/yafaray/primitive_triangle.h"
#include "geometry/uv.h"
#include "scene/scene.h"
#include "common/logger.h"
#include "common/param.h"
#include <array>

BEGIN_YAFARAY

std::unique_ptr<Object> MeshObject::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("MeshObject::factory");
		params.logContents(logger);
	}
	std::string name, light_name, visibility, base_object_name;
	bool is_base_object = false, has_uv = false, has_orco = false;
	int num_faces = 0, num_vertices = 0;
	int object_index = 0;
	params.getParam("name", name);
	params.getParam("light_name", light_name);
	params.getParam("visibility", visibility);
	params.getParam("is_base_object", is_base_object);
	params.getParam("object_index", object_index);
	params.getParam("num_faces", num_faces);
	params.getParam("num_vertices", num_vertices);
	params.getParam("has_uv", has_uv);
	params.getParam("has_orco", has_orco);
	auto object = std::unique_ptr<MeshObject>(new MeshObject(num_vertices, num_faces, has_uv, has_orco));
	object->setName(name);
	object->setLight(scene.getLight(light_name));
	object->setVisibility(visibility::fromString(visibility));
	object->useAsBaseObject(is_base_object);
	object->setObjectIndex(object_index);
	return object;
}

MeshObject::MeshObject(int num_vertices, int num_faces, bool has_uv, bool has_orco)
{
	faces_.reserve(num_faces);
	points_.reserve(num_vertices);
	if(has_orco) orco_points_.reserve(num_vertices);
	if(has_uv) uv_values_.reserve(num_vertices);
}

MeshObject::~MeshObject()
{
}

void MeshObject::addFace(std::unique_ptr<FacePrimitive> face)
{
	if(face)
	{
		face->setSelfIndex(faces_.size());
		faces_.emplace_back(std::move(face));
	}
}

void MeshObject::addFace(const std::vector<int> &vertices, const std::vector<int> &vertices_uv, const Material *mat)
{
	std::unique_ptr<FacePrimitive> face;
	if(vertices.size() == 3) face = std::unique_ptr<FacePrimitive>(new TrianglePrimitive(vertices, vertices_uv, *this));
	else return; //Other primitives are not supported
	face->setMaterial(mat);
	if(hasNormalsExported()) face->setNormalsIndices(vertices);
	addFace(std::move(face));
}

void MeshObject::calculateNormals()
{
	for(auto &face : faces_) face->calculateGeometricNormal();
}

bool MeshObject::calculateObject(const Material *)
{
	faces_.shrink_to_fit();
	points_.shrink_to_fit();
	if(!orco_points_.empty()) orco_points_.shrink_to_fit();
	if(!uv_values_.empty()) uv_values_.shrink_to_fit();
	calculateNormals();
	return true;
}

const std::vector<const Primitive *> MeshObject::getPrimitives() const
{
	std::vector<const Primitive *> primitives;
	primitives.reserve(faces_.size());
	for(const auto &face : faces_) primitives.push_back(face.get());
	return primitives;
}

void MeshObject::addNormal(const Vec3 &n)
{
	const size_t points_size = points_.size();
	if(normals_.size() < points_size) normals_.reserve(points_size);
	normals_.push_back(n);
}

float getAngleSine_global(const std::array<int, 3> &triangle_indices, const std::vector<Point3> &vertices)
{
	const Vec3 edge_1 = vertices[triangle_indices[1]] - vertices[triangle_indices[0]];
	const Vec3 edge_2 = vertices[triangle_indices[2]] - vertices[triangle_indices[0]];
	return edge_1.sinFromVectors(edge_2);
}

bool MeshObject::smoothNormals(Logger &logger, float angle)
{
	const size_t points_size = points_.size();
	normals_.resize(points_size, {0, 0, 0});

	if(angle >= 180)
	{
		for(auto &face : faces_)
		{
			const Vec3 n = face->getGeometricNormal();
			const std::vector<int> vert_indices = face->getVerticesIndices();
			const size_t num_indices = vert_indices.size();
			for(size_t relative_vertex = 0; relative_vertex < num_indices; ++relative_vertex)
			{
				normals_[vert_indices[relative_vertex]] += n * getAngleSine_global({vert_indices[relative_vertex], vert_indices[(relative_vertex + 1) % num_indices], vert_indices[(relative_vertex + 2) % num_indices]}, points_);
			}
			face->setNormalsIndices(vert_indices);
		}
		for(size_t idx = 0; idx < normals_.size(); ++idx) normals_[idx].normalize();
	}
	else if(angle > 0.1f) // angle dependant smoothing
	{
		const float angle_threshold = math::cos(math::degToRad(angle));
		// create list of faces that include given vertex
		std::vector<std::vector<FacePrimitive *>> points_faces(points_size);
		std::vector<std::vector<float>> points_angles_sines(points_size);
		for(auto &face : faces_)
		{
			const std::vector<int> vert_indices = face->getVerticesIndices();
			const size_t num_indices = vert_indices.size();
			for(size_t relative_vertex = 0; relative_vertex < num_indices; ++relative_vertex)
			{
				points_angles_sines[vert_indices[relative_vertex]].push_back(getAngleSine_global({vert_indices[relative_vertex], vert_indices[(relative_vertex + 1) % num_indices], vert_indices[(relative_vertex + 2) % num_indices]}, points_));
				points_faces[vert_indices[relative_vertex]].push_back(face.get());
			}
		}
		for(size_t point_id = 0; point_id < points_size; ++point_id)
		{
			int j = 0;
			std::vector<Vec3> vertex_normals;
			std::vector<int> vertex_normals_indices;
			for(const auto &point_face : points_faces[point_id])
			{
				bool smooth = false;
				// calculate vertex normal for face
				const Vec3 face_normal = point_face->getGeometricNormal();
				Vec3 vertex_normal = face_normal * points_angles_sines[point_id][j];
				int k = 0;
				for(const auto &point_face_2 : points_faces[point_id])
				{
					if(point_face == point_face_2)
					{
						k++;
						continue;
					}
					Vec3 face_2_normal = point_face_2->getGeometricNormal();
					if((face_normal * face_2_normal) > angle_threshold)
					{
						smooth = true;
						vertex_normal += face_2_normal * points_angles_sines[point_id][k];
					}
					k++;
				}
				int normal_idx = -1;
				if(smooth)
				{
					vertex_normal.normalize();
					//search for existing normal
					const size_t num_vertex_normals = vertex_normals.size();
					for(size_t vertex_normal_id = 0; vertex_normal_id < num_vertex_normals; ++vertex_normal_id)
					{
						if(vertex_normal * vertex_normals[vertex_normal_id] > 0.999f)
						{
							normal_idx = vertex_normals_indices[vertex_normal_id];
							break;
						}
					}
					// create new if none found
					if(normal_idx == -1)
					{
						normal_idx = normals_.size();
						vertex_normals.push_back(vertex_normal);
						vertex_normals_indices.push_back(normal_idx);
						normals_.push_back(vertex_normal);
					}
				}
				// set vertex normal to idx
				const std::vector<int> vertices_indices = point_face->getVerticesIndices();
				std::vector<int> normals_indices = point_face->getNormalsIndices();
				bool smooth_ok = false;
				const size_t num_vertices = vertices_indices.size();
				for(size_t relative_vertex = 0; relative_vertex < num_vertices; ++relative_vertex)
				{
					if(vertices_indices[relative_vertex] == static_cast<int>(point_id))
					{
						normals_indices[relative_vertex] = normal_idx;
						smooth_ok = true;
						break;
					}
				}
				if(smooth_ok)
				{
					point_face->setNormalsIndices(normals_indices);
					j++;
				}
				else
				{
					logger.logError("Mesh smoothing error!");
					return false;
				}
			}
		}
	}
	setSmooth(true);
	return true;
}

MeshObject *MeshObject::getMeshFromObject(Logger &logger, Object *object)
{
	if(!object->isMesh())
	{
		logger.logError("Scene: the object '", object->getName(), "' is not a mesh object");
		return nullptr;
	}
	else return static_cast<MeshObject *>(object);
}

/*int MeshObject::convertToBezierControlPoints()
{
	const int n = points_.size();
	if(n % 3 == 0)
	{
		//convert point 2 to quadratic bezier control point
		points_[n - 2] = 2.f * points_[n - 2] - 0.5f * (points_[n - 3] + points_[n - 1]);
	}
	return (n - 1) / 3;
}*/

END_YAFARAY
