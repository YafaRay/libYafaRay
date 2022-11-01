/****************************************************************************
 *      meshlight.cc: a light source using a triangle mesh as shape
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

#include <limits>

#include "light/light_object_light.h"
#include "background/background.h"
#include "texture/texture.h"
#include "param/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "accelerator/accelerator.h"
#include "geometry/primitive/primitive_face.h"
#include <memory>

namespace yafaray {

ObjectLight::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(object_name_);
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(samples_);
	PARAM_LOAD(double_sided_);
}

ParamMap ObjectLight::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(object_name_);
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(samples_);
	PARAM_SAVE(double_sided_);
	PARAM_SAVE_END;
}

ParamMap ObjectLight::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Light>, ParamResult> ObjectLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto light {std::make_unique<ThisClassType_t>(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(light), param_result};
}

ObjectLight::ObjectLight(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map):
		ParentClassType_t{logger, param_result, name, param_map, Flags::None}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void ObjectLight::initIs()
{
	num_primitives_ = primitives_.size();
	std::vector<float> areas(num_primitives_);
	double total_area = 0.0;
	for(size_t i = 0; i < num_primitives_; ++i)
	{
		areas[i] = primitives_[i]->surfaceArea(0.f);
		total_area += areas[i];
	}
	area_dist_ = std::make_unique<Pdf1D>(areas);
	area_ = static_cast<float>(total_area);
	inv_area_ = static_cast<float>(1.f / total_area);
	ParamMap params;
	params["type"] = std::string("yafaray-kdtree-original"); //Do not remove the std::string(), entering directly a string literal can be confused with bool
	params["depth"] = -1;
	accelerator_ = Accelerator::factory(logger_, primitives_, params).first;
}

void ObjectLight::init(Scene &scene)
{
	auto [object, object_id, object_result]{scene.getObject(params_.object_name_)};
	if(object)
	{
		primitives_ = object->getPrimitives();
		initIs();
		object->setLight(this);
		if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": primitives:", num_primitives_, ", double sided:", params_.double_sided_, ", area:", area_, " color:", color_);
	}
}

std::pair<Point3f, Vec3f> ObjectLight::sampleSurface(float s_1, float s_2, float time) const
{
	const auto [prim_num, prim_pdf]{area_dist_->dSample(s_1)};
	if(prim_num >= area_dist_->size())
	{
		logger_.logWarning(getClassName(), ": Sampling error!");
		return {};
	}
	float delta = area_dist_->cdf(prim_num);
	float ss_1;
	if(prim_num > 0)
	{
		delta -= area_dist_->cdf(prim_num - 1);
		ss_1 = (s_1 - area_dist_->cdf(prim_num - 1)) / delta;
	}
	else ss_1 = s_1 / delta;
	return primitives_[prim_num]->sample({ss_1, s_2}, time);
	//	++stats[primNum];
}

Rgb ObjectLight::totalEnergy() const { return (params_.double_sided_ ? 2.f * color_ * area_ : color_ * area_); }

std::pair<bool, Ray> ObjectLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	const auto [p, n]{sampleSurface(s.s_1_, s.s_2_, time)};
	Vec3f ldir{p - surface_p};
	//normalize vec and compute inverse square distance
	const float dist_sqr = ldir.lengthSquared();
	const float dist = math::sqrt(dist_sqr);
	if(dist <= 0.f) return {};
	ldir *= 1.f / dist;
	float cos_angle = -(ldir * n);
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0.f)
	{
		if(params_.double_sided_) cos_angle = -cos_angle;
		else return {};
	}
	s.col_ = color_;
	// pdf = distance^2 / area * cos(norm, ldir);
	const float area_mul_cosangle = area_ * cos_angle;
	//TODO: replace the hardcoded value (1e-8f) by a macro for min/max values: here used, to avoid dividing by zero
	s.pdf_ = dist_sqr * math::num_pi<> / ((area_mul_cosangle == 0.f) ? 1e-8f : area_mul_cosangle);
	s.flags_ = flags_;
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = n;
	}
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> ObjectLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	float ipdf = area_;
	auto [p, n]{sampleSurface(s_3, s_4, 0.f)};
	const Uv<Vec3f> duv{Vec3f::createCoordsSystem(n)};
	Vec3f dir;
	if(params_.double_sided_)
	{
		ipdf *= 2.f;
		if(s_1 > 0.5f) dir = sample::cosHemisphere(-n, duv, (s_1 - 0.5f) * 2.f, s_2);
		else dir = sample::cosHemisphere(n, duv, s_1 * 2.f, s_2);
	}
	else dir = sample::cosHemisphere(n, duv, s_1, s_2);
	Ray ray{std::move(p), std::move(dir), time};
	return {std::move(ray), ipdf, color_};
}

std::pair<Vec3f, Rgb> ObjectLight::emitSample(LSample &s, float time) const
{
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	std::tie(s.sp_->p_, s.sp_->ng_) = sampleSurface(s.s_3_, s.s_4_, time);
	s.sp_->n_ = s.sp_->ng_;
	const Uv<Vec3f> duv{Vec3f::createCoordsSystem(s.sp_->ng_)};
	Vec3f dir;
	if(params_.double_sided_)
	{
		if(s.s_1_ > 0.5f) dir = sample::cosHemisphere(-s.sp_->ng_, duv, (s.s_1_ - 0.5f) * 2.f, s.s_2_);
		else dir = sample::cosHemisphere(s.sp_->ng_, duv, s.s_1_ * 2.f, s.s_2_);
		s.dir_pdf_ = 0.5f * std::abs(s.sp_->ng_ * dir);
	}
	else
	{
		dir = sample::cosHemisphere(s.sp_->ng_, duv, s.s_1_, s.s_2_);
		s.dir_pdf_ = std::abs(s.sp_->ng_ * dir);
	}
	s.flags_ = flags_;
	return {std::move(dir), color_};
}

std::tuple<bool, float, Rgb> ObjectLight::intersect(const Ray &ray, float &t) const
{
	if(!accelerator_) return {};
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::max();
	// intersect with tree:
	const IntersectData intersect_data = accelerator_->intersect(ray, t_max);
	if(!intersect_data.isHit()) return {};
	const Vec3f n{intersect_data.primitive_->getGeometricNormal(intersect_data.uv_, 0, false)};
	float cos_angle = ray.dir_ * (-n);
	if(cos_angle <= 0.f)
	{
		if(params_.double_sided_) cos_angle = std::abs(cos_angle);
		else return {};
	}
	const float idist_sqr = 1.f / (t * t);
	const float ipdf = idist_sqr * area_ * cos_angle * math::div_1_by_pi<>;
	return {true, ipdf, color_};
}

float ObjectLight::illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const
{
	Vec3f wo{surface_p - light_p};
	const float r_2 = wo.normalizeAndReturnLengthSquared();
	const float cos_n = wo * light_ng;
	return cos_n > 0 ? r_2 * math::num_pi<> / (area_ * cos_n) : (params_.double_sided_ ? r_2 * math::num_pi<> / (area_ * -cos_n) : 0.f);
}

std::array<float, 3> ObjectLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	const float area_pdf = inv_area_ * math::num_pi<>;
	const float cos_wo = wo * surface_n;
	const float dir_pdf = cos_wo > 0.f ? (params_.double_sided_ ? cos_wo * 0.5f : cos_wo) : (params_.double_sided_ ? -cos_wo * 0.5f : 0.f);
	return {area_pdf, dir_pdf, cos_wo};
}

std::tuple<bool, Ray, Rgb> ObjectLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray
