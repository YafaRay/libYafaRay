/****************************************************************************
 *      sunsky.cc: a light source using the background
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

#include "background/background_sunsky.h"
#include "color/spectrum_sun.h"
#include "common/logger.h"
#include "param/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"
#include "math/interpolation_curve.h"

namespace yafaray {

// sunsky, from 'A Practical Analytic Model For DayLight' by Preetham, Shirley & Smits.
// http://www.cs.utah.edu/vissim/papers/sunsky/
// based on the actual code by Brian Smits
// and a thread on gamedev.net on skycolor algorithms

SunSkyBackground::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(from_);
	PARAM_LOAD(turb_);
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

ParamMap SunSkyBackground::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(from_);
	PARAM_SAVE(turb_);
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
	PARAM_SAVE_END;
}

ParamMap SunSkyBackground::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Background>, ParamError> SunSkyBackground::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto background{std::make_unique<ThisClassType_t>(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	if(background->params_.background_light_)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = background->params_.light_samples_;
		bgp["with_caustic"] = background->ParentClassType_t::params_.with_caustic_;
		bgp["with_diffuse"] = background->ParentClassType_t::params_.with_diffuse_;
		bgp["cast_shadows"] = background->ParentClassType_t::params_.cast_shadows_;

		auto bglight{Light::factory(logger, scene, "light_sky", bgp).first};
		bglight->setBackground(background.get());
		background->addLight(std::move(bglight));
	}
	if(background->params_.add_sun_)
	{
		Rgb suncol = computeAttenuatedSunlight(math::acos(std::abs(background->params_.from_[Axis::Z])), background->params_.turb_);//(*new_sunsky)(vector3d_t(dir.x, dir.y, dir.z));
		const double angle = 0.27;
		const double cos_angle = math::cos(math::degToRad(angle));
		const float invpdf = (2.f * math::num_pi<> * (1.f - cos_angle));
		suncol *= invpdf * background->ParentClassType_t::params_.power_;

		if(logger.isVerbose()) logger.logVerbose("Sunsky: sun color = ", suncol);

		ParamMap bgp;
		bgp["type"] = std::string("sunlight");
		bgp["direction"] = background->params_.from_;
		bgp["color"] = suncol;
		bgp["angle"] = Parameter(angle);
		bgp["power"] = Parameter(background->params_.sun_power_);
		bgp["cast_shadows"] = background->params_.cast_shadows_sun_;
		bgp["with_caustic"] = background->ParentClassType_t::params_.with_caustic_;
		bgp["with_diffuse"] = background->ParentClassType_t::params_.with_diffuse_;

		auto bglight{Light::factory(logger, scene, "light_sun", bgp).first};
		bglight->setBackground(background.get());
		background->addLight(std::move(bglight));
	}
	return {std::move(background), param_error};
}

SunSkyBackground::SunSkyBackground(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	sun_dir_.normalize();
	theta_s_ = math::acos(sun_dir_[Axis::Z]);
	theta_2_ = theta_s_ * theta_s_;
	theta_3_ = theta_2_ * theta_s_;
	phi_s_ = std::atan2(sun_dir_[Axis::Y], sun_dir_[Axis::X]);
	t_ = params_.turb_;
	t_2_ = params_.turb_ * params_.turb_;
	const double chi = (4.0 / 9.0 - t_ / 120.0) * (math::num_pi<double> - 2.0 * theta_s_);
	zenith_Y_ = (4.0453 * t_ - 4.9710) * std::tan(chi) - 0.2155 * t_ + 2.4192;
	zenith_Y_ *= 1000;  // conversion from kcd/m^2 to cd/m^2

	zenith_x_ = (0.00165 * theta_3_ - 0.00375 * theta_2_ + 0.00209 * theta_s_) * t_2_ +
			   (-0.02903 * theta_3_ + 0.06377 * theta_2_ - 0.03202 * theta_s_ + 0.00394) * t_ +
				(0.11693 * theta_3_ - 0.21196 * theta_2_ + 0.06052 * theta_s_ + 0.25886);

	zenith_y_ = (0.00275 * theta_3_ - 0.00610 * theta_2_ + 0.00317 * theta_s_) * t_2_ +
				(-0.04214 * theta_3_ + 0.08970 * theta_2_ - 0.04153 * theta_s_ + 0.00516) * t_ +
				(0.15346 * theta_3_ - 0.26756 * theta_2_ + 0.06670 * theta_s_ + 0.26688);

	perez_Y_[0] = (0.17872 * t_ - 1.46303) * params_.a_var_;
	perez_Y_[1] = (-0.35540 * t_ + 0.42749) * params_.b_var_;
	perez_Y_[2] = (-0.02266 * t_ + 5.32505) * params_.c_var_;
	perez_Y_[3] = (0.12064 * t_ - 2.57705) * params_.d_var_;
	perez_Y_[4] = (-0.06696 * t_ + 0.37027) * params_.e_var_;

	perez_x_[0] = (-0.01925 * t_ - 0.25922) * params_.a_var_;
	perez_x_[1] = (-0.06651 * t_ + 0.00081) * params_.b_var_;
	perez_x_[2] = (-0.00041 * t_ + 0.21247) * params_.c_var_;
	perez_x_[3] = (-0.06409 * t_ - 0.89887) * params_.d_var_;
	perez_x_[4] = (-0.00325 * t_ + 0.04517) * params_.e_var_;

	perez_y_[0] = (-0.01669 * t_ - 0.26078) * params_.a_var_;
	perez_y_[1] = (-0.09495 * t_ + 0.00921) * params_.b_var_;
	perez_y_[2] = (-0.00792 * t_ + 0.21023) * params_.c_var_;
	perez_y_[3] = (-0.04405 * t_ - 1.65369) * params_.d_var_;
	perez_y_[4] = (-0.01092 * t_ + 0.05291) * params_.e_var_;
}

double SunSkyBackground::perezFunction(const std::array<double, 5> &lam, double theta, double gamma, double lvz) const
{
	double e_1, e_2, e_3, e_4;
	if(lam[1] <= 230.)
		e_1 = math::exp(lam[1]);
	else
		e_1 = 7.7220185e99;
	if((e_2 = lam[3] * theta_s_) <= 230.)
		e_2 = math::exp(e_2);
	else
		e_2 = 7.7220185e99;
	if((e_3 = lam[1] / math::cos(theta)) <= 230.)
		e_3 = math::exp(e_3);
	else
		e_3 = 7.7220185e99;
	if((e_4 = lam[3] * gamma) <= 230.)
		e_4 = math::exp(e_4);
	else
		e_4 = 7.7220185e99;
	const double den = (1 + lam[0] * e_1) * (1 + lam[2] * e_2 + lam[4] * math::cos(theta_s_) * math::cos(theta_s_));
	const double num = (1 + lam[0] * e_3) * (1 + lam[2] * e_4 + lam[4] * math::cos(gamma) * math::cos(gamma));
	return (lvz * num / den);
}


double SunSkyBackground::angleBetween(double thetav, double phiv) const
{
	const double cospsi = math::sin(thetav) * math::sin(theta_s_) * math::cos(phi_s_ - phiv) + math::cos(thetav) * math::cos(theta_s_);
	if(cospsi > 1)  return 0.0;
	if(cospsi < -1) return math::num_pi<double>;
	return math::acos(cospsi);
}

inline Rgb SunSkyBackground::getSkyCol(const Vec3f &dir) const
{
	Vec3f iw {dir};
	iw.normalize();
	double hfade = 1, nfade = 1;

	Rgb skycolor(0.0);
	double theta = math::acos(iw[Axis::Z]);
	if(theta > (0.5 * math::num_pi<double>))
	{
		// this stretches horizon color below horizon, must be possible to do something better...
		// to compensate, simple fade to black
		hfade = 1.0 - (theta * math::div_1_by_pi<double> - 0.5) * 2.0;
		hfade = hfade * hfade * (3.0 - 2.0 * hfade);
		theta = 0.5 * math::num_pi<double>;
	}
	// compensation for nighttime exaggerated blue
	// also simple fade out towards zenith
	if(theta_s_ > (0.5 * math::num_pi<double>))
	{
		if(theta <= 0.5 * math::num_pi<double>)
		{
			nfade = 1.0 - (0.5 - theta * math::div_1_by_pi<double>) * 2.0;
			nfade *= 1.0 - (theta_s_ * math::div_1_by_pi<double> - 0.5) * 2.0;
			nfade = nfade * nfade * (3.0 - 2.0 * nfade);
		}
	}
	double phi;
	if((iw[Axis::Y] == 0.0) && (iw[Axis::X] == 0.0))
		phi = math::num_pi<double> * 0.5;
	else
		phi = std::atan2(iw[Axis::Y], iw[Axis::X]);

	const double gamma = angleBetween(theta, phi);
	// Compute xyY values
	const double x = perezFunction(perez_x_, theta, gamma, zenith_x_);
	const double y = perezFunction(perez_y_, theta, gamma, zenith_y_);
	// Luminance scale 1.0/15000.0
	const double Y = 6.666666667e-5 * nfade * hfade * perezFunction(perez_Y_, theta, gamma, zenith_Y_);

	if(y == 0.f) return skycolor;

	// conversion to RGB, from gamedev.net thread on skycolor computation
	const double X = (x / y) * Y;
	const double z = ((1.0 - x - y) / y) * Y;

	skycolor.set((3.240479 * X - 1.537150 * Y - 0.498535 * z),
	             (-0.969256 * X + 1.875992 * Y + 0.041556 * z),
	             (0.055648 * X - 0.204043 * Y + 1.057311 * z));
	skycolor.clampRgb01();
	return skycolor;
}

Rgb SunSkyBackground::eval(const Vec3f &dir, bool use_ibl_blur) const
{
	return ParentClassType_t::params_.power_ * getSkyCol(dir);
}

Rgb SunSkyBackground::computeAttenuatedSunlight(float theta, int turbidity)
{
	const IrregularCurve k_o_curve(spectrum_sun::k_o_wavelength_amplitudes);
	const IrregularCurve k_g_curve(spectrum_sun::k_g_wavelength_amplitudes);
	const IrregularCurve k_wa_curve(spectrum_sun::k_wa_wavelength_amplitudes);
	//RiRegularSpectralCurve   solCurve(solAmplitudes, 380, 750, 38);  // every 10 nm  IN WRONG UNITS
	// Need a factor of 100 (done below)
	std::array<float, spectrum_sun::sol_amplitudes.size()> data;  // (750 - 380) / 10  + 1

	const float beta = 0.04608365822050f * turbidity - 0.04586025928522f;
	const float alpha = 1.3f, l_ozone = .35f, w = 2.0f;

	Rgb sun_xyz(0.f);
	const float m = 1.f / (math::cos(theta) + 0.000940f * math::pow(1.6386f - theta, -1.253f)); // Relative Optical Mass

	size_t i = 0;
	float lambda = 380.f;
	for(const float sol_amplitude : spectrum_sun::sol_amplitudes)
	{
		const float u_l = lambda * 0.001f;
		// Rayleigh Scattering
		// Results agree with the graph (pg 115, MI) */
		const float tau_r = math::exp(-m * 0.008735f * math::pow(u_l, -4.08f));
		// Aerosal (water + dust) attenuation
		// beta - amount of aerosols present
		// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
		// Results agree with the graph (pg 121, MI)
		const float tau_a = math::exp(-m * beta * math::pow(u_l, -alpha));  // lambda should be in um
		// Attenuation due to ozone absorption
		// lOzone - amount of ozone in cm(NTP)
		// Results agree with the graph (pg 128, MI)
		const float tau_o = math::exp(-m * k_o_curve.getSample(lambda) * l_ozone);
		// Attenuation due to mixed gases absorption
		// Results agree with the graph (pg 131, MI)
		const float tau_g = math::exp(-1.41f * k_g_curve.getSample(lambda) * m / math::pow(1.f + 118.93f * k_g_curve.getSample(lambda) * m, 0.45f));
		// Attenuation due to water vapor absorbtion
		// w - precipitable water vapor in centimeters (standard = 2)
		// Results agree with the graph (pg 132, MI)
		const float tau_wa = math::exp(-0.2385f * k_wa_curve.getSample(lambda) * w * m /
									   math::pow(1.f + 20.07f * k_wa_curve.getSample(lambda) * w * m, 0.45f));

		data[i] = 100.f * sol_amplitude * tau_r * tau_a * tau_o * tau_g * tau_wa; // 100 comes from solCurve being
		// in wrong units.
		sun_xyz += spectrum::wl2Xyz(lambda) * data[i];

		lambda += 10.f;
		++i;
	}
	sun_xyz *= 0.02631578947368421053f;
	return {
			3.240479f * sun_xyz.r_ - 1.537150f * sun_xyz.g_ - 0.498535f * sun_xyz.b_,
			-0.969256f * sun_xyz.r_ + 1.875992f * sun_xyz.g_ + 0.041556f * sun_xyz.b_,
			0.055648f * sun_xyz.r_ - 0.204043f * sun_xyz.g_ + 1.057311f * sun_xyz.b_
	};
}

} //namespace yafaray
