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

#include "geometry/primitive_basic.h"
#include "geometry/object_primitive.h"
#include "geometry/object_geom.h"
#include "common/param.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "scene/scene.h"

BEGIN_YAFARAY

Bound Sphere::getBound() const
{
	Vec3 r(radius_ * 1.0001);
	return Bound(center_ - r, center_ + r);
}

bool Sphere::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	Vec3 vf = ray.from_ - center_;
	float ea = ray.dir_ * ray.dir_;
	float eb = 2.0 * (vf * ray.dir_);
	float ec = vf * vf - radius_ * radius_;
	float osc = eb * eb - 4.0 * ea * ec;
	if(osc < 0) return false;
	osc = math::sqrt(osc);
	float sol_1 = (-eb - osc) / (2.0 * ea);
	float sol_2 = (-eb + osc) / (2.0 * ea);
	float sol = sol_1;
	if(sol < ray.tmin_)
	{
		sol = sol_2;
		if(sol < ray.tmin_) return false;
	}
	//if(sol > ray.tmax) return false; //tmax = -1 is not substituted yet...
	*t = sol;
	return true;
}

void Sphere::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	Vec3 normal = hit - center_;
	sp.orco_p_ = normal;
	normal.normalize();
	sp.material_ = material_;
	sp.n_ = normal;
	sp.ng_ = normal;
	//sp.origin = (void*)this;
	sp.has_orco_ = true;
	sp.p_ = hit;
	Vec3::createCs(sp.n_, sp.nu_, sp.nv_);
	sp.u_ = atan2(normal.y_, normal.x_) * M_1_PI + 1;
	sp.v_ = 1.f - math::acos(normal.z_) * M_1_PI;
	sp.light_ = nullptr;
}

ObjectGeometric *sphereFactory__(ParamMap &params, const Scene &scene)
{
	Point3 center(0.f, 0.f, 0.f);
	double radius(1.f);
	const Material *mat;
	std::string matname;
	params.getParam("center", center);
	params.getParam("radius", radius);
	params.getParam("material", matname);
	if(matname.empty()) return nullptr;
	mat = scene.getMaterial(matname);
	if(!mat) return nullptr;
	Sphere *sphere = new Sphere(center, radius, mat);
	return new PrimitiveObject(sphere);
}

END_YAFARAY
