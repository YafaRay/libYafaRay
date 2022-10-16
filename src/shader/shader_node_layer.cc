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
#include "shader/node/node_finder.h"
#include "math/interpolation.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

LayerNode::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(input_);
	PARAM_LOAD(upper_layer_);
	PARAM_LOAD(upper_color_);
	PARAM_LOAD(upper_value_);
	PARAM_LOAD(def_col_);
	PARAM_LOAD(colfac_);
	PARAM_LOAD(def_val_);
	PARAM_LOAD(valfac_);
	PARAM_LOAD(do_color_);
	PARAM_LOAD(do_scalar_);
	PARAM_LOAD(color_input_);
	PARAM_LOAD(use_alpha_);
	PARAM_LOAD(no_rgb_);
	PARAM_LOAD(stencil_);
	PARAM_LOAD(negative_);
	PARAM_ENUM_LOAD(blend_mode_);
}

ParamMap LayerNode::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(input_);
	PARAM_SAVE(upper_layer_);
	PARAM_SAVE(upper_color_);
	PARAM_SAVE(upper_value_);
	PARAM_SAVE(def_col_);
	PARAM_SAVE(colfac_);
	PARAM_SAVE(def_val_);
	PARAM_SAVE(valfac_);
	PARAM_SAVE(do_color_);
	PARAM_SAVE(do_scalar_);
	PARAM_SAVE(color_input_);
	PARAM_SAVE(use_alpha_);
	PARAM_SAVE(no_rgb_);
	PARAM_SAVE(stencil_);
	PARAM_SAVE(negative_);
	PARAM_ENUM_SAVE(blend_mode_);
	PARAM_SAVE_END;
}

ParamMap LayerNode::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<ShaderNode>, ParamError> LayerNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto shader_node {std::make_unique<ThisClassType_t>(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	return {std::move(shader_node), param_error};
}

LayerNode::LayerNode(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	//logger.logParams(getAsParamMap(false).print()); //TEST CODE ONLY, REMOVE!!
	if(params_.no_rgb_) flags_ |= Flags{Flags::RgbToInt};
	if(params_.stencil_) flags_ |= Flags{Flags::Stencil};
	if(params_.negative_) flags_ |= Flags{Flags::Negative};
	if(params_.use_alpha_) flags_ |= Flags{Flags::AlphaMix};
}

void LayerNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	Rgba texcolor;
	float tin = 0.f, ta = 1.f;
	// == get result of upper layer (or base values) ==
	Rgba rcol = (upper_layer_) ? upper_layer_->getColor(node_tree_data) : params_.upper_color_;
	float rval = (upper_layer_) ? upper_layer_->getScalar(node_tree_data) : params_.upper_value_;
	float stencil_tin = rcol.a_;

	// == get texture input color ==
	bool tex_rgb = params_.color_input_;

	if(params_.color_input_)
	{
		texcolor = input_->getColor(node_tree_data);
		ta = texcolor.a_;
	}
	else tin = input_->getScalar(node_tree_data);

	if(flags_.has(Flags::RgbToInt))
	{
		tin = texcolor.col2Bri();
		tex_rgb = false;
	}

	if(flags_.has(Flags::Negative))
	{
		if(tex_rgb) texcolor = Rgba(1.f) - texcolor;
		tin = 1.f - tin;
	}

	if(flags_.has(Flags::Stencil))
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
	if(params_.do_color_)
	{
		if(!tex_rgb) texcolor = params_.def_col_;
		else tin = ta;

		float tin_truncated_range;
		if(tin > 1.f) tin_truncated_range = 1.f;
		else if(tin < 0.f) tin_truncated_range = 0.f;
		else tin_truncated_range = tin;

		rcol = Rgba{textureRgbBlend(texcolor, rcol, tin_truncated_range, stencil_tin * params_.colfac_, params_.blend_mode_)};
		rcol.clampRgb0();
	}

	// intensity type modulation
	if(params_.do_scalar_)
	{
		if(tex_rgb)
		{
			if(params_.use_alpha_)
			{
				tin = ta;
				if(flags_.has(Flags::Negative)) tin = 1.f - tin;
			}
			else
			{
				tin = texcolor.col2Bri();
			}
		}

		rval = textureValueBlend(params_.def_val_, rval, tin, stencil_tin * params_.valfac_, params_.blend_mode_);
		if(rval < 0.f) rval = 0.f;
	}
	rcol.a_ = stencil_tin;
	node_tree_data[getId()] = { rcol, rval };
}

void LayerNode::evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	float rdu = 0.f, rdv = 0.f;
	float stencil_tin = 1.f;

	// == get result of upper layer (or base values) ==
	if(upper_layer_)
	{
		const Rgba ucol = upper_layer_->getColor(node_tree_data);
		rdu = ucol.r_, rdv = ucol.g_;
		stencil_tin = ucol.a_;
	}

	// == get texture input derivative ==
	const Rgba texcolor = input_->getColor(node_tree_data);
	float tdu = texcolor.r_;
	float tdv = texcolor.g_;

	if(flags_.has(Flags::Negative))
	{
		tdu = -tdu;
		tdv = -tdv;
	}
	// derivative modulation

	rdu += tdu;
	rdv += tdv;

	node_tree_data[getId()] = { {rdu, rdv, 0.f, stencil_tin}, 0.f };
}

bool LayerNode::configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find)
{
	if(!params_.input_.empty())
	{
		input_ = find(params_.input_);
		if(!input_)
		{
			logger.logWarning("LayerNode: Couldn't get input ", params_.input_);
			return false;
		}
	}
	else
	{
		logger.logWarning("LayerNode: input not set");
		return false;
	}

	if(!params_.upper_layer_.empty())
	{
		upper_layer_ = find(params_.upper_layer_);
		if(!upper_layer_)
		{
			if(logger.isVerbose()) logger.logVerbose("LayerNode: Couldn't get upper_layer ", params_.upper_layer_);
			return false;
		}
	}
	return true;
}

std::vector<const ShaderNode *> LayerNode::getDependencies() const
{
	std::vector<const ShaderNode *> dependencies;
	if(input_) dependencies.emplace_back(input_);
	if(upper_layer_) dependencies.emplace_back(upper_layer_);
	return dependencies;
}

Rgb LayerNode::textureRgbBlend(const Rgb &tex, const Rgb &out, float fact, float facg, BlendMode blend_mode)
{
	switch(blend_mode.value())
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

		case BlendMode::Mix:
		default:
			fact *= facg;
			return fact * tex + (1.f - fact) * out;
	}
}

float LayerNode::textureValueBlend(float tex, float out, float fact, float facg, BlendMode blend_mode, bool flip)
{
	fact *= facg;
	float facm = 1.f - fact;
	if(flip) std::swap(fact, facm);

	switch(blend_mode.value())
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

} //namespace yafaray
