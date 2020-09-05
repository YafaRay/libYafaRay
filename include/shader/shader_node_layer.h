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

#ifndef YAFARAY_SHADER_NODE_LAYER_H
#define YAFARAY_SHADER_NODE_LAYER_H

#include "shader/shader_node.h"

BEGIN_YAFARAY

class LayerNode final : public ShaderNode
{
	public:
		enum class Flags : unsigned int { None = 0, RgbToInt = 1 << 0, Stencil = 1 << 1, Negative = 1 << 2, AlphaMix = 1 << 3 };
		static ShaderNode *factory(const ParamMap &params, RenderEnvironment &render);

	private:
		LayerNode(const Flags &flags, float col_fac, float var_fac, float def_val, Rgba def_col, BlendMode mmod);
		static constexpr bool hasFlag(const LayerNode::Flags &f_1, const LayerNode::Flags &f_2);
		bool hasFlag(const LayerNode::Flags &f) const { return hasFlag(flags_, f); }
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const override;
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const override;
		virtual void evalDerivative(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const override;
		virtual bool isViewDependant() const override;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) override;
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;
		virtual bool getDependencies(std::vector<const ShaderNode *> &dep) const override;

		const ShaderNode *input_, *upper_layer_;
		Flags flags_;
		float colfac_;
		float valfac_;
		float default_val_, upper_val_;
		Rgba default_col_, upper_col_;
		BlendMode mode_;
		bool do_color_, do_scalar_, color_input_, use_alpha_;
};

inline constexpr LayerNode::Flags operator&(const LayerNode::Flags &f_1, const LayerNode::Flags &f_2)
{
	return static_cast<LayerNode::Flags>(static_cast<unsigned int>(f_1) & static_cast<unsigned int>(f_2));
}

inline constexpr LayerNode::Flags operator|(const LayerNode::Flags &f_1, const LayerNode::Flags &f_2)
{
	return static_cast<LayerNode::Flags>(static_cast<unsigned int>(f_1) | static_cast<unsigned int>(f_2));
}

inline LayerNode::Flags operator|=(LayerNode::Flags &f_1, const LayerNode::Flags &f_2)
{
	return f_1 = (f_1 | f_2);
}

inline constexpr bool LayerNode::hasFlag(const LayerNode::Flags &f_1, const LayerNode::Flags &f_2)
{
	return ((f_1 & f_2) != LayerNode::Flags::None);
}

END_YAFARAY

#endif // YAFARAY_SHADER_NODE_LAYER_H
