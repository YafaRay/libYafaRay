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

#ifndef LIBYAFARAY_VOLUME_NOISE_H
#define LIBYAFARAY_VOLUME_NOISE_H

#include "volume/region/density/volume_region_density.h"

namespace yafaray {

class Texture;

class NoiseVolumeRegion final : public DensityVolumeRegion
{
	public:
		inline static std::string getClassName() { return "NoiseVolumeRegion"; }
		static std::pair<VolumeRegion *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		[[nodiscard]] Type type() const override { return Type::Noise; }
		const struct Params
		{
			PARAM_INIT_PARENT(DensityVolumeRegion);
			PARAM_DECL(float, sharpness_, 1.f, "sharpness", "");
			PARAM_DECL(float, density_, 1.f, "density", "");
			PARAM_DECL(float, cover_, 1.f, "cover", "");
			PARAM_DECL(std::string, texture_, "", "texture", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		NoiseVolumeRegion(Logger &logger, ParamError &param_error, const ParamMap &param_map, const Texture *noise);
		float density(const Point3f &p) const override;

		const Texture *tex_dist_noise_ = nullptr;
		const float sharpness_{params_.sharpness_ * params_.sharpness_};
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_NOISE_H