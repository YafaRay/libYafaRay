/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "shader/shader_node.h"
#include "shader/shader_node_basic.h"
#include "shader/shader_node_layer.h"
#include "common/param.h"

BEGIN_YAFARAY

ShaderNode *ShaderNode::factory(const ParamMap &params, const Scene &scene)
{
	Y_DEBUG PRTEXT(**ShaderNode) PREND; params.printDebug();
	std::string type;
	params.getParam("type", type);
	if(type == "texture_mapper") return TextureMapperNode::factory(params, scene);
	else if(type == "value") return ValueNode::factory(params, scene);
	else if(type == "mix") return MixNode::factory(params, scene);
	else if(type == "layer") return LayerNode::factory(params, scene);
	else return nullptr;
}

Rgb ShaderNode::textureRgbBlend(const Rgb &tex, const Rgb &out, float fact, float facg, const BlendMode &blend_mode)
{
	switch(blend_mode)
	{
		case BlendMode::Mult:
			fact *= facg;
			return (Rgb(1.f - facg) + fact * tex) * out;

		case BlendMode::Screen:
		{
			Rgb white(1.0);
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

float ShaderNode::textureValueBlend(float tex, float out, float fact, float facg, const BlendMode &blend_mode, bool flip)
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
			float col = fact * tex;
			if(col < out) return col;
			return out;
		}

		case BlendMode::Light:
		{
			float col = fact * tex;
			if(col > out) return col;
			return out;
		}

		//case BlendMode::Mix:
		default:
			return fact * tex + facm * out;
	}
}

END_YAFARAY
