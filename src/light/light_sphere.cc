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

namespace yafaray {

SphereLight::SphereLight(Logger &logger, const Point3f &c, float rad, const Rgb &col, float inte, int nsam, bool b_light_enabled, bool b_cast_shadows):
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

std::pair<bool, Uv<float>> SphereLight::sphereIntersect(const Point3f &from, const Vec3f &dir, const Point3f &c, float r_2)
{
	const Vec3f vf{from - c};
	const float ea = dir * dir;
	const float eb = 2.f * vf * dir;
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
	return {true, std::move(uv)};
}

std::pair<bool, Ray<float>> SphereLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	Vec3f cdir{center_ - surface_p};
	const float dist_sqr = cdir.lengthSquared();
	if(dist_sqr <= square_radius_) return {}; //only emit light on the outside!
	const float dist = math::sqrt(dist_sqr);
	const float idist_sqr = 1.f / (dist_sqr);
	const float cos_alpha = math::sqrt(1.f - square_radius_ * idist_sqr);
	cdir *= 1.f / dist;
	const Uv<Vec3f> duv{Vec3f::createCoordsSystem(cdir)};
	Vec3f dir{sample::cone(cdir, duv, cos_alpha, s.s_1_, s.s_2_)};
	const auto [hit, uv]{sphereIntersect(surface_p, dir, center_, square_radius_epsilon_)};
	if(!hit)
	{
		return {};
	}
	s.pdf_ = 1.f / (2.f * (1.f - cos_alpha));
	s.col_ = color_;
	s.flags_ = flags_;
	if(s.sp_)
	{
		s.sp_->p_ = surface_p + uv.u_ * dir;
		s.sp_->n_ = s.sp_->ng_ = (s.sp_->p_ - center_).normalize();
	}
	Ray<float> ray{surface_p, std::move(dir), time, 0.f, uv.u_};
	return {true, std::move(ray)};
}

std::tuple<bool, float, Rgb> SphereLight::intersect(const Ray<float> &ray, float &) const
{
	if(const auto[hit, uv]{sphereIntersect(ray.from_, ray.dir_, center_, square_radius_)}; hit)
	{
		const Vec3f cdir{center_ - ray.from_};
		const float dist_sqr = cdir.lengthSquared();
		if(dist_sqr <= square_radius_) return {}; //only emit light on the outside!
		const float idist_sqr = 1.f / (dist_sqr);
		const float cos_alpha = math::sqrt(1.f - square_radius_ * idist_sqr);
		const float ipdf = 2.f * (1.f - cos_alpha);
		return {true, ipdf, color_};
	}
	else return {};
}

float SphereLight::illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &) const
{
	Vec3f cdir{center_ - surface_p};
	float dist_sqr = cdir.lengthSquared();
	if(dist_sqr <= square_radius_) return 0.f; //only emit light on the outside!
	float idist_sqr = 1.f / (dist_sqr);
	float cos_alpha = math::sqrt(1.f - square_radius_ * idist_sqr);
	return 1.f / (2.f * (1.f - cos_alpha));
}

std::array<float, 3> SphereLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	const float area_pdf = inv_area_ * math::num_pi<>;
	const float cos_wo = wo * surface_n;
	//! unfinished! use real normal, sp.N might be approximation by mesh...
	const float dir_pdf = cos_wo > 0 ? cos_wo : 0.f;
	return {area_pdf, dir_pdf, cos_wo};
}

std::tuple<Ray<float>, float, Rgb> SphereLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	const Vec3f sdir{sample::sphere(s_3, s_4)};
	Point3f from{center_ + radius_ * sdir};
	const Uv<Vec3f> duv{Vec3f::createCoordsSystem(sdir)};
	Vec3f dir{sample::cosHemisphere(sdir, duv, s_1, s_2)};
	Ray<float> ray{std::move(from), std::move(dir), time};
	return {std::move(ray), area_, color_};
}

std::pair<Vec3f, Rgb> SphereLight::emitSample(LSample &s, float time) const
{
	const Vec3f sdir{sample::sphere(s.s_3_, s.s_4_)};
	s.sp_->p_ = center_ + radius_ * sdir;
	s.sp_->n_ = s.sp_->ng_ = sdir;
	const Uv<Vec3f> duv{Vec3f::createCoordsSystem(sdir)};
	Vec3f dir{sample::cosHemisphere(sdir, duv, s.s_1_, s.s_2_)};
	s.dir_pdf_ = std::abs(sdir * dir);
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	s.flags_ = flags_;
	return {std::move(dir), color_};
}

Light * SphereLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3f from{{0.f, 0.f, 0.f}};
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

std::tuple<bool, Ray<float>, Rgb> SphereLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray
