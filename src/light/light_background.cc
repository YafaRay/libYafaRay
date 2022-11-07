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

#include <memory>
#include "background/background.h"
#include "texture/texture.h"
#include "param/param.h"
#include "scene/scene.h"
#include "geometry/surface.h"
#include "sampler/sample_pdf1d.h"
#include "geometry/ray.h"

namespace yafaray {

BackgroundLight::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(samples_);
	PARAM_LOAD(abs_intersect_);
	PARAM_LOAD(ibl_clamp_sampling_);
}

ParamMap BackgroundLight::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(samples_);
	PARAM_SAVE(abs_intersect_);
	PARAM_SAVE(ibl_clamp_sampling_);
	PARAM_SAVE_END;
}

ParamMap BackgroundLight::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Light>, ParamResult> BackgroundLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto light {std::make_unique<ThisClassType_t>(logger, param_result, param_map, scene.getLights())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(light), param_result};
}

BackgroundLight::BackgroundLight(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const SceneItems<Light> &lights):
		ParentClassType_t{logger, param_result, param_map, Flags::None, lights}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void BackgroundLight::init(Scene &scene)
{
	background_ = scene.getBackground();
	const int nv = max_vsamples_;
	std::vector<float> fv(nv);
	const float inv_nv = 1.f / static_cast<float>(nv);
	u_dist_ = std::vector<std::unique_ptr<Pdf1D>>(nv);
	for(int y = 0; y < nv; y++)
	{
		const float fy = (static_cast<float>(y) + 0.5f) * inv_nv;
		const float sintheta = sinSample(fy);
		const int nu = min_samples_ + static_cast<int>(sintheta * (max_usamples_ - min_samples_));
		std::vector<float> fu(nu);
		const float inv_nu = 1.f / static_cast<float>(nu);
		for(int x = 0; x < nu; x++)
		{
			const float fx = (static_cast<float>(x) + 0.5f) * inv_nu;
			const Vec3f wi{Texture::invSphereMap({fx, fy})};
			fu[x] = background_->eval(wi, true).energy() * sintheta;
		}
		u_dist_[y] = std::make_unique<Pdf1D>(std::move(fu));
		fv[y] = u_dist_[y]->integral();
	}
	v_dist_ = std::make_unique<Pdf1D>(std::move(fv));
	const Bound w = scene.getSceneBound();
	world_center_ = 0.5f * (w.a_ + w.g_);
	world_radius_ = 0.5f * (w.g_ - w.a_).length();
	a_pdf_ = world_radius_ * world_radius_;
	world_pi_factor_ = math::mult_pi_by_2<> * a_pdf_;
}

inline std::pair<float, Uv<float>> BackgroundLight::calcFromSample(float s_1, float s_2, bool inv) const
{
	auto [v, pdf_2]{v_dist_->sample(s_2)};
	const int iv = clampSample(addOff(v), v_dist_->size());
	auto [u, pdf_1]{u_dist_[iv]->sample(s_1)};
	u *= u_dist_[iv]->invSize();
	v *= v_dist_->invSize();
	if(inv) return {calcInvPdf(pdf_1, pdf_2, v), {u, v}};
	else return {calcPdf(pdf_1, pdf_2, v), {u, v}};
}

inline std::pair<float, Uv<float>> BackgroundLight::calcFromDir(const Vec3f &dir, bool inv) const
{
	const Uv<float> uv{Texture::sphereMap(static_cast<Point3f>(dir))}; // Returns u,v pair in [0,1] range
	const int iv = clampSample(addOff(uv.v_ * v_dist_->size()), v_dist_->size());
	const int iu = clampSample(addOff(uv.u_ * u_dist_[iv]->size()), u_dist_[iv]->size());
	const float pdf_1 = u_dist_[iv]->function(iu) * u_dist_[iv]->invIntegral();
	const float pdf_2 = v_dist_->function(iv) * v_dist_->invIntegral();
	if(inv) return {calcInvPdf(pdf_1, pdf_2, uv.v_), uv};
	return {calcPdf(pdf_1, pdf_2, uv.v_), uv};
}

std::pair<Vec3f, float> BackgroundLight::sampleDir(float s_1, float s_2, bool inv) const
{
	const auto[pdf, uv]{calcFromSample(s_1, s_2, inv)};
	Vec3f dir{Texture::invSphereMap(uv)};
	return {std::move(dir), pdf};
}

// dir points from surface point to background
float BackgroundLight::dirPdf(const Vec3f &dir) const
{
	return calcFromDir(dir, false).first;
}

float BackgroundLight::calcPdf(float p_0, float p_1, float s)
{
	return std::max(sigma_, p_0 * p_1 * math::div_1_by_2pi<> * clampZero(sinSample(s)));
}

float BackgroundLight::calcInvPdf(float p_0, float p_1, float s)
{
	return std::max(sigma_, math::mult_pi_by_2<> * sinSample(s) * clampZero(p_0 * p_1));
}

std::pair<bool, Ray> BackgroundLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	const auto [pdf, uv]{calcFromSample(s.s_1_, s.s_2_, false)};
	s.pdf_ = pdf;
	Vec3f dir{Texture::invSphereMap(uv)};
	s.col_ = background_->eval(dir, true);
	Ray ray{surface_p, std::move(dir), time};
	return {true, std::move(ray)};
}

std::tuple<bool, float, Rgb> BackgroundLight::intersect(const Ray &ray, float &) const
{
	const auto [ipdf, uv]{calcFromDir(params_.abs_intersect_ ? ray.dir_: -ray.dir_, true)};
	const Point3f ray_dir{Texture::invSphereMap(uv)};
	Rgb col{background_->eval(ray_dir, true)};
	col.clampProportionalRgb(params_.ibl_clamp_sampling_); //trick to reduce light sampling noise at the expense of realism and inexact overall light. 0.f disables clamping
	return {true, ipdf, std::move(col)};
}

Rgb BackgroundLight::totalEnergy() const
{
	return background_->eval({{0.5f, 0.5f, 0.5f}}, true) * world_pi_factor_;
}

std::tuple<Ray, float, Rgb> BackgroundLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	auto[dir, ipdf]{sampleDir(s_3, s_4, true)};
	const Rgb pcol{background_->eval(dir, true)};
	dir = -dir;
	const Uv<Vec3f> coords{Vec3f::createCoordsSystem(dir)};
	const Uv<float> uv{Vec3f::shirleyDisk(s_1, s_2)};
	const Vec3f offs{uv.u_ * coords.u_ + uv.v_ * coords.v_};
	Point3f from{world_center_ + world_radius_ * (offs - dir)};
	Ray ray{std::move(from), std::move(dir), time};
	return {std::move(ray), ipdf, pcol * a_pdf_};
}

std::pair<Vec3f, Rgb> BackgroundLight::emitSample(LSample &s, float time) const
{
	auto[dir, ipdf]{sampleDir(s.s_1_, s.s_2_, true)};
	s.dir_pdf_ = ipdf;
	Rgb pcol{background_->eval(dir, true)};
	dir = -dir;
	const Uv<Vec3f> coords{Vec3f::createCoordsSystem(dir)};
	const Uv<float> uv{Vec3f::shirleyDisk(s.s_1_, s.s_2_)};
	const Vec3f offs{uv.u_ * coords.u_ + uv.v_ * coords.v_};
	s.sp_->p_ = world_center_ + world_radius_ * offs - world_radius_ * dir;
	s.sp_->n_ = dir;
	s.sp_->ng_ = dir;
	s.area_pdf_ = 1.f;
	s.flags_ = flags_;
	return {std::move(dir), std::move(pcol)};
}

float BackgroundLight::illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &) const
{
	const Vec3f dir{(light_p - surface_p).normalize()};
	return dirPdf(dir);
}

std::array<float, 3> BackgroundLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	Vec3f wi{wo};
	wi.normalize();
	const float cos_wo = wi[Axis::Z];
	wi = -wi;
	const float dir_pdf = dirPdf(wi);
	const float area_pdf = 1.f;
	return {area_pdf, dir_pdf, cos_wo};
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
	return math::sin(s * math::num_pi<>);
}

std::tuple<bool, Ray, Rgb> BackgroundLight::illuminate(const Point3f &surface_p, float time) const
{
	return {};
}

} //namespace yafaray

