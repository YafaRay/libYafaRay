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

#include "light/light_background_portal.h"
#include "background/background.h"
#include "common/param.h"
#include "common/scene.h"
#include "utility/util_sample.h"
#include "utility/util_mcqmc.h"
#include "common/kdtree.h"
#include "common/triangle.h"

BEGIN_YAFARAY

BackgroundPortalLight::BackgroundPortalLight(const std::string &object_name, int sampl, float pow, bool light_enabled, bool cast_shadows):
		object_name_(object_name), samples_(sampl), power_(pow), tree_(nullptr)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;
	mesh_ = nullptr;
	a_pdf_ = 0.f;
	area_dist_ = nullptr;
	tris_ = nullptr;
}

BackgroundPortalLight::~BackgroundPortalLight()
{
	if(area_dist_) delete area_dist_;
	area_dist_ = nullptr;
	if(tris_) delete[] tris_;
	tris_ = nullptr;
	if(tree_)
	{
		delete tree_;
		tree_ = nullptr;
	}
}

void BackgroundPortalLight::initIs()
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
	//delete[] tris;
	delete[] areas;
	if(tree_) delete tree_;
	tree_ = new KdTree<Triangle>(tris_, n_tris_, -1, 1, 0.8, 0.33);
}

void BackgroundPortalLight::init(Scene &scene)
{
	bg_ = scene.getBackground();
	Bound w = scene.getSceneBound();
	float world_radius = 0.5 * (w.g_ - w.a_).length();
	a_pdf_ = world_radius * world_radius;

	world_center_ = 0.5 * (w.a_ + w.g_);
	mesh_ = scene.getMesh(object_name_);
	if(mesh_)
	{
		mesh_->setVisibility(false);

		initIs();
		Y_VERBOSE << "bgPortalLight: Triangles:" << n_tris_ << ", Area:" << area_ << YENDL;
		mesh_->setLight(this);
	}
}

void BackgroundPortalLight::sampleSurface(Point3 &p, Vec3 &n, float s_1, float s_2) const
{
	float prim_pdf;
	int prim_num = area_dist_->dSample(s_1, &prim_pdf);
	if(prim_num >= area_dist_->count_)
	{
		Y_WARNING << "bgPortalLight: Sampling error!" << YENDL;
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
}

Rgb BackgroundPortalLight::totalEnergy() const
{
	Ray wo;
	wo.from_ = world_center_;
	Rgb energy, col;
	for(int i = 0; i < 1000; ++i) //exaggerated?
	{
		wo.dir_ = sampleSphere__(((float) i + 0.5f) / 1000.f, riVdC__(i));
		col = bg_->eval(wo, true);
		for(int j = 0; j < n_tris_; j++)
		{
			float cos_n = -wo.dir_ * tris_[j]->getNormal(); //not 100% sure about sign yet...
			if(cos_n > 0) energy += col * cos_n * tris_[j]->surfaceArea();
		}
	}

	return energy * M_1_PI * 0.001f;
}

bool BackgroundPortalLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	Vec3 n;
	Point3 p;
	sampleSurface(p, n, s.s_1_, s.s_2_);

	Vec3 ldir = p - sp.p_;
	//normalize vec and compute inverse square distance
	float dist_sqr = ldir.lengthSqr();
	float dist = fSqrt__(dist_sqr);
	if(dist <= 0.0) return false;
	ldir *= 1.f / dist;
	float cos_angle = -(ldir * n);
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0) return false;

	// fill direction
	wi.tmax_ = dist;
	wi.dir_ = ldir;

	s.col_ = bg_->eval(wi, true) * power_;
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf_ = dist_sqr * M_PI / (area_ * cos_angle);
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
	Vec3 normal, du, dv;
	ipdf = area_;
	sampleSurface(ray.from_, normal, s_3, s_4);
	createCs__(normal, du, dv);

	ray.dir_ = sampleCosHemisphere__(normal, du, dv, s_1, s_2);
	Ray r_2(ray.from_, -ray.dir_);
	return bg_->eval(r_2, true);
}

Rgb BackgroundPortalLight::emitSample(Vec3 &wo, LSample &s) const
{
	s.area_pdf_ = inv_area_ * M_PI;
	sampleSurface(s.sp_->p_, s.sp_->ng_, s.s_3_, s.s_4_);
	s.sp_->n_ = s.sp_->ng_;
	Vec3 du, dv;
	createCs__(s.sp_->ng_, du, dv);

	wo = sampleCosHemisphere__(s.sp_->ng_, du, dv, s.s_1_, s.s_2_);
	s.dir_pdf_ = std::fabs(s.sp_->ng_ * wo);

	s.flags_ = flags_;
	Ray r_2(s.sp_->p_, -wo);
	return bg_->eval(r_2, true);
}

bool BackgroundPortalLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
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
	if(cos_angle <= 0) return false;
	float idist_sqr = 1.f / (t * t);
	ipdf = idist_sqr * area_ * cos_angle * (1.f / M_PI);
	col = bg_->eval(ray, true) * power_;

	col.clampProportionalRgb(clamp_intersect_); //trick to reduce light sampling noise at the expense of realism and inexact overall light. 0.f disables clamping

	return true;
}

float BackgroundPortalLight::illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const
{
	Vec3 wo = sp.p_ - sp_light.p_;
	float r_2 = wo.normLenSqr();
	float cos_n = wo * sp_light.ng_;
	return cos_n > 0 ? (r_2 * M_PI / (area_ * cos_n)) : 0.f;
}

void BackgroundPortalLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = inv_area_ * M_PI;
	cos_wo = wo * sp.n_;
	dir_pdf = cos_wo > 0.f ? cos_wo : 0.f;
}


Light *BackgroundPortalLight::factory(ParamMap &params, Scene &scene)
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

	BackgroundPortalLight *light = new BackgroundPortalLight(object_name, samples, pow, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
