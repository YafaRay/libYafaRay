/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      background_darksky.h: SkyLight, "Real" Sunlight and Sky Background
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

#ifndef LIBYAFARAY_BACKGROUND_DARKSKY_H
#define LIBYAFARAY_BACKGROUND_DARKSKY_H

#include <memory>
#include "background.h"
#include "geometry/vector.h"
#include "color/color_conversion.h"

namespace yafaray {

class DarkSkyBackground final : public Background
{
		using ThisClassType_t = DarkSkyBackground; using ParentClassType_t = Background;

	public:
		inline static std::string getClassName() { return "DarkSkyBackground"; }
		static std::pair<std::unique_ptr<Background>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		DarkSkyBackground(Logger &logger, ParamError &param_error, const ParamMap &param_map);

	private:
		struct ColorSpace : public Enum<ColorSpace>
		{
			inline static const EnumMap<ValueType_t> map_{{
					{"CIE (E)", ColorConv::CieRgbECs, ""},
					{"CIE (D50)", ColorConv::CieRgbD50Cs, ""},
					{"sRGB (D65)", ColorConv::SRgbD65Cs, ""},
					{"sRGB (D50)", ColorConv::SRgbD50Cs, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::DarkSky; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Vec3f, from_, (Vec3f{{1, 1, 1}}), "from", "same as sunlight, position interpreted as direction");
			PARAM_DECL(float , turb_, 4.f, "turbidity", "turbidity of atmosphere");
			PARAM_DECL(float , altitude_, 0.f, "altitude", "");
			PARAM_DECL(float , bright_, 1.f, "bright", "");
			PARAM_DECL(float, exposure_, 1.f, "exposure", "");
			PARAM_ENUM_DECL(ColorSpace, color_space_, ColorConv::CieRgbECs, "color_space", "");
			PARAM_DECL(bool , night_, true, "night", "");
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
		Rgb operator()(const Vec3f &dir, bool use_ibl_blur) const override;
		Rgb eval(const Vec3f &dir, bool use_ibl_blur) const override;
		Rgb getAttenuatedSunColor();
		Rgb getSkyCol(const Vec3f &dir) const;
		double prePerez(const std::array<double, 6> &perez) const;
		Rgb getSunColorFromSunRad();
		static double perezFunction(const std::array<double, 6> &lam, double cos_theta, double gamma, double cos_gamma, double lvz);

		Vec3f sun_dir_{params_.from_};
		double theta_s_;
		double theta_2_, theta_3_;
		double cos_theta_s_, cos_theta_2_;
		double t_, t_2_;
		double zenith_Y_, zenith_x_, zenith_y_;
		std::array<double, 6> perez_Y_, perez_x_, perez_y_;
		const float bright_{params_.bright_ * (params_.night_ ? 0.5f : 1.f)};
		const float power_{ParentClassType_t::params_.power_ * bright_};
		const ColorConv color_conv_{true, true, static_cast<ColorConv::ColorSpace>(params_.color_space_.value()), params_.exposure_};
};

} //namespace yafaray

#endif // LIBYAFARAY_BACKGROUND_DARKSKY_H