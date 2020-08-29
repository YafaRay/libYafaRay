/****************************************************************************
 *		bglight.cc: a light source using the background
 *      This is part of the yafaray package
 *      Copyright (C) 2006 Mathias Wein (Lynx)
 *      Copyright (C) 2009 Rodrigo Placencia (DarkTide)
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

#include <lights/bglight.h>
#include <core_api/background.h>
#include <core_api/texture.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <core_api/surface.h>
#include <utilities/sample_utils.h>

BEGIN_YAFRAY

#define MAX_VSAMPLES 360
#define MAX_USAMPLES 720
#define MIN_SAMPLES 16

#define SMPL_OFF 0.4999f

#define SIGMA 0.000001f

#define ADD_OFF(v) (v + SMPL_OFF)
#define CLAMP_SAMPLE(s, m) std::max(0, std::min((int)(s), m - 1))

#define MULT_PDF(p0, p1) (p0 * p1)
#define CALC_PDF(p0, p1, s) std::max( SIGMA, MULT_PDF(p0, p1) * (float)M_1_2PI * clampZero__(sinSample__(s)) )
#define CALC_INV_PDF(p0, p1, s) std::max( SIGMA, (float)M_2PI * sinSample__(s) * clampZero__(MULT_PDF(p0, p1)) )

inline float clampZero__(float val)
{
	if(val > 0.f) return 1.f / val;
	else return 0.f;
}

inline float sinSample__(float s)
{
	return fSin__(s * M_PI);
}

BackgroundLight::BackgroundLight(int sampl, bool invert_intersect, bool light_enabled, bool cast_shadows):
		Light(LightNone), samples_(sampl), abs_inter_(invert_intersect)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;
	background_ = nullptr;
	u_dist_ = nullptr;
	v_dist_ = nullptr;
}

BackgroundLight::~BackgroundLight()
{
	for(int i = 0; i < v_dist_->count_; i++) delete u_dist_[i];
	if(u_dist_) delete[] u_dist_;
	u_dist_ = nullptr;
	if(v_dist_) delete v_dist_;
	v_dist_ = nullptr;
}

void BackgroundLight::init(Scene &scene)
{
	float *fu = new float[MAX_USAMPLES];
	float *fv = new float[MAX_VSAMPLES];
	float inu = 0, inv = 0;
	float fx = 0.f, fy = 0.f;
	float sintheta = 0.f;
	int nu = 0, nv = MAX_VSAMPLES;

	Ray ray;
	ray.from_ = Point3(0.f);
	inv = 1.f / (float)nv;
	u_dist_ = new Pdf1D *[nv];
	for(int y = 0; y < nv; y++)
	{
		fy = ((float)y + 0.5f) * inv;
		sintheta = sinSample__(fy);
		nu = MIN_SAMPLES + (int)(sintheta * (MAX_USAMPLES - MIN_SAMPLES));
		inu = 1.f / (float)nu;

		for(int x = 0; x < nu; x++)
		{
			fx = ((float)x + 0.5f) * inu;
			invSpheremap__(fx, fy, ray.dir_);
			fu[x] = background_->eval(ray, true).energy() * sintheta;
		}

		u_dist_[y] = new Pdf1D(fu, nu);
		fv[y] = u_dist_[y]->integral_;
	}

	v_dist_ = new Pdf1D(fv, nv);

	delete[] fv;
	delete[] fu;

	Bound w = scene.getSceneBound();
	world_center_ = 0.5 * (w.a_ + w.g_);
	world_radius_ = 0.5 * (w.g_ - w.a_).length();
	a_pdf_ = world_radius_ * world_radius_;
	ia_pdf_ = 1.f / a_pdf_;
	world_pi_factor_ = (M_2PI * a_pdf_);
}

inline float BackgroundLight::calcFromSample(float s_1, float s_2, float &u, float &v, bool inv) const
{
	int iv;
	float pdf_1 = 0.f, pdf_2 = 0.f;
	v = v_dist_->sample(s_2, &pdf_2);
	iv = CLAMP_SAMPLE(ADD_OFF(v), v_dist_->count_);
	u = u_dist_[iv]->sample(s_1, &pdf_1);
	u *= u_dist_[iv]->inv_count_;
	v *= v_dist_->inv_count_;
	if(inv)return CALC_INV_PDF(pdf_1, pdf_2, v);
	return CALC_PDF(pdf_1, pdf_2, v);
}

inline float BackgroundLight::calcFromDir(const Vec3 &dir, float &u, float &v, bool inv) const
{
	int iv, iu;
	float pdf_1 = 0.f, pdf_2 = 0.f;
	spheremap__(dir, u, v); // Returns u,v pair in [0,1] range
	iv = CLAMP_SAMPLE(ADD_OFF(v * v_dist_->count_), v_dist_->count_);
	iu = CLAMP_SAMPLE(ADD_OFF(u * u_dist_[iv]->count_), u_dist_[iv]->count_);
	pdf_1 = u_dist_[iv]->func_[iu] * u_dist_[iv]->inv_integral_;
	pdf_2 = v_dist_->func_[iv] * v_dist_->inv_integral_;
	if(inv)return CALC_INV_PDF(pdf_1, pdf_2, v);
	return CALC_PDF(pdf_1, pdf_2, v);
}

void BackgroundLight::sampleDir(float s_1, float s_2, Vec3 &dir, float &pdf, bool inv) const
{
	float u = 0.f, v = 0.f;
	pdf = calcFromSample(s_1, s_2, u, v, inv);
	invSpheremap__(u, v, dir);
}

// dir points from surface point to background
float BackgroundLight::dirPdf(const Vec3 dir) const
{
	float u = 0.f, v = 0.f;
	return calcFromDir(dir, u, v);
}

bool BackgroundLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;
	float u = 0.f, v = 0.f;
	wi.tmax_ = -1.0;
	s.pdf_ = calcFromSample(s.s_1_, s.s_2_, u, v, false);
	invSpheremap__(u, v, wi.dir_);
	s.col_ = background_->eval(wi, true);
	return true;
}

bool BackgroundLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	float u = 0.f, v = 0.f;
	Ray tr = ray;
	Vec3 abs_dir = tr.dir_;
	if(abs_inter_) abs_dir = -abs_dir;
	ipdf = calcFromDir(abs_dir, u, v, true);
	invSpheremap__(u, v, tr.dir_);
	col = background_->eval(tr, true);
	col.clampProportionalRgb(clamp_intersect_); //trick to reduce light sampling noise at the expense of realism and inexact overall light. 0.f disables clamping
	return true;
}

Rgb BackgroundLight::totalEnergy() const
{
	Rgb energy = background_->eval(Ray(Point3(0, 0, 0), Vec3(0.5, 0.5, 0.5)), true) * world_pi_factor_;
	return energy;
}

Rgb BackgroundLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	Rgb pcol;
	Vec3 u_vec, v_vec;
	Vec3 offs;
	float u, v;

	sampleDir(s_3, s_4, ray.dir_, ipdf, true);
	pcol = background_->eval(ray, true);
	ray.dir_ = -ray.dir_;

	createCs__(ray.dir_, u_vec, v_vec);
	shirleyDisk__(s_1, s_2, u, v);
	offs = u * u_vec + v * v_vec;
	ray.from_ = world_center_ + world_radius_ * (offs - ray.dir_);
	return pcol * a_pdf_;
}

Rgb BackgroundLight::emitSample(Vec3 &wo, LSample &s) const
{
	Rgb pcol;
	Vec3 u_vec, v_vec;
	Vec3 offs;
	float u, v;
	sampleDir(s.s_1_, s.s_2_, wo, s.dir_pdf_, true);

	pcol = background_->eval(Ray(Point3(0, 0, 0), wo), true);
	wo = -wo;

	createCs__(wo, u_vec, v_vec);
	shirleyDisk__(s.s_1_, s.s_2_, u, v);

	offs = u * u_vec + v * v_vec;

	s.sp_->p_ = world_center_ + world_radius_ * offs - world_radius_ * wo;
	s.sp_->n_ = s.sp_->ng_ = wo;
	s.area_pdf_ = 1.f;
	s.flags_ = flags_;

	return pcol;
}

float BackgroundLight::illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const
{
	Vec3 dir = (sp_light.p_ - sp.p_).normalize();
	return dirPdf(dir);
}

void BackgroundLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	Vec3 wi = wo;
	wi.normalize();
	cos_wo = wi.z_;
	wi = -wi;
	dir_pdf = dirPdf(wi);
	area_pdf = 1.f;
}

Light *BackgroundLight::factory(ParamMap &params, RenderEnvironment &render)
{
	int samples = 16;
	bool shoot_d = true;
	bool shoot_c = true;
	bool abs_int = false;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool p_only = false;

	params.getParam("samples", samples);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("abs_intersect", abs_int);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("photon_only", p_only);

	BackgroundLight *light = new BackgroundLight(samples, abs_int, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("bglight", BackgroundLight::factory);
	}
}

END_YAFRAY

