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

#ifndef YAFARAY_SHADER_NODE_VALUE_H
#define YAFARAY_SHADER_NODE_VALUE_H

#include "shader/shader_node.h"

namespace yafaray {

class ValueNode final : public ShaderNode
{
	public:
		inline static std::string getClassName() { return "ValueNode"; }
		static std::pair<ShaderNode *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		[[nodiscard]] Type type() const override { return Type::Value; }
		const struct Params
		{
			PARAM_INIT_PARENT(ShaderNode);
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float, value_, 1.f, "scalar", "");
			PARAM_DECL(float, alpha_, 1.f, "alpha", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		ValueNode(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override { return true; };

		Rgba color_{params_.color_, params_.alpha_};
};

} //namespace yafaray

#endif // YAFARAY_SHADER_NODE_VALUE_H