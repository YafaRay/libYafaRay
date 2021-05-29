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
#include "common/flags.h"

BEGIN_YAFARAY

class LayerNode final : public ShaderNode
{
	public:
		struct Flags : public yafaray4::Flags
		{
			Flags() = default;
			Flags(unsigned int flags) : yafaray4::Flags(flags) { }
			enum Enum : unsigned int { None = 0, RgbToInt = 1 << 0, Stencil = 1 << 1, Negative = 1 << 2, AlphaMix = 1 << 3 };
		};
		static std::unique_ptr<ShaderNode> factory(Logger &logger, const ParamMap &params, const Scene &scene);

	private:
		enum class BlendMode { Mix, Add, Mult, Sub, Screen, Div, Diff, Dark, Light };
		static Rgb textureRgbBlend(const Rgb &tex, const Rgb &out, float fact, float facg, const BlendMode &blend_mode);
		static float textureValueBlend(float tex, float out, float fact, float facg, const BlendMode &blend_mode, bool flip = false);

		LayerNode(const Flags &flags, float col_fac, float var_fac, float def_val, const Rgba &def_col, const BlendMode &blend_mode);
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const override;
		virtual void evalDerivative(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const override;
		virtual bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override;
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;
		virtual bool getDependencies(std::vector<const ShaderNode *> &dep) const override;

		const ShaderNode *input_ = nullptr, *upper_layer_ = nullptr;
		Flags flags_;
		float colfac_;
		float valfac_;
		float default_val_, upper_val_;
		Rgba default_col_, upper_col_;
		BlendMode blend_mode_;
		bool do_color_ = false, do_scalar_ = false, color_input_ = false, use_alpha_ = false;
};

END_YAFARAY

#endif // YAFARAY_SHADER_NODE_LAYER_H
