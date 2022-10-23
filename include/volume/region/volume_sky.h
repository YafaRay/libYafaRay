#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef LIBYAFARAY_VOLUME_SKY_H
#define LIBYAFARAY_VOLUME_SKY_H

#include "volume_region.h"

namespace yafaray {

class SkyVolumeRegion final : public VolumeRegion
{
		using ThisClassType_t = SkyVolumeRegion; using ParentClassType_t = VolumeRegion;

	public:
		inline static std::string getClassName() { return "SkyVolumeRegion"; }
		static std::pair<std::unique_ptr<VolumeRegion>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		SkyVolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::Sky; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			//FIXME DAVID: in SkyVolume some parameters from VolumeRegion are NOT used: s_a_ = Rgb(0.f); s_s_ = Rgb(0.f); g_ = 0.f;
		} params_;
		float p(const Vec3f &w_l, const Vec3f &w_s) const override;
		float phaseRayleigh(const Vec3f &w_l, const Vec3f &w_s) const;
		float phaseMie(const Vec3f &w_l, const Vec3f &w_s) const;
		Rgb sigmaA(const Point3f &p, const Vec3f &v) const override;
		Rgb sigmaS(const Point3f &p, const Vec3f &v) const override;
		Rgb emission(const Point3f &p, const Vec3f &v) const override;
		Rgb tau(const Ray &ray, float step, float offset) const override;

		const Rgb s_ray_{VolumeRegion::params_.sigma_a_, VolumeRegion::params_.sigma_a_, VolumeRegion::params_.sigma_a_ / 3.f};
		const Rgb s_mie_{VolumeRegion::params_.sigma_s_};
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_SKY_H