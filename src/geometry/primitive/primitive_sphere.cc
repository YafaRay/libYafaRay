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

#include <cmath>
#include <memory>
#include "geometry/object/object_basic.h"
#include "common/param.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "scene/scene.h"
#include "common/logger.h"

BEGIN_YAFARAY

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

Bound SpherePrimitive::getBound(const Matrix4 *) const
{
	const Vec3 r{radius_ * 1.0001f};
	return {center_ - r, center_ + r};
}

IntersectData SpherePrimitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	const Vec3 vf{ray.from_ - center_};
	const float ea = ray.dir_ * ray.dir_;
	const float eb = 2.0 * (vf * ray.dir_);
	const float ec = vf * vf - radius_ * radius_;
	float osc = eb * eb - 4.0 * ea * ec;
	if(osc < 0) return {};
	osc = math::sqrt(osc);
	const float sol_1 = (-eb - osc) / (2.0 * ea);
	const float sol_2 = (-eb + osc) / (2.0 * ea);
	float sol = sol_1;
	if(sol < ray.tmin_)
	{
		sol = sol_2;
		if(sol < ray.tmin_) return {};
	}
	//if(sol > ray.tmax) return false; //tmax = -1 is not substituted yet...
	return {sol};
}

std::unique_ptr<const SurfacePoint> SpherePrimitive::getSurface(const RayDifferentials *ray_differentials, const Point3 &hit, const IntersectData &intersect_data, const Matrix4 *obj_to_world, const Camera *camera) const
{
	auto sp = std::make_unique<SurfacePoint>(this);
	sp->intersect_data_ = intersect_data;
	Vec3 normal{hit - center_};
	sp->orco_p_ = static_cast<Point3>(normal);
	normal.normalize();
	sp->n_ = normal;
	sp->ng_ = normal;
	//sp->origin = (void*)this;
	sp->has_orco_ = true;
	sp->p_ = hit;
	std::tie(sp->nu_, sp->nv_) = Vec3::createCoordsSystem(sp->n_);
	sp->u_ = std::atan2(normal.y(), normal.x()) * math::div_1_by_pi<> + 1;
	sp->v_ = 1.f - math::acos(normal.z()) * math::div_1_by_pi<>;
	sp->differentials_ = sp->calcSurfaceDifferentials(ray_differentials);
	sp->mat_data_ = std::unique_ptr<const MaterialData>(sp->getMaterial()->initBsdf(*sp, camera));
	return sp;
}

float SpherePrimitive::surfaceArea(const Matrix4 *obj_to_world, float time) const
{
	return 0; //FIXME
}

Vec3 SpherePrimitive::getGeometricNormal(const Matrix4 *obj_to_world, float u, float v, float time) const
{
	return {}; //FIXME
}

std::pair<Point3, Vec3> SpherePrimitive::sample(float s_1, float s_2, const Matrix4 *obj_to_world, float time) const
{
	return {}; //FIXME
}

END_YAFARAY
