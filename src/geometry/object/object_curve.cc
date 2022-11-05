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

CurveObject::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(strand_start_);
	PARAM_LOAD(strand_end_);
	PARAM_LOAD(strand_shape_);
}

ParamMap CurveObject::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(strand_start_);
	PARAM_SAVE(strand_end_);
	PARAM_SAVE(strand_shape_);
	PARAM_SAVE_END;
}

ParamMap CurveObject::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<CurveObject>, ParamResult> CurveObject::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto object{std::make_unique<ThisClassType_t>(param_result, param_map, scene.getObjects(), scene.getMaterials())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	object->setLight(scene.getLight(object->Object::params_.light_name_));
	return {std::move(object), param_result};
}

CurveObject::CurveObject(ParamResult &param_result, const ParamMap &param_map, const SceneItems <Object> &objects, const SceneItems<Material> &materials) :
		ParentClassType_t{param_result, param_map, objects, materials}, params_{param_result, param_map}
{
	//if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

bool CurveObject::calculateObject(size_t material_id)
{
	const int points_size = ParentClassType_t::numVertices(0);
	for(int time_step = 0; time_step < numTimeSteps(); ++time_step)
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
		if(i == 0) addFace(FaceIndices{{VertexIndices{a_1, iu}, VertexIndices{a_3, iu}, VertexIndices{a_2, iu}}}, material_id);
		// Fill and strand UVs
		addFace(FaceIndices{{VertexIndices{a_1, iu}, VertexIndices{b_2, iv}, VertexIndices{b_1, iv}}}, material_id);
		addFace(FaceIndices{{VertexIndices{a_1, iu}, VertexIndices{b_2, iu}, VertexIndices{b_2, iv}}}, material_id);
		addFace(FaceIndices{{VertexIndices{a_2, iu}, VertexIndices{b_3, iv}, VertexIndices{b_2, iv}}}, material_id);
		addFace(FaceIndices{{VertexIndices{a_2, iu}, VertexIndices{a_3, iu}, VertexIndices{b_3, iv}}}, material_id);
		addFace(FaceIndices{{VertexIndices{b_3, iv}, VertexIndices{a_3, iu}, VertexIndices{a_1, iu}}}, material_id);
		addFace(FaceIndices{{VertexIndices{b_3, iv}, VertexIndices{a_1, iu}, VertexIndices{b_1, iv}}}, material_id);
	}
	// Close top
	addFace(FaceIndices{{VertexIndices{i, iv}, VertexIndices{2 * i + points_size, iv}, VertexIndices{2 * i + points_size + 1, iv}}}, material_id);
	return ParentClassType_t::calculateObject(material_id);
}



} //namespace yafaray
