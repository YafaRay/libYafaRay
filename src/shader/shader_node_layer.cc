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

#include "shader/shader_node_layer.h"
#include "common/param.h"
#include "common/logger.h"
#include "common/string.h"

BEGIN_YAFARAY

LayerNode::LayerNode(const Flags &flags, float col_fac, float var_fac, float def_val, const Rgba &def_col, const BlendMode &blend_mode):
		flags_(flags), colfac_(col_fac), valfac_(var_fac), default_val_(def_val),
		default_col_(def_col), blend_mode_(blend_mode)
{}

void LayerNode::eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
{
	Rgba texcolor;
	float tin = 0.f, ta = 1.f;
	// == get result of upper layer (or base values) ==
	Rgba rcol = (upper_layer_) ? upper_layer_->getColor(stack) : upper_col_;
	float rval = (upper_layer_) ? upper_layer_->getScalar(stack) : upper_val_;
	float stencil_tin = rcol.a_;

	// == get texture input color ==
	bool tex_rgb = color_input_;

	if(color_input_)
	{
		texcolor = input_->getColor(stack);
		ta = texcolor.a_;
	}
	else tin = input_->getScalar(stack);

	if(flags_.hasAny(Flags::RgbToInt))
	{
		tin = texcolor.col2Bri();
		tex_rgb = false;
	}

	if(flags_.hasAny(Flags::Negative))
	{
		if(tex_rgb) texcolor = Rgba(1.f) - texcolor;
		tin = 1.f - tin;
	}

	if(flags_.hasAny(Flags::Stencil))
	{
		if(tex_rgb) // only scalar input affects stencil...?
		{
			const float fact = ta;
			ta *= stencil_tin;
			stencil_tin *= fact;
		}
		else
		{
			const float fact = tin;
			tin *= stencil_tin;
			stencil_tin *= fact;
		}
	}

	// color type modulation
	if(do_color_)
	{
		if(!tex_rgb) texcolor = default_col_;
		else tin = ta;

		float tin_truncated_range;
		if(tin > 1.f) tin_truncated_range = 1.f;
		else if(tin < 0.f) tin_truncated_range = 0.f;
		else tin_truncated_range = tin;

		rcol = textureRgbBlend(texcolor, rcol, tin_truncated_range, stencil_tin * colfac_, blend_mode_);
		rcol.clampRgb0();
	}

	// intensity type modulation
	if(do_scalar_)
	{
		if(tex_rgb)
		{
			if(use_alpha_)
			{
				tin = ta;
				if(flags_.hasAny(Flags::Negative)) tin = 1.f - tin;
			}
			else
			{
				tin = texcolor.col2Bri();
			}
		}

		rval = textureValueBlend(default_val_, rval, tin, stencil_tin * valfac_, blend_mode_);
		if(rval < 0.f) rval = 0.f;
	}
	rcol.a_ = stencil_tin;
	stack[this->getId()] = NodeResult(rcol, rval);
}

void LayerNode::evalDerivative(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
{
	float rdu = 0.f, rdv = 0.f;
	float stencil_tin = 1.f;

	// == get result of upper layer (or base values) ==
	if(upper_layer_)
	{
		const Rgba ucol = upper_layer_->getColor(stack);
		rdu = ucol.r_, rdv = ucol.g_;
		stencil_tin = ucol.a_;
	}

	// == get texture input derivative ==
	const Rgba texcolor = input_->getColor(stack);
	float tdu = texcolor.r_;
	float tdv = texcolor.g_;

	if(flags_.hasAny(Flags::Negative))
	{
		tdu = -tdu;
		tdv = -tdv;
	}
	// derivative modulation

	rdu += tdu;
	rdv += tdv;

	stack[this->getId()] = NodeResult(Rgba(rdu, rdv, 0.f, stencil_tin), 0.f);
}

bool LayerNode::configInputs(const ParamMap &params, const NodeFinder &find)
{
	std::string name;
	if(params.getParam("input", name))
	{
		input_ = find(name);
		if(!input_)
		{
			Y_WARNING << "LayerNode: Couldn't get input " << name << YENDL;
			return false;
		}
	}
	else
	{
		Y_WARNING << "LayerNode: input not set" << YENDL;
		return false;
	}

	if(params.getParam("upper_layer", name))
	{
		upper_layer_ = find(name);
		if(!upper_layer_)
		{
			Y_VERBOSE << "LayerNode: Couldn't get upper_layer " << name << YENDL;
			return false;
		}
	}
	else
	{
		if(!params.getParam("upper_color", upper_col_))
		{
			upper_col_ = Rgb(0.f);
		}
		if(!params.getParam("upper_value", upper_val_))
		{
			upper_val_ = 0.f;
		}
	}
	return true;
}

bool LayerNode::getDependencies(std::vector<const ShaderNode *> &dep) const
{
	// input actually needs to exist, but well...
	if(input_) dep.push_back(input_);
	if(upper_layer_) dep.push_back(upper_layer_);
	return !dep.empty();
}

Rgb LayerNode::textureRgbBlend(const Rgb &tex, const Rgb &out, float fact, float facg, const BlendMode &blend_mode)
{
	switch(blend_mode)
	{
		case BlendMode::Mult:
			fact *= facg;
			return (Rgb(1.f - facg) + fact * tex) * out;

		case BlendMode::Screen:
		{
			const Rgb white(1.0);
			fact *= facg;
			return white - (Rgb(1.f - facg) + fact * (white - tex)) * (white - out);
		}

		case BlendMode::Sub:
			fact = -fact;
		case BlendMode::Add:
			fact *= facg;
			return fact * tex + out;

		case BlendMode::Div:
		{
			fact *= facg;
			Rgb itex(tex);
			itex.invertRgb();
			return (1.f - fact) * out + fact * out * itex;
		}

		case BlendMode::Diff:
		{
			fact *= facg;
			Rgb tmo(tex - out);
			tmo.absRgb();
			return (1.f - fact) * out + fact * tmo;
		}

		case BlendMode::Dark:
		{
			fact *= facg;
			Rgb col(fact * tex);
			col.darkenRgb(out);
			return col;
		}

		case BlendMode::Light:
		{
			fact *= facg;
			Rgb col(fact * tex);
			col.lightenRgb(out);
			return col;
		}

			//case BlendMode::Mix:
		default:
			fact *= facg;
			return fact * tex + (1.f - fact) * out;
	}

}

float LayerNode::textureValueBlend(float tex, float out, float fact, float facg, const BlendMode &blend_mode, bool flip)
{
	fact *= facg;
	float facm = 1.f - fact;
	if(flip) std::swap(fact, facm);

	switch(blend_mode)
	{
		case BlendMode::Mult:
			facm = 1.f - facg;
			return (facm + fact * tex) * out;

		case BlendMode::Screen:
			facm = 1.f - facg;
			return 1.f - (facm + fact * (1.f - tex)) * (1.f - out);

		case BlendMode::Sub:
			fact = -fact;
		case BlendMode::Add:
			return fact * tex + out;

		case BlendMode::Div:
			if(tex == 0.f) return 0.f;
			return facm * out + fact * out / tex;

		case BlendMode::Diff:
			return facm * out + fact * std::abs(tex - out);

		case BlendMode::Dark:
		{
			const float col = fact * tex;
			if(col < out) return col;
			return out;
		}

		case BlendMode::Light:
		{
			const float col = fact * tex;
			if(col > out) return col;
			return out;
		}

			//case BlendMode::Mix:
		default:
			return fact * tex + facm * out;
	}
}

std::unique_ptr<ShaderNode> LayerNode::factory(const ParamMap &params, const Scene &scene)
{
	Rgb def_col(1.f);
	bool do_color = true, do_scalar = false, color_input = true, use_alpha = false;
	bool stencil = false, no_rgb = false, negative = false;
	double def_val = 1.0, colfac = 1.0, valfac = 1.0;
	std::string blend_mode_str;

	params.getParam("def_col", def_col);
	params.getParam("colfac", colfac);
	params.getParam("def_val", def_val);
	params.getParam("valfac", valfac);
	params.getParam("do_color", do_color);
	params.getParam("do_scalar", do_scalar);
	params.getParam("color_input", color_input);
	params.getParam("use_alpha", use_alpha);
	params.getParam("noRGB", no_rgb);
	params.getParam("stencil", stencil);
	params.getParam("negative", negative);
	params.getParam("blend_mode", blend_mode_str);

	Flags flags = Flags::None;
	if(no_rgb) flags |= Flags::RgbToInt;
	if(stencil) flags |= Flags::Stencil;
	if(negative) flags |= Flags::Negative;
	if(use_alpha) flags |= Flags::AlphaMix;

	BlendMode blend_mode = BlendMode::Mix;
	if(blend_mode_str == "add") blend_mode = BlendMode::Add;
	else if(blend_mode_str == "multiply") blend_mode = BlendMode::Mult;
	else if(blend_mode_str == "subtract") blend_mode = BlendMode::Sub;
	else if(blend_mode_str == "screen") blend_mode = BlendMode::Screen;
	else if(blend_mode_str == "divide") blend_mode = BlendMode::Div;
	else if(blend_mode_str == "difference") blend_mode = BlendMode::Diff;
	else if(blend_mode_str == "darken") blend_mode = BlendMode::Dark;
	else if(blend_mode_str == "lighten") blend_mode = BlendMode::Light;
	//else if(blend_mode_str == "overlay") blend_mode = BlendMode::Overlay;

	auto node = std::unique_ptr<LayerNode>(new LayerNode(flags, colfac, valfac, def_val, def_col, blend_mode));
	node->do_color_ = do_color;
	node->do_scalar_ = do_scalar;
	node->color_input_ = color_input;
	node->use_alpha_ = use_alpha;

	return node;
}

END_YAFARAY
