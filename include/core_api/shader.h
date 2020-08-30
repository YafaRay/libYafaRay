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

#ifndef YAFARAY_SHADER_H
#define YAFARAY_SHADER_H

#include <yafray_constants.h>
#include "scene.h"
#include "color.h"
#include <list>
#include <map>

BEGIN_YAFRAY

class ParamMap;
class ShaderNode;

struct NodeResult
{
	NodeResult() {}
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

class YAFRAYCORE_EXPORT NodeFinder
{
	public:
		virtual const ShaderNode *operator()(const std::string &name) const = 0;
		virtual ~NodeFinder() {};
};

enum MixModes { MnMix = 0, MnAdd, MnMult, MnSub, MnScreen, MnDiv, MnDiff, MnDark, MnLight, MnOverlay };

/*!	shader nodes are as the name implies elements of a node based shading tree.
	Note that a "shader" only associates a color or scalar with a surface point,
	nothing more and nothing less. The material behaviour is implemented in the
	material_t class, NOT the shader classes.
*/
class YAFRAYCORE_EXPORT ShaderNode
{
	public:
		virtual ~ShaderNode() {}
		/*! evaluate the shader for given surface point; result has to be put on stack (using stack[ID]).
			i know, could've passed const stack and have nodeResult_t return val, but this should be marginally
			more efficient, so do me the favour and just don't mess up other stack elements ;) */
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const = 0;
		/*! evaluate the shader for given surface point and directions; otherwise same behavious than the other eval.
			Should only be called when the node returns true on isViewDependant() */
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const = 0;
		/*! evaluate the shader partial derivatives for given surface point (e.g. for bump mapping);
			attention: uses color component of node stack to store result, so only use a stack for either eval or evalDeriv! */
		virtual void evalDerivative(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const
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
		unsigned int id_;
};

///////////////////////////

inline Rgb textureRgbBlend__(const Rgb &tex, const Rgb &out, float fact, float facg, MixModes blendtype)
{

	switch(blendtype)
	{
		case MnMult:
			fact *= facg;
			return (Rgb(1.f - facg) + fact * tex) * out;

		case MnScreen:
		{
			Rgb white(1.0);
			fact *= facg;
			return white - (Rgb(1.f - facg) + fact * (white - tex)) * (white - out);
		}

		case MnSub:
			fact = -fact;
		case MnAdd:
			fact *= facg;
			return fact * tex + out;

		case MnDiv:
		{
			fact *= facg;
			Rgb itex(tex);
			itex.invertRgb();
			return (1.f - fact) * out + fact * out * itex;
		}

		case MnDiff:
		{
			fact *= facg;
			Rgb tmo(tex - out);
			tmo.absRgb();
			return (1.f - fact) * out + fact * tmo;
		}

		case MnDark:
		{
			fact *= facg;
			Rgb col(fact * tex);
			col.darkenRgb(out);
			return col;
		}

		case MnLight:
		{
			fact *= facg;
			Rgb col(fact * tex);
			col.lightenRgb(out);
			return col;
		}

		default:
		case MnMix:
			fact *= facg;
			return fact * tex + (1.f - fact) * out;
	}

}

inline float textureValueBlend__(float tex, float out, float fact, float facg, MixModes blendtype, bool flip = false)
{
	fact *= facg;
	float facm = 1.f - fact;
	if(flip) std::swap(fact, facm);

	switch(blendtype)
	{
		case MnMult:
			facm = 1.f - facg;
			return (facm + fact * tex) * out;

		case MnScreen:
			facm = 1.f - facg;
			return 1.f - (facm + fact * (1.f - tex)) * (1.f - out);

		case MnSub:
			fact = -fact;
		case MnAdd:
			return fact * tex + out;

		case MnDiv:
			if(tex == 0.f) return 0.f;
			return facm * out + fact * out / tex;

		case MnDiff:
			return facm * out + fact * std::fabs(tex - out);

		case MnDark:
		{
			float col = fact * tex;
			if(col < out) return col;
			return out;
		}

		case MnLight:
		{
			float col = fact * tex;
			if(col > out) return col;
			return out;
		}

		default:
		case MnMix:
			return fact * tex + facm * out;
	}
}


END_YAFRAY

#endif // YAFARAY_SHADER_H
