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

#ifndef LIBYAFARAY_TEXTURE_MUSGRAVE_H
#define LIBYAFARAY_TEXTURE_MUSGRAVE_H

#include "texture/texture.h"
#include "noise/noise_generator.h"

namespace yafaray {

class NoiseGenerator;
class Musgrave;

class MusgraveTexture final : public Texture
{
	public:
		inline static std::string getClassName() { return "MusgraveTexture"; }
		static std::pair<Texture *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		struct MusgraveType : public Enum<MusgraveType>
		{
			enum : decltype(type()) { Fbm, MultiFractal, HeteroTerrain, HybridDmf, RidgedDmf };
			inline static const EnumMap<decltype(type())> map_{{
					{"fBm", Fbm, ""},
					{"multifractal", MultiFractal, ""},
					{"heteroterrain", HeteroTerrain, ""},
					{"hybridmf", HybridDmf, ""},
					{"ridgedmf", RidgedDmf, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Musgrave; }
		const struct Params
		{
			PARAM_INIT_PARENT(Texture);
			PARAM_ENUM_DECL(MusgraveType, musgrave_type_, MusgraveType::Fbm, "musgrave_type", "");
			PARAM_ENUM_DECL(NoiseGenerator::NoiseType, noise_type_, NoiseGenerator::NoiseType::PerlinImproved, "noise_type", "");
			PARAM_DECL(Rgb, color_1_, Rgb{0.f}, "color1", "");
			PARAM_DECL(Rgb, color_2_, Rgb{1.f}, "color2", "");
			PARAM_DECL(float , h_, 1.f, "H", "");
			PARAM_DECL(float , lacunarity_, 2.f, "lacunarity", "");
			PARAM_DECL(float , octaves_, 2.f, "octaves", "");
			PARAM_DECL(float , offset_, 1.f, "offset", "");
			PARAM_DECL(float , gain_, 1.f, "gain", "");
			PARAM_DECL(float , intensity_, 1.f, "intensity", "");
			PARAM_DECL(float, size_, 1.f, "size", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		MusgraveTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const override;

		const std::unique_ptr<NoiseGenerator> n_gen_{NoiseGenerator::newNoise(params_.noise_type_)};
		std::unique_ptr<Musgrave> m_gen_;
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_MUSGRAVE_H
