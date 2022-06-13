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
#include "common/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "common/logger.h"
#include "geometry/ray.h"

BEGIN_YAFARAY

//int hit_t1=0, hit_t2=0;

AreaLight::AreaLight(Logger &logger, const Point3 &c, const Vec3 &v_1, const Vec3 &v_2,
					 const Rgb &col, float inte, int nsam, bool light_enabled, bool cast_shadows):
		Light(logger), corner_(c), to_x_(v_1), to_y_(v_2), samples_(nsam)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;

	fnormal_ = to_y_ ^ to_x_; //f normal is "flipped" normal direction...
	color_ = col * inte * math::num_pi<>;
	area_ = fnormal_.normLen();
	inv_area_ = 1.f / area_;

	normal_ = -fnormal_;
	duv_.u_ = to_x_;
	duv_.u_.normalize();
	duv_.v_ = normal_ ^ duv_.u_;
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

std::pair<bool, Ray> AreaLight::illumSample(const Point3 &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	//get point on area light and vector to surface point:
	const Point3 p{corner_ + s.s_1_ * to_x_ + s.s_2_ * to_y_};
	Vec3 ldir{p - surface_p};
	//normalize vec and compute inverse square distance
	const float dist_sqr = ldir.lengthSqr();
	const float dist = math::sqrt(dist_sqr);
	if(dist <= 0.f) return {};
	ldir *= 1.f / dist;
	const float cos_angle = ldir * fnormal_;
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0.f) return {};
	s.col_ = color_;
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf_ = dist_sqr * math::num_pi<> / (area_ * cos_angle);
	s.flags_ = Light::Flags::None; // no delta functions...
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = normal_;
	}
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> AreaLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	const float ipdf = area_/*  * num_pi */; // really two num_pi?
	Point3 from{corner_ + s_3 * to_x_ + s_4 * to_y_};
	Vec3 dir{sample::cosHemisphere(normal_, duv_, s_1, s_2)};
	Ray ray{std::move(from), std::move(dir), time};
	return {std::move(ray), ipdf, color_};
}

std::pair<Vec3, Rgb> AreaLight::emitSample(LSample &s, float time) const
{
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	s.sp_->p_ = corner_ + s.s_3_ * to_x_ + s.s_4_ * to_y_;
	const Vec3 dir{sample::cosHemisphere(normal_, duv_, s.s_1_, s.s_2_)};
	s.sp_->n_ = s.sp_->ng_ = normal_;
	s.dir_pdf_ = std::abs(normal_ * dir);
	s.flags_ = Light::Flags::None; // no delta functions...
	return {std::move(dir), color_}; // still not 100% sure this is correct without cosine...
}

bool AreaLight::triIntersect(const Point3 &a, const Point3 &b, const Point3 &c, const Ray &ray, float &t)
{
	const Vec3 edge_1{b - a};
	const Vec3 edge_2{c - a};
	const Vec3 pvec{ray.dir_ ^ edge_2};
	const float det = edge_1 * pvec;
	if(det == 0.f) return false;
	const float inv_det = 1.f / det;
	const Vec3 tvec{ray.from_ - a};
	const float u = (tvec * pvec) * inv_det;
	if(u < 0.f || u > 1.f) return false;
	const Vec3 qvec{tvec ^ edge_1};
	const float v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.f) || ((u + v) > 1.f)) return false;
	t = edge_2 * qvec * inv_det;
	return true;
}

std::tuple<bool, float, Rgb> AreaLight::intersect(const Ray &ray, float &t) const
{
	const float cos_angle = ray.dir_ * fnormal_;
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0.f
		|| (!triIntersect(corner_, c_2_, c_3_, ray, t) && !triIntersect(corner_, c_3_, c_4_, ray, t))
		|| t <= 1.0e-10f) return {};
	// pdf = distance^2 / area * cos(norm, ldir); ipdf = 1/pdf;
	const float ipdf = 1.f / (t * t) * area_ * cos_angle * math::div_1_by_pi<>;
	return {true, ipdf, color_};
}

float AreaLight::illumPdf(const Point3 &surface_p, const Point3 &light_p, const Vec3 &) const
{
	Vec3 wi{light_p - surface_p};
	float r_2 = wi.normLenSqr();
	float cos_n = wi * fnormal_;
	return cos_n > 0 ? r_2 * math::num_pi<> / (area_ * cos_n) : 0.f;
}

void AreaLight::emitPdf(const Vec3 &surface_n, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * math::num_pi<>;
	cos_wo = wo * surface_n;
	dir_pdf = cos_wo > 0 ? cos_wo : 0.f;
}

Light * AreaLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3 corner{0.f, 0.f, 0.f};
	Point3 p_1{0.f, 0.f, 0.f};
	Point3 p_2{0.f, 0.f, 0.f};
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

	auto light = new AreaLight(logger, corner, p_1 - corner, p_2 - corner, color, power, samples, light_enabled, cast_shadows);

	light->object_name_ = object_name;
	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

std::tuple<bool, Ray, Rgb> AreaLight::illuminate(const Point3 &surface_p, float time) const
{
	return {};
}

END_YAFARAY
