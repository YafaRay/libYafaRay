/****************************************************************************
 *      arealight.cc: a rectangular area light source
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

#include "light/light_area.h"
#include "geometry/surface.h"
#include "geometry/object/object_basic.h"
#include "common/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "common/logger.h"

BEGIN_YAFARAY

//int hit_t1=0, hit_t2=0;

AreaLight::AreaLight(Logger &logger, const Point3 &c, const Vec3 &v_1, const Vec3 &v_2,
					 const Rgb &col, float inte, int nsam, bool light_enabled, bool cast_shadows):
		Light(logger), corner_(c), to_x_(v_1), to_y_(v_2), samples_(nsam)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;

	fnormal_ = to_y_ ^ to_x_; //f normal is "flipped" normal direction...
	color_ = col * inte * math::num_pi;
	area_ = fnormal_.normLen();
	inv_area_ = 1.0 / area_;

	normal_ = -fnormal_;
	du_ = to_x_;
	du_.normalize();
	dv_ = normal_ ^ du_;
	c_2_ = corner_ + to_x_;
	c_3_ = corner_ + (to_x_ + to_y_);
	c_4_ = corner_ + to_y_;
}

void AreaLight::init(Scene &scene)
{
	if(!object_name_.empty())
	{
		Object *obj = scene.getObject(object_name_);
		if(obj) obj->setLight(this);
		else logger_.logError("AreaLight: '" + name_ + "': associated object '" + object_name_ + "' could not be found!");
	}
}

Rgb AreaLight::totalEnergy() const { return color_ * area_; }

bool AreaLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	//get point on area light and vector to surface point:
	Point3 p = corner_ + s.s_1_ * to_x_ + s.s_2_ * to_y_;
	Vec3 ldir = p - sp.p_;
	//normalize vec and compute inverse square distance
	float dist_sqr = ldir.lengthSqr();
	float dist = math::sqrt(dist_sqr);
	if(dist <= 0.0) return false;
	ldir *= 1.f / dist;
	float cos_angle = ldir * fnormal_;
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0) return false;

	// fill direction
	wi.tmax_ = dist;
	wi.dir_ = ldir;

	s.col_ = color_;
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf_ = dist_sqr * math::num_pi / (area_ * cos_angle);
	s.flags_ = Light::Flags::None; // no delta functions...
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = normal_;
	}
	return true;
}

Rgb AreaLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	ipdf = area_/*  * num_pi */; // really two num_pi?
	ray.from_ = corner_ + s_3 * to_x_ + s_4 * to_y_;
	ray.dir_ = sample::cosHemisphere(normal_, du_, dv_, s_1, s_2);
	return color_;
}

Rgb AreaLight::emitSample(Vec3 &wo, LSample &s) const
{
	s.area_pdf_ = inv_area_ * math::num_pi;
	s.sp_->p_ = corner_ + s.s_3_ * to_x_ + s.s_4_ * to_y_;
	wo = sample::cosHemisphere(normal_, du_, dv_, s.s_1_, s.s_2_);
	s.sp_->n_ = s.sp_->ng_ = normal_;
	s.dir_pdf_ = std::abs(normal_ * wo);
	s.flags_ = Light::Flags::None; // no delta functions...
	return color_; // still not 100% sure this is correct without cosine...
}

bool AreaLight::triIntersect(const Point3 &a, const Point3 &b, const Point3 &c, const Ray &ray, float &t)
{
	Vec3 edge_1, edge_2, tvec, pvec, qvec;
	float det, inv_det, u, v;
	edge_1 = b - a;
	edge_2 = c - a;
	pvec = ray.dir_ ^ edge_2;
	det = edge_1 * pvec;
	if(det == 0.0) return false;
	inv_det = 1.0 / det;
	tvec = ray.from_ - a;
	u = (tvec * pvec) * inv_det;
	if(u < 0.0 || u > 1.0) return false;
	qvec = tvec ^ edge_1;
	v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.0) || ((u + v) > 1.0)) return false;
	t = edge_2 * qvec * inv_det;

	return true;
}

bool AreaLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	float cos_angle = ray.dir_ * fnormal_;
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0) return false;

	if(!triIntersect(corner_, c_2_, c_3_, ray, t))
	{
		if(!triIntersect(corner_, c_3_, c_4_, ray, t)) return false;
	}
	if(!(t > 1.0e-10f)) return false;

	col = color_;
	// pdf = distance^2 / area * cos(norm, ldir); ipdf = 1/pdf;
	ipdf = 1.f / (t * t) * area_ * cos_angle * math::div_1_by_pi;
	return true;
}

float AreaLight::illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const
{
	Vec3 wi = sp_light.p_ - sp.p_;
	float r_2 = wi.normLenSqr();
	float cos_n = wi * fnormal_;
	return cos_n > 0 ? r_2 * math::num_pi / (area_ * cos_n) : 0.f;
}

void AreaLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * math::num_pi;
	cos_wo = wo * sp.n_;
	dir_pdf = cos_wo > 0 ? cos_wo : 0.f;
}

std::unique_ptr<Light> AreaLight::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	Point3 corner(0.0);
	Point3 p_1(0.0);
	Point3 p_2(0.0);
	Rgb color(1.0);
	float power = 1.0;
	int samples = 4;
	std::string object_name;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;
	bool p_only = false;

	params.getParam("corner", corner);
	params.getParam("point1", p_1);
	params.getParam("point2", p_2);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("samples", samples);
	params.getParam("object_name", object_name);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);

	auto light = std::unique_ptr<AreaLight>(new AreaLight(logger, corner, p_1 - corner, p_2 - corner, color, power, samples, light_enabled, cast_shadows));

	light->object_name_ = object_name;
	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
