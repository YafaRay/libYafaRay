/****************************************************************************
 *      light_ies.cc: IES Light
 *      This is part of the libYafaRay package
 *      Copyright (C) 2009  Bert Buchholz and Rodrigo Placencia
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

#include "light/light_ies.h"

#include <memory>
#include "geometry/surface.h"
#include "sampler/sample.h"
#include "light/light_ies_data.h"
#include "geometry/ray.h"
#include "param/param.h"

namespace yafaray {

IesLight::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(from_);
	PARAM_LOAD(to_);
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(file_);
	PARAM_LOAD(samples_);
	PARAM_LOAD(soft_shadows_);
	PARAM_LOAD(cone_angle_);
}

ParamMap IesLight::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(from_);
	PARAM_SAVE(to_);
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(file_);
	PARAM_SAVE(samples_);
	PARAM_SAVE(soft_shadows_);
	PARAM_SAVE(cone_angle_);
	PARAM_SAVE_END;
}

ParamMap IesLight::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Light>, ParamError> IesLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto light {std::make_unique<ThisClassType_t>(logger, param_error, name, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	if(!light->isIesOk()) return {nullptr, ParamError{ResultFlags::ErrorWhileCreating}};
	return {std::move(light), param_error};
}

IesLight::IesLight(Logger &logger, ParamError &param_error, const std::string &name, const ParamMap &param_map):
		ParentClassType_t{logger, param_error, name, param_map, Flags::Singular}, params_{param_error, param_map},
		ies_data_{std::make_unique<IesData>()},
		ies_ok_{ies_data_->parseIesFile(logger, params_.file_)}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	if(ies_ok_)
	{
		ndir_ = (params_.from_ - params_.to_);
		ndir_.normalize();
		dir_ = -ndir_;

		duv_ = Vec3f::createCoordsSystem(dir_);
		cos_end_ = math::cos(ies_data_->getMaxVAngle());

		color_ = params_.color_ * params_.power_;
		tot_energy_ = math::mult_pi_by_2<> * (1.f - 0.5f * cos_end_);
	}
}

Uv<float> IesLight::getAngles(const Vec3f &dir, float costheta)
{
	float u = (dir[Axis::Z] >= 1.f) ? 0.f : math::radToDeg(math::acos(dir[Axis::Z]));
	if(dir[Axis::Y] < 0) u = 360.f - u;
	const float v = (costheta >= 1.f) ? 0.f : math::radToDeg(math::acos(costheta));
	return {u, v};
}

std::tuple<bool, Ray, Rgb> IesLight::illuminate(const Point3f &surface_p, float time) const
{
	if(photonOnly()) return {};
	Vec3f ldir{params_.from_ - surface_p};
	const float dist_sqrt = ldir.lengthSquared();
	const float dist = math::sqrt(dist_sqrt);
	const float i_dist_sqrt = 1.f / dist_sqrt;
	if(dist == 0.f) return {};
	ldir *= 1.f / dist; //normalize
	const float cos_a = ndir_ * ldir;
	if(cos_a < cos_end_) return {};
	const Uv<float> uv{getAngles(ldir, cos_a)};
	Rgb col{color_ * ies_data_->getRadiance(uv.u_, uv.v_) * i_dist_sqrt};
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray), std::move(col)};
}

std::pair<bool, Ray> IesLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	Vec3f ldir{params_.from_ - surface_p};
	const float dist_sqrt = ldir.lengthSquared();
	const float dist = math::sqrt(dist_sqrt);
	const float i_dist_sqrt = 1.f / dist_sqrt;
	if(dist == 0.f) return {};
	ldir *= 1.f / dist; //normalize
	const float cos_a = ndir_ * ldir;
	if(cos_a < cos_end_) return {};
	Vec3f dir{sample::cone(ldir, duv_, cos_a, s.s_1_, s.s_2_)};
	const Uv<float> uv{getAngles(dir, cos_a)};
	const float rad = ies_data_->getRadiance(uv.u_, uv.v_);
	if(rad == 0.f) return {};
	s.col_ = color_ * i_dist_sqrt;
	s.pdf_ = 1.f / rad;
	Ray ray{surface_p, std::move(dir), time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> IesLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	Vec3f dir{sample::cone(dir_, duv_, cos_end_, s_1, s_2)};
	float cos_a = dir * dir_;
	if(cos_a < cos_end_) return {};
	const Uv<float> uv{getAngles(dir, cos_a)};
	const float rad = ies_data_->getRadiance(uv.u_, uv.v_);
	Ray ray{params_.from_, std::move(dir), time};
	return {std::move(ray), rad, color_};
}

std::pair<Vec3f, Rgb> IesLight::emitSample(LSample &s, float time) const
{
	s.sp_->p_ = params_.from_;
	s.flags_ = flags_;
	Vec3f dir{sample::cone(dir_, duv_, cos_end_, s.s_3_, s.s_4_)};
	const Uv<float> uv{getAngles(dir, dir * dir_)};
	const float rad = ies_data_->getRadiance(uv.u_, uv.v_);
	s.dir_pdf_ = (rad > 0.f) ? (tot_energy_ / rad) : 0.f;
	s.area_pdf_ = 1.f;
	return {std::move(dir), color_ * rad * tot_energy_};
}

std::array<float, 3> IesLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	const float cos_wo = 1.f;
	const float area_pdf = 1.f;
	float dir_pdf = 0.f;
	const float cos_a = dir_ * wo;
	if(cos_a >= cos_end_)
	{
		const Uv<float> uv{getAngles(wo, cos_a)};
		float rad = ies_data_->getRadiance(uv.u_, uv.v_);
		dir_pdf = (rad > 0.f) ? (tot_energy_ / rad) : 0.f;
	}
	return {area_pdf, dir_pdf, cos_wo};
}

} //namespace yafaray
