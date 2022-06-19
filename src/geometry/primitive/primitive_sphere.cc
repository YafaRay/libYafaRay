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
#include "common/param.h"

namespace yafaray {

Primitive *SpherePrimitive::factory(const ParamMap &params, const Scene &scene, const Object &object)
{
/*	if(logger_.isDebug())
	{
		logger.logDebug("**SpherePrimitive");
		params.logContents();
	}*/
	Point3 center(0.f, 0.f, 0.f);
	double radius(1.f);
	const std::unique_ptr<const Material> *material;
	std::string matname;
	params.getParam("center", center);
	params.getParam("radius", radius);
	params.getParam("material", matname);
	if(matname.empty()) return nullptr;
	material = scene.getMaterial(matname);
	if(!material) return nullptr;
	return new SpherePrimitive(center, radius, material, object);
}

Bound SpherePrimitive::getBound() const
{
	const Vec3 r{radius_ * 1.0001f};
	return {center_ - r, center_ + r};
}

Bound SpherePrimitive::getBound(const Matrix4 &obj_to_world) const
{
	const Vec3 r{radius_ * 1.0001f};
	return {center_ - r, center_ + r};
}

std::pair<float, Uv<float>> SpherePrimitive::intersect(const Point3 &from, const Vec3 &dir, float time) const
{
	const Vec3 vf{from - center_};
	const float ea = dir * dir;
	const float eb = 2.0 * (vf * dir);
	const float ec = vf * vf - radius_ * radius_;
	float osc = eb * eb - 4.0 * ea * ec;
	if(osc < 0) return {};
	osc = math::sqrt(osc);
	const float sol_1 = (-eb - osc) / (2.0 * ea);
	const float sol_2 = (-eb + osc) / (2.0 * ea);
	float sol = sol_1;
	if(sol < 0.f) //used to be "< ray.tmin_", was that necessary?
	{
		sol = sol_2;
		if(sol < 0.f) return {}; //used to be "< ray.tmin_", was that necessary?
	}
	//if(sol > ray.tmax) return false; //tmax = -1 is not substituted yet...
	return {sol, {}};
}

std::pair<float, Uv<float>> SpherePrimitive::intersect(const Point3 &from, const Vec3 &dir, float time, const Matrix4 &obj_to_world) const
{
	const Vec3 vf{from - center_};
	const float ea = dir * dir;
	const float eb = 2.0 * (vf * dir);
	const float ec = vf * vf - radius_ * radius_;
	float osc = eb * eb - 4.0 * ea * ec;
	if(osc < 0) return {};
	osc = math::sqrt(osc);
	const float sol_1 = (-eb - osc) / (2.0 * ea);
	const float sol_2 = (-eb + osc) / (2.0 * ea);
	float sol = sol_1;
	if(sol < 0.f) //used to be "< ray.tmin_", was that necessary?
	{
		sol = sol_2;
		if(sol < 0.f) return {}; //used to be "< ray.tmin_", was that necessary?
	}
	//if(sol > ray.tmax) return false; //tmax = -1 is not substituted yet...
	return {sol, {}};
}

std::unique_ptr<const SurfacePoint> SpherePrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->time_ = time;
	Vec3 normal{hit_point - center_};
	sp->orco_p_ = static_cast<Point3>(normal);
	normal.normalize();
	sp->n_ = normal;
	sp->ng_ = normal;
	//sp->origin = (void*)this;
	sp->has_orco_ = true;
	sp->p_ = hit_point;
	sp->uvn_ = Vec3::createCoordsSystem(sp->n_);
	sp->uv_.u_ = std::atan2(normal.y(), normal.x()) * math::div_1_by_pi<> + 1;
	sp->uv_.v_ = 1.f - math::acos(normal.z()) * math::div_1_by_pi<>;
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

std::unique_ptr<const SurfacePoint> SpherePrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit_point, float time, const Uv<float> &intersect_uv, const Camera *camera, const Matrix4 &obj_to_world) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->time_ = time;
	Vec3 normal{hit_point - center_};
	sp->orco_p_ = static_cast<Point3>(normal);
	normal.normalize();
	sp->n_ = normal;
	sp->ng_ = normal;
	//sp->origin = (void*)this;
	sp->has_orco_ = true;
	sp->p_ = hit_point;
	sp->uvn_ = Vec3::createCoordsSystem(sp->n_);
	sp->uv_.u_ = std::atan2(normal.y(), normal.x()) * math::div_1_by_pi<> + 1;
	sp->uv_.v_ = 1.f - math::acos(normal.z()) * math::div_1_by_pi<>;
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

float SpherePrimitive::surfaceArea(float time) const
{
	return 0; //FIXME
}

float SpherePrimitive::surfaceArea(float time, const Matrix4 &obj_to_world) const
{
	return 0; //FIXME
}

Vec3 SpherePrimitive::getGeometricNormal(const Uv<float> &uv, float time, bool) const
{
	return {}; //FIXME
}

Vec3 SpherePrimitive::getGeometricNormal(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const
{
	return {}; //FIXME
}

std::pair<Point3, Vec3> SpherePrimitive::sample(const Uv<float> &uv, float time) const
{
	return {}; //FIXME
}

std::pair<Point3, Vec3> SpherePrimitive::sample(const Uv<float> &uv, float time, const Matrix4 &obj_to_world) const
{
	return {}; //FIXME
}

} //namespace yafaray
