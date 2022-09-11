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

#ifndef YAFARAY_SHADER_NODE_MIX_SCREEN_H
#define YAFARAY_SHADER_NODE_MIX_SCREEN_H

#include "shader/shader_node_mix.h"

namespace yafaray {

class ScreenNode: public MixNode
{
	public:
		ScreenNode(Logger &logger, ParamError &param_error, const ParamMap &param_map) : MixNode{logger, param_error, param_map} { }
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			const float factor_reversed = 1.f - inputs.factor_;
			const Rgba color { Rgba{1.f} - (Rgba{factor_reversed} + inputs.factor_ * (Rgba{1.f} - inputs.in_2_.col_)) * (Rgba{1.f} - inputs.in_1_.col_) };
			const float scalar = 1.f - (factor_reversed + inputs.factor_ * (1.f - inputs.in_2_.f_)) * (1.f - inputs.in_1_.f_);
			node_tree_data[getId()] = { color, scalar };
		}
};

} //namespace yafaray

#endif // YAFARAY_SHADER_NODE_MIX_SCREEN_H
