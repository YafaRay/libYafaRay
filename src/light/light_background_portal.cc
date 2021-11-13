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
#include "common/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "accelerator/accelerator.h"
#include "geometry/surface.h"
#include "sampler/sample_pdf1d.h"
#include "geometry/object/object_mesh.h"
#include "geometry/primitive/primitive_face.h"
#include <limits>

BEGIN_YAFARAY

BackgroundPortalLight::BackgroundPortalLight(Logger &logger, const std::string &object_name, int sampl, float pow, bool light_enabled, bool cast_shadows):
		Light(logger), object_name_(object_name), samples_(sampl), power_(pow)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;
	a_pdf_ = 0.f;
}

void BackgroundPortalLight::initIs()
{
	num_primitives_ = base_object_->numPrimitives();
	primitives_ = base_object_->getPrimitives();
	const auto areas = std::unique_ptr<float[]>(new float[num_primitives_]);
	double total_area = 0.0;
	for(int i = 0; i < num_primitives_; ++i)
	{
		areas[i] = primitives_[i]->surfaceArea();
		total_area += areas[i];
	}
	area_dist_ = std::unique_ptr<Pdf1D>(new Pdf1D(areas.get(), num_primitives_));
	area_ = static_cast<float>(total_area);
	inv_area_ = static_cast<float>(1.0 / total_area);
	accelerator_ = nullptr;

	ParamMap params;
	params["type"] = std::string("yafaray-kdtree-original"); //Do not remove the std::string(), entering directly a string literal can be confused with bool until C++17 new string literals
	params["num_primitives"] = num_primitives_;
	params["depth"] = -1;
	params["leaf_size"] = 1;
	params["cost_ratio"] = 0.8f;
	params["empty_bonus"] = 0.33f;

	accelerator_ = Accelerator::factory(logger_, primitives_, params);
}

void BackgroundPortalLight::init(Scene &scene)
{
	bg_ = scene.getBackground();
	const Bound w = scene.getSceneBound();
	const float world_radius = 0.5 * (w.g_ - w.a_).length();
	a_pdf_ = world_radius * world_radius;

	world_center_ = 0.5 * (w.a_ + w.g_);
	base_object_ = scene.getObject(object_name_);
	if(base_object_)
	{
		base_object_->setVisibility(Visibility::Invisible);
		initIs();
		if(logger_.isVerbose()) logger_.logVerbose("bgPortalLight: Triangles:", num_primitives_, ", Area:", area_);
		base_object_->setLight(this);
	}
}

void BackgroundPortalLight::sampleSurface(Point3 &p, Vec3 &n, float s_1, float s_2) const
{
	float prim_pdf;
	const int prim_num = area_dist_->dSample(logger_, s_1, &prim_pdf);
	if(prim_num >= area_dist_->count_)
	{
		logger_.logWarning("bgPortalLight: Sampling error!");
		return;
	}
	float ss_1, delta = area_dist_->cdf_[prim_num + 1];
	if(prim_num > 0)
	{
		delta -= area_dist_->cdf_[prim_num];
		ss_1 = (s_1 - area_dist_->cdf_[prim_num]) / delta;
	}
	else ss_1 = s_1 / delta;
	primitives_[prim_num]->sample(ss_1, s_2, p, n);
}

Rgb BackgroundPortalLight::totalEnergy() const
{
	Ray wo;
	wo.from_ = world_center_;
	Rgb energy{0.f};
	for(int i = 0; i < 1000; ++i) //exaggerated?
	{
		wo.dir_ = sample::sphere(((float) i + 0.5f) / 1000.f, sample::riVdC(i));
		const Rgb col = bg_->eval(wo.dir_, true);
		for(int j = 0; j < num_primitives_; j++)
		{
			float cos_n = -wo.dir_ * primitives_[j]->getGeometricNormal(); //not 100% sure about sign yet...
			if(cos_n > 0) energy += col * cos_n * primitives_[j]->surfaceArea();
		}
	}
	return energy * math::div_1_by_pi * 0.001f;
}

bool BackgroundPortalLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	Vec3 n;
	Point3 p;
	sampleSurface(p, n, s.s_1_, s.s_2_);

	Vec3 ldir = p - sp.p_;
	//normalize vec and compute inverse square distance
	const float dist_sqr = ldir.lengthSqr();
	const float dist = math::sqrt(dist_sqr);
	if(dist <= 0.0) return false;
	ldir *= 1.f / dist;
	const float cos_angle = -(ldir * n);
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0) return false;

	// fill direction
	wi.tmax_ = dist;
	wi.dir_ = ldir;

	s.col_ = bg_->eval(wi.dir_, true) * power_;
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf_ = dist_sqr * math::num_pi / (area_ * cos_angle);
	s.flags_ = flags_;
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = n;
	}
	return true;
}

Rgb BackgroundPortalLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	ipdf = area_;
	Vec3 normal, du, dv;
	sampleSurface(ray.from_, normal, s_3, s_4);
	Vec3::createCs(normal, du, dv);
	ray.dir_ = sample::cosHemisphere(normal, du, dv, s_1, s_2);
	const Ray r_2(ray.from_, -ray.dir_);
	return bg_->eval(r_2.dir_, true);
}

Rgb BackgroundPortalLight::emitSample(Vec3 &wo, LSample &s) const
{
	s.area_pdf_ = inv_area_ * math::num_pi;
	sampleSurface(s.sp_->p_, s.sp_->ng_, s.s_3_, s.s_4_);
	s.sp_->n_ = s.sp_->ng_;
	Vec3 du, dv;
	Vec3::createCs(s.sp_->ng_, du, dv);

	wo = sample::cosHemisphere(s.sp_->ng_, du, dv, s.s_1_, s.s_2_);
	s.dir_pdf_ = std::abs(s.sp_->ng_ * wo);

	s.flags_ = flags_;
	const Ray r_2(s.sp_->p_, -wo);
	return bg_->eval(r_2.dir_, true);
}

bool BackgroundPortalLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	if(!accelerator_) return false;
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::infinity();
	// intersect with tree:
	const AcceleratorIntersectData accelerator_intersect_data = accelerator_->intersect(ray, t_max);
	if(!accelerator_intersect_data.hit_) { return false; }
	const Vec3 n = accelerator_intersect_data.hit_primitive_->getGeometricNormal();
	const float cos_angle = ray.dir_ * (-n);
	if(cos_angle <= 0.f) return false;
	const float idist_sqr = 1.f / (t * t);
	ipdf = idist_sqr * area_ * cos_angle * math::div_1_by_pi;
	col = bg_->eval(ray.dir_, true) * power_;
	col.clampProportionalRgb(clamp_intersect_); //trick to reduce light sampling noise at the expense of realism and inexact overall light. 0.f disables clamping
	return true;
}

float BackgroundPortalLight::illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const
{
	Vec3 wo = sp.p_ - sp_light.p_;
	const float r_2 = wo.normLenSqr();
	const float cos_n = wo * sp_light.ng_;
	return cos_n > 0 ? (r_2 * math::num_pi / (area_ * cos_n)) : 0.f;
}

void BackgroundPortalLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * math::num_pi;
	cos_wo = wo * sp.n_;
	dir_pdf = cos_wo > 0.f ? cos_wo : 0.f;
}


std::unique_ptr<Light> BackgroundPortalLight::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	int samples = 4;
	std::string object_name;
	float pow = 1.0f;
	bool shoot_d = true;
	bool shoot_c = true;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool p_only = false;

	params.getParam("object_name", object_name);
	params.getParam("samples", samples);
	params.getParam("power", pow);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);

	auto light = std::unique_ptr<BackgroundPortalLight>(new BackgroundPortalLight(logger, object_name, samples, pow, light_enabled, cast_shadows));

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
