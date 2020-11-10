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

#include "light/light_meshlight.h"
#include "background/background.h"
#include "texture/texture.h"
#include "common/param.h"
#include "scene/scene.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "accelerator/accelerator_kdtree.h"
#include "geometry/triangle.h"
#include "geometry/object_triangle.h"
#include "geometry/surface.h"

BEGIN_YAFARAY

MeshLight::MeshLight(const std::string &object_name, const Rgb &col, int sampl, bool dbl_s, bool light_enabled, bool cast_shadows):
		object_name_(object_name), double_sided_(dbl_s), color_(col), samples_(sampl), tree_(nullptr)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;
	mesh_ = nullptr;
	area_dist_ = nullptr;
	tris_ = nullptr;
	//initIS();
}

MeshLight::~MeshLight()
{
	if(area_dist_) delete area_dist_;
	area_dist_ = nullptr;
	if(tris_) delete[] tris_;
	tris_ = nullptr;
	if(tree_) delete tree_;
	tree_ = nullptr;
}

void MeshLight::initIs()
{
	n_tris_ = mesh_->numPrimitives();
	tris_ = new const Triangle *[n_tris_];
	mesh_->getPrimitives(tris_);
	float *areas = new float[n_tris_];
	double total_area = 0.0;
	for(int i = 0; i < n_tris_; ++i)
	{
		areas[i] = tris_[i]->surfaceArea();
		total_area += areas[i];
	}
	area_dist_ = new Pdf1D(areas, n_tris_);
	area_ = (float)total_area;
	inv_area_ = (float)(1.0 / total_area);
	delete[] areas;
	if(tree_) delete tree_;

	ParamMap params;
	params["type"] = std::string("kdtree"); //Do not remove the std::string(), entering directly a string literal can be confused with bool until C++17 new string literals
	params["num_primitives"] = n_tris_;
	params["depth"] = -1;
	params["leaf_size"] = 1;
	params["cost_ratio"] = 0.8f;
	params["empty_bonus"] = 0.33f;

	tree_ = Accelerator<Triangle>::factory(tris_, params);
}

void MeshLight::init(Scene &scene)
{
	mesh_ = scene.getMesh(object_name_);
	if(mesh_)
	{
		initIs();
		// tell the mesh that a meshlight is associated with it (not sure if this is the best place though):
		mesh_->setLight(this);

		Y_VERBOSE << "MeshLight: triangles:" << n_tris_ << ", double sided:" << double_sided_ << ", area:" << area_ << " color:" << color_ << YENDL;
	}
}

void MeshLight::sampleSurface(Point3 &p, Vec3 &n, float s_1, float s_2) const
{
	float prim_pdf;
	int prim_num = area_dist_->dSample(s_1, &prim_pdf);
	if(prim_num >= area_dist_->count_)
	{
		Y_WARNING << "MeshLight: Sampling error!" << YENDL;
		return;
	}
	float ss_1, delta = area_dist_->cdf_[prim_num + 1];
	if(prim_num > 0)
	{
		delta -= area_dist_->cdf_[prim_num];
		ss_1 = (s_1 - area_dist_->cdf_[prim_num]) / delta;
	}
	else ss_1 = s_1 / delta;
	tris_[prim_num]->sample(ss_1, s_2, p, n);
	//	++stats[primNum];
}

Rgb MeshLight::totalEnergy() const { return (double_sided_ ? 2.f * color_ * area_ : color_ * area_); }

bool MeshLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	Vec3 n;
	Point3 p;
	sampleSurface(p, n, s.s_1_, s.s_2_);

	Vec3 ldir = p - sp.p_;
	//normalize vec and compute inverse square distance
	float dist_sqr = ldir.lengthSqr();
	float dist = math::sqrt(dist_sqr);
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
	float area_mul_cosangle = area_ * cos_angle;
	//TODO: replace the hardcoded value (1e-8f) by a macro for min/max values: here used, to avoid dividing by zero
	s.pdf_ = dist_sqr * M_PI / ((area_mul_cosangle == 0.f) ? 1e-8f : area_mul_cosangle);
	s.flags_ = flags_;
	if(s.sp_)
	{
		s.sp_->p_ = p;
		s.sp_->n_ = s.sp_->ng_ = n;
	}
	return true;
}

Rgb MeshLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	Vec3 normal, du, dv;
	ipdf = area_;
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

Rgb MeshLight::emitSample(Vec3 &wo, LSample &s) const
{
	s.area_pdf_ = inv_area_ * M_PI;
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

bool MeshLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	if(!tree_) return false;
	float dis;
	IntersectData bary;
	Triangle *hitt = nullptr;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else dis = ray.tmax_;
	// intersect with tree:
	if(!tree_->intersect(ray, dis, &hitt, t, bary)) { return false; }

	Vec3 n = hitt->getNormal();
	float cos_angle = ray.dir_ * (-n);
	if(cos_angle <= 0)
	{
		if(double_sided_) cos_angle = std::abs(cos_angle);
		else return false;
	}
	float idist_sqr = 1.f / (t * t);
	ipdf = idist_sqr * area_ * cos_angle * (1.f / M_PI);
	col = color_;

	return true;
}

float MeshLight::illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const
{
	Vec3 wo = sp.p_ - sp_light.p_;
	float r_2 = wo.normLenSqr();
	float cos_n = wo * sp_light.ng_;
	return cos_n > 0 ? r_2 * M_PI / (area_ * cos_n) : (double_sided_ ? r_2 * M_PI / (area_ * -cos_n) : 0.f);
}

void MeshLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * M_PI;
	cos_wo = wo * sp.n_;
	dir_pdf = cos_wo > 0.f ? (double_sided_ ? cos_wo * 0.5f : cos_wo) : (double_sided_ ? -cos_wo * 0.5f : 0.f);
}


Light *MeshLight::factory(ParamMap &params, const Scene &scene)
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

	MeshLight *light = new MeshLight(object_name, color * (float)power * M_PI, samples, double_s, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
