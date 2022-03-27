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

#ifndef YAFARAY_SHADER_NODE_H
#define YAFARAY_SHADER_NODE_H

#include "common/yafaray_common.h"
#include "scene/scene.h"
#include "color/color.h"
#include "common/collection.h"
#include <list>
#include <map>

BEGIN_YAFARAY

class ParamMap;
class ShaderNode;
class MaterialData;

struct NodeResult final
{
	NodeResult() = default;
	NodeResult(Rgba color, float fval): col_(color), f_(fval) {}
	Rgba col_ {0.f};
	float f_ = 0.f;
};

class NodeTreeData final
{
	public:
		explicit NodeTreeData(size_t number_of_nodes) : node_results_(number_of_nodes) { }
		const NodeResult &operator()(unsigned int id) const { return node_results_[id]; }
		NodeResult &operator[](unsigned int id) { return node_results_[id]; }
	private:
		std::vector<NodeResult> node_results_;
};

class NodeFinder final : public Collection<std::string, const ShaderNode *>
{
	public:
		explicit NodeFinder(const std::map<std::string, std::unique_ptr<ShaderNode>> &table) { for(const auto &s : table) items_[s.first] = s.second.get(); }
};

class DuDv final
{
	public:
		DuDv(float du, float dv) : du_(du), dv_(dv) { }
		float getDu() const { return du_; }
		float getDv() const { return dv_; }

	private:
		float du_ = 0.f;
		float dv_ = 0.f;
};

/*!	shader nodes are as the name implies elements of a node based shading tree.
	Note that a "shader" only associates a color or scalar with a surface point,
	nothing more and nothing less. The material behaviour is implemented in the
	material_t class, NOT the shader classes.
*/
class ShaderNode
{
	public:
		static ShaderNode *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		virtual ~ShaderNode() = default;
		unsigned int getId() const { return id_; }
		void setId(unsigned int id) const { id_ = id; } //It is set as "const" but it actually modifies the (mutable) id_
		/*! evaluate the shader for given surface point; result has to be put on node_tree_data (using node_tree_data[ID]).
			i know, could've passed const node_tree_data and have nodeResult_t return val, but this should be marginally
			more efficient, so do me the favour and just don't mess up other node_tree_data elements ;) */
		virtual void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const = 0;
		/*! evaluate the shader partial derivatives for given surface point (e.g. for bump mapping);
			attention: uses color component of node node_tree_data to store result, so only use a node_tree_data for either eval or evalDeriv! */
		virtual void evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
		{ node_tree_data[id_] = NodeResult(Rgba(0.f), 0.f); }
		/*! configure the inputs. gets the same paramMap the factory functions get, but shader nodes
			may be created in any order and linked afterwards, so inputs may not exist yet on instantiation */
		virtual bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) = 0;
		//! return the nodes on which the output of this one depends
		/*! you may only call this after successfully calling configInputs!
			\param dep empty (!) vector to return the dependencies
			\return true if there exist dependencies, false if it does not depend on any other nodes */
		virtual std::vector<const ShaderNode *> getDependencies() const { return {}; }
		/*! get the color value calculated on eval */
		Rgba getColor(const NodeTreeData &node_tree_data) const { return node_tree_data(id_).col_; }
		/*! get the scalar value calculated on eval */
		float getScalar(const NodeTreeData &node_tree_data) const { return node_tree_data(id_).f_; }
		//! get the (approximate) partial derivatives df/dNU and df/dNV
		/*! where f is the shader function, and NU/NV/N build the shading coordinate system
			\param du df/dNU
			\param dv df/dNV	*/
		DuDv getDuDv(const NodeTreeData &node_tree_data) const { return { node_tree_data(id_).col_.r_, node_tree_data(id_).col_.g_}; }
		/* virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const {du=0.f, dv=0.f;} */

	private:
		mutable unsigned int id_;
};

END_YAFARAY

#endif // YAFARAY_SHADER_NODE_H
