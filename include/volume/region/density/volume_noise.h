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
template <typename T> class Items;

class NoiseVolumeRegion final : public DensityVolumeRegion
{
		using ThisClassType_t = NoiseVolumeRegion; using ParentClassType_t = DensityVolumeRegion;

	public:
		inline static std::string getClassName() { return "NoiseVolumeRegion"; }
		static std::pair<std::unique_ptr<VolumeRegion>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		NoiseVolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items<Texture> &textures, size_t texture_id);

	private:
		[[nodiscard]] Type type() const override { return Type::Noise; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(float, sharpness_, 1.f, "sharpness", "");
			PARAM_DECL(float, density_, 1.f, "density", "");
			PARAM_DECL(float, cover_, 1.f, "cover", "");
			PARAM_DECL(std::string, texture_, "", "texture", "");
		} params_;
		float density(const Point3f &p) const override;

		size_t texture_id_{math::invalid<size_t>};
		const Items<Texture> &textures_;
		const float sharpness_{params_.sharpness_ * params_.sharpness_};
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_NOISE_H