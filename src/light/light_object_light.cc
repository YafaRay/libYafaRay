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
#include "common/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "accelerator/accelerator.h"
#include "geometry/surface.h"
#include "geometry/object/object_mesh.h"
#include "geometry/primitive/primitive_face.h"
#include <limits>
#include <memory>

BEGIN_YAFARAY

ObjectLight::ObjectLight(Logger &logger, const std::string &object_name, const Rgb &col, int sampl, bool dbl_s, bool light_enabled, bool cast_shadows):
		Light(logger), object_name_(object_name), double_sided_(dbl_s), color_(col), samples_(sampl)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;
	//initIs();
}

void ObjectLight::initIs()
{
	num_primitives_ = base_object_->numPrimitives();
	primitives_ = base_object_->getPrimitives();
	std::vector<float> areas(num_primitives_);
	double total_area = 0.0;
	for(int i = 0; i < num_primitives_; ++i)
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
	accelerator_ = std::unique_ptr<const Accelerator>(Accelerator::factory(logger_, primitives_, params));
}

void ObjectLight::init(Scene &scene)
{
	base_object_ = scene.getObject(object_name_);
	if(base_object_)
	{
		initIs();
		// tell the mesh that a meshlight is associated with it (not sure if this is the best place though):
		base_object_->setLight(this);

		if(logger_.isVerbose()) logger_.logVerbose("ObjectLight: primitives:", num_primitives_, ", double sided:", double_sided_, ", area:", area_, " color:", color_);
	}
}

std::pair<Point3, Vec3> ObjectLight::sampleSurface(float s_1, float s_2, float time) const
{
	float prim_pdf;
	const size_t prim_num = area_dist_->dSample(s_1, prim_pdf);
	if(prim_num >= area_dist_->size())
	{
		logger_.logWarning("ObjectLight: Sampling error!");
		return {};
	}
	float ss_1, delta = area_dist_->cdf(prim_num);
	if(prim_num > 0)
	{
		delta -= area_dist_->cdf(prim_num - 1);
		ss_1 = (s_1 - area_dist_->cdf(prim_num - 1)) / delta;
	}
	else ss_1 = s_1 / delta;
	return primitives_[prim_num]->sample({ss_1, s_2}, time);
	//	++stats[primNum];
}

Rgb ObjectLight::totalEnergy() const { return (double_sided_ ? 2.f * color_ * area_ : color_ * area_); }

std::pair<bool, Ray> ObjectLight::illumSample(const Point3 &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	const auto [p, n]{sampleSurface(s.s_1_, s.s_2_, time)};
	Vec3 ldir{p - surface_p};
	//normalize vec and compute inverse square distance
	const float dist_sqr = ldir.lengthSqr();
	const float dist = math::sqrt(dist_sqr);
	if(dist <= 0.f) return {};
	ldir *= 1.f / dist;
	float cos_angle = -(ldir * n);
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0.f)
	{
		if(double_sided_) cos_angle = -cos_angle;
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
	const Uv<Vec3> duv{Vec3::createCoordsSystem(n)};
	Vec3 dir;
	if(double_sided_)
	{
		ipdf *= 2.f;
		if(s_1 > 0.5f) dir = sample::cosHemisphere(-n, duv, (s_1 - 0.5f) * 2.f, s_2);
		else dir = sample::cosHemisphere(n, duv, s_1 * 2.f, s_2);
	}
	else dir = sample::cosHemisphere(n, duv, s_1, s_2);
	Ray ray{std::move(p), std::move(dir), time};
	return {std::move(ray), ipdf, color_};
}

std::pair<Vec3, Rgb> ObjectLight::emitSample(LSample &s, float time) const
{
	s.area_pdf_ = inv_area_ * math::num_pi<>;
	std::tie(s.sp_->p_, s.sp_->ng_) = sampleSurface(s.s_3_, s.s_4_, time);
	s.sp_->n_ = s.sp_->ng_;
	const Uv<Vec3> duv{Vec3::createCoordsSystem(s.sp_->ng_)};
	Vec3 dir;
	if(double_sided_)
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
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::infinity();
	// intersect with tree:
	const AcceleratorIntersectData accelerator_intersect_data = accelerator_->intersect(ray, t_max);
	if(!accelerator_intersect_data.isHit()) return {};
	const Vec3 n{accelerator_intersect_data.primitive()->getGeometricNormal(accelerator_intersect_data.uv(), 0)};
	float cos_angle = ray.dir_ * (-n);
	if(cos_angle <= 0.f)
	{
		if(double_sided_) cos_angle = std::abs(cos_angle);
		else return {};
	}
	const float idist_sqr = 1.f / (t * t);
	const float ipdf = idist_sqr * area_ * cos_angle * math::div_1_by_pi<>;
	return {true, ipdf, color_};
}

float ObjectLight::illumPdf(const Point3 &surface_p, const Point3 &light_p, const Vec3 &light_ng) const
{
	Vec3 wo{surface_p - light_p};
	const float r_2 = wo.normLenSqr();
	const float cos_n = wo * light_ng;
	return cos_n > 0 ? r_2 * math::num_pi<> / (area_ * cos_n) : (double_sided_ ? r_2 * math::num_pi<> / (area_ * -cos_n) : 0.f);
}

void ObjectLight::emitPdf(const Vec3 &surface_n, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * math::num_pi<>;
	cos_wo = wo * surface_n;
	dir_pdf = cos_wo > 0.f ? (double_sided_ ? cos_wo * 0.5f : cos_wo) : (double_sided_ ? -cos_wo * 0.5f : 0.f);
}


Light * ObjectLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	bool double_s = false;
	Rgb color(1.0);
	double power = 1.0;
	int samples = 4;
	std::string object_name;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;
	bool p_only = false;

	params.getParam("object_name", object_name);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("samples", samples);
	params.getParam("double_sided", double_s);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);

	auto light = new ObjectLight(logger, object_name, color * static_cast<float>(power) * math::num_pi<>, samples, double_s, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

std::tuple<bool, Ray, Rgb> ObjectLight::illuminate(const Point3 &surface_p, float time) const
{
	return {};
}

END_YAFARAY
