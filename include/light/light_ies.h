#pragma once
/****************************************************************************
 *      light_ies.h: IES Light
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

#ifndef LIBYAFARAY_LIGHT_IES_H
#define LIBYAFARAY_LIGHT_IES_H

#include "common/logger.h"
#include "light/light.h"
#include "geometry/vector.h"

namespace yafaray {

class ParamMap;
class Scene;
class IesData;

class IesLight final : public Light
{
		using ThisClassType_t = IesLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "IesLight"; }
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		IesLight(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const SceneItems<Light> &lights);

	private:
		[[nodiscard]] Type type() const override { return Type::Ies; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Vec3f, from_, Vec3f{0.f}, "from", "");
			PARAM_DECL(Vec3f, to_, (Vec3f{{0.f, 0.f, -1.f}}), "to", "");
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(std::string, file_, "", "file", "");
			PARAM_DECL(int, samples_, 16, "samples", "");
			PARAM_DECL(bool, soft_shadows_, false, "soft_shadows", "");
			PARAM_DECL(float, cone_angle_, 180.f, "cone_angle", "Cone angle in degrees");
		} params_;
		Rgb totalEnergy() const override{ return color_ * tot_energy_;};
		int nSamples() const override { return params_.samples_; };
		bool diracLight() const override { return !params_.soft_shadows_; }
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const override;
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override;
		std::array<float, 3> emitPdf(const Vec3f &surface_n, const Vec3f &wo) const override;
		bool isIesOk() const { return ies_ok_; };
		[[nodiscard]] static Uv<float> getAngles(const Vec3f &dir, float costheta);

		Vec3f dir_; //!< orientation of the spot cone
		Vec3f ndir_; //!< negative orientation (-dir)
		Uv<Vec3f> duv_; //!< form a coordinate system with dir, to sample directions
		float cos_end_; //<! cosStart is actually larger than cosEnd, because cos goes from +1 to -1
		Rgb color_; //<! color, premulitplied by light intensity
		float tot_energy_;
		std::unique_ptr<IesData> ies_data_;
		bool ies_ok_;
};

} //namespace yafaray

#endif // LIBYAFARAY_LIGHT_IES_H
