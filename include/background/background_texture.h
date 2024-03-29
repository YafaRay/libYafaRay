#pragma once
/****************************************************************************
 *      background_texture.h: a background using the texture class
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef LIBYAFARAY_BACKGROUND_TEXTURE_H
#define LIBYAFARAY_BACKGROUND_TEXTURE_H

#include <memory>
#include "background.h"
#include "color/color.h"

namespace yafaray {

class Texture;

class TextureBackground final : public Background
{
		using ThisClassType_t = TextureBackground; using ParentClassType_t = Background;

	public:
		inline static std::string getClassName() { return "TextureBackground"; }
		static std::pair<std::unique_ptr<Background>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params, const Items<Texture> &textures);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		TextureBackground(Logger &logger, ParamResult &param_result, const ParamMap &param_map, size_t texture_id, const Items <Texture> &textures);
		std::vector<std::pair<std::string, ParamMap>> getRequestedIblLights() const override;
		bool usesIblBlur() const override { return ParentClassType_t::params_.ibl_ && params_.ibl_blur_ > 0.f; }
		size_t getTextureId() const override { return texture_id_; }

	private:
		struct Projection : public Enum<Projection>
		{
			enum : ValueType_t { Spherical, Angular };
			inline static const EnumMap<ValueType_t> map_{{
					{"sphere", Spherical, ""},
					{"angular", Angular, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Texture; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float, rotation_, 0.f, "rotation", "");
			PARAM_DECL(float, ibl_blur_, 0.f, "smartibl_blur", "");
			PARAM_DECL(float, ibl_clamp_sampling_, 0.f, "ibl_clamp_sampling", "A value higher than 0.f 'clamps' the light intersection colors to that value, to reduce light sampling noise at the expense of realism and inexact overall light (0.f disables clamping)");
			PARAM_ENUM_DECL(Projection, projection_, Projection::Spherical, "mapping", "");
			PARAM_DECL(std::string, texture_name_, "", "texture", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		Rgb eval(const Vec3f &dir, bool use_ibl_blur) const override;
		static std::string lightName(){ return "background::light"; }

		size_t texture_id_{0};
		float sin_r_, cos_r_;
		const float rotation_{2.f * params_.rotation_ / 360.f};
		bool with_ibl_blur_ = false;
		float ibl_blur_mipmap_level_{math::pow(params_.ibl_blur_, 2.f)}; //Calculated based on the IBL_Blur parameter. As mipmap levels have half size each, this parameter is not linear
		const Items<Texture> &textures_;
};

} //namespace yafaray

#endif //LIBYAFARAY_BACKGROUND_TEXTURE_H