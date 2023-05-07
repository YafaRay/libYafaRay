#pragma once
/****************************************************************************
 *      background_sunsky.h: a light source using the background
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

#ifndef LIBYAFARAY_BACKGROUND_SUNSKY_H
#define LIBYAFARAY_BACKGROUND_SUNSKY_H

#include "background.h"
#include "color/color.h"
#include "geometry/vector.h"

namespace yafaray {

// sunsky, from 'A Practical Analytic Model For DayLight' by Preetham, Shirley & Smits.
// http://www.cs.utah.edu/vissim/papers/sunsky/
// based on the actual code by Brian Smits
// and a thread on gamedev.net on skycolor algorithms

class SunSkyBackground final : public Background
{
		using ThisClassType_t = SunSkyBackground; using ParentClassType_t = Background;

	public:
		inline static std::string getClassName() { return "SunSkyBackground"; }
		static std::pair<std::unique_ptr<Background>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		SunSkyBackground(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		std::vector<std::pair<std::string, ParamMap>> getRequestedIblLights() const override;

	private:
		[[nodiscard]] Type type() const override { return Type::SunSky; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(Vec3f, from_, (Vec3f{{1, 1, 1}}), "from", "same as sunlight, position interpreted as direction");
			PARAM_DECL(float , turb_, 4.f, "turbidity", "turbidity of atmosphere");
			PARAM_DECL(bool , add_sun_, false, "add_sun", "automatically add real sunlight");
			PARAM_DECL(float , sun_power_, 1.f, "sun_power", "sunlight power");
			PARAM_DECL(bool , background_light_, false, "background_light", "");
			PARAM_DECL(int, light_samples_, 8, "light_samples", "");
			PARAM_DECL(bool , cast_shadows_sun_, true, "cast_shadows_sun", "");
			PARAM_DECL(float, a_var_, 1.f, "a_var", "color variation parameters, default is normal");
			PARAM_DECL(float, b_var_, 1.f, "b_var", "color variation parameters, default is normal");
			PARAM_DECL(float, c_var_, 1.f, "c_var", "color variation parameters, default is normal");
			PARAM_DECL(float, d_var_, 1.f, "d_var", "color variation parameters, default is normal");
			PARAM_DECL(float, e_var_, 1.f, "e_var", "color variation parameters, default is normal");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		Rgb eval(const Vec3f &dir, bool use_ibl_blur) const override;
		Rgb getSkyCol(const Vec3f &dir) const;
		static Rgb computeAttenuatedSunlight(float theta, int turbidity);
		static std::string lightSkyName(){ return getClassName() + "::light_sky"; }
		static std::string lightSunName(){ return getClassName() + "::light_sun"; }

		Vec3f sun_dir_{params_.from_};
		double theta_s_, phi_s_;	// sun coords
		double theta_2_, theta_3_, t_, t_2_;
		double zenith_Y_, zenith_x_, zenith_y_;
		std::array<double, 5> perez_Y_, perez_x_, perez_y_;
		double angleBetween(double thetav, double phiv) const;
		double perezFunction(const std::array<double, 5> &lam, double theta, double gamma, double lvz) const;
};

} //namespace yafaray

#endif // LIBYAFARAY_BACKGROUND_SUNSKY_H