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

#ifndef LIBYAFARAY_TEXTURE_DISTORTED_NOISE_H
#define LIBYAFARAY_TEXTURE_DISTORTED_NOISE_H

#include "texture/texture.h"
#include "noise/noise_generator.h"

namespace yafaray {

class DistortedNoiseTexture final : public Texture
{
		using ThisClassType_t = DistortedNoiseTexture; using ParentClassType_t = Texture;

	public:
		inline static std::string getClassName() { return "DistortedNoiseTexture"; }
		static std::pair<std::unique_ptr<Texture>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		DistortedNoiseTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::DistortedNoise; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_ENUM_DECL(NoiseGenerator::NoiseType, noise_type_1_, NoiseGenerator::NoiseType::PerlinImproved, "noise_type1", "");
			PARAM_ENUM_DECL(NoiseGenerator::NoiseType, noise_type_2_, NoiseGenerator::NoiseType::PerlinImproved, "noise_type2", "");
			PARAM_DECL(Rgb, color_1_, Rgb{0.f}, "color1", "");
			PARAM_DECL(Rgb, color_2_, Rgb{1.f}, "color2", "");
			PARAM_DECL(float, distort_, 1.f, "distort", "");
			PARAM_DECL(float, size_, 1.f, "size", "");
		} params_;
		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const override;

		const std::unique_ptr<NoiseGenerator> n_gen_1_{NoiseGenerator::newNoise(params_.noise_type_1_)};
		const std::unique_ptr<NoiseGenerator> n_gen_2_{NoiseGenerator::newNoise(params_.noise_type_2_)};
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_DISTORTED_NOISE_H
