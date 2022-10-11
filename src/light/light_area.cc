/****************************************************************************
 *      arealight.cc: a rectangular area light source
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

#include "light/light_area.h"
#include "geometry/surface.h"
#include "param/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "common/logger.h"
#include "geometry/ray.h"

namespace yafaray {

AreaLight::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(corner_);
	PARAM_LOAD(point_1_);
	PARAM_LOAD(point_2_);
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(samples_);
	PARAM_LOAD(object_name_);
}

ParamMap AreaLight::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(corner_);
	PARAM_SAVE(point_1_);
	PARAM_SAVE(point_2_);
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(samples_);
	PARAM_SAVE(object_name_);
	PARAM_SAVE_END;
}

ParamMap AreaLight::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Light>, ParamError> AreaLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto light {std::make_unique<ThisClassType_t>(logger, param_error, name, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	return {std::move(light), param_error};
}

AreaLight::AreaLight(Logger &logger, ParamError &param_error, const std::string &name, const ParamMap &param_map):
		ParentClassType_t{logger, param_error, name, param_map, Flags::None}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void AreaLight::init(const Scene &scene)
{
	if(!params_.object_name_.empty())
	{
		Object *obj = scene.getObject(params_.object_name_);
		if(obj) obj->setLight(this);
		else logger_.logError("AreaLight: '" + name_ + "': associated object '" + params_.object_name_ + "' could not be found!");
	}
}

Rgb AreaLight::totalEnergy() const { return color_ * area_; }

std::pair<bool, Ray> AreaLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	//get point on area light and vector to surface point:
	const Point3f p{area_quad_[0] + s.s_1_ * to_x_ + s.s_2_ * to_y_};
	Vec3f ldir{p - surface_p};
	//normalize vec and compute inverse square distance
	const float dist_sqr = ldir.lengthSquared();
	const float dist = math::sqrt(dist_sqr);
	if(dist <= 0.f) return {};
	ldir *= 1.f / dist;
	const float cos_angle = ldir * normal_flipped_;
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0.f) return {};
	s.col_ = color_;
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf_ = dist_sqr * math::num_pi<> / (area_ * cos_angle);
	s.flags_ = Flags::None; // no delta functions...
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = normal_;
	}
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> AreaLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	const float ipdf = area_/*  * num_pi */; // really two num_pi?
	Point3f from{area_quad_[0] + s_3 * to_x_ + s_4 * to_y_};
	Vec3f dir{sample::cosHemisphere(normal_, duv_, s_1, s_2)};
	Ray ray{std::move(from), std::move(dir), time};
	return {std::move(ray), ipdf, color_};
}

std::pair<Vec3f, Rgb> AreaLight::emitSample(LSample &s, float time) const
{
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	s.sp_->p_ = area_quad_[0] + s.s_3_ * to_x_ + s.s_4_ * to_y_;
	const Vec3f dir{sample::cosHemisphere(normal_, duv_, s.s_1_, s.s_2_)};
	s.sp_->n_ = s.sp_->ng_ = normal_;
	s.dir_pdf_ = std::abs(normal_ * dir);
	s.flags_ = Flags::None; // no delta functions...
	return {std::move(dir), color_}; // still not 100% sure this is correct without cosine...
}

std::tuple<bool, float, Rgb> AreaLight::intersect(const Ray &ray, float &t) const
{
	const float cos_angle = ray.dir_ * normal_flipped_;
	if(cos_angle <= 0.f) return {}; //no light if point is behind area light (single sided!)
	const auto intersect_data{area_quad_.intersect(ray.from_, ray.dir_)};
	if(intersect_data.first <= 0.f) return {};
	else t = intersect_data.first;
	const float ipdf = 1.f / (t * t) * area_ * cos_angle * math::div_1_by_pi<>; // pdf = distance^2 / area * cos(norm, ldir); ipdf = 1/pdf;
	return {true, ipdf, color_};
}

float AreaLight::illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &) const
{
	Vec3f wi{light_p - surface_p};
	float r_2 = wi.normalizeAndReturnLengthSquared();
	float cos_n = wi * normal_flipped_;
	return cos_n > 0 ? r_2 * math::num_pi<> / (area_ * cos_n) : 0.f;
}

std::array<float, 3> AreaLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	const float area_pdf = inv_area_ * math::num_pi<>;
	const float cos_wo = wo * surface_n;
	const float dir_pdf = cos_wo > 0 ? cos_wo : 0.f;
	return {area_pdf, dir_pdf, cos_wo};
}

std::tuple<bool, Ray, Rgb> AreaLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray
