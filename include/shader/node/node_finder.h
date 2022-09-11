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

#ifndef YAFARAY_NODE_FINDER_H
#define YAFARAY_NODE_FINDER_H

#include "common/collection.h"
#include <string>
#include <memory>

namespace yafaray {

class ShaderNode;

class NodeFinder final : public Collection<std::string, const ShaderNode *>
{
	public:
		explicit NodeFinder(const std::map<std::string, std::unique_ptr<ShaderNode>> &table) { for(const auto &[shader_name, shader] : table) items_[shader_name] = shader.get(); }
};

} //namespace yafaray

#endif //YAFARAY_NODE_FINDER_H
