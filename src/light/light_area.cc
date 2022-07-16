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

namespace yafaray {

//int hit_t1=0, hit_t2=0;

AreaLight::AreaLight(Logger &logger, const Point3f &c, const Vec3f &v_1, const Vec3f &v_2,
					 const Rgb &col, float inte, int nsam, bool light_enabled, bool cast_shadows):
		Light(logger), area_quad_{{c, c + v_1, c + v_1 + v_2, c + v_2}}, to_x_(v_1), to_y_(v_2),
		fnormal_{v_2 ^ v_1}, //f normal is "flipped" normal direction...
		normal_{-fnormal_},
		duv_{v_1.normalized(), normal_ ^ v_1.normalized()},
		color_{col * inte * math::num_pi<>},
		area_{fnormal_.normalized().length()},
		inv_area_{1.f / area_},
		samples_(nsam)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;
}

void AreaLight::init(const Scene &scene)
{
	if(!object_name_.empty())
	{
		Object *obj = scene.getObject(object_name_);
		if(obj) obj->setLight(this);
		else logger_.logError("AreaLight: '" + name_ + "': associated object '" + object_name_ + "' could not be found!");
	}
}

Rgb AreaLight::totalEnergy() const { return color_ * area_; }

std::pair<bool, Ray> AreaLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	//get point on area light and vector to surface point:
	const Point3f p{area_quad_[0] + s.s_1_ * to_x_ + s.s_2_ * to_y_};
	Vec3f ldir{p - surface_p};
	//normalize vec and compute inverse square distance
	const float dist_sqr = ldir.lengthSquared();
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
	Point3f from{area_quad_[0] + s_3 * to_x_ + s_4 * to_y_};
	Vec3f dir{sample::cosHemisphere(normal_, duv_, s_1, s_2)};
	Ray ray{std::move(from), std::move(dir), time};
	return {std::move(ray), ipdf, color_};
}

std::pair<Vec3f, Rgb> AreaLight::emitSample(LSample &s, float time) const
{
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	s.sp_->p_ = area_quad_[0] + s.s_3_ * to_x_ + s.s_4_ * to_y_;
	const Vec3f dir{sample::cosHemisphere(normal_, duv_, s.s_1_, s.s_2_)};
	s.sp_->n_ = s.sp_->ng_ = normal_;
	s.dir_pdf_ = std::abs(normal_ * dir);
	s.flags_ = Light::Flags::None; // no delta functions...
	return {std::move(dir), color_}; // still not 100% sure this is correct without cosine...
}

std::tuple<bool, float, Rgb> AreaLight::intersect(const Ray &ray, float &t) const
{
	const float cos_angle = ray.dir_ * fnormal_;
	if(cos_angle <= 0.f) return {}; //no light if point is behind area light (single sided!)
	const auto intersect_data{area_quad_.intersect(ray.from_, ray.dir_)};
	if(intersect_data.first <= 0.f) return {};
	else t = intersect_data.first;
	const float ipdf = 1.f / (t * t) * area_ * cos_angle * math::div_1_by_pi<>; // pdf = distance^2 / area * cos(norm, ldir); ipdf = 1/pdf;
	return {true, ipdf, color_};
}

float AreaLight::illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &) const
{
	Vec3f wi{light_p - surface_p};
	float r_2 = wi.normalizeAndReturnLengthSquared();
	float cos_n = wi * fnormal_;
	return cos_n > 0 ? r_2 * math::num_pi<> / (area_ * cos_n) : 0.f;
}

std::array<float, 3> AreaLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	const float area_pdf = inv_area_ * math::num_pi<>;
	const float cos_wo = wo * surface_n;
	const float dir_pdf = cos_wo > 0 ? cos_wo : 0.f;
	return {area_pdf, dir_pdf, cos_wo};
}

Light * AreaLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3f corner{{0.f, 0.f, 0.f}};
	Point3f p_1{{0.f, 0.f, 0.f}};
	Point3f p_2{{0.f, 0.f, 0.f}};
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

std::tuple<bool, Ray, Rgb> AreaLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray
