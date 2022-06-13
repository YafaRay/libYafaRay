/****************************************************************************
 *      light_sphere.cc: a spherical area light source
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

#include "light/light_sphere.h"
#include "geometry/surface.h"
#include "common/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "common/logger.h"
#include "geometry/ray.h"
#include "geometry/bound.h"

BEGIN_YAFARAY

SphereLight::SphereLight(Logger &logger, const Point3 &c, float rad, const Rgb &col, float inte, int nsam, bool b_light_enabled, bool b_cast_shadows):
		Light(logger), center_(c), radius_(rad), samples_(nsam)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	color_ = col * inte;
	square_radius_ = radius_ * radius_;
	square_radius_epsilon_ = square_radius_ * 1.000003815; // ~0.2% larger radius squared
	area_ = square_radius_ * 4.0 * math::num_pi<>;
	inv_area_ = 1.f / area_;
}

void SphereLight::init(Scene &scene)
{
	if(!object_name_.empty())
	{
		Object *obj = scene.getObject(object_name_);
		if(obj) obj->setLight(this);
		else logger_.logError("SphereLight: '" + name_ + "': associated object '" + object_name_ + "' could not be found!");
	}
}

Rgb SphereLight::totalEnergy() const { return color_ * area_ /* * num_pi */; }

std::pair<bool, Uv<float>> SphereLight::sphereIntersect(const Ray &ray, const Point3 &c, float r_2)
{
	const Vec3 vf{ray.from_ - c};
	const float ea = ray.dir_ * ray.dir_;
	const float eb = 2.f * vf * ray.dir_;
	const float ec = vf * vf - r_2;
	const float osc = eb * eb - 4.f * ea * ec;
	if(osc < 0.f)
	{
		// assume tangential hit/miss condition => Pythagoras
		return {false, {math::sqrt(ec / ea), 0.f}};
	}
	const float osc_sqrt = math::sqrt(osc);
	const Uv<float> uv{
			(-eb - osc_sqrt) / (2.f * ea),
			(-eb + osc_sqrt) / (2.f * ea)
	};
	return {true, uv};
}

bool SphereLight::illumSample(const Point3 &surface_p, LSample &s, Ray &wi, float time) const
{
	if(photonOnly()) return false;

	Vec3 cdir{center_ - surface_p};
	float dist_sqr = cdir.lengthSqr();
	if(dist_sqr <= square_radius_) return false; //only emit light on the outside!

	float dist = math::sqrt(dist_sqr);
	float idist_sqr = 1.f / (dist_sqr);
	float cos_alpha = math::sqrt(1.f - square_radius_ * idist_sqr);
	cdir *= 1.f / dist;
	const Uv<Vec3> duv{Vec3::createCoordsSystem(cdir)};
	wi.dir_ = sample::cone(cdir, duv, cos_alpha, s.s_1_, s.s_2_);
	float d_1, d_2;
	if(const auto [hit, uv]{sphereIntersect(wi, center_, square_radius_epsilon_)}; !hit)
	{
		return false;
	}
	wi.tmax_ = d_1;

	s.pdf_ = 1.f / (2.f * (1.f - cos_alpha));
	s.col_ = color_;
	s.flags_ = flags_;
	if(s.sp_)
	{
		s.sp_->p_ = wi.from_ + d_1 * wi.dir_;
		s.sp_->n_ = s.sp_->ng_ = (s.sp_->p_ - center_).normalize();
	}
	return true;
}

std::tuple<bool, float, Rgb> SphereLight::intersect(const Ray &ray, float &) const
{
	if(const auto[hit, uv]{sphereIntersect(ray, center_, square_radius_)}; hit)
	{
		const Vec3 cdir{center_ - ray.from_};
		const float dist_sqr = cdir.lengthSqr();
		if(dist_sqr <= square_radius_) return {}; //only emit light on the outside!
		const float idist_sqr = 1.f / (dist_sqr);
		const float cos_alpha = math::sqrt(1.f - square_radius_ * idist_sqr);
		const float ipdf = 2.f * (1.f - cos_alpha);
		return {true, ipdf, color_};
	}
	else return {};
}

float SphereLight::illumPdf(const Point3 &surface_p, const Point3 &light_p, const Vec3 &) const
{
	Vec3 cdir{center_ - surface_p};
	float dist_sqr = cdir.lengthSqr();
	if(dist_sqr <= square_radius_) return 0.f; //only emit light on the outside!
	float idist_sqr = 1.f / (dist_sqr);
	float cos_alpha = math::sqrt(1.f - square_radius_ * idist_sqr);
	return 1.f / (2.f * (1.f - cos_alpha));
}

void SphereLight::emitPdf(const Vec3 &surface_n, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * math::num_pi<>;
	cos_wo = wo * surface_n;
	//! unfinished! use real normal, sp.N might be approximation by mesh...
	dir_pdf = cos_wo > 0 ? cos_wo : 0.f;
}

std::tuple<Ray, float, Rgb> SphereLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	const Vec3 sdir{sample::sphere(s_3, s_4)};
	Point3 from{center_ + radius_ * sdir};
	const Uv<Vec3> duv{Vec3::createCoordsSystem(sdir)};
	Vec3 dir{sample::cosHemisphere(sdir, duv, s_1, s_2)};
	Ray ray{std::move(from), std::move(dir), time};
	return {std::move(ray), area_, color_};
}

std::pair<Vec3, Rgb> SphereLight::emitSample(LSample &s, float time) const
{
	const Vec3 sdir{sample::sphere(s.s_3_, s.s_4_)};
	s.sp_->p_ = center_ + radius_ * sdir;
	s.sp_->n_ = s.sp_->ng_ = sdir;
	const Uv<Vec3> duv{Vec3::createCoordsSystem(sdir)};
	Vec3 dir{sample::cosHemisphere(sdir, duv, s.s_1_, s.s_2_)};
	s.dir_pdf_ = std::abs(sdir * dir);
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	s.flags_ = flags_;
	return {std::move(dir), color_};
}

Light * SphereLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3 from{0.f, 0.f, 0.f};
	Rgb color(1.0);
	float power = 1.0;
	float radius = 1.f;
	int samples = 4;
	std::string object_name;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;
	bool p_only = false;

	params.getParam("from", from);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("radius", radius);
	params.getParam("samples", samples);
	params.getParam("object_name", object_name);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);

	auto light = new SphereLight(logger, from, radius, color, power, samples, light_enabled, cast_shadows);

	light->object_name_ = object_name;
	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
