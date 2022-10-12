/****************************************************************************
 *      std_primitives.cc: standard geometric primitives
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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


#include "geometry/primitive/primitive_sphere.h"
#include "geometry/surface.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

SpherePrimitive::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(center_);
	PARAM_LOAD(radius_);
	PARAM_LOAD(material_name_);
}

ParamMap SpherePrimitive::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(center_);
	PARAM_SAVE(radius_);
	PARAM_SAVE(material_name_);
	PARAM_SAVE_END;
}

ParamMap SpherePrimitive::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	return result;
}

std::pair<std::unique_ptr<Primitive>, ParamError> SpherePrimitive::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const Object &object)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + "::factory 'raw' ParamMap\n" + param_map.logContents());
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	const Params params{param_error, param_map};
	const std::unique_ptr<const Material> *material;
	if(params.material_name_.empty()) return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	material = scene.getMaterial(params.material_name_);
	if(!material) return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	auto primitive {std::make_unique<SpherePrimitive>(logger, param_error, param_map, material, object)};
	if(param_error.notOk()) logger.logWarning(param_error.print<SpherePrimitive>(name, {"type"}));
	return {std::move(primitive), param_error};
}

SpherePrimitive::SpherePrimitive(Logger &logger, ParamError &param_error, const ParamMap &param_map, const std::unique_ptr<const Material> *material, const Object &base_object) : params_{param_error, param_map}, base_object_{base_object}, material_{material}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

Bound<float> SpherePrimitive::getBound() const
{
	const Vec3f r{params_.radius_ * 1.0001f};
	return {params_.center_ - r, params_.center_ + r};
}

Bound<float> SpherePrimitive::getBound(const Matrix4f &obj_to_world) const
{
	const Vec3f r{params_.radius_ * 1.0001f};
	return {params_.center_ - r, params_.center_ + r};
}

std::pair<float, Uv<float>> SpherePrimitive::intersect(const Point3f &from, const Vec3f &dir, float time) const
{
	const Vec3f vf{from - params_.center_};
	const float ea = dir * dir;
	const float eb = 2.f * (vf * dir);
	const float ec = vf * vf - params_.radius_ * params_.radius_;
	float osc = eb * eb - 4.f * ea * ec;
	if(osc < 0) return {};
	osc = math::sqrt(osc);
	const float sol_1 = (-eb - osc) / (2.f * ea);
	const float sol_2 = (-eb + osc) / (2.f * ea);
	float sol = sol_1;
	if(sol < 0.f) //used to be "< ray.tmin_", was that necessary?
	{
		sol = sol_2;
		if(sol < 0.f) return {}; //used to be "< ray.tmin_", was that necessary?
	}
	//if(sol > ray.tmax) return false; //tmax = -1 is not substituted yet...
	return {sol, {}};
}

std::pair<float, Uv<float>> SpherePrimitive::intersect(const Point3f &from, const Vec3f &dir, float time, const Matrix4f &obj_to_world) const
{
	const Vec3f vf{from - params_.center_};
	const float ea = dir * dir;
	const float eb = 2.f * (vf * dir);
	const float ec = vf * vf - params_.radius_ * params_.radius_;
	float osc = eb * eb - 4.f * ea * ec;
	if(osc < 0) return {};
	osc = math::sqrt(osc);
	const float sol_1 = (-eb - osc) / (2.f * ea);
	const float sol_2 = (-eb + osc) / (2.f * ea);
	float sol = sol_1;
	if(sol < 0.f) //used to be "< ray.tmin_", was that necessary?
	{
		sol = sol_2;
		if(sol < 0.f) return {}; //used to be "< ray.tmin_", was that necessary?
	}
	//if(sol > ray.tmax) return false; //tmax = -1 is not substituted yet...
	return {sol, {}};
}

std::unique_ptr<const SurfacePoint> SpherePrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->time_ = time;
	Vec3f normal{hit_point - params_.center_};
	sp->orco_p_ = static_cast<Point3f>(normal);
	normal.normalize();
	sp->n_ = normal;
	sp->ng_ = normal;
	//sp->origin = (void*)this;
	sp->has_orco_ = true;
	sp->p_ = hit_point;
	sp->uvn_ = Vec3f::createCoordsSystem(sp->n_);
	sp->uv_.u_ = std::atan2(normal[Axis::Y], normal[Axis::X]) * math::div_1_by_pi<> + 1;
	sp->uv_.v_ = 1.f - math::acos(normal[Axis::Z]) * math::div_1_by_pi<>;
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

std::unique_ptr<const SurfacePoint> SpherePrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3f &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4f &obj_to_world) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->time_ = time;
	Vec3f normal{hit_point - params_.center_};
	sp->orco_p_ = static_cast<Point3f>(normal);
	normal.normalize();
	sp->n_ = normal;
	sp->ng_ = normal;
	//sp->origin = (void*)this;
	sp->has_orco_ = true;
	sp->p_ = hit_point;
	sp->uvn_ = Vec3f::createCoordsSystem(sp->n_);
	sp->uv_.u_ = std::atan2(normal[Axis::Y], normal[Axis::X]) * math::div_1_by_pi<> + 1;
	sp->uv_.v_ = 1.f - math::acos(normal[Axis::Z]) * math::div_1_by_pi<>;
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

float SpherePrimitive::surfaceArea(float time) const
{
	return 0; //FIXME
}

float SpherePrimitive::surfaceArea(float time, const Matrix4f &obj_to_world) const
{
	return 0; //FIXME
}

Vec3f SpherePrimitive::getGeometricNormal(const Uv<float> &uv, float time, bool) const
{
	return {}; //FIXME
}

Vec3f SpherePrimitive::getGeometricNormal(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const
{
	return {}; //FIXME
}

std::pair<Point3f, Vec3f> SpherePrimitive::sample(const Uv<float> &uv, float time) const
{
	return {}; //FIXME
}

std::pair<Point3f, Vec3f> SpherePrimitive::sample(const Uv<float> &uv, float time, const Matrix4f &obj_to_world) const
{
	return {}; //FIXME
}

} //namespace yafaray
