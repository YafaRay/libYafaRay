/****************************************************************************
 *      directional.cc: a directional light, with optinal limited radius
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

#include "light/light_directional.h"
#include "geometry/surface.h"
#include "common/param.h"
#include "scene/scene.h"
#include "common/logger.h"

BEGIN_YAFARAY

DirectionalLight::DirectionalLight(Logger &logger, const Point3 &pos, Vec3 dir, const Rgb &col, float inte, bool inf, float rad, bool b_light_enabled, bool b_cast_shadows):
		Light(logger, Light::Flags::DiracDir), position_(pos), direction_(dir), radius_(rad), infinite_(inf)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	color_ = col * inte;
	intensity_ = color_.energy();
	direction_.normalize();
	Vec3::createCs(direction_, du_, dv_);
	Vec3 &d = direction_;
	major_axis_ = (d.x_ > d.y_) ? ((d.x_ > d.z_) ? 0 : 2) : ((d.y_ > d.z_) ? 1 : 2);
}

void DirectionalLight::init(Scene &scene)
{
	// calculate necessary parameters for photon mapping if the light
	//  is set to illuminate the whole scene:
	Bound w = scene.getSceneBound();
	world_radius_ = 0.5 * (w.g_ - w.a_).length();
	if(infinite_)
	{
		position_ = 0.5 * (w.a_ + w.g_);
		radius_ = world_radius_;
	}
	area_pdf_ = 1.f / (radius_ * radius_); // Pi cancels out with our weird conventions :p
	if(logger_.isVerbose()) logger_.logVerbose("DirectionalLight: pos ", position_, " world radius: ", world_radius_);
}


bool DirectionalLight::illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const
{
	if(photonOnly()) return false;

	// check if the point is outside of the illuminated cylinder (non-infinite lights)
	if(!infinite_)
	{
		Vec3 vec = position_ - sp.p_;
		float dist = (direction_ ^ vec).length();
		if(dist > radius_) return false;
		wi.tmax_ = (vec * direction_);
		if(wi.tmax_ <= 0.0) return false;
	}
	else
	{
		wi.tmax_ = -1.0;
	}
	wi.dir_ = direction_;

	col = color_;
	return true;
}

bool DirectionalLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	s.pdf_ = 1.0;
	return illuminate(sp, s.col_, wi);
}

Rgb DirectionalLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	//todo
	ray.dir_ = -direction_;
	float u, v;
	Vec3::shirleyDisk(s_1, s_2, u, v);
	ray.from_ = position_ + radius_ * (u * du_ + v * dv_);
	if(infinite_) ray.from_ += direction_ * world_radius_;
	ipdf = math::num_pi * radius_ * radius_; //4.0f * num_pi;
	return color_;
}

Rgb DirectionalLight::emitSample(Vec3 &wo, LSample &s) const
{
	//todo
	wo = -direction_;
	s.sp_->n_ = wo;
	s.flags_ = flags_;
	float u, v;
	Vec3::shirleyDisk(s.s_1_, s.s_2_, u, v);
	s.sp_->p_ = position_ + radius_ * (u * du_ + v * dv_);
	if(infinite_) s.sp_->p_ += direction_ * world_radius_;
	s.area_pdf_ = area_pdf_;
	s.dir_pdf_ = 1.f;
	return color_;
}

std::unique_ptr<Light> DirectionalLight::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	Point3 from(0.0);
	Point3 dir(0.0, 0.0, 1.0);
	Rgb color(1.0);
	float power = 1.0;
	float rad = 1.0;
	bool inf = true;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;
	bool p_only = false;

	params.getParam("direction", dir);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("infinite", inf);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);

	if(!inf)
	{
		if(!params.getParam("from", from))
		{
			if(params.getParam("position", from) && logger.isVerbose()) logger.logVerbose("DirectionalLight: Deprecated parameter 'position', use 'from' instead");
		}
		params.getParam("radius", rad);
	}

	auto light = std::unique_ptr<DirectionalLight>(new DirectionalLight(logger, from, Vec3(dir.x_, dir.y_, dir.z_), color, power, inf, rad, light_enabled, cast_shadows));

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY

