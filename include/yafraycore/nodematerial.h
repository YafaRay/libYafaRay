#pragma once

#ifndef YAFARAY_NODEMATERIAL_H
#define YAFARAY_NODEMATERIAL_H

#include <yafray_constants.h>
#include <core_api/material.h>
#include <core_api/shader.h>

BEGIN_YAFRAY

class RenderEnvironment;

enum NodeTypeE { ViewDep = 1, ViewIndep = 1 << 1 };

class YAFRAYCORE_EXPORT NodeMaterial: public Material
{
	public:
		NodeMaterial(): req_node_mem_(0) {}
	protected:
		/*! load nodes from parameter map list */
		bool loadNodes(const std::list<ParamMap> &params_list, RenderEnvironment &render);
		/** parse node shaders to fill nodeList */
		void parseNodes(const ParamMap &params, std::vector<ShaderNode *> &roots, std::map<std::string, ShaderNode *> &node_list);
		/* put nodes in evaluation order in "allSorted" given all root nodes;
		   sets reqNodeMem to the amount of memory the node stack requires for evaluation of all nodes */
		void solveNodesOrder(const std::vector<ShaderNode *> &roots);
		void getNodeList(const ShaderNode *root, std::vector<ShaderNode *> &nodes);
		void evalNodes(const RenderState &state, const SurfacePoint &sp, const std::vector<ShaderNode *> &nodes, NodeStack &stack) const
		{
			auto end = nodes.end();
			for(auto iter = nodes.begin(); iter != end; ++iter)(*iter)->eval(stack, state, sp);
		}
		void evalBump(NodeStack &stack, const RenderState &state, SurfacePoint &sp, const ShaderNode *bump_s) const;
		/*! filter out nodes with specific properties */
		void filterNodes(const std::vector<ShaderNode *> &input, std::vector<ShaderNode *> &output, int flags);
		virtual ~NodeMaterial();

		std::vector<ShaderNode *> all_nodes_, all_sorted_, all_viewdep_, all_viewindep_, bump_nodes_;
		std::map<std::string, ShaderNode *> m_shaders_table_;
		size_t req_node_mem_;
};

END_YAFRAY

#endif // YAFARAY_NODEMATERIAL_H
