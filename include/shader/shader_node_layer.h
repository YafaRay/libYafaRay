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
#include "texture/texture.h"
#include "common/environment.h"

BEGIN_YAFARAY

// texture flag
#define TXF_RGBTOINT    1
#define TXF_STENCIL     2
#define TXF_NEGATIVE    4
#define TXF_ALPHAMIX    8

class LayerNode: public ShaderNode
{
	public:
		LayerNode(unsigned tflag, float col_fac, float var_fac, float def_val, Rgba def_col, MixModes mmod);
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const;
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const;
		virtual void evalDerivative(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const;
		virtual bool isViewDependant() const;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find);
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;
		virtual bool getDependencies(std::vector<const ShaderNode *> &dep) const;
		static ShaderNode *factory(const ParamMap &params, RenderEnvironment &render);
	protected:
		const ShaderNode *input_, *upper_layer_;
		unsigned int texflag_;
		float colfac_;
		float valfac_;
		float default_val_, upper_val_;
		Rgba default_col_, upper_col_;
		MixModes mode_;
		bool do_color_, do_scalar_, color_input_, use_alpha_;
};



END_YAFARAY

#endif // YAFARAY_SHADER_NODE_LAYER_H
