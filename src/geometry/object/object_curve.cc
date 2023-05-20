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
#include "param/param.h"
#include "scene/scene.h"
#include "geometry/primitive/face_indices.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> CurveObject::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(strand_start_);
	PARAM_META(strand_end_);
	PARAM_META(strand_shape_);
	return param_meta_map;
}

CurveObject::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(strand_start_);
	PARAM_LOAD(strand_end_);
	PARAM_LOAD(strand_shape_);
}

ParamMap CurveObject::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(strand_start_);
	PARAM_SAVE(strand_end_);
	PARAM_SAVE(strand_shape_);
	return param_map;
}

std::pair<std::unique_ptr<CurveObject>, ParamResult> CurveObject::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto object{std::make_unique<CurveObject>(param_result, param_map, scene.getObjects(), scene.getMaterials(), scene.getLights())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	const auto [light, light_id, light_result]{scene.getLight(object->Object::params_.light_name_)};
	object->setLight(light_id);
	return {std::move(object), param_result};
}

CurveObject::CurveObject(ParamResult &param_result, const ParamMap &param_map, const Items <Object> &objects, const Items<Material> &materials, const Items<Light> &lights) :
		ParentClassType_t{param_result, param_map, objects, materials, lights}, params_{param_result, param_map}
{
	//if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

bool CurveObject::calculateObject(size_t material_id)
{
	const int points_size = ParentClassType_t::numVertices(0);
	for(unsigned char time_step = 0; time_step < numTimeSteps(); ++time_step)
	{
		const std::vector<Point3f> &points = getPoints(time_step);
		// Vertex extruding
		Uv<Vec3f> uv{Vec3f{0.f}, Vec3f{0.f}};
		for(int i = 0; i < points_size; i++)
		{
			const Point3f o{points[i]};
			float r;//current radius
			if(params_.strand_shape_ < 0)
			{
				r = params_.strand_start_ + math::pow((float) i / (points_size - 1), 1 + params_.strand_shape_) * (params_.strand_end_ - params_.strand_start_);
			}
			else
			{
				r = params_.strand_start_ + (1 - math::pow(((float) (points_size - i - 1)) / (points_size - 1), 1 - params_.strand_shape_)) * (params_.strand_end_ - params_.strand_start_);
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
		if(i == 0) addFace(FaceIndices<int>{{VertexIndices<int>{a_1, iu}, VertexIndices<int>{a_3, iu}, VertexIndices<int>{a_2, iu}}}, material_id);
		// Fill and strand UVs
		addFace(FaceIndices<int>{{VertexIndices<int>{a_1, iu}, VertexIndices<int>{b_2, iv}, VertexIndices<int>{b_1, iv}}}, material_id);
		addFace(FaceIndices<int>{{VertexIndices<int>{a_1, iu}, VertexIndices<int>{b_2, iu}, VertexIndices<int>{b_2, iv}}}, material_id);
		addFace(FaceIndices<int>{{VertexIndices<int>{a_2, iu}, VertexIndices<int>{b_3, iv}, VertexIndices<int>{b_2, iv}}}, material_id);
		addFace(FaceIndices<int>{{VertexIndices<int>{a_2, iu}, VertexIndices<int>{a_3, iu}, VertexIndices<int>{b_3, iv}}}, material_id);
		addFace(FaceIndices<int>{{VertexIndices<int>{b_3, iv}, VertexIndices<int>{a_3, iu}, VertexIndices<int>{a_1, iu}}}, material_id);
		addFace(FaceIndices<int>{{VertexIndices<int>{b_3, iv}, VertexIndices<int>{a_1, iu}, VertexIndices<int>{b_1, iv}}}, material_id);
	}
	// Close top
	addFace(FaceIndices<int>{{VertexIndices<int>{i, iv}, VertexIndices<int>{2 * i + points_size, iv}, VertexIndices<int>{2 * i + points_size + 1, iv}}}, material_id);
	return ParentClassType_t::calculateObject(material_id);
}

std::string CurveObject::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<object>" << std::endl;
	ss << std::string(indent_level + 1, '\t') << "<object_parameters name=\"" << getName() << "\">" << std::endl;
	ss << param_map.exportMap(indent_level + 2, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level + 1, '\t') << "</object_parameters>" << std::endl;
	ss << std::string(indent_level, '\t') << "</object>" << std::endl;
	return ss.str();
}


} //namespace yafaray
