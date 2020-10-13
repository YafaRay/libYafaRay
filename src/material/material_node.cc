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

#include "material/material_node.h"
#include "common/logger.h"
#include "common/param.h"
#include "shader/shader_node.h"

BEGIN_YAFARAY

class ShaderNodeFinder: public NodeFinder
{
	public:
		ShaderNodeFinder(const std::map<std::string, ShaderNode *> &table): node_table_(table) {}
		virtual const ShaderNode *operator()(const std::string &name) const;

		virtual ~ShaderNodeFinder() {};
	protected:
		const std::map<std::string, ShaderNode *> &node_table_;
};

const ShaderNode *ShaderNodeFinder::operator()(const std::string &name) const
{
	auto i = node_table_.find(name);
	if(i != node_table_.end()) return i->second;
	else return nullptr;
}

void recursiveSolver__(ShaderNode *node, std::vector<ShaderNode *> &sorted)
{
	if(node->getId() != 0) return;
	node->setId(1);
	std::vector<const ShaderNode *> deps;
	if(node->getDependencies(deps))
	{
		for(auto i = deps.begin(); i != deps.end(); ++i)
			// someone tell me a smarter way than casting away a const...
			if((*i)->getId() == 0) recursiveSolver__((ShaderNode *) *i, sorted);
	}
	sorted.push_back(node);
}

void recursiveFinder__(const ShaderNode *node, std::set<const ShaderNode *> &tree)
{
	std::vector<const ShaderNode *> deps;
	if(node->getDependencies(deps))
	{
		for(auto i = deps.begin(); i != deps.end(); ++i)
		{
			tree.insert(*i);
			recursiveFinder__(*i, tree);
		}
	}
	tree.insert(node);
}

NodeMaterial::~NodeMaterial()
{
	//clear nodes map:
	for(auto i = m_shaders_table_.begin(); i != m_shaders_table_.end(); ++i) delete i->second;
	m_shaders_table_.clear();
}

void NodeMaterial::evalNodes(const RenderData &render_data, const SurfacePoint &sp, const std::vector<ShaderNode *> &nodes, NodeStack &stack) const {
	auto end = nodes.end();
	for(auto iter = nodes.begin(); iter != end; ++iter)(*iter)->eval(stack, render_data, sp);
}

void NodeMaterial::solveNodesOrder(const std::vector<ShaderNode *> &roots)
{
	//set all IDs = 0 to indicate "not tested yet"
	for(unsigned int i = 0; i < all_nodes_.size(); ++i) all_nodes_[i]->setId(0);
	for(unsigned int i = 0; i < roots.size(); ++i) recursiveSolver__(roots[i], all_sorted_);
	if(all_nodes_.size() != all_sorted_.size()) Y_WARNING << "NodeMaterial: Unreachable nodes!" << YENDL;
	//give the nodes an index to be used as the "stack"-index.
	//using the order of evaluation can't hurt, can it?
	for(unsigned int i = 0; i < all_sorted_.size(); ++i)
	{
		ShaderNode *n = all_sorted_[i];
		n->setId(i);
		// sort nodes in view depandant and view independant
		// not sure if this is a good idea...should not include bump-only nodes
		//if(n->isViewDependant()) allViewdep.push_back(n);
		//else allViewindep.push_back(n);
	}
	req_node_mem_ = all_sorted_.size() * sizeof(NodeResult);
}

/*! get a list of all nodes that are in the tree given by root
	prerequisite: nodes have been successfully loaded and stored into allSorted
	since "solveNodesOrder" sorts allNodes, calling getNodeList afterwards gives
	a list in evaluation order. multiple calls are merged in "nodes" */

void NodeMaterial::getNodeList(const ShaderNode *root, std::vector<ShaderNode *> &nodes)
{
	std::set<const ShaderNode *> in_tree;
	for(unsigned int i = 0; i < nodes.size(); ++i) in_tree.insert(nodes[i]);
	recursiveFinder__(root, in_tree);
	auto send = in_tree.end();
	auto end = all_sorted_.end();
	nodes.clear();
	for(auto i = all_sorted_.begin(); i != end; ++i) if(in_tree.find(*i) != send) nodes.push_back(*i);
}

void NodeMaterial::filterNodes(const std::vector<ShaderNode *> &input, std::vector<ShaderNode *> &output, int flags)
{
	for(unsigned int i = 0; i < input.size(); ++i)
	{
		ShaderNode *n = input[i];
		bool vp = n->isViewDependant();
		if((vp && flags & ViewDep) || (!vp && flags & ViewIndep)) output.push_back(n);
	}
}

void NodeMaterial::evalBump(NodeStack &stack, const RenderData &render_data, SurfacePoint &sp, const ShaderNode *bump_s) const
{
	auto end = bump_nodes_.end();
	for(auto iter = bump_nodes_.begin(); iter != end; ++iter)(*iter)->evalDerivative(stack, render_data, sp);
	float du, dv;
	bump_s->getDerivative(stack, du, dv);
	applyBump(sp, du, dv);
}

bool NodeMaterial::loadNodes(const std::list<ParamMap> &params_list, Scene &scene)
{
	bool error = false;
	std::string type;
	std::string name;
	std::string element;

	for(const auto &i : params_list)
	{
		if(i.getParam("element", element))
		{
			if(element != "shader_node") continue;
		}
		else Y_WARNING << "NodeMaterial: No element type given; assuming shader node" << YENDL;

		if(! i.getParam("name", name))
		{
			Y_ERROR << "NodeMaterial: Name of shader node not specified!" << YENDL;
			error = true;
			break;
		}

		if(m_shaders_table_.find(name) != m_shaders_table_.end())
		{
			Y_ERROR << "NodeMaterial: Multiple nodes with identically names!" << YENDL;
			error = true;
			break;
		}

		if(! i.getParam("type", type))
		{
			Y_ERROR << "NodeMaterial: Type of shader node not specified!" << YENDL;
			error = true;
			break;
		}

		ShaderNode *shader = ShaderNode::factory(i, scene);
		if(shader)
		{
			m_shaders_table_[name] = shader;
			all_nodes_.push_back(shader);
			Y_VERBOSE << "NodeMaterial: Added ShaderNode '" << name << "'! (" << (void *)shader << ")" << YENDL;
		}
		else
		{
			Y_ERROR << "NodeMaterial: No shader node could be constructed.'" << type << "'!" << YENDL;
			error = true;
			break;
		}
	}

	if(!error) //configure node inputs
	{
		ShaderNodeFinder finder(m_shaders_table_);
		int n = 0;
		for(const auto &i : params_list)
		{
			if(!all_nodes_[n]->configInputs(i, finder))
			{
				Y_ERROR << "NodeMaterial: Shader node configuration failed! (n=" << n << ")" << YENDL;
				error = true; break;
			}
			++n;
		}
	}

	if(error)
	{
		//clear nodes map:
		for(auto i = m_shaders_table_.begin(); i != m_shaders_table_.end(); ++i) delete i->second;
		m_shaders_table_.clear();
	}

	return !error;
}

void NodeMaterial::parseNodes(const ParamMap &params, std::vector<ShaderNode *> &roots, std::map<std::string, ShaderNode *> &node_list)
{
	std::string name;

	for(auto current_node = node_list.begin(); current_node != node_list.end(); ++current_node)
	{
		if(params.getParam(current_node->first, name))
		{
			auto i = m_shaders_table_.find(name);

			if(i != m_shaders_table_.end())
			{
				current_node->second = i->second;
				roots.push_back(current_node->second);
			}
			else Y_WARNING << "Shader node " << current_node->first << " '" << name << "' does not exist!" << YENDL;
		}
	}
}

END_YAFARAY
