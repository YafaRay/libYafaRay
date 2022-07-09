/****************************************************************************
 *      light_point.cc: a simple point light source
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

#include "light/light_point.h"
#include "geometry/surface.h"
#include "sampler/sample.h"
#include "geometry/ray.h"
#include "common/param.h"

namespace yafaray {

PointLight::PointLight(Logger &logger, const Point3f &pos, const Rgb &col, float inte, bool b_light_enabled, bool b_cast_shadows):
		Light(logger, Light::Flags::Singular), position_(pos)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	color_ = col * inte;
	intensity_ = color_.energy();
}

std::tuple<bool, Ray, Rgb> PointLight::illuminate(const Point3f &surface_p, float time) const
{
	if(photonOnly()) return {};
	Vec3f ldir{position_ - surface_p};
	const float dist_sqr = ldir[Axis::X] * ldir[Axis::X] + ldir[Axis::Y] * ldir[Axis::Y] + ldir[Axis::Z] * ldir[Axis::Z];
	const float dist = math::sqrt(dist_sqr);
	if(dist == 0.f) return {};
	const float idist_sqr = 1.f / dist_sqr;
	ldir *= 1.f / dist;
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray), color_ * idist_sqr};
}

std::pair<bool, Ray> PointLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	// bleh...
	Vec3f ldir{position_ - surface_p};
	float dist_sqr = ldir[Axis::X] * ldir[Axis::X] + ldir[Axis::Y] * ldir[Axis::Y] + ldir[Axis::Z] * ldir[Axis::Z];
	float dist = math::sqrt(dist_sqr);
	if(dist == 0.f) return {};
	ldir *= 1.f / dist;
	s.flags_ = flags_;
	s.col_ =  color_;
	s.pdf_ = dist_sqr;
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> PointLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	Vec3f dir{sample::sphere(s_1, s_2)};
	Ray ray{position_, std::move(dir), time};
	return {std::move(ray), 4.f * math::num_pi<>, color_};
}

std::pair<Vec3f, Rgb> PointLight::emitSample(LSample &s, float time) const
{
	s.sp_->p_ = position_;
	Vec3f dir{sample::sphere(s.s_1_, s.s_2_)};
	s.flags_ = flags_;
	s.dir_pdf_ = 0.25f;
	s.area_pdf_ = 1.f;
	return {std::move(dir), color_};
}

std::array<float, 3> PointLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	const float area_pdf = 1.f;
	const float dir_pdf = 0.25f;
	const float cos_wo = 1.f;
	return {area_pdf, dir_pdf, cos_wo};
}

Light * PointLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3f from{{0.f, 0.f, 0.f}};
	Rgb color(1.0);
	float power = 1.0;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;
	bool p_only = false;

	params.getParam("from", from);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);


	auto light = new PointLight(logger, from, color, power, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

} //namespace yafaray
