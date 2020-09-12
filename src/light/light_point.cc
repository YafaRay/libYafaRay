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
#include "common/surface.h"
#include "utility/util_sample.h"
#include "common/param.h"

BEGIN_YAFARAY

PointLight::PointLight(const Point3 &pos, const Rgb &col, float inte, bool b_light_enabled, bool b_cast_shadows):
		Light(Light::Flags::Singular), position_(pos)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	color_ = col * inte;
	intensity_ = color_.energy();
}

bool PointLight::illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const
{
	if(photonOnly()) return false;

	Vec3 ldir(position_ - sp.p_);
	float dist_sqr = ldir.x_ * ldir.x_ + ldir.y_ * ldir.y_ + ldir.z_ * ldir.z_;
	float dist = fSqrt__(dist_sqr);
	float idist_sqr = 0.0;
	if(dist == 0.0) return false;

	idist_sqr = 1.f / (dist_sqr);
	ldir *= 1.f / dist;

	wi.tmax_ = dist;
	wi.dir_ = ldir;

	col = color_ * (float)idist_sqr;
	return true;
}

bool PointLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	// bleh...
	Vec3 ldir(position_ - sp.p_);
	float dist_sqr = ldir.x_ * ldir.x_ + ldir.y_ * ldir.y_ + ldir.z_ * ldir.z_;
	float dist = fSqrt__(dist_sqr);
	if(dist == 0.0) return false;

	ldir *= 1.f / dist;

	wi.tmax_ = dist;
	wi.dir_ = ldir;

	s.flags_ = flags_;
	s.col_ =  color_;
	s.pdf_ = dist_sqr;
	return true;
}

Rgb PointLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	ray.from_ = position_;
	ray.dir_ = sampleSphere__(s_1, s_2);
	ipdf = 4.0f * M_PI;
	return color_;
}

Rgb PointLight::emitSample(Vec3 &wo, LSample &s) const
{
	s.sp_->p_ = position_;
	wo = sampleSphere__(s.s_1_, s.s_2_);
	s.flags_ = flags_;
	s.dir_pdf_ = 0.25f;
	s.area_pdf_ = 1.f;
	return color_;
}

void PointLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = 1.f;
	dir_pdf = 0.25f;
	cos_wo = 1.f;
}

Light *PointLight::factory(ParamMap &params, Scene &scene)
{
	Point3 from(0.0);
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


	PointLight *light = new PointLight(from, color, power, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY