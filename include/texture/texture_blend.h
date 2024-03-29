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

#ifndef LIBYAFARAY_TEXTURE_BLEND_H
#define LIBYAFARAY_TEXTURE_BLEND_H

#include "texture/texture.h"

namespace yafaray {

class BlendTexture final : public Texture
{
		using ThisClassType_t = BlendTexture; using ParentClassType_t = Texture;

	public:
		inline static std::string getClassName() { return "BlendTexture"; }
		static std::pair<std::unique_ptr<Texture>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		BlendTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures);

	private:
		struct BlendType : public Enum<BlendType>
		{
			enum : ValueType_t { Linear, Quadratic, Easing, Diagonal, Spherical, QuadraticSphere, Radial };
			inline static const EnumMap<ValueType_t> map_{{
					{"linear", Linear, "Linear progression"},
					{"quadratic", Quadratic, "Quadratic progression"},
					{"easing", Easing, "'Easing' progression"},
					{"diagonal", Diagonal, "Diagonal progression"},
					{"sphere", Spherical, "Spherical progression"},
					{"quad_sphere", QuadraticSphere, "Quadratic Sphere progression"},
					{"radial", Radial, "Radial progression"},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Blend; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_ENUM_DECL(BlendType, blend_type_, BlendType::Linear, "blend_type", "Blend type for blend texture");
			PARAM_DECL(bool, use_flip_axis_, false, "use_flip_axis", "");
		} params_;
		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const override;
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_BLEND_H
