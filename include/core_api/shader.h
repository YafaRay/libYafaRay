
#ifndef Y_SHADER_H
#define Y_SHADER_H

#include <yafray_config.h>

#include "scene.h"
#include "ray.h"
#include "surface.h"


__BEGIN_YAFRAY

class paraMap_t;
class shaderNode_t;

struct nodeResult_t
{
	nodeResult_t(colorA_t color, CFLOAT fval): col(color), f(fval) {}
	colorA_t col;
	CFLOAT f;
};

class nodeStack_t
{
	public:
		nodeStack_t(void *data){ dat = (nodeResult_t *)data; }
		const nodeResult_t& operator()( unsigned int ID) const
		{
			return *(dat+ID);
		}
		nodeResult_t& operator[]( unsigned int ID)
		{
			return *(dat+ID);
		}
	private:
		nodeResult_t *dat;
};

class YAFRAYCORE_EXPORT nodeFinder_t
{
	public:
		virtual const shaderNode_t* operator()(const std::string &name) const = 0;
		virtual ~nodeFinder_t() {};
};

enum mix_modes{ MN_MIX=0, MN_ADD, MN_MULT, MN_SUB, MN_SCREEN, MN_DIV, MN_DIFF, MN_DARK, MN_LIGHT, MN_OVERLAY };

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
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const = 0;
		/*! evaluate the shader for given surface point and directions; otherwise same behavious than the other eval.
			Should only be called when the node returns true on isViewDependant() */
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const = 0;
		/*! evaluate the shader partial derivatives for given surface point (e.g. for bump mapping);
			attention: uses color component of node stack to store result, so only use a stack for either eval or evalDeriv! */
		virtual void evalDerivative(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
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
		virtual bool getDependencies(std::vector<const shaderNode_t*> &dep) const { return false; }
		/*! get the color value calculated on eval */
		colorA_t getColor(const nodeStack_t &stack)const { return stack(this->ID).col; }
		/*! get the scalar value calculated on eval */
		CFLOAT getScalar(const nodeStack_t &stack)const { return stack(this->ID).f; }
		//! get the (approximate) partial derivatives df/dNU and df/dNV
		/*! where f is the shader function, and NU/NV/N build the shading coordinate system
			\param du df/dNU
			\param dv df/dNV	*/
		void getDerivative(const nodeStack_t &stack, CFLOAT &du, CFLOAT &dv)const
			{ du = stack(this->ID).col.R; dv = stack(this->ID).col.G; }
		/* virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv)const {du=0.f, dv=0.f;} */
		unsigned int ID;
};

	

__END_YAFRAY

#endif // Y_SHADER_H
