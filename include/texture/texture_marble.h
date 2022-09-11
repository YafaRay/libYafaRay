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

#ifndef LIBYAFARAY_TEXTURE_MARBLE_H
#define LIBYAFARAY_TEXTURE_MARBLE_H

#include "texture/texture.h"
#include "noise/noise_generator.h"

namespace yafaray {

class MarbleTexture final : public Texture
{
	public:
		inline static std::string getClassName() { return "MarbleTexture"; }
		static std::pair<Texture *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		struct Shape : public Enum<Shape>
		{
			enum : decltype(type()) {Sin, Saw, Tri};
			inline static const EnumMap<decltype(type())> map_{{
					{"sin", Sin, ""},
					{"saw", Saw, ""},
					{"tri", Tri, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Marble; }
		const struct Params
		{
			PARAM_INIT_PARENT(Texture);
			PARAM_ENUM_DECL(Shape, shape_, Shape::Sin, "shape", "");
			PARAM_ENUM_DECL(NoiseGenerator::NoiseType, noise_type_, NoiseGenerator::NoiseType::PerlinImproved, "noise_type", "");
			PARAM_DECL(Rgb, color_1_, Rgb{0.f}, "color1", "");
			PARAM_DECL(Rgb, color_2_, Rgb{1.f}, "color2", "");
			PARAM_DECL(int, octaves_, 2, "depth", "");
			PARAM_DECL(float, size_, 1.f, "size", "");
			PARAM_DECL(bool , hard_, false, "hard", "");
			PARAM_DECL(float, sharpness_, 1.f, "sharpness", "");
			PARAM_DECL(float, turbulence_, 1.f, "turbulence", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		MarbleTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map);

		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const override;

		const float sharpness_{(params_.sharpness_ > 1.f) ? 1.f / params_.sharpness_ : 1.f};
		const std::unique_ptr<NoiseGenerator> n_gen_{NoiseGenerator::newNoise(params_.noise_type_)};
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_MARBLE_H
