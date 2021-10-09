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

void ObjectLight::sampleSurface(Point3 &p, Vec3 &n, float s_1, float s_2) const
{
	float prim_pdf;
	const int prim_num = area_dist_->dSample(logger_, s_1, &prim_pdf);
	if(prim_num >= area_dist_->count_)
	{
		logger_.logWarning("ObjectLight: Sampling error!");
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
	//	++stats[primNum];
}

Rgb ObjectLight::totalEnergy() const { return (double_sided_ ? 2.f * color_ * area_ : color_ * area_); }

bool ObjectLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
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
	float cos_angle = -(ldir * n);
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0)
	{
		if(double_sided_) cos_angle = -cos_angle;
		else return false;
	}

	// fill direction
	wi.tmax_ = dist;
	wi.dir_ = ldir;

	s.col_ = color_;
	// pdf = distance^2 / area * cos(norm, ldir);
	const float area_mul_cosangle = area_ * cos_angle;
	//TODO: replace the hardcoded value (1e-8f) by a macro for min/max values: here used, to avoid dividing by zero
	s.pdf_ = dist_sqr * math::num_pi / ((area_mul_cosangle == 0.f) ? 1e-8f : area_mul_cosangle);
	s.flags_ = flags_;
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = n;
	}
	return true;
}

Rgb ObjectLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	ipdf = area_;
	Vec3 normal, du, dv;
	sampleSurface(ray.from_, normal, s_3, s_4);
	Vec3::createCs(normal, du, dv);
	if(double_sided_)
	{
		ipdf *= 2.f;
		if(s_1 > 0.5f) ray.dir_ = sample::cosHemisphere(-normal, du, dv, (s_1 - 0.5f) * 2.f, s_2);
		else 		ray.dir_ = sample::cosHemisphere(normal, du, dv, s_1 * 2.f, s_2);
	}
	else ray.dir_ = sample::cosHemisphere(normal, du, dv, s_1, s_2);
	return color_;
}

Rgb ObjectLight::emitSample(Vec3 &wo, LSample &s) const
{
	s.area_pdf_ = inv_area_ * math::num_pi;
	sampleSurface(s.sp_->p_, s.sp_->ng_, s.s_3_, s.s_4_);
	s.sp_->n_ = s.sp_->ng_;
	Vec3 du, dv;
	Vec3::createCs(s.sp_->ng_, du, dv);
	if(double_sided_)
	{
		if(s.s_1_ > 0.5f) wo = sample::cosHemisphere(-s.sp_->ng_, du, dv, (s.s_1_ - 0.5f) * 2.f, s.s_2_);
		else 		wo = sample::cosHemisphere(s.sp_->ng_, du, dv, s.s_1_ * 2.f, s.s_2_);
		s.dir_pdf_ = 0.5f * std::abs(s.sp_->ng_ * wo);
	}
	else
	{
		wo = sample::cosHemisphere(s.sp_->ng_, du, dv, s.s_1_, s.s_2_);
		s.dir_pdf_ = std::abs(s.sp_->ng_ * wo);
	}
	s.flags_ = flags_;
	return color_;
}

bool ObjectLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	if(!accelerator_) return false;
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::infinity();
	// intersect with tree:
	const AcceleratorIntersectData accelerator_intersect_data = accelerator_->intersect(ray, t_max);
	if(!accelerator_intersect_data.hit_) { return false; }
	const Vec3 n = accelerator_intersect_data.hit_primitive_->getGeometricNormal();
	float cos_angle = ray.dir_ * (-n);
	if(cos_angle <= 0.f)
	{
		if(double_sided_) cos_angle = std::abs(cos_angle);
		else return false;
	}
	const float idist_sqr = 1.f / (t * t);
	ipdf = idist_sqr * area_ * cos_angle * math::div_1_by_pi;
	col = color_;
	return true;
}

float ObjectLight::illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const
{
	Vec3 wo = sp.p_ - sp_light.p_;
	const float r_2 = wo.normLenSqr();
	const float cos_n = wo * sp_light.ng_;
	return cos_n > 0 ? r_2 * math::num_pi / (area_ * cos_n) : (double_sided_ ? r_2 * math::num_pi / (area_ * -cos_n) : 0.f);
}

void ObjectLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * math::num_pi;
	cos_wo = wo * sp.n_;
	dir_pdf = cos_wo > 0.f ? (double_sided_ ? cos_wo * 0.5f : cos_wo) : (double_sided_ ? -cos_wo * 0.5f : 0.f);
}


std::unique_ptr<Light> ObjectLight::factory(Logger &logger, ParamMap &params, const Scene &scene)
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

	auto light = std::unique_ptr<ObjectLight>(new ObjectLight(logger, object_name, color * (float)power * math::num_pi, samples, double_s, light_enabled, cast_shadows));

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
