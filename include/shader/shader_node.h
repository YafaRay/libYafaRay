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

#include "constants.h"
#include "scene/scene.h"
#include "color/color.h"
#include <list>
#include <map>

BEGIN_YAFARAY

class ParamMap;
class ShaderNode;

struct NodeResult
{
	NodeResult() = default;
	NodeResult(Rgba color, float fval): col_(color), f_(fval) {}
	Rgba col_;
	float f_;
};

class NodeStack
{
	public:
		NodeStack() { dat_ = nullptr; }
		NodeStack(void *data) { dat_ = (NodeResult *)data; }
		const NodeResult &operator()(unsigned int id) const
		{
			return dat_[id];//*(dat+ID);
		}
		NodeResult &operator[](unsigned int id)
		{
			return dat_[id];//*(dat+ID);
		}
	private:
		NodeResult *dat_;
};

class NodeFinder
{
	public:
		virtual const ShaderNode *operator()(const std::string &name) const = 0;
		virtual ~NodeFinder() = default;
};

/*!	shader nodes are as the name implies elements of a node based shading tree.
	Note that a "shader" only associates a color or scalar with a surface point,
	nothing more and nothing less. The material behaviour is implemented in the
	material_t class, NOT the shader classes.
*/
class ShaderNode
{
	public:
		static ShaderNode *factory(const ParamMap &params, const Scene &scene);
		virtual ~ShaderNode() = default;
		unsigned int getId() const { return id_; }
		void setId(unsigned int id) { id_ = id; }
		/*! evaluate the shader for given surface point; result has to be put on stack (using stack[ID]).
			i know, could've passed const stack and have nodeResult_t return val, but this should be marginally
			more efficient, so do me the favour and just don't mess up other stack elements ;) */
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const = 0;
		/*! evaluate the shader partial derivatives for given surface point (e.g. for bump mapping);
			attention: uses color component of node stack to store result, so only use a stack for either eval or evalDeriv! */
		virtual void evalDerivative(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{stack[this->id_] = NodeResult(Rgba(0.f), 0.f);}
		/*! indicate whether the shader value depends on wi and wo */
		virtual bool isViewDependant() const { return false; }
		/*! configure the inputs. gets the same paramMap the factory functions get, but shader nodes
			may be created in any order and linked afterwards, so inputs may not exist yet on instantiation */
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) = 0;
		//! return the nodes on which the output of this one depends
		/*! you may only call this after successfully calling configInputs!
			\param dep empty (!) vector to return the dependencies
			\return true if there exist dependencies, false if it does not depend on any other nodes */
		virtual bool getDependencies(std::vector<const ShaderNode *> &dep) const { return false; }
		/*! get the color value calculated on eval */
		Rgba getColor(const NodeStack &stack) const { return stack(this->id_).col_; }
		/*! get the scalar value calculated on eval */
		float getScalar(const NodeStack &stack) const { return stack(this->id_).f_; }
		//! get the (approximate) partial derivatives df/dNU and df/dNV
		/*! where f is the shader function, and NU/NV/N build the shading coordinate system
			\param du df/dNU
			\param dv df/dNV	*/
		void getDerivative(const NodeStack &stack, float &du, float &dv) const
		{ du = stack(this->id_).col_.r_; dv = stack(this->id_).col_.g_; }
		/* virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const {du=0.f, dv=0.f;} */

	private:
		unsigned int id_;
};

END_YAFARAY

#endif // YAFARAY_SHADER_NODE_H
