/****************************************************************************
 *      light_sun.cc: a directional light with soft shadows
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

#include "light/light_sun.h"
#include "sampler/sample.h"
#include "common/param.h"
#include "scene/scene.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include <limits>

namespace yafaray {

SunLight::SunLight(Logger &logger, Vec3f dir, const Rgb &col, float inte, float angle, int n_samples, bool b_light_enabled, bool b_cast_shadows):
		Light(logger), direction_(dir), samples_(n_samples)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	color_ = col * inte;
	direction_.normalize();
	duv_ = Vec3f::createCoordsSystem(dir);
	if(angle > 80.f) angle = 80.f;
	cos_angle_ = math::cos(math::degToRad(angle));
	invpdf_ = math::mult_pi_by_2<> * (1.f - cos_angle_);
	pdf_ = 1.0 / invpdf_;
	pdf_ = std::min(pdf_, math::sqrt(std::numeric_limits<float>::max())); //Using the square root of the maximum possible float value when the invpdf_ is zero, because there are calculations in the integrators squaring the pdf value and it could be overflowing (NaN) in that case.
	col_pdf_ = color_ * pdf_;
}

void SunLight::init(Scene &scene)
{
	// calculate necessary parameters for photon mapping
	const Bound w = scene.getSceneBound();
	world_radius_ = 0.5f * (w.g_ - w.a_).length();
	world_center_ = 0.5f * (w.a_ + w.g_);
	e_pdf_ = math::num_pi<> * world_radius_ * world_radius_;
}

std::pair<bool, Ray> SunLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	//sample direction uniformly inside cone:
	Vec3f dir{sample::cone(direction_, duv_, cos_angle_, s.s_1_, s.s_2_)};
	s.col_ = col_pdf_;
	// ipdf: inverse of uniform cone pdf; calculated in constructor.
	s.pdf_ = pdf_;
	Ray ray{surface_p, std::move(dir), time};
	return {true, std::move(ray)};
}

std::tuple<bool, float, Rgb> SunLight::intersect(const Ray &ray, float &t) const
{
	const float cosine = ray.dir_ * direction_;
	if(cosine < cos_angle_) return {};
	t = -1.f;
	return {true, invpdf_, col_pdf_};
}

std::tuple<Ray, float, Rgb> SunLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	const Vec3f ldir{sample::cone(direction_, duv_, cos_angle_, s_3, s_4)};
	const Uv<Vec3f> duv_2{sample::minRot(direction_, duv_.u_, ldir)};
	const Uv<float> uv{Vec3f::shirleyDisk(s_3, s_4)};
	Point3f from{world_center_ + world_radius_ * (uv.u_ * duv_2.u_ + uv.v_ * duv_2.v_ + ldir)};
	Ray ray{std::move(from), -ldir, time};
	return {std::move(ray), invpdf_, col_pdf_ * e_pdf_};
}

Light * SunLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3f dir{{0.f, 0.f, 1.f}};
	Rgb color(1.0);
	float power = 1.0;
	float angle = 0.27; //angular (half-)size of the real sun;
	int samples = 4;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;
	bool p_only = false;

	params.getParam("direction", dir);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("angle", angle);
	params.getParam("samples", samples);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);

	auto light = new SunLight(logger, dir, color, power, angle, samples, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

std::tuple<bool, Ray, Rgb> SunLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray
