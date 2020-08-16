#pragma once

#ifndef Y_SHADER_H
#define Y_SHADER_H

#include <yafray_constants.h>
#include "scene.h"
#include "color.h"
#include <list>
#include <map>

__BEGIN_YAFRAY

class paraMap_t;
class shaderNode_t;

struct nodeResult_t
{
	nodeResult_t() {}
	nodeResult_t(colorA_t color, float fval): col(color), f(fval) {}
	colorA_t col;
	float f;
};

class nodeStack_t
{
	public:
		nodeStack_t() { dat = nullptr; }
		nodeStack_t(void *data) { dat = (nodeResult_t *)data; }
		const nodeResult_t &operator()(unsigned int ID) const
		{
			return dat[ID];//*(dat+ID);
		}
		nodeResult_t &operator[](unsigned int ID)
		{
			return dat[ID];//*(dat+ID);
		}
	private:
		nodeResult_t *dat;
};

class YAFRAYCORE_EXPORT nodeFinder_t
{
	public:
		virtual const shaderNode_t *operator()(const std::string &name) const = 0;
		virtual ~nodeFinder_t() {};
};

enum mix_modes { MN_MIX = 0, MN_ADD, MN_MULT, MN_SUB, MN_SCREEN, MN_DIV, MN_DIFF, MN_DARK, MN_LIGHT, MN_OVERLAY };

/*!	shader nodes are as the name implies elements of a node based shading tree.
	Note that a "shader" only associates a color or scalar with a surface point,
	nothing more and nothing less. The material behaviour is implemented in the
	material_t class, NOT the shader classes.
*/
class YAFRAYCORE_EXPORT shaderNode_t
{
	public:
		virtual ~shaderNode_t() {}
		/*! evaluate the shader for given surface point; result has to be put on stack (using stack[ID]).
			i know, could've passed const stack and have nodeResult_t return val, but this should be marginally
			more efficient, so do me the favour and just don't mess up other stack elements ;) */
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp) const = 0;
		/*! evaluate the shader for given surface point and directions; otherwise same behavious than the other eval.
			Should only be called when the node returns true on isViewDependant() */
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi) const = 0;
		/*! evaluate the shader partial derivatives for given surface point (e.g. for bump mapping);
			attention: uses color component of node stack to store result, so only use a stack for either eval or evalDeriv! */
		virtual void evalDerivative(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp) const
		{stack[this->ID] = nodeResult_t(colorA_t(0.f), 0.f);}
		/*! indicate whether the shader value depends on wi and wo */
		virtual bool isViewDependant() const { return false; }
		/*! configure the inputs. gets the same paramMap the factory functions get, but shader nodes
			may be created in any order and linked afterwards, so inputs may not exist yet on instantiation */
		virtual bool configInputs(const paraMap_t &params, const nodeFinder_t &find) = 0;
		//! return the nodes on which the output of this one depends
		/*! you may only call this after successfully calling configInputs!
			\param dep empty (!) vector to return the dependencies
			\return true if there exist dependencies, false if it does not depend on any other nodes */
		virtual bool getDependencies(std::vector<const shaderNode_t *> &dep) const { return false; }
		/*! get the color value calculated on eval */
		colorA_t getColor(const nodeStack_t &stack) const { return stack(this->ID).col; }
		/*! get the scalar value calculated on eval */
		float getScalar(const nodeStack_t &stack) const { return stack(this->ID).f; }
		//! get the (approximate) partial derivatives df/dNU and df/dNV
		/*! where f is the shader function, and NU/NV/N build the shading coordinate system
			\param du df/dNU
			\param dv df/dNV	*/
		void getDerivative(const nodeStack_t &stack, float &du, float &dv) const
		{ du = stack(this->ID).col.R; dv = stack(this->ID).col.G; }
		/* virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const {du=0.f, dv=0.f;} */
		unsigned int ID;
};

///////////////////////////

inline color_t texture_rgb_blend(const color_t &tex, const color_t &out, float fact, float facg, mix_modes blendtype)
{

	switch(blendtype)
	{
		case MN_MULT:
			fact *= facg;
			return (color_t(1.f - facg) + fact * tex) * out;

		case MN_SCREEN:
		{
			color_t white(1.0);
			fact *= facg;
			return white - (color_t(1.f - facg) + fact * (white - tex)) * (white - out);
		}

		case MN_SUB:
			fact = -fact;
		case MN_ADD:
			fact *= facg;
			return fact * tex + out;

		case MN_DIV:
		{
			fact *= facg;
			color_t itex(tex);
			itex.invertRGB();
			return (1.f - fact) * out + fact * out * itex;
		}

		case MN_DIFF:
		{
			fact *= facg;
			color_t tmo(tex - out);
			tmo.absRGB();
			return (1.f - fact) * out + fact * tmo;
		}

		case MN_DARK:
		{
			fact *= facg;
			color_t col(fact * tex);
			col.darkenRGB(out);
			return col;
		}

		case MN_LIGHT:
		{
			fact *= facg;
			color_t col(fact * tex);
			col.lightenRGB(out);
			return col;
		}

		default:
		case MN_MIX:
			fact *= facg;
			return fact * tex + (1.f - fact) * out;
	}

}

inline float texture_value_blend(float tex, float out, float fact, float facg, mix_modes blendtype, bool flip = false)
{
	fact *= facg;
	float facm = 1.f - fact;
	if(flip) std::swap(fact, facm);

	switch(blendtype)
	{
		case MN_MULT:
			facm = 1.f - facg;
			return (facm + fact * tex) * out;

		case MN_SCREEN:
			facm = 1.f - facg;
			return 1.f - (facm + fact * (1.f - tex)) * (1.f - out);

		case MN_SUB:
			fact = -fact;
		case MN_ADD:
			return fact * tex + out;

		case MN_DIV:
			if(tex == 0.f) return 0.f;
			return facm * out + fact * out / tex;

		case MN_DIFF:
			return facm * out + fact * std::fabs(tex - out);

		case MN_DARK:
		{
			float col = fact * tex;
			if(col < out) return col;
			return out;
		}

		case MN_LIGHT:
		{
			float col = fact * tex;
			if(col > out) return col;
			return out;
		}

		default:
		case MN_MIX:
			return fact * tex + facm * out;
	}
}


__END_YAFRAY

#endif // Y_SHADER_H
