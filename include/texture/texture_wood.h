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

#ifndef LIBYAFARAY_TEXTURE_WOOD_H
#define LIBYAFARAY_TEXTURE_WOOD_H

#include "texture/texture.h"
#include "noise/noise_generator.h"

namespace yafaray {

class WoodTexture final : public Texture
{
		using ThisClassType_t = WoodTexture; using ParentClassType_t = Texture;

	public:
		inline static std::string getClassName() { return "WoodTexture"; }
		static std::pair<std::unique_ptr<Texture>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		WoodTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	private:
		struct Shape : public Enum<Shape>
		{
			enum : ValueType_t { Sin, Saw, Tri };
			inline static const EnumMap<ValueType_t> map_{{
					{"sin", Sin, ""},
					{"saw", Saw, ""},
					{"tri", Tri, ""},
				}};
		};
		struct WoodType : public Enum<WoodType>
		{
			enum : ValueType_t { Bands, Rings };
			inline static const EnumMap<ValueType_t> map_{{
					{"bands", Bands, ""},
					{"rings", Rings, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Wood; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_ENUM_DECL(WoodType, wood_type_, WoodType::Bands, "wood_type", "");
			PARAM_ENUM_DECL(Shape, shape_, Shape::Sin, "shape", "");
			PARAM_ENUM_DECL(NoiseGenerator::NoiseType, noise_type_, NoiseGenerator::NoiseType::PerlinImproved, "noise_type", "");
			PARAM_DECL(Rgb, color_1_, Rgb{0.f}, "color1", "");
			PARAM_DECL(Rgb, color_2_, Rgb{1.f}, "color2", "");
			PARAM_DECL(int, octaves_, 2, "depth", "");
			PARAM_DECL(float, turbulence_, 1.f, "turbulence", "");
			PARAM_DECL(float, size_, 1.f, "size", "");
			PARAM_DECL(bool , hard_, false, "hard", "");
		} params_;
		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const override;

		const std::unique_ptr<NoiseGenerator> n_gen_{NoiseGenerator::newNoise(params_.noise_type_)};
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_WOOD_H
