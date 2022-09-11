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

#include "shader/shader_node_mix.h"
#include "shader/mix/shader_node_mix_add.h"
#include "shader/mix/shader_node_mix_mult.h"
#include "shader/mix/shader_node_mix_sub.h"
#include "shader/mix/shader_node_mix_screen.h"
#include "shader/mix/shader_node_mix_diff.h"
#include "shader/mix/shader_node_mix_dark.h"
#include "shader/mix/shader_node_mix_light.h"
#include "shader/mix/shader_node_mix_overlay.h"
#include "param/param.h"
#include "shader/node/node_finder.h"
#include "common/logger.h"

namespace yafaray {

/* ==========================================
/  A simple mix node, could be used to derive other math nodes
/ ========================================== */

MixNode::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(input_1_);
	PARAM_LOAD(color_1_);
	PARAM_LOAD(value_1_);
	PARAM_LOAD(input_2_);
	PARAM_LOAD(color_2_);
	PARAM_LOAD(value_2_);
	PARAM_LOAD(input_factor_);
	PARAM_LOAD(factor_);
	PARAM_ENUM_LOAD(blend_mode_);
}

ParamMap MixNode::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(input_1_);
	PARAM_SAVE(color_1_);
	PARAM_SAVE(value_1_);
	PARAM_SAVE(input_2_);
	PARAM_SAVE(color_2_);
	PARAM_SAVE(value_2_);
	PARAM_SAVE(input_factor_);
	PARAM_SAVE(factor_);
	PARAM_ENUM_SAVE(blend_mode_);
	PARAM_SAVE_END;
}

ParamMap MixNode::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ShaderNode::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<ShaderNode *, ParamError> MixNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	ShaderNode *result = nullptr;
	std::string blend_mode_str;
	param_map.getParam(Params::blend_mode_meta_.name(), blend_mode_str);
	const BlendMode blend_mode{blend_mode_str};
	switch(blend_mode.value())
	{
		case BlendMode::Mult: result = new MultNode(logger, param_error, param_map); break;
		case BlendMode::Screen: result = new ScreenNode(logger, param_error, param_map); break;
		case BlendMode::Sub: result = new SubNode(logger, param_error, param_map); break;
		case BlendMode::Add: result = new AddNode(logger, param_error, param_map); break;
			//case BlendMode::Div: result = DivNode(logger, param_error, param_map); break; //FIXME Why isn't there a DivNode?
		case BlendMode::Diff: result = new DiffNode(logger, param_error, param_map); break;
		case BlendMode::Dark: result = new DarkNode(logger, param_error, param_map); break;
		case BlendMode::Light: result = new LightNode(logger, param_error, param_map); break;
		case BlendMode::Overlay: result = new OverlayNode(logger, param_error, param_map); break;
		case BlendMode::Mix:
		default: result = new MixNode(logger, param_error, param_map); break;
	}
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<MixNode>(name, {"type"}));
	return {result, param_error};
}

MixNode::MixNode(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		ShaderNode{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void MixNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	const float factor = node_factor_ ? node_factor_->getScalar(node_tree_data) : params_.factor_;
	const Rgba col_1 = node_in_1_ ? node_in_1_->getColor(node_tree_data) : params_.color_1_;
	const float f_1 = node_in_1_ ? node_in_1_->getScalar(node_tree_data) : params_.value_1_;
	const Rgba col_2 = node_in_2_ ? node_in_2_->getColor(node_tree_data) : params_.color_2_;
	const float f_2 = node_in_2_ ? node_in_2_->getScalar(node_tree_data) : params_.value_2_;
	node_tree_data[getId()] = {
			math::lerp(col_1, col_2, factor),
			math::lerp(f_1, f_2, factor)
	};
}

bool MixNode::configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find)
{
	if(!params_.input_1_.empty())
	{
		node_in_1_ = find(params_.input_1_);
		if(!node_in_1_)
		{
			logger.logError("MixNode: Couldn't get input1 ", params_.input_1_);
			return false;
		}
	}
	if(!params_.input_2_.empty())
	{
		node_in_2_ = find(params_.input_2_);
		if(!node_in_2_)
		{
			logger.logError("MixNode: Couldn't get input2 ", params_.input_2_);
			return false;
		}
	}
	if(!params_.input_factor_.empty())
	{
		node_factor_ = find(params_.input_factor_);
		if(!node_factor_)
		{
			logger.logError("MixNode: Couldn't get factor ", params_.input_factor_);
			return false;
		}
	}
	return true;
}

std::vector<const ShaderNode *> MixNode::getDependencies() const
{
	std::vector<const ShaderNode *> dependencies;
	if(node_in_1_) dependencies.emplace_back(node_in_1_);
	if(node_in_2_) dependencies.emplace_back(node_in_2_);
	if(node_factor_) dependencies.emplace_back(node_factor_);
	return dependencies;
}

MixNode::Inputs MixNode::getInputs(const NodeTreeData &node_tree_data) const
{
	const float factor = node_factor_ ? node_factor_->getScalar(node_tree_data) : params_.factor_;
	NodeResult in_1 = node_in_1_ ?
		NodeResult{node_in_1_->getColor(node_tree_data), node_in_1_->getScalar(node_tree_data)} :
		NodeResult{params_.color_1_, params_.value_1_};
	NodeResult in_2 = node_in_2_ ?
		NodeResult{node_in_2_->getColor(node_tree_data), node_in_2_->getScalar(node_tree_data)} :
		NodeResult{params_.color_2_, params_.value_2_};
	return {std::move(in_1), std::move(in_2), factor};
}

} //namespace yafaray
