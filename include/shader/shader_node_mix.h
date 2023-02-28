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

#ifndef LIBYAFARAY_SHADER_NODE_MIX_H
#define LIBYAFARAY_SHADER_NODE_MIX_H

#include "shader/shader_node.h"

namespace yafaray {

class MixNode : public ShaderNode
{
		using ThisClassType_t = MixNode; using ParentClassType_t = ShaderNode;

	public:
		inline static std::string getClassName() { return "MixNode"; }
		static std::pair<std::unique_ptr<ShaderNode>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		MixNode(Logger &logger, ParamResult &param_result, const ParamMap &param_map);

	protected:
		struct Inputs
		{
			Inputs(NodeResult input_1, NodeResult input_2, float factor) : in_1_{std::move(input_1)}, in_2_{std::move(input_2)}, factor_{factor} { }
			NodeResult in_1_;
			NodeResult in_2_;
			float factor_;
		};
		Inputs getInputs(const NodeTreeData &node_tree_data) const;

	private:
		struct BlendMode : Enum<BlendMode>
		{
			using Enum::Enum;
			enum : ValueType_t { Mix, Add, Mult, Sub, Screen, Div, Diff, Dark, Light, Overlay };
			inline static const EnumMap<ValueType_t> map_{{
					{"mix", Mix, ""},
					{"add", Add, ""},
					{"multiply", Mult, ""},
					{"subtract", Sub, ""},
					{"screen", Screen, ""},
					//{"divide", Div, ""},
					{"difference", Diff, ""},
					{"darken", Dark, ""},
					{"lighten", Light, ""},
					{"overlay", Overlay, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Mix; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(std::string, input_1_, "", "input1", "");
			PARAM_DECL(Rgba, color_1_, Rgba{0.f}, "color1", "");
			PARAM_DECL(float, value_1_, 0.f, "value1", "");
			PARAM_DECL(std::string, input_2_, "", "input2", "");
			PARAM_DECL(Rgba, color_2_, Rgba{0.f}, "color2", "");
			PARAM_DECL(float, value_2_, 0.f, "value2", "");
			PARAM_DECL(std::string, input_factor_, "", "input_factor", "");
			PARAM_DECL(float, factor_, 0.5f, "cfactor", "");
			PARAM_ENUM_DECL(BlendMode, blend_mode_, BlendMode::Mix, "blend_mode", "");
		} params_;
		bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override;
		std::vector<const ShaderNode *> getDependencies() const override;

		const ShaderNode *node_in_1_ = nullptr;
		const ShaderNode *node_in_2_ = nullptr;
		const ShaderNode *node_factor_ = nullptr;
};

} //namespace yafaray

#endif // LIBYAFARAY_SHADER_NODE_MIX_H
