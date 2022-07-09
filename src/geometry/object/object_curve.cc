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

#include "geometry/object/object_curve.h"
#include "common/logger.h"
#include "common/param.h"
#include "scene/scene.h"

namespace yafaray {

Object * CurveObject::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("CurveObject::factory");
		params.logContents(logger);
	}
	std::string light_name, visibility, base_object_name;
	bool is_base_object = false, has_uv = false, has_orco = false;
	int num_vertices = 0;
	int object_index = 0;
	float strand_start = 0.01f;
	float strand_end = 0.01f;
	float strand_shape = 0.f;
	bool motion_blur_bezier = false;
	float time_range_start = 0.f;
	float time_range_end = 1.f;
	params.getParam("light_name", light_name);
	params.getParam("visibility", visibility);
	params.getParam("is_base_object", is_base_object);
	params.getParam("object_index", object_index);
	params.getParam("num_vertices", num_vertices);
	params.getParam("strand_start", strand_start);
	params.getParam("strand_end", strand_end);
	params.getParam("strand_shape", strand_shape);
	params.getParam("has_uv", has_uv);
	params.getParam("has_orco", has_orco);
	params.getParam("motion_blur_bezier", motion_blur_bezier);
	params.getParam("time_range_start", time_range_start);
	params.getParam("time_range_end", time_range_end);
	auto object = new CurveObject(num_vertices, strand_start, strand_end, strand_shape, has_uv, has_orco, motion_blur_bezier, time_range_start, time_range_end);
	object->setName(name);
	object->setLight(scene.getLight(light_name));
	object->setVisibility(visibility::fromString(visibility));
	object->useAsBaseObject(is_base_object);
	object->setIndex(object_index);
	return object;
}

CurveObject::CurveObject(int num_vertices, float strand_start, float strand_end, float strand_shape, bool has_uv, bool has_orco, bool motion_blur_bezier, float time_range_start, float time_range_end) : MeshObject(num_vertices, 2 * (num_vertices - 1), has_uv, has_orco, motion_blur_bezier, time_range_start, time_range_end), strand_start_(strand_start), strand_end_(strand_end), strand_shape_(strand_shape)
{
}

bool CurveObject::calculateObject(const std::unique_ptr<const Material> *material)
{
	const int points_size = MeshObject::numVertices(0);
	for(int time_step = 0; time_step < numTimeSteps(); ++time_step)
	{
		const std::vector<Point3f> &points = getPoints(time_step);
		// Vertex extruding
		Uv<Vec3f> uv{Vec3f{0.f}, Vec3f{0.f}};
		for(int i = 0; i < points_size; i++)
		{
			const Point3f o{points[i]};
			float r;//current radius
			if(strand_shape_ < 0)
			{
				r = strand_start_ + math::pow((float) i / (points_size - 1), 1 + strand_shape_) * (strand_end_ - strand_start_);
			}
			else
			{
				r = strand_start_ + (1 - math::pow(((float) (points_size - i - 1)) / (points_size - 1), 1 - strand_shape_)) * (strand_end_ - strand_start_);
			}
			// Last point keep previous tangent plane
			if(i < points_size - 1)
			{
				Vec3f normal{points[i + 1] - points[i]};
				normal.normalize();
				uv = Vec3f::createCoordsSystem(normal);
			}
			// TODO: thikness?
			Point3f a{o - (0.5f * r * uv.v_) - 1.5f * r / math::sqrt(3.f) * uv.u_};
			Point3f b{o - (0.5f * r * uv.v_) + 1.5f * r / math::sqrt(3.f) * uv.u_};

			addPoint(std::move(a), time_step);
			addPoint(std::move(b), time_step);
		}
	}

	// Face fill
	int iv = 0;
	int i;
	for(i = 0; i < points_size - 1; i++)
	{
		// 1D particles UV mapping
		const float su = static_cast<float>(i) / (points_size - 1);
		const float sv = su + 1. / (points_size - 1);
		const int iu = addUvValue({su, su});
		iv = addUvValue({sv, sv});
		const int a_1 = i;
		const int a_2 = 2 * i + points_size;
		const int a_3 = a_2 + 1;
		const int b_1 = i + 1;
		const int b_2 = a_2 + 2;
		const int b_3 = b_2 + 1;
		// Close bottom
		if(i == 0) addFace({a_1, a_3, a_2}, {iu, iu, iu}, material);
		// Fill and strand UVs
		addFace({a_1, b_2, b_1}, {iu, iv, iv}, material);
		addFace({a_1, a_2, b_2}, {iu, iu, iv}, material);
		addFace({a_2, b_3, b_2}, {iu, iv, iv}, material);
		addFace({a_2, a_3, b_3}, {iu, iu, iv}, material);
		addFace({b_3, a_3, a_1}, {iv, iu, iu}, material);
		addFace({b_3, a_1, b_1}, {iv, iu, iv}, material);
	}
	// Close top
	addFace({i, 2 * i + points_size, 2 * i + points_size + 1}, {iv, iv, iv}, material);
	return MeshObject::calculateObject(material);
}



} //namespace yafaray
