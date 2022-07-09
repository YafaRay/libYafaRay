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
#include "geometry/ray.h"
#include "geometry/bound.h"

namespace yafaray {

DirectionalLight::DirectionalLight(Logger &logger, const Point3f &pos, Vec3f dir, const Rgb &col, float inte, bool inf, float rad, bool b_light_enabled, bool b_cast_shadows):
		Light(logger, Light::Flags::DiracDir), position_(pos), direction_(dir), radius_(rad), infinite_(inf)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	color_ = col * inte;
	intensity_ = color_.energy();
	direction_.normalize();
	duv_ = Vec3f::createCoordsSystem(direction_);
	const Vec3f &d{direction_};
	major_axis_ = (d[Axis::X] > d[Axis::Y]) ? ((d[Axis::X] > d[Axis::Z]) ? 0 : 2) : ((d[Axis::Y] > d[Axis::Z]) ? 1 : 2);
}

void DirectionalLight::init(Scene &scene)
{
	// calculate necessary parameters for photon mapping if the light
	//  is set to illuminate the whole scene:
	Bound w = scene.getSceneBound();
	world_radius_ = 0.5f * (w.g_ - w.a_).length();
	if(infinite_)
	{
		position_ = 0.5f * (w.a_ + w.g_);
		radius_ = world_radius_;
	}
	area_pdf_ = 1.f / (radius_ * radius_); // Pi cancels out with our weird conventions :p
	if(logger_.isVerbose()) logger_.logVerbose("DirectionalLight: pos ", position_, " world radius: ", world_radius_);
}


std::tuple<bool, Ray, Rgb> DirectionalLight::illuminate(const Point3f &surface_p, float time) const
{
	if(photonOnly()) return {};
	// check if the point is outside of the illuminated cylinder (non-infinite lights)
	float tmax;
	if(!infinite_)
	{
		const Vec3f vec{position_ - surface_p};
		float dist = (direction_ ^ vec).length();
		if(dist > radius_) return {};
		tmax = vec * direction_;
		if(tmax <= 0.f) return {};
	}
	else tmax = -1.f;
	Ray ray{surface_p, direction_, time, 0.f, tmax};
	return {true, std::move(ray), color_};
}

std::pair<bool, Ray> DirectionalLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	s.pdf_ = 1.f;
	auto[hit, ray, col]{illuminate(surface_p, time)};
	return {hit, std::move(ray)};
}

std::tuple<Ray, float, Rgb> DirectionalLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	const Uv<float> uv{Vec3f::shirleyDisk(s_1, s_2)};
	Point3f from{position_ + radius_ * (uv.u_ * duv_.u_ + uv.v_ * duv_.v_)};
	if(infinite_) from += direction_ * world_radius_;
	const float ipdf = math::num_pi<> * radius_ * radius_; //4.0f * num_pi;
	Ray ray{std::move(from), -direction_, time};
	return {std::move(ray), ipdf, color_};
}

std::pair<Vec3f, Rgb> DirectionalLight::emitSample(LSample &s, float time) const
{
	s.sp_->n_ = -direction_;
	s.flags_ = flags_;
	const Uv<float> uv{Vec3f::shirleyDisk(s.s_1_, s.s_2_)};
	s.sp_->p_ = position_ + radius_ * (uv.u_ * duv_.u_ + uv.v_ * duv_.v_);
	if(infinite_) s.sp_->p_ += direction_ * world_radius_;
	s.area_pdf_ = area_pdf_;
	s.dir_pdf_ = 1.f;
	return {-direction_, color_};
}

Light * DirectionalLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3f from{{0.f, 0.f, 0.f}};
	Point3f dir{{0.f, 0.f, 1.f}};
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

	auto light = new DirectionalLight(logger, from, dir, color, power, inf, rad, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

} //namespace yafaray

