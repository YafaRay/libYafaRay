/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      darksky.cc: SkyLight, "Real" Sunlight and Sky Background
 *      Created on: 20/03/2009
 *
 *      Based on the original implementation by Alejandro Conty (jandro), Mathias Wein (Lynx), anyone else???
 *      Actual implementation by Rodrigo Placencia (Darktide)
 *
 *      Based on 'A Practical Analytic Model For DayLight" by Preetham, Shirley & Smits.
 *      http://www.cs.utah.edu/vissim/papers/sunsky/
 *      based on the actual code by Brian Smits
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

#include "background/background_darksky.h"
#include "common/logger.h"
#include "param/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "color/spectral_data.h"
#include "math/interpolation_curve.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> DarkSkyBackground::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(from_);
	PARAM_META(turb_);
	PARAM_META(altitude_);
	PARAM_META(bright_);
	PARAM_META(exposure_);
	PARAM_META(night_);
	PARAM_META(color_space_);
	PARAM_META(add_sun_);
	PARAM_META(sun_power_);
	PARAM_META(background_light_);
	PARAM_META(light_samples_);
	PARAM_META(cast_shadows_sun_);
	PARAM_META(a_var_);
	PARAM_META(b_var_);
	PARAM_META(c_var_);
	PARAM_META(d_var_);
	PARAM_META(e_var_);
	return param_meta_map;
}

DarkSkyBackground::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(from_);
	PARAM_LOAD(turb_);
	PARAM_LOAD(altitude_);
	PARAM_LOAD(bright_);
	PARAM_LOAD(exposure_);
	PARAM_LOAD(night_);
	PARAM_ENUM_LOAD(color_space_);
	PARAM_LOAD(add_sun_);
	PARAM_LOAD(sun_power_);
	PARAM_LOAD(background_light_);
	PARAM_LOAD(light_samples_);
	PARAM_LOAD(cast_shadows_sun_);
	PARAM_LOAD(a_var_);
	PARAM_LOAD(b_var_);
	PARAM_LOAD(c_var_);
	PARAM_LOAD(d_var_);
	PARAM_LOAD(e_var_);
}

ParamMap DarkSkyBackground::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(from_);
	PARAM_SAVE(turb_);
	PARAM_SAVE(altitude_);
	PARAM_SAVE(bright_);
	PARAM_SAVE(exposure_);
	PARAM_SAVE(night_);
	PARAM_ENUM_SAVE(color_space_);
	PARAM_SAVE(add_sun_);
	PARAM_SAVE(sun_power_);
	PARAM_SAVE(background_light_);
	PARAM_SAVE(light_samples_);
	PARAM_SAVE(cast_shadows_sun_);
	PARAM_SAVE(a_var_);
	PARAM_SAVE(b_var_);
	PARAM_SAVE(c_var_);
	PARAM_SAVE(d_var_);
	PARAM_SAVE(e_var_);
	return param_map;
}

std::pair<std::unique_ptr<Background>, ParamResult> DarkSkyBackground::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto background{std::make_unique<DarkSkyBackground>(logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(background), param_result};
}

DarkSkyBackground::DarkSkyBackground(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	sun_dir_[Axis::Z] += params_.altitude_;
	sun_dir_.normalize();
	theta_s_ = math::acos(sun_dir_[Axis::Z]);
	if(logger_.isVerbose())
	{
		logger_.logVerbose(getClassName(), ": Night mode [ ", (params_.night_ ? "ON" : "OFF"), " ]");
		logger_.logVerbose(getClassName(), ": Solar Declination in Degrees (", math::radToDeg(theta_s_), ")");
		logger_.logVerbose(getClassName(), ": RGB Clamping active.");
		logger_.logVerbose(getClassName(), ": Altitude ", params_.altitude_);
	}

	cos_theta_s_ = math::cos(theta_s_);
	cos_theta_2_ = cos_theta_s_ * cos_theta_s_;

	theta_2_ = theta_s_ * theta_s_;
	theta_3_ = theta_2_ * theta_s_;

	t_ = params_.turb_;
	t_2_ = params_.turb_ * params_.turb_;

	const double chi = (0.44444444 - (t_ / 120.0)) * (math::num_pi<double> - (2.0 * theta_s_));

	zenith_Y_ = (4.0453 * t_ - 4.9710) * std::tan(chi) - 0.2155 * t_ + 2.4192;
	zenith_Y_ *= 1000;  // conversion from kcd/m^2 to cd/m^2

	zenith_x_ =
			(0.00165 * theta_3_ - 0.00374 * theta_2_ + 0.00209 * theta_s_) * t_2_ +
			(-0.02902 * theta_3_ + 0.06377 * theta_2_ - 0.03202 * theta_s_ + 0.00394) * t_ +
			(0.11693 * theta_3_ - 0.21196 * theta_2_ + 0.06052 * theta_s_ + 0.25885);

	zenith_y_ =
			(0.00275 * theta_3_ - 0.00610 * theta_2_ + 0.00316 * theta_s_) * t_2_ +
			(-0.04214 * theta_3_ + 0.08970 * theta_2_ - 0.04153 * theta_s_ + 0.00515) * t_ +
			(0.15346 * theta_3_ - 0.26756 * theta_2_ + 0.06669 * theta_s_ + 0.26688);

	perez_Y_[0] = ((0.17872 * t_) - 1.46303) * params_.a_var_;
	perez_Y_[1] = ((-0.35540 * t_) + 0.42749) * params_.b_var_;
	perez_Y_[2] = ((-0.02266 * t_) + 5.32505) * params_.c_var_;
	perez_Y_[3] = ((0.12064 * t_) - 2.57705) * params_.d_var_;
	perez_Y_[4] = ((-0.06696 * t_) + 0.37027) * params_.e_var_;
	perez_Y_[5] = prePerez(perez_Y_);

	perez_x_[0] = ((-0.01925 * t_) - 0.25922);
	perez_x_[1] = ((-0.06651 * t_) + 0.00081);
	perez_x_[2] = ((-0.00041 * t_) + 0.21247);
	perez_x_[3] = ((-0.06409 * t_) - 0.89887);
	perez_x_[4] = ((-0.00325 * t_) + 0.04517);
	perez_x_[5] = prePerez(perez_x_);

	perez_y_[0] = ((-0.01669 * t_) - 0.26078);
	perez_y_[1] = ((-0.09495 * t_) + 0.00921);
	perez_y_[2] = ((-0.00792 * t_) + 0.21023);
	perez_y_[3] = ((-0.04405 * t_) - 1.65369);
	perez_y_[4] = ((-0.01092 * t_) + 0.05291);
	perez_y_[5] = prePerez(perez_y_);
}

Rgb DarkSkyBackground::getAttenuatedSunColor() const
{
	Rgb light_color(1.0);
	light_color = getSunColorFromSunRad();
	if(params_.night_) light_color *= Rgb(0.8, 0.8, 1.0);
	return light_color;
}

Rgb DarkSkyBackground::getSunColorFromSunRad() const
{
	const double b = (0.04608365822050 * t_) - 0.04586025928522;
	const double a = 1.3;
	const double l = 0.35;
	const double w = 2.0;

	const IrregularCurve ko(spectral_data::ko_wavelength_amplitudes);
	const IrregularCurve kg(spectral_data::kg_wavelength_amplitudes);
	const IrregularCurve kwa(spectral_data::kwa_wavelength_amplitudes);
	const RegularCurve sun_radiance_curve(spectral_data::sun_radiance, 380, 750);

	const double m = 1.0 / (cos_theta_s_ + 0.15 * std::pow(93.885f - math::radToDeg(theta_s_), -1.253f));
	const double mw = m * w;
	const double lm = -m * l;

	const double m_1 = -0.008735;
	const double m_b = -b;
	const double am = -a * m;
	const double m_4 = -4.08 * m;

	Rgb s_xyz(0.0);
	Rgb spdf(0.0);
	for(int wavelength = 380; wavelength < 750; wavelength += 5)
	{
		const double u_l = wavelength * 0.001;
		const double kg_lm = kg(wavelength) * m;
		const double kwa_lmw = kwa(wavelength) * mw;

		const double rayleigh = std::exp(m_1 * std::pow(u_l, m_4));
		const double angstrom = std::exp(m_b * std::pow(u_l, am));
		const double ozone = std::exp(ko(wavelength) * lm);
		const double gas = std::exp((-1.41 * kg_lm) / std::pow(1 + 118.93 * kg_lm, 0.45));
		const double water = std::exp((-0.2385 * kwa_lmw) / std::pow(1 + 20.07 * kwa_lmw, 0.45));
		spdf = Rgb{static_cast<float>(sun_radiance_curve(wavelength) * rayleigh * angstrom * ozone * gas * water)};
		s_xyz += spectral_data::chromaMatch(wavelength) * spdf * 0.013513514;
	}
	return color_conv_.fromXyz(s_xyz, true);
}

double DarkSkyBackground::prePerez(const std::array<double, 6> &perez) const
{
	const double p_num = ((1 + perez[0] * std::exp(perez[1])) * (1 + (perez[2] * std::exp(perez[3] * theta_s_)) + (perez[4] * cos_theta_2_)));
	if(p_num == 0.0) return 0.0;
	return 1.0 / p_num;
}

double DarkSkyBackground::perezFunction(const std::array<double, 6> &lam, double cos_theta, double gamma, double cos_gamma, double lvz)
{
	const double num = ((1 + lam[0] * std::exp(lam[1] / cos_theta)) * (1 + lam[2] * std::exp(lam[3] * gamma) + lam[4] * cos_gamma));
	return lvz * num * lam[5];
}

inline Rgb DarkSkyBackground::getSkyCol(const Vec3f &dir) const
{
	Vec3f iw {dir};
	iw[Axis::Z] += params_.altitude_;
	iw.normalize();

	double cos_theta = iw[Axis::Z];
	if(cos_theta <= 0.0) cos_theta = 1e-6;
	double cos_gamma = iw * sun_dir_;
	const double cos_gamma_2 = cos_gamma * cos_gamma;
	const double gamma = std::acos(cos_gamma);

	const double x = perezFunction(perez_x_, cos_theta, gamma, cos_gamma_2, zenith_x_);
	const double y = perezFunction(perez_y_, cos_theta, gamma, cos_gamma_2, zenith_y_);
	const double Y = perezFunction(perez_Y_, cos_theta, gamma, cos_gamma_2, zenith_Y_) * 6.66666667e-5;

	Rgb sky_col = color_conv_.fromxyY(x, y, Y);
	if(params_.night_) sky_col *= Rgb(0.05, 0.05, 0.08);
	return sky_col * bright_;
}

Rgb DarkSkyBackground::operator()(const Vec3f &dir, bool use_ibl_blur) const
{
	return getSkyCol(dir);
}

Rgb DarkSkyBackground::eval(const Vec3f &dir, bool use_ibl_blur) const
{
	return getSkyCol(dir) * power_;
}

std::vector<std::pair<std::string, ParamMap>> DarkSkyBackground::getRequestedIblLights() const
{
	std::vector<std::pair<std::string, ParamMap>> requested_lights;
	if(params_.background_light_)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = params_.light_samples_;
		bgp["with_caustic"] = ParentClassType_t::params_.with_caustic_;
		bgp["with_diffuse"] = ParentClassType_t::params_.with_diffuse_;
		bgp["cast_shadows"] = ParentClassType_t::params_.cast_shadows_;
		requested_lights.emplace_back(ThisClassType_t::lightSkyName(), std::move(bgp));
	}
	if(params_.add_sun_ && math::radToDeg(math::acos(params_.from_[Axis::Z])) < 100.0)
	{
		const Vec3f direction{params_.from_.normalized()};
		const Rgb suncol = getAttenuatedSunColor();
		const double angle = 0.5 * (2.0 - direction[Axis::Z]);
		ParamMap bgp;
		bgp["type"] = std::string("sunlight");
		bgp["direction"] = direction;
		bgp["color"] = suncol;
		bgp["angle"] = Parameter(angle);
		bgp["power"] = Parameter(params_.sun_power_ * (params_.night_ ? 0.5f : 1.f));
		bgp["samples"] = params_.light_samples_;
		bgp["with_caustic"] = ParentClassType_t::params_.with_caustic_;
		bgp["with_diffuse"] = ParentClassType_t::params_.with_diffuse_;
		bgp["cast_shadows"] = ParentClassType_t::params_.cast_shadows_;
		requested_lights.emplace_back(ThisClassType_t::lightSunName(), std::move(bgp));
	}
	return requested_lights;
}

} //namespace yafaray
