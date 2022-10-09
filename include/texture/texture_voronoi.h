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

#ifndef LIBYAFARAY_TEXTURE_VORONOI_H
#define LIBYAFARAY_TEXTURE_VORONOI_H

#include "texture/texture.h"
#include "noise/generator/noise_voronoi.h"

namespace yafaray {

class VoronoiTexture final : public Texture
{
	public:
		inline static std::string getClassName() { return "VoronoiTexture"; }
		static std::pair<Texture *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		struct ColorMode : public Enum<ColorMode>
		{
			enum : ValueType_t { IntensityWithoutColor, Position, PositionOutline, PositionOutlineIntensity };
			inline static const EnumMap<ValueType_t> map_{{
					{"intensity-without-color", IntensityWithoutColor, ""},
					{"position", Position, ""},
					{"position-outline", PositionOutline, ""},
					{"position-outline-intensity", PositionOutlineIntensity, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Voronoi; }
		const struct Params
		{
			PARAM_INIT_PARENT(Texture);
			PARAM_ENUM_DECL(VoronoiNoiseGenerator::DMetricType, distance_metric_, VoronoiNoiseGenerator::DMetricType::DistReal, "distance_metric", "");
			PARAM_ENUM_DECL(ColorMode, color_mode_, ColorMode::IntensityWithoutColor, "color_mode", "");
			PARAM_DECL(Rgb, color_1_, Rgb{0.f}, "color1", "");
			PARAM_DECL(Rgb, color_2_, Rgb{1.f}, "color2", "");
			PARAM_DECL(float, size_, 1.f, "size", "");
			PARAM_DECL(float, weight_1_, 1.f, "weight1", "feature 1 weight");
			PARAM_DECL(float, weight_2_, 0.f, "weight2", "feature 2 weight");
			PARAM_DECL(float, weight_3_, 0.f, "weight3", "feature 3 weight");
			PARAM_DECL(float, weight_4_, 0.f, "weight4", "feature 4 weight");
			PARAM_DECL(float, mk_exponent_, 2.5f, "mk_exponent", "Minkovsky exponent");
			PARAM_DECL(float, intensity_, 1.f, "intensity", "Intensity scale");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		VoronoiTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const override;

		const float aw_1_{std::abs(params_.weight_1_)};
		const float aw_2_{std::abs(params_.weight_2_)};
		const float aw_3_{std::abs(params_.weight_3_)};
		const float aw_4_{std::abs(params_.weight_4_)};
		float intensity_scale_{aw_1_ + aw_2_ + aw_3_ + aw_4_};
		VoronoiNoiseGenerator v_gen_{{NoiseGenerator::NoiseType::VoronoiF1}, params_.distance_metric_, params_.mk_exponent_};
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_VORONOI_H
