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
#include "param/param.h"
#include "scene/scene.h"
#include "geometry/ray.h"
#include "geometry/bound.h"
#include <limits>

namespace yafaray {

SunLight::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(direction_);
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(angle_);
	PARAM_LOAD(samples_);
}

ParamMap SunLight::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(direction_);
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(angle_);
	PARAM_SAVE(samples_);
	PARAM_SAVE_END;
}

ParamMap SunLight::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Light>, ParamResult> SunLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto light {std::make_unique<ThisClassType_t>(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(light), param_result};
}

SunLight::SunLight(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map):
		ParentClassType_t{logger, param_result, name, param_map, Flags::None}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	float angle{params_.angle_};
	if(angle > 80.f) angle = 80.f;
	cos_angle_ = math::cos(math::degToRad(angle));
	invpdf_ = math::mult_pi_by_2<> * (1.f - cos_angle_);
	pdf_ = 1.f / invpdf_;
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

std::tuple<bool, Ray, Rgb> SunLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray
