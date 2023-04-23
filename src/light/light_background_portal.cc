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

#include "light/light_background_portal.h"
#include "background/background.h"
#include "param/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "accelerator/accelerator.h"
#include "sampler/sample_pdf1d.h"
#include "geometry/object/object_mesh.h"
#include "geometry/primitive/primitive_face.h"
#include <limits>
#include <memory>

namespace yafaray {

std::map<std::string, const ParamMeta *> BackgroundPortalLight::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(samples_);
	PARAM_META(object_name_);
	PARAM_META(power_);
	PARAM_META(ibl_clamp_sampling_);
	return param_meta_map;
}

BackgroundPortalLight::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(samples_);
	PARAM_LOAD(object_name_);
	PARAM_LOAD(power_);
	PARAM_LOAD(ibl_clamp_sampling_);
}

ParamMap BackgroundPortalLight::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	param_map.setParam(Params::object_name_meta_, objects_.findNameFromId(object_id_).first);
	PARAM_SAVE(samples_);
	PARAM_SAVE(power_);
	PARAM_SAVE(ibl_clamp_sampling_);
	return param_map;
}

std::pair<std::unique_ptr<Light>, ParamResult> BackgroundPortalLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto light {std::make_unique<BackgroundPortalLight>(logger, param_result, name, param_map, scene.getObjects(), scene.getLights())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(light), param_result};
}

BackgroundPortalLight::BackgroundPortalLight(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map, const Items<Object> &objects, const Items<Light> &lights):
		ParentClassType_t{logger, param_result, param_map, Flags::None, lights}, params_{param_result, param_map}, objects_{objects}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

void BackgroundPortalLight::initIs()
{
	const size_t num_primitives{primitives_.size()};
	std::vector<float> areas(num_primitives);
	double total_area = 0.0;
	for(size_t i = 0; i < num_primitives; ++i)
	{
		areas[i] = primitives_[i]->surfaceArea(0.f);
		total_area += areas[i];
	}
	area_dist_ = std::make_unique<Pdf1D>(areas);
	area_ = static_cast<float>(total_area);
	inv_area_ = static_cast<float>(1.0 / total_area);
	ParamMap params;
	params["type"] = std::string("yafaray-kdtree-original"); //Do not remove the std::string(), entering directly a string literal can be confused with bool
	params["depth"] = -1;
	accelerator_ = Accelerator::factory(logger_, nullptr, primitives_, params).first;
}

size_t BackgroundPortalLight::init(const Scene &scene)
{
	background_ = scene.getBackground();
	const Bound w = scene.getSceneBound();
	/* FIXME: this is unused for some reason, removing from members and commenting this out. What is this supposed to do?
	const float world_radius = 0.5f * (w.g_ - w.a_).length();
	a_pdf_ = world_radius * world_radius; */

	world_center_ = 0.5f * (w.a_ + w.g_);

	if(object_id_ == math::invalid<size_t>) object_id_ = objects_.findIdFromName(params_.object_name_).first;
	const auto [object, object_result]{scene.getObject(object_id_)};
	if(object)
	{
		primitives_ = object->getPrimitives();
		initIs();
		if(logger_.isVerbose()) logger_.logVerbose(getClassName(), ": number of primitives: ", primitives_.size(), ", area: ", area_);
		return object_id_;
	}
	else
	{
		logger_.logError(getClassName(), ": '" + getName() + "': associated object '" + params_.object_name_ + "' could not be found!");
		return math::invalid<size_t>;
	}
}

std::pair<Point3f, Vec3f> BackgroundPortalLight::sampleSurface(float s_1, float s_2, float time) const
{
	const auto [prim_num, prim_pdf]{area_dist_->dSample(s_1)};
	if(prim_num >= area_dist_->size())
	{
		logger_.logWarning("bgPortalLight: Sampling error!");
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
}

Rgb BackgroundPortalLight::totalEnergy() const
{
	Ray wo;
	wo.from_ = world_center_;
	Rgb energy{0.f};
	for(int i = 0; i < 1000; ++i) //exaggerated?
	{
		wo.dir_ = sample::sphere(((float) i + 0.5f) / 1000.f, sample::riVdC(i));
		const Rgb col = background_->eval(wo.dir_, true);
		for(const auto primitive : primitives_)
		{
			float cos_n = -wo.dir_ * primitive->getGeometricNormal({}, 0.f, false); //not 100% sure about sign yet...
			if(cos_n > 0) energy += col * cos_n * primitive->surfaceArea(0.f);
		}
	}
	return energy * math::div_1_by_pi<> * 0.001f;
}

std::pair<bool, Ray> BackgroundPortalLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	const auto [p, n]{sampleSurface(s.s_1_, s.s_2_, time)};
	Vec3f ldir{p - surface_p};
	//normalize vec and compute inverse square distance
	const float dist_sqr = ldir.lengthSquared();
	const float dist = math::sqrt(dist_sqr);
	if(dist <= 0.f) return {};
	ldir *= 1.f / dist;
	const float cos_angle = -(ldir * n);
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0.f) return {};
	s.col_ = background_->eval(ldir, true) * params_.power_;
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf_ = dist_sqr * math::num_pi<> / (area_ * cos_angle);
	s.flags_ = flags_;
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = n;
	}
	Ray ray{surface_p, ldir, time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> BackgroundPortalLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	const auto [p, n]{sampleSurface(s_3, s_4, time)};
	const Uv<Vec3f> duv{Vec3f::createCoordsSystem(n)};
	const Vec3f dir{sample::cosHemisphere(n, duv, s_1, s_2)};
	Ray ray{p, -dir, time}; //FIXME: is it correct to use p or should we use the coordinates 0,0,0 for ray origin?
	return {std::move(ray), true, background_->eval(-dir, true)};
}

std::pair<Vec3f, Rgb> BackgroundPortalLight::emitSample(LSample &s, float time) const
{
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	std::tie(s.sp_->p_, s.sp_->ng_) = sampleSurface(s.s_3_, s.s_4_, time);
	s.sp_->n_ = s.sp_->ng_;
	const Uv<Vec3f> duv{Vec3f::createCoordsSystem(s.sp_->ng_)};
	Vec3f dir{sample::cosHemisphere(s.sp_->ng_, duv, s.s_1_, s.s_2_)};
	s.dir_pdf_ = std::abs(s.sp_->ng_ * dir);
	s.flags_ = flags_;
	return {-dir, background_->eval(-dir, true)};
}

std::tuple<bool, float, Rgb> BackgroundPortalLight::intersect(const Ray &ray, float &t) const
{
	if(!accelerator_) return {};
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::max();
	// intersect with tree:
	const IntersectData intersect_data = accelerator_->intersect(ray, t_max);
	if(!intersect_data.isHit()) return {};
	const Vec3f n{intersect_data.primitive_->getGeometricNormal({}, ray.time_, false)};
	const float cos_angle = ray.dir_ * (-n);
	if(cos_angle <= 0.f) return {};
	const float idist_sqr = 1.f / (t * t);
	const float ipdf = idist_sqr * area_ * cos_angle * math::div_1_by_pi<>;
	Rgb col{background_->eval(ray.dir_, true) * params_.power_};
	col.clampProportionalRgb(params_.ibl_clamp_sampling_); //trick to reduce light sampling noise at the expense of realism and inexact overall light. 0.f disables clamping
	return {true, ipdf, col};
}

float BackgroundPortalLight::illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const
{
	Vec3f wo{surface_p - light_p};
	const float r_2 = wo.normalizeAndReturnLengthSquared();
	const float cos_n = wo * light_ng;
	return cos_n > 0 ? (r_2 * math::num_pi<> / (area_ * cos_n)) : 0.f;
}

std::array<float, 3> BackgroundPortalLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	const float area_pdf = inv_area_ * math::num_pi<>;
	const float cos_wo = wo * surface_n;
	const float dir_pdf = cos_wo > 0.f ? cos_wo : 0.f;
	return {area_pdf, dir_pdf, cos_wo};
}

std::tuple<bool, Ray, Rgb> BackgroundPortalLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray
