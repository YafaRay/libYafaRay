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

#include "geometry/object/object_mesh_bezier.h"
#include "common/logger.h"
#include "common/param.h"
#include "geometry/primitive/primitive_triangle_bezier.h"
#include "scene/scene.h"

BEGIN_YAFARAY

Object * MeshBezierObject::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("MeshBezierObject::factory");
		params.logContents(logger);
	}
		std::string light_name, visibility, base_object_name;
		bool is_base_object = false, has_uv = false, has_orco = false;
		int num_faces = 0, num_vertices = 0;
		int object_index = 0;
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
		params.getParam("time_range_start", time_range_start);
		params.getParam("time_range_end", time_range_end);
		auto object = new MeshBezierObject(num_vertices, num_faces, has_uv, has_orco, time_range_start, time_range_end);
		object->setName(name);
		object->setLight(scene.getLight(light_name));
		object->setVisibility(visibility::fromString(visibility));
		object->useAsBaseObject(is_base_object);
		object->setIndex(object_index);
		return object;
}

MeshBezierObject::MeshBezierObject(int num_vertices, int num_faces, bool has_uv, bool has_orco, float time_range_start, float time_range_end) : MeshObject(num_vertices * 3, num_faces, has_uv, has_orco), time_range_start_(time_range_start), time_range_end_(time_range_end)
{
}

void MeshBezierObject::addFace(const std::vector<int> &vertices, const std::vector<int> &vertices_uv, const std::unique_ptr<const Material> *material)
{
	std::unique_ptr<FacePrimitive> face;
	if(vertices.size() == 3) face = std::make_unique<TriangleBezierPrimitive>(vertices, vertices_uv, *this);
	else return; //Other primitives are not supported
	face->setMaterial(material);
	if(hasVerticesNormals()) face->setVerticesNormalsIndices(vertices);
	MeshObject::addFace(std::move(face));
}

Vec3 MeshBezierObject::getVertexNormal(BezierTimeStep time_step, int index) const
{
	return vertices_normals_vectors_[static_cast<int>(time_step) * numVertices() / 3 + index];
}

Point3 MeshBezierObject::getVertex(BezierTimeStep time_step, int index) const
{
	return points_[static_cast<int>(time_step) * numVertices() / 3 + index];
}

Point3 MeshBezierObject::getOrcoVertex(BezierTimeStep time_step, int index) const
{
	return orco_points_[static_cast<int>(time_step) * numVertices() / 3 + index];
}

bool MeshBezierObject::calculateObject(const std::unique_ptr<const Material> *material)
{
	convertToBezierControlPoints();
	return MeshObject::calculateObject(material);
}

void MeshBezierObject::convertToBezierControlPoints()
{
	//convert previous vertex for time_mid (vertex 1) to quadratic Bezier control point p1. This way when the vertex 1 is calculated as part of the Bezier curve, it will match the original vertex 1 coordinates.
	const int num_vertices = numVertices() / 3;
	for(int i = 0; i < num_vertices; ++i)
	{
		points_[num_vertices + i] = Point3{2.f * points_[num_vertices + i] - 0.5f * (points_[i] + points_[2 * num_vertices + i])};
	}
}

END_YAFARAY