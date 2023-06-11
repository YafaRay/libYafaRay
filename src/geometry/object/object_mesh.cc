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
#include "geometry/primitive/primitive_polygon.h"
#include "scene/scene.h"
#include "common/logger.h"
#include "param/param.h"
#include "math/interpolation.h"
#include "material/material.h"
#include <array>
#include <memory>

namespace yafaray {

std::map<std::string, const ParamMeta *> MeshObject::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(num_faces_);
	PARAM_META(num_vertices_);
	PARAM_META(has_uv_);
	PARAM_META(has_orco_);
	return param_meta_map;
}

MeshObject::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(num_faces_);
	PARAM_LOAD(num_vertices_);
	PARAM_LOAD(has_uv_);
	PARAM_LOAD(has_orco_);
}

ParamMap MeshObject::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(num_faces_);
	PARAM_SAVE(num_vertices_);
	PARAM_SAVE(has_uv_);
	PARAM_SAVE(has_orco_);
	return param_map;
}

std::pair<std::unique_ptr<MeshObject>, ParamResult> MeshObject::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto object{std::make_unique<MeshObject>(param_result, param_map, scene.getObjects(), scene.getMaterials(), scene.getLights())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	const auto [light, light_id, light_result]{scene.getLight(object->ParentClassType_t::params_.light_name_)};
	object->setLight(light_id);
	return {std::move(object), param_result};
}

MeshObject::MeshObject(ParamResult &param_result, const ParamMap &param_map, const Items <Object> &objects, const Items<Material> &materials, const Items<Light> &lights) : ParentClassType_t{param_result, param_map, objects, materials, lights}, params_{param_result, param_map}
{
	//if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	const int num_faces = calculateNumFaces();
	faces_.reserve(num_faces);
	if(params_.has_uv_) uv_values_.reserve(params_.num_vertices_);
	if(ParentClassType_t::params_.motion_blur_bezier_) time_steps_.resize(3);
	for(size_t i = 0; i < time_steps_.size(); ++i)
	{
		const float time_factor_float = time_steps_.size() > 1 ? static_cast<float>(i) / static_cast<float>(time_steps_.size() - 1) : 0.f;
		time_steps_[i].time_ = math::lerp(ParentClassType_t::params_.time_range_start_, ParentClassType_t::params_.time_range_end_, time_factor_float);
		time_steps_[i].points_.reserve(params_.num_vertices_);
		if(params_.has_orco_) time_steps_[i].orco_points_.reserve(params_.num_vertices_);
	}
}

MeshObject::~MeshObject() = default; //This "default" custom destructor seems unnecessary but do not remove it, this is to prevent a compilation error

void MeshObject::addFace(std::unique_ptr<FacePrimitive> &&face)
{
	if(face)
	{
		faces_.emplace_back(std::move(face));
	}
}

void MeshObject::addFace(const FaceIndices<int> &face_indices, size_t material_id)
{
	std::unique_ptr<FacePrimitive> face;
	if(face_indices.isQuad())
	{
		if(hasMotionBlurBezier() && !ParentClassType_t::isBaseObject()) face = std::make_unique<PrimitivePolygon<float, 4, MotionBlurType::Bezier>>(face_indices, *this);
		else face = std::make_unique<PrimitivePolygon<float, 4, MotionBlurType::None>>(face_indices, *this);
	}
	else
	{
		if(hasMotionBlurBezier() && !ParentClassType_t::isBaseObject()) face = std::make_unique<PrimitivePolygon<float, 3, MotionBlurType::Bezier>>(face_indices, *this);
		else face = std::make_unique<PrimitivePolygon<float, 3, MotionBlurType::None>>(face_indices, *this);
	}
	face->setMaterial(material_id);
	if(hasVerticesNormals(0)) face->generateInitialVerticesNormalsIndices();
	addFace(std::move(face));
}

bool MeshObject::calculateObject(size_t)
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

void MeshObject::addVertexNormal(Vec3f &&n, unsigned char time_step)
{
	const size_t points_size = time_steps_[time_step].points_.size();
	if(time_steps_[time_step].vertices_normals_.size() < points_size) time_steps_[time_step].vertices_normals_.reserve(points_size);
	time_steps_[time_step].vertices_normals_.emplace_back(n);
}

float MeshObject::getAngleSine(const std::array<int, 3> &triangle_indices, const std::vector<Point3f> &vertices)
{
	const Vec3f edge_1{vertices[triangle_indices[1]] - vertices[triangle_indices[0]]};
	const Vec3f edge_2{vertices[triangle_indices[2]] - vertices[triangle_indices[0]]};
	return edge_1.sinFromVectors(edge_2);
}

bool MeshObject::smoothVerticesNormals(Logger &logger, float angle)
{
	const size_t points_size = numVertices(0);
	for(auto &time_step : time_steps_) time_step.vertices_normals_.resize(points_size, {{0, 0, 0}});

	if(angle >= 180.f)
	{
		for(unsigned char time_step = 0; time_step < numTimeSteps(); ++time_step)
		{
			for(auto &face : faces_)
			{
				const Vec3f n{face->getGeometricNormal({0.f, 0.f}, static_cast<float>(time_step) / static_cast<float>(numTimeSteps()), false)};
				const FaceIndices<int> &indices{face->getFaceIndices()};
				const int num_indices{indices.numVertices()};
				for(int vertex_number = 0; vertex_number < num_indices; ++vertex_number)
				{
					time_steps_[time_step].vertices_normals_[indices[vertex_number].vertex_] += n * getAngleSine({indices[vertex_number].vertex_, indices[(vertex_number + 1) % num_indices].vertex_, indices[(vertex_number + 2) % num_indices].vertex_}, time_steps_[time_step].points_);
				}
				face->generateInitialVerticesNormalsIndices();
			}
			for(auto &normal : time_steps_[time_step].vertices_normals_) normal.normalize();
		}
	}
	else if(angle > 0.1f) // angle dependant smoothing
	{
		for(unsigned char time_step = 0; time_step < numTimeSteps(); ++time_step)
		{
			const float angle_threshold = math::cos(math::degToRad(angle));
			// create list of faces that include given vertex
			std::vector<std::vector<FacePrimitive *>> points_faces(points_size);
			std::vector<std::vector<float>> points_angles_sines(points_size);
			for(auto &face : faces_)
			{
				const FaceIndices indices{face->getFaceIndices()};
				const int num_indices{indices.numVertices()};
				for(int vertex_number = 0; vertex_number < num_indices; ++vertex_number)
				{
					points_angles_sines[indices[vertex_number].vertex_].emplace_back(getAngleSine({indices[vertex_number].vertex_, indices[(vertex_number + 1) % num_indices].vertex_, indices[(vertex_number + 2) % num_indices].vertex_}, time_steps_[time_step].points_));
					points_faces[indices[vertex_number].vertex_].emplace_back(face.get());
				}
			}
			for(size_t point_id = 0; point_id < points_size; ++point_id)
			{
				int j = 0;
				std::vector<Vec3f> vertex_normals;
				std::vector<int> vertex_normals_indices;
				for(const auto &point_face : points_faces[point_id])
				{
					bool smooth = false;
					// calculate vertex normal for face
					const Vec3f face_normal{point_face->getGeometricNormal({}, static_cast<float>(time_step) / static_cast<float>(numTimeSteps()), false)};
					Vec3f vertex_normal{face_normal * points_angles_sines[point_id][j]};
					int k = 0;
					for(const auto &point_face_2 : points_faces[point_id])
					{
						if(point_face == point_face_2)
						{
							k++;
							continue;
						}
						const Vec3f face_2_normal{point_face_2->getGeometricNormal({}, static_cast<float>(time_step) / static_cast<float>(numTimeSteps()), false)};
						if((face_normal * face_2_normal) > angle_threshold)
						{
							smooth = true;
							vertex_normal += face_2_normal * points_angles_sines[point_id][k];
						}
						k++;
					}
					int normal_idx{math::invalid<int>};
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
						if(normal_idx == math::invalid<int>)
						{
							normal_idx = static_cast<int>(time_steps_[time_step].vertices_normals_.size());
							vertex_normals.emplace_back(vertex_normal);
							vertex_normals_indices.emplace_back(normal_idx);
							time_steps_[time_step].vertices_normals_.emplace_back(vertex_normal);
						}
					}
					// set vertex normal to idx
					FaceIndices<int> &face_indices{point_face->getFaceIndices()};
					bool smooth_ok = false;
					const int num_vertices{face_indices.numVertices()};
					for(int vertex_number = 0; vertex_number < num_vertices; ++vertex_number)
					{
						if(face_indices[vertex_number].vertex_ == static_cast<int>(point_id))
						{
							face_indices[vertex_number].normal_ = normal_idx;
							smooth_ok = true;
							break;
						}
					}
					if(smooth_ok) ++j;
					else
					{
						logger.logError("Mesh smoothing error!");
						return false;
					}
				}
			}
		}
	}
	setSmooth(true, angle);
	return true;
}

void MeshObject::convertToBezierControlPoints()
{
	//convert previous vertex for time_mid (vertex 1) to quadratic Bezier control point p1. This way when the vertex 1 is calculated as part of the Bezier curve, it will match the original vertex 1 coordinates.
	const int num_vertices = numVertices(0);
	if(numTimeSteps() < 3) return;
	for(int i = 0; i < num_vertices; ++i)
	{
		time_steps_[1].points_[i] = math::bezierFindControlPoint<float, Point3f>({time_steps_[0].points_[i], time_steps_[1].points_[i], time_steps_[2].points_[i]});
	}
}

std::string MeshObject::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<object>" << std::endl;
	ss << std::string(indent_level + 1, '\t') << "<parameters name=\"" << getName() << "\">" << std::endl;
	ss << param_map.exportMap(indent_level + 2, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level + 1, '\t') << "</parameters>" << std::endl;
	for(const auto &time_step : time_steps_)
	{
		for(size_t i = 0; i < time_step.points_.size(); ++i)
		{
			const auto &point{time_step.points_[i]};
			ss << std::string(indent_level + 1, '\t') << "<p x=\"" << point[0] << "\" y=\"" << point[1] << "\" z=\"" << point[2] << "\"";
			if(i < time_step.orco_points_.size())
			{
				const auto &orco{time_step.orco_points_[i]};
				ss << " ox=\"" << orco[0] << "\" oy=\"" << orco[1] << "\" oz=\"" << orco[2] << "\"";
			}
			ss << "/>" << std::endl;
		}
		for(const auto &vertex_normal : time_step.vertices_normals_)
		{
			ss << std::string(indent_level + 1, '\t') << "<n x=\"" << vertex_normal[0] << "\" y=\"" << vertex_normal[1] << "\" z=\"" << vertex_normal[2] << "\"/>" << std::endl;
		}
	}
	const Material *material_previous{nullptr};
	for(const auto primitive : getPrimitives())
	{
		const Material *material{primitive->getMaterial()};
		if(material_previous != material)
		{
			ss << std::string(indent_level + 1, '\t') << "<material_ref sval=\"" << material->getName() << "\"/>" << std::endl;
			material_previous = material;
		}
		ss << primitive->exportToString(indent_level + 1, container_export_type, only_export_non_default_parameters);
	}
	if(isSmooth()) ss << std::string(indent_level + 1, '\t') << "<smooth angle=\"" << getSmoothAngle() << "\"/>" << std::endl;
	ss << std::string(indent_level, '\t') << "</object>" << std::endl;
	return ss.str();
	//FIXME PENDING UV and TIME_STEPS!
}

} //namespace yafaray
