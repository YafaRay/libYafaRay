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

const ShaderNode *NodeFinder::operator()(const std::string &name) const
{
	auto i = node_table_.find(name);
	if(i != node_table_.end()) return i->second;
	else return nullptr;
}

END_YAFARAY
