/****************************************************************************
 *      light_spot.cc: a spot light with soft edge
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

#include "light/light_spot.h"

#include <memory>
#include "geometry/surface.h"
#include "geometry/ray.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

SpotLight::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(from_);
	PARAM_LOAD(to_);
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(cone_angle_);
	PARAM_LOAD(falloff_);
	PARAM_LOAD(soft_shadows_);
	PARAM_LOAD(shadow_fuzzyness_);
	PARAM_LOAD(samples_);
}

ParamMap SpotLight::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(from_);
	PARAM_SAVE(to_);
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(cone_angle_);
	PARAM_SAVE(falloff_);
	PARAM_SAVE(soft_shadows_);
	PARAM_SAVE(shadow_fuzzyness_);
	PARAM_SAVE(samples_);
	PARAM_SAVE_END;
}

ParamMap SpotLight::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Light>, ParamResult> SpotLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto light {std::make_unique<ThisClassType_t>(logger, param_result, param_map, scene.getLights())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(light), param_result};
}

SpotLight::SpotLight(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const SceneItems<Light> &lights):
		ParentClassType_t{logger, param_result, param_map, Flags::Singular, lights}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	const float rad_angle{math::degToRad(params_.cone_angle_)};
	const float rad_inner_angle{rad_angle * (1.f - params_.falloff_)};
	cos_start_ = math::cos(rad_inner_angle);
	cos_end_ = math::cos(rad_angle);
	icos_diff_ = 1.f / (cos_start_ - cos_end_);

	std::vector<float> f(65);
	for(int i = 0; i < 65; i++)
	{
		const float v = static_cast<float>(i) / 64.f;
		f[i] = v * v * (3.f - 2.f * v);
	}
	pdf_ = std::make_unique<Pdf1D>(f);

	/* the integral of the smoothstep is 0.5, and since it gets applied to the cos, and each delta cos
		corresponds to a constant surface are of the (partial) emitting sphere, we can actually simply
		compute the energie emitted from both areas, the constant and blending one...
		1  cosStart  cosEnd              -1
		|------|--------|-----------------|
	*/

	interv_1_ = 1.f - cos_start_;
	interv_2_ = 0.5f * (cos_start_ - cos_end_); // as said, energy linear to delta cos, integral is 0.5;
	float sum = std::abs(interv_1_) + std::abs(interv_2_);
	if(sum > 0.f) sum = 1.f / sum;
	interv_1_ *= sum;
	interv_2_ *= sum;
}

Rgb SpotLight::totalEnergy() const
{
	return color_ * math::mult_pi_by_2<> * (1.f - 0.5f * (cos_start_ + cos_end_));
}

std::tuple<bool, Ray, Rgb> SpotLight::illuminate(const Point3f &surface_p, float time) const
{
	if(photonOnly()) return {};
	Vec3f ldir{params_.from_ - surface_p};
	const float dist_sqr = ldir * ldir;
	const float dist = math::sqrt(dist_sqr);
	if(dist == 0.f) return {};
	const float idist_sqr = 1.f / dist_sqr;
	ldir *= 1.f / dist; //normalize
	const float cos_a = ndir_ * ldir;
	if(cos_a < cos_end_) return {}; //outside cone
	float v = 1.f;
	if(cos_a < cos_start_) //affected by falloff
	{
		v = (cos_a - cos_end_) * icos_diff_;
		v = v * v * (3.f - 2.f * v);
	}
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray), color_ * v * idist_sqr};
}

std::pair<bool, Ray> SpotLight::illumSample(const Point3f &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	Vec3f ldir{params_.from_ - surface_p};
	const float dist_sqr = ldir * ldir;
	if(dist_sqr == 0.f) return {};
	const float dist = math::sqrt(dist_sqr);
	ldir *= 1.f / dist; //normalize
	const float cos_a = ndir_ * ldir;
	if(cos_a < cos_end_) return {}; //outside cone
	Vec3f dir{sample::cone(ldir, duv_, cos_end_, s.s_1_ * params_.shadow_fuzzyness_, s.s_2_ * params_.shadow_fuzzyness_)};
	if(cos_a >= cos_start_) // not affected by falloff
	{
		s.col_ = color_;
	}
	else
	{
		float v = (cos_a - cos_end_) * icos_diff_;
		v = v * v * (3.f - 2.f * v);
		s.col_ = color_ * v;
	}
	s.flags_ = flags_;
	s.pdf_ = dist_sqr;

	//FIXME: I don't quite understand how pdf is calculated in the spotlight, but something looks wrong when dist<1.f, so I'm applying a manual correction to keep s.pdf >= 1 always, and "move" the effect of the distance to the color itself. This is a horrible patch but at least solves a problem causing darker light when distance between spotlight and surface is less than 1.f
	if(s.pdf_ < 1.f)
	{
		s.pdf_ = 1.f;
		s.col_ = s.col_ / dist_sqr;
	}

	Ray ray{surface_p, std::move(dir), time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> SpotLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	if(s_3 <= interv_1_) // sample from cone not affected by falloff:
	{
		Vec3f dir{sample::cone(dir_, duv_, cos_start_, s_1, s_2)};
		const float ipdf = math::mult_pi_by_2<> * (1.f - cos_start_) / interv_1_;
		Ray ray{params_.from_, std::move(dir), time};
		return {std::move(ray), ipdf, color_};
	}
	else // sample in the falloff area
	{
		auto[sm_2, spdf]{pdf_->sample(s_2)};
		sm_2 *= pdf_->invSize();
		const float ipdf = math::mult_pi_by_2<> * (cos_start_ - cos_end_) / (interv_2_ * spdf);
		const double cos_ang = cos_end_ + (cos_start_ - cos_end_) * (double)sm_2;
		const double sin_ang = math::sqrt(1.0 - cos_ang * cos_ang);
		const float t_1 = math::mult_pi_by_2<> * s_1;
		Vec3f dir{(duv_.u_ * math::cos(t_1) + duv_.v_ * math::sin(t_1)) * (float)sin_ang + dir_ * (float)cos_ang};
		Rgb col{color_ * spdf * pdf_->integral()}; // scale is just the actual falloff function, since spdf is func * invIntegral...
		Ray ray{params_.from_, std::move(dir), time};
		return {std::move(ray), ipdf, std::move(col)};
	}
}

std::pair<Vec3f, Rgb> SpotLight::emitSample(LSample &s, float time) const
{
	s.sp_->p_ = params_.from_;
	s.area_pdf_ = 1.f;
	s.flags_ = flags_;
	if(s.s_3_ <= interv_1_) // sample from cone not affected by falloff:
	{
		Vec3f dir{sample::cone(dir_, duv_, cos_start_, s.s_1_, s.s_2_)};
		s.dir_pdf_ = interv_1_ / (math::mult_pi_by_2<> * (1.f - cos_start_));
		return {std::move(dir), color_};
	}
	else // sample in the falloff area
	{
		auto[sm_2, spdf]{pdf_->sample(s.s_2_)};
		sm_2 *= pdf_->invSize();
		s.dir_pdf_ = (interv_2_ * spdf) / (math::mult_pi_by_2<> * (cos_start_ - cos_end_));
		const double cos_ang = cos_end_ + (cos_start_ - cos_end_) * (double)sm_2;
		const double sin_ang = math::sqrt(1.0 - cos_ang * cos_ang);
		const float t_1 = math::mult_pi_by_2<> * s.s_1_;
		Vec3f dir{(duv_.u_ * math::cos(t_1) + duv_.v_ * math::sin(t_1)) * (float)sin_ang + dir_ * (float)cos_ang};
		const float v = sm_2 * sm_2 * (3.f - 2.f * sm_2);
		return {std::move(dir), color_ * v};
	}
}

std::array<float, 3> SpotLight::emitPdf(const Vec3f &surface_n, const Vec3f &wo) const
{
	float area_pdf = 1.f;
	const float cos_wo = 1.f;
	const float cosa = dir_ * wo;
	float dir_pdf;
	if(cosa < cos_end_) dir_pdf = 0.f;
	else if(cosa >= cos_start_) // not affected by falloff
	{
		dir_pdf = interv_1_ / (math::mult_pi_by_2<> * (1.f - cos_start_));
	}
	else
	{
		float v = (cosa - cos_end_) * icos_diff_;
		v = v * v * (3.f - 2.f * v);
		dir_pdf = interv_2_ * v * 2.f / (math::mult_pi_by_2<> * (cos_start_ - cos_end_));   //divide by integral of v (0.5)?
	}
	return {area_pdf, dir_pdf, cos_wo};
}
std::tuple<bool, float, Rgb> SpotLight::intersect(const Ray &ray, float &t) const
{
	const float cos_a = dir_ * ray.dir_;
	if(cos_a == 0.f) return {};
	t = (dir_ * (params_.from_ - ray.from_)) / cos_a;
	if(t < 0.f) return {};
	const Point3f p{ray.from_ + t * ray.dir_};
	if(dir_ * (p - params_.from_) == 0.f)
	{
		if(p * p <= 1e-2f)
		{
			const float cosa = dir_ * ray.dir_;
			if(cosa < cos_end_) return {}; //outside cone
			Rgb col{color_};
			if(cosa < cos_start_) //affected by falloff
			{
				float v = (cosa - cos_end_) * icos_diff_;
				v = v * v * (3.f - 2.f * v);
				col *= v;
			}
			const float ipdf = 1.f / (t * t);
			if(logger_.isVerbose()) logger_.logVerbose("SpotLight: ipdf, color = ", ipdf, ", ", color_);
			return {true, ipdf, col};
		}
	}
	return {};
}

} //namespace yafaray
