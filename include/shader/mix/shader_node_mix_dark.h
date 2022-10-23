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

#ifndef LIBYAFARAY_SHADER_NODE_MIX_DARK_H
#define LIBYAFARAY_SHADER_NODE_MIX_DARK_H

#include "shader/shader_node_mix.h"

namespace yafaray {

class DarkNode: public MixNode
{
	public:
		DarkNode(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : MixNode{logger, param_result, param_map} { }
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			inputs.in_2_.col_ *= inputs.factor_;
			if(inputs.in_2_.col_.r_ < inputs.in_1_.col_.r_) inputs.in_1_.col_.r_ = inputs.in_2_.col_.r_;
			if(inputs.in_2_.col_.g_ < inputs.in_1_.col_.g_) inputs.in_1_.col_.g_ = inputs.in_2_.col_.g_;
			if(inputs.in_2_.col_.b_ < inputs.in_1_.col_.b_) inputs.in_1_.col_.b_ = inputs.in_2_.col_.b_;
			if(inputs.in_2_.col_.a_ < inputs.in_1_.col_.a_) inputs.in_1_.col_.a_ = inputs.in_2_.col_.a_;
			inputs.in_2_.f_ *= inputs.factor_;
			if(inputs.in_2_.f_ < inputs.in_1_.f_) inputs.in_1_.f_ = inputs.in_2_.f_;
			node_tree_data[getId()] = std::move(inputs.in_1_);
		}
};

} //namespace yafaray

#endif // LIBYAFARAY_SHADER_NODE_MIX_DARK_H
