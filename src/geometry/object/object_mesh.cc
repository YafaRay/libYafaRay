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

#include "geometry/object/object_mesh.h"
#include "geometry/primitive/primitive_triangle.h"
#include "geometry/primitive/primitive_quad.h"
#include "geometry/primitive/primitive_triangle_bezier.h"
#include "geometry/primitive/primitive_quad_bezier.h"
#include "geometry/uv.h"
#include "scene/scene.h"
#include "common/logger.h"
#include "common/param.h"
#include "math/interpolation.h"
#include <array>
#include <memory>

BEGIN_YAFARAY

Object * MeshObject::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("MeshObject::factory");
		params.logContents(logger);
	}
	std::string light_name, visibility, base_object_name;
	bool is_base_object = false, has_uv = false, has_orco = false;
	int num_faces = 0, num_vertices = 0;
	int object_index = 0;
	bool motion_blur_bezier = false;
	float time_range_start = 0.f;
	float time_range_end = 1.f;
	params.getParam("light_name", light_name);
	params.getParam("visibility", visibility);
	params.getParam("is_base_object", is_base_object);
	params.getParam("object_index", object_index);
	params.getParam("num_faces", num_faces);
	params.getParam("num_vertices", num_vertices);
	params.getParam("has_uv", has_uv);
	params.getParam("has_orco", has_orco);
	params.getParam("motion_blur_bezier", motion_blur_bezier);
	params.getParam("time_range_start", time_range_start);
	params.getParam("time_range_end", time_range_end);
	auto object = new MeshObject(num_vertices, num_faces, has_uv, has_orco, motion_blur_bezier, time_range_start, time_range_end);
	object->setName(name);
	object->setLight(scene.getLight(light_name));
	object->setVisibility(visibility::fromString(visibility));
	object->useAsBaseObject(is_base_object);
	object->setIndex(object_index);
	return object;
}

MeshObject::MeshObject(int num_vertices, int num_faces, bool has_uv, bool has_orco, bool motion_blur_bezier, float time_range_start, float time_range_end) : motion_blur_bezier_(motion_blur_bezier)
{
	faces_.reserve(num_faces);
	if(has_uv) uv_values_.reserve(num_vertices);
	if(motion_blur_bezier) time_steps_.resize(3);
	for(size_t i = 0; i < time_steps_.size(); ++i)
	{
		const float time_factor_float = time_steps_.size() > 1 ? static_cast<float>(i) / static_cast<float>(time_steps_.size() - 1) : 0.f;
		time_steps_[i].time_ = math::lerp(time_range_start, time_range_end, time_factor_float);
		time_steps_[i].points_.reserve(num_vertices);
		if(has_orco) time_steps_[i].orco_points_.reserve(num_vertices);
	}
}

MeshObject::~MeshObject() = default;

void MeshObject::addFace(std::unique_ptr<FacePrimitive> &&face)
{
	if(face)
	{
		faces_.emplace_back(std::move(face));
	}
}

void MeshObject::addFace(std::vector<int> &&vertices, std::vector<int> &&vertices_uv, const std::unique_ptr<const Material> *material)
{
	std::unique_ptr<FacePrimitive> face;
	if(vertices.size() == 3)
	{
		if(hasMotionBlurBezier() && !ObjectBase::isBaseObject()) face = std::make_unique<TriangleBezierPrimitive>(std::move(vertices), std::move(vertices_uv), *this);
		else face = std::make_unique<TrianglePrimitive>(std::move(vertices), std::move(vertices_uv), *this);
	}
	else if(vertices.size() == 4)
	{
		if(hasMotionBlurBezier() && !ObjectBase::isBaseObject()) face = std::make_unique<QuadBezierPrimitive>(std::move(vertices), std::move(vertices_uv), *this);
		else face = std::make_unique<QuadPrimitive>(std::move(vertices), std::move(vertices_uv), *this);
	}
	else return; //Other primitives are not supported
	face->setMaterial(material);
	if(hasVerticesNormals(0)) face->generateInitialVerticesNormalsIndices();
	addFace(std::move(face));
}

bool MeshObject::calculateObject(const std::unique_ptr<const Material> *)
{
	if(hasMotionBlurBezier()) convertToBezierControlPoints();
	faces_.shrink_to_fit();
	for(auto &time_step : time_steps_)
	{
		time_step.points_.shrink_to_fit();
		time_step.orco_points_.shrink_to_fit();
		time_step.vertices_normals_.shrink_to_fit();
	}
	uv_values_.shrink_to_fit();
	return true;
}

std::vector<const Primitive *> MeshObject::getPrimitives() const
{
	std::vector<const Primitive *> primitives;
	primitives.reserve(faces_.size());
	for(const auto &face : faces_) primitives.emplace_back(face.get());
	return primitives;
}

void MeshObject::addVertexNormal(Vec3 &&n, int time_step)
{
	const size_t points_size = time_steps_[time_step].points_.size();
	if(time_steps_[time_step].vertices_normals_.size() < points_size) time_steps_[time_step].vertices_normals_.reserve(points_size);
	time_steps_[time_step].vertices_normals_.emplace_back(n);
}

float MeshObject::getAngleSine(const std::array<int, 3> &triangle_indices, const std::vector<Point3> &vertices)
{
	const Vec3 edge_1{vertices[triangle_indices[1]] - vertices[triangle_indices[0]]};
	const Vec3 edge_2{vertices[triangle_indices[2]] - vertices[triangle_indices[0]]};
	return edge_1.sinFromVectors(edge_2);
}

bool MeshObject::smoothVerticesNormals(Logger &logger, float angle)
{
	const size_t points_size = numVertices(0);
	for(auto &time_step : time_steps_) time_step.vertices_normals_.resize(points_size, {0, 0, 0});

	if(angle >= 180)
	{
		for(int time_step = 0; time_step < numTimeSteps(); ++time_step)
		{
			for(auto &face : faces_)
			{
				const Vec3 n{face->getGeometricNormal({0.f, 0.f}, static_cast<float>(time_step) / static_cast<float>(numTimeSteps()), false)};
				const std::vector<int> &vert_indices = face->getVerticesIndices();
				const size_t num_indices = vert_indices.size();
				for(size_t relative_vertex = 0; relative_vertex < num_indices; ++relative_vertex)
				{
					time_steps_[time_step].vertices_normals_[vert_indices[relative_vertex]] += n * getAngleSine({vert_indices[relative_vertex], vert_indices[(relative_vertex + 1) % num_indices], vert_indices[(relative_vertex + 2) % num_indices]}, time_steps_[time_step].points_);
				}
				face->generateInitialVerticesNormalsIndices();
			}
			for(auto &normal : time_steps_[time_step].vertices_normals_) normal.normalize();
		}
	}
	else if(angle > 0.1f) // angle dependant smoothing
	{
		for(int time_step = 0; time_step < numTimeSteps(); ++time_step)
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
					points_angles_sines[vert_indices[relative_vertex]].emplace_back(getAngleSine({vert_indices[relative_vertex], vert_indices[(relative_vertex + 1) % num_indices], vert_indices[(relative_vertex + 2) % num_indices]}, time_steps_[time_step].points_));
					points_faces[vert_indices[relative_vertex]].emplace_back(face.get());
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
					const Vec3 face_normal{point_face->getGeometricNormal({}, static_cast<float>(time_step) / static_cast<float>(numTimeSteps()), false)};
					Vec3 vertex_normal{face_normal * points_angles_sines[point_id][j]};
					int k = 0;
					for(const auto &point_face_2 : points_faces[point_id])
					{
						if(point_face == point_face_2)
						{
							k++;
							continue;
						}
						const Vec3 face_2_normal{point_face_2->getGeometricNormal({}, static_cast<float>(time_step) / static_cast<float>(numTimeSteps()), false)};
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
							normal_idx = static_cast<int>(time_steps_[time_step].vertices_normals_.size());
							vertex_normals.emplace_back(vertex_normal);
							vertex_normals_indices.emplace_back(normal_idx);
							time_steps_[time_step].vertices_normals_.emplace_back(vertex_normal);
						}
					}
					// set vertex normal to idx
					const std::vector<int> vertices_indices = point_face->getVerticesIndices();
					std::vector<int> normals_indices = point_face->getVerticesNormalsIndices();
					if(normals_indices.empty()) normals_indices = std::vector<int>(vertices_indices.size());
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
						point_face->setVerticesNormalsIndices(std::move(normals_indices));
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
	}
	setSmooth(true);
	return true;
}

void MeshObject::convertToBezierControlPoints()
{
	//convert previous vertex for time_mid (vertex 1) to quadratic Bezier control point p1. This way when the vertex 1 is calculated as part of the Bezier curve, it will match the original vertex 1 coordinates.
	const int num_vertices = numVertices(0);
	if(numTimeSteps() < 3) return;
	for(int i = 0; i < num_vertices; ++i)
	{
		time_steps_[1].points_[i] = math::bezierFindControlPoint<Point3>({time_steps_[0].points_[i], time_steps_[1].points_[i], time_steps_[2].points_[i]});
	}
}

END_YAFARAY
