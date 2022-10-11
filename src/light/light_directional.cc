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
#include "param/param.h"
#include "scene/scene.h"
#include "common/logger.h"
#include "geometry/ray.h"

namespace yafaray {

DirectionalLight::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(from_);
	PARAM_LOAD(direction_);
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(radius_);
	PARAM_LOAD(infinite_);
}

ParamMap DirectionalLight::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(from_);
	PARAM_SAVE(direction_);
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(radius_);
	PARAM_SAVE(infinite_);
	PARAM_SAVE_END;
}

ParamMap DirectionalLight::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Light::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Light *, ParamError> DirectionalLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new DirectionalLight(logger, param_error, name, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<DirectionalLight>(name, {"type"}));
	return {result, param_error};
}

DirectionalLight::DirectionalLight(Logger &logger, ParamError &param_error, const std::string &name, const ParamMap &param_map):
		Light{logger, param_error, name, param_map, Light::Flags::DiracDir}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void DirectionalLight::init(const Scene &scene)
{
	// calculate necessary parameters for photon mapping if the light
	//  is set to illuminate the whole scene:
	const Bound w{scene.getSceneBound()};
	world_radius_ = 0.5f * (w.g_ - w.a_).length();
	if(params_.infinite_)
	{
		position_ = 0.5f * (w.a_ + w.g_);
		radius_ = world_radius_;
		area_pdf_ = 1.f / (radius_ * radius_); // Pi cancels out with our weird conventions :p
	}
	if(logger_.isVerbose()) logger_.logVerbose("DirectionalLight: pos ", position_, " world radius: ", world_radius_);
}

std::tuple<bool, Ray, Rgb> DirectionalLight::illuminate(const Point3f &surface_p, float time) const
{
	if(photonOnly()) return {};
	// check if the point is outside of the illuminated cylinder (non-infinite lights)
	float tmax;
	if(!params_.infinite_)
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
	if(params_.infinite_) from += direction_ * world_radius_;
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
	if(params_.infinite_) s.sp_->p_ += direction_ * world_radius_;
	s.area_pdf_ = area_pdf_;
	s.dir_pdf_ = 1.f;
	return {-direction_, color_};
}

} //namespace yafaray

