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

BEGIN_YAFARAY

SunLight::SunLight(Logger &logger, Vec3 dir, const Rgb &col, float inte, float angle, int n_samples, bool b_light_enabled, bool b_cast_shadows):
		Light(logger), direction_(dir), samples_(n_samples)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	color_ = col * inte;
	direction_.normalize();
	Vec3::createCs(dir, du_, dv_);
	if(angle > 80.f) angle = 80.f;
	cos_angle_ = math::cos(math::degToRad(angle));
	invpdf_ = (math::mult_pi_by_2 * (1.f - cos_angle_));
	pdf_ = 1.0 / invpdf_;
	pdf_ = std::min(pdf_, math::sqrt(std::numeric_limits<float>::max())); //Using the square root of the maximum possible float value when the invpdf_ is zero, because there are calculations in the integrators squaring the pdf value and it could be overflowing (NaN) in that case.
	col_pdf_ = color_ * pdf_;
}

void SunLight::init(Scene &scene)
{
	// calculate necessary parameters for photon mapping
	Bound w = scene.getSceneBound();
	world_radius_ = 0.5 * (w.g_ - w.a_).length();
	world_center_ = 0.5 * (w.a_ + w.g_);
	e_pdf_ = (math::num_pi * world_radius_ * world_radius_);
}

bool SunLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	//sample direction uniformly inside cone:
	wi.dir_ = sample::cone(direction_, du_, dv_, cos_angle_, s.s_1_, s.s_2_);
	wi.tmax_ = -1.f;

	s.col_ = col_pdf_;
	// ipdf: inverse of uniform cone pdf; calculated in constructor.
	s.pdf_ = pdf_;

	return true;
}

bool SunLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	float cosine = ray.dir_ * direction_;
	if(cosine < cos_angle_) return false;
	col = col_pdf_;
	t = -1;
	ipdf = invpdf_;
	return true;
}

Rgb SunLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	float u, v;
	Vec3::shirleyDisk(s_3, s_4, u, v);

	Vec3 ldir = sample::cone(direction_, du_, dv_, cos_angle_, s_3, s_4);
	Vec3 du_2, dv_2;

	sample::minRot(direction_, du_, ldir, du_2, dv_2);

	ipdf = invpdf_;
	ray.from_ = world_center_ + world_radius_ * (u * du_2 + v * dv_2 + ldir);
	ray.tmax_ = -1;
	ray.dir_ = -ldir;
	return col_pdf_ * e_pdf_;
}


std::unique_ptr<Light> SunLight::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	Point3 dir(0.0, 0.0, 1.0);
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

	auto light = std::unique_ptr<SunLight>(new SunLight(logger, Vec3(dir.x_, dir.y_, dir.z_), color, power, angle, samples, light_enabled, cast_shadows));

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
