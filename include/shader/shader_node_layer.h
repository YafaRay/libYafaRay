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

#ifndef LIBYAFARAY_SHADER_NODE_LAYER_H
#define LIBYAFARAY_SHADER_NODE_LAYER_H

#include "shader/shader_node.h"

namespace yafaray {

class LayerNode final : public ShaderNode
{
	public:
		inline static std::string getClassName() { return "LayerNode"; }
		static std::pair<ShaderNode *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		struct Flags : Enum<Flags>
		{
			using Enum::Enum;
			enum : decltype(type()) { None = 0, RgbToInt = 1 << 0, Stencil = 1 << 1, Negative = 1 << 2, AlphaMix = 1 << 3 };
			inline static const EnumMap<decltype(type())> map_{{
					{"None", None, ""},
					{"RgbToInt", RgbToInt, ""},
					{"Stencil", Stencil, ""},
					{"Negative", Negative, ""},
					{"AlphaMix", AlphaMix, ""},
				}};
		};
		struct BlendMode : Enum<BlendMode>
		{
			using Enum::Enum;
			enum : decltype(type()) { Mix, Add, Mult, Sub, Screen, Div, Diff, Dark, Light };
			inline static const EnumMap<decltype(type())> map_{{
					{"mix", Mix, ""},
					{"add", Add, ""},
					{"multiply", Mult, ""},
					{"subtract", Sub, ""},
					{"screen", Screen, ""},
					{"divide", Div, ""},
					{"difference", Diff, ""},
					{"darken", Dark, ""},
					{"lighten", Light, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Layer; }
		const struct Params
		{
			PARAM_INIT_PARENT(ShaderNode);
			PARAM_DECL(std::string, input_, "", "input", "");
			PARAM_DECL(std::string, upper_layer_, "", "upper_layer", "");
			PARAM_DECL(Rgba, upper_color_, Rgba{0.f}, "upper_color", "");
			PARAM_DECL(float, upper_value_, 0.f, "upper_value", "");
			PARAM_DECL(Rgba, def_col_, Rgba{1.f}, "def_col", "");
			PARAM_DECL(float, colfac_, 1.f, "colfac", "");
			PARAM_DECL(float, def_val_, 1.f, "def_val", "");
			PARAM_DECL(float, valfac_, 1.f, "valfac", "");
			PARAM_DECL(bool, do_color_, true, "do_color", "");
			PARAM_DECL(bool, do_scalar_, false, "do_scalar", "");
			PARAM_DECL(bool, color_input_, true, "color_input", "");
			PARAM_DECL(bool, use_alpha_, false, "use_alpha", "");
			PARAM_DECL(bool, no_rgb_, false, "noRGB", "");
			PARAM_DECL(bool, stencil_, false, "stencil", "");
			PARAM_DECL(bool, negative_, false, "negative", "");
			PARAM_ENUM_DECL(BlendMode, blend_mode_, BlendMode::Mix, "blend_mode", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		static Rgb textureRgbBlend(const Rgb &tex, const Rgb &out, float fact, float facg, BlendMode blend_mode);
		static float textureValueBlend(float tex, float out, float fact, float facg, BlendMode blend_mode, bool flip = false);

		LayerNode(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		void evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override;
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;
		std::vector<const ShaderNode *> getDependencies() const override;

		const ShaderNode *input_ = nullptr;
		const ShaderNode *upper_layer_ = nullptr;
		Flags flags_{Flags::None};
};

} //namespace yafaray

#endif // LIBYAFARAY_SHADER_NODE_LAYER_H
