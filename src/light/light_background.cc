/****************************************************************************
 *      bglight.cc: a light source using the background
 *      This is part of the libYafaRay package
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

#include "light/light_background.h"
#include "background/background.h"
#include "texture/texture.h"
#include "common/param.h"
#include "scene/scene.h"
#include "geometry/surface.h"
#include "sampler/sample_pdf1d.h"

BEGIN_YAFARAY

constexpr int BackgroundLight::max_vsamples_;
constexpr int BackgroundLight::max_usamples_;
constexpr int BackgroundLight::min_samples_;

constexpr float BackgroundLight::smpl_off_;
constexpr float BackgroundLight::sigma_;

#define MULT_PDF(p0, p1) (p0 * p1)
#define CALC_PDF(p0, p1, s) std::max( sigma_, MULT_PDF(p0, p1) * (float)math::div_1_by_2pi * clampZero(sinSample(s)) )
#define CALC_INV_PDF(p0, p1, s) std::max( sigma_, (float)math::mult_pi_by_2 * sinSample(s) * clampZero(MULT_PDF(p0, p1)) )

BackgroundLight::BackgroundLight(Logger &logger, int sampl, bool invert_intersect, bool light_enabled, bool cast_shadows):
		Light(logger, Light::Flags::None), samples_(sampl), abs_inter_(invert_intersect)
{
	light_enabled_ = light_enabled;
	cast_shadows_ = cast_shadows;
	background_ = nullptr;
}

void BackgroundLight::init(Scene &scene)
{
	auto fu = std::unique_ptr<float[]>(new float[max_usamples_]);
	auto fv = std::unique_ptr<float[]>(new float[max_vsamples_]);
	const int nv = max_vsamples_;

	Ray ray;
	ray.from_ = Point3(0.f);
	const float inv = 1.f / (float)nv;
	u_dist_ = std::unique_ptr<std::unique_ptr<Pdf1D>[]>(new std::unique_ptr<Pdf1D>[nv]);
	for(int y = 0; y < nv; y++)
	{
		const float fy = ((float)y + 0.5f) * inv;
		const float sintheta = sinSample(fy);
		const int nu = min_samples_ + (int)(sintheta * (max_usamples_ - min_samples_));
		const float inu = 1.f / (float)nu;

		for(int x = 0; x < nu; x++)
		{
			const float fx = ((float)x + 0.5f) * inu;
			Texture::invSphereMap(fx, fy, ray.dir_);
			fu[x] = background_->eval(ray.dir_, true).energy() * sintheta;
		}

		u_dist_[y] = std::unique_ptr<Pdf1D>(new Pdf1D(fu.get(), nu));
		fv[y] = u_dist_[y]->integral_;
	}

	v_dist_ = std::unique_ptr<Pdf1D>(new Pdf1D(fv.get(), nv));

	Bound w = scene.getSceneBound();
	world_center_ = 0.5 * (w.a_ + w.g_);
	world_radius_ = 0.5 * (w.g_ - w.a_).length();
	a_pdf_ = world_radius_ * world_radius_;
	world_pi_factor_ = (math::mult_pi_by_2 * a_pdf_);
}

inline float BackgroundLight::calcFromSample(float s_1, float s_2, float &u, float &v, bool inv) const
{
	int iv;
	float pdf_1 = 0.f, pdf_2 = 0.f;
	v = v_dist_->sample(logger_, s_2, &pdf_2);
	iv = clampSample(addOff(v), v_dist_->count_);
	u = u_dist_[iv]->sample(logger_, s_1, &pdf_1);
	u *= u_dist_[iv]->inv_count_;
	v *= v_dist_->inv_count_;
	if(inv)return CALC_INV_PDF(pdf_1, pdf_2, v);
	return CALC_PDF(pdf_1, pdf_2, v);
}

inline float BackgroundLight::calcFromDir(const Vec3 &dir, float &u, float &v, bool inv) const
{
	float pdf_1 = 0.f, pdf_2 = 0.f;
	Texture::sphereMap(dir, u, v); // Returns u,v pair in [0,1] range
	const int iv = clampSample(addOff(v * v_dist_->count_), v_dist_->count_);
	const int iu = clampSample(addOff(u * u_dist_[iv]->count_), u_dist_[iv]->count_);
	pdf_1 = u_dist_[iv]->func_[iu] * u_dist_[iv]->inv_integral_;
	pdf_2 = v_dist_->func_[iv] * v_dist_->inv_integral_;
	if(inv)return CALC_INV_PDF(pdf_1, pdf_2, v);
	return CALC_PDF(pdf_1, pdf_2, v);
}

void BackgroundLight::sampleDir(float s_1, float s_2, Vec3 &dir, float &pdf, bool inv) const
{
	float u = 0.f, v = 0.f;
	pdf = calcFromSample(s_1, s_2, u, v, inv);
	Texture::invSphereMap(u, v, dir);
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
	Texture::invSphereMap(u, v, wi.dir_);
	s.col_ = background_->eval(wi.dir_, true);
	return true;
}

bool BackgroundLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	Ray tr {ray, Ray::DifferentialsCopy::No};
	Vec3 abs_dir = tr.dir_;
	if(abs_inter_) abs_dir = -abs_dir;
	float u = 0.f, v = 0.f;
	ipdf = calcFromDir(abs_dir, u, v, true);
	Texture::invSphereMap(u, v, tr.dir_);
	col = background_->eval(tr.dir_, true);
	col.clampProportionalRgb(clamp_intersect_); //trick to reduce light sampling noise at the expense of realism and inexact overall light. 0.f disables clamping
	return true;
}

Rgb BackgroundLight::totalEnergy() const
{
	const Rgb energy = background_->eval({0.5, 0.5, 0.5}, true) * world_pi_factor_;
	return energy;
}

Rgb BackgroundLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	sampleDir(s_3, s_4, ray.dir_, ipdf, true);
	const Rgb pcol = background_->eval(ray.dir_, true);
	ray.dir_ = -ray.dir_;

	Vec3 u_vec, v_vec;
	Vec3::createCs(ray.dir_, u_vec, v_vec);
	float u, v;
	Vec3::shirleyDisk(s_1, s_2, u, v);
	const Vec3 offs = u * u_vec + v * v_vec;
	ray.from_ = world_center_ + world_radius_ * (offs - ray.dir_);
	return pcol * a_pdf_;
}

Rgb BackgroundLight::emitSample(Vec3 &wo, LSample &s) const
{
	sampleDir(s.s_1_, s.s_2_, wo, s.dir_pdf_, true);

	const Rgb pcol = background_->eval(wo, true);
	wo = -wo;

	Vec3 u_vec, v_vec;
	Vec3::createCs(wo, u_vec, v_vec);
	float u, v;
	Vec3::shirleyDisk(s.s_1_, s.s_2_, u, v);

	const Vec3 offs = u * u_vec + v * v_vec;

	s.sp_->p_ = world_center_ + world_radius_ * offs - world_radius_ * wo;
	s.sp_->n_ = s.sp_->ng_ = wo;
	s.area_pdf_ = 1.f;
	s.flags_ = flags_;

	return pcol;
}

float BackgroundLight::illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const
{
	const Vec3 dir = (sp_light.p_ - sp.p_).normalize();
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

std::unique_ptr<Light> BackgroundLight::factory(Logger &logger, ParamMap &params, const Scene &scene)
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

	auto light = std::unique_ptr<BackgroundLight>(new BackgroundLight(logger, samples, abs_int, light_enabled, cast_shadows));

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

constexpr float BackgroundLight::addOff(float v)
{
	return v + smpl_off_;
}

int BackgroundLight::clampSample(int s, int m)
{
	return std::max(0, std::min(s, m - 1));
}

float BackgroundLight::clampZero(float val)
{
	if(val > 0.f) return 1.f / val;
	else return 0.f;
}

float BackgroundLight::sinSample(float s)
{
	return math::sin(s * math::num_pi);
}

END_YAFARAY

