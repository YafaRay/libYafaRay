
#ifndef Y_BASICNODES_H
#define Y_BASICNODES_H

#include <core_api/shader.h>
#include <core_api/texture.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY

enum TEX_COORDS {TXC_UV, TXC_GLOB, TXC_ORCO, TXC_TRAN, TXC_NOR, TXC_REFL, TXC_WIN, TXC_STICK, TXC_STRESS, TXC_TAN };
enum TEX_PROJ {TXP_PLAIN=0, TXP_CUBE, TXP_TUBE, TXP_SPHERE};

class textureMapper_t: public shaderNode_t
{
	public:
		textureMapper_t(const texture_t *texture);
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const;
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const;
		virtual void evalDerivative(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const;
		virtual bool configInputs(const paraMap_t &params, const nodeFinder_t &find) { return true; };
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv)const;
		static shaderNode_t* factory(const paraMap_t &params,renderEnvironment_t &render);
	protected:
		void setup();
		void getCoords(point3d_t &texpt, vector3d_t &Ng, const surfacePoint_t &sp, const renderState_t &state) const;
		point3d_t doMapping(const point3d_t &p, const vector3d_t &N)const;
		TEX_COORDS 	tex_coords;
		TEX_PROJ tex_maptype;
		int map_x, map_y, map_z; //!< axis mapping; 0:set to zero, 1:x, 2:y, 3:z
		point3d_t pDU, pDV, pDW;
		float dU, dV, dW;
		const texture_t *tex;
		vector3d_t scale;
		vector3d_t offset;
		float bumpStr;
		bool doScalar;
		matrix4x4_t mtx;
};

class valueNode_t: public shaderNode_t
{
	public:
		valueNode_t(colorA_t col, float val): color(col), value(val) {}
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const;
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const;
		virtual bool configInputs(const paraMap_t &params, const nodeFinder_t &find) { return true; };
		static shaderNode_t* factory(const paraMap_t &params,renderEnvironment_t &render);
	protected:
		colorA_t color;
		float value;
};

class mixNode_t: public shaderNode_t
{
	public:
		mixNode_t();
		mixNode_t(float val);
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const;
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const;
		virtual bool configInputs(const paraMap_t &params, const nodeFinder_t &find);
		virtual bool getDependencies(std::vector<const shaderNode_t*> &dep) const;
		static shaderNode_t* factory(const paraMap_t &params,renderEnvironment_t &render);
	protected:
		void getInputs(nodeStack_t &stack, colorA_t &cin1, colorA_t &cin2, CFLOAT &fin1, CFLOAT &fin2, CFLOAT &f2) const;
		colorA_t col1, col2;
		float val1, val2, cfactor;
		const shaderNode_t *input1;
		const shaderNode_t *input2;
		const shaderNode_t *factor;
};

inline void mixNode_t::getInputs(nodeStack_t &stack, colorA_t &cin1, colorA_t &cin2, CFLOAT &fin1, CFLOAT &fin2, CFLOAT &f2) const
{
	f2 = (factor) ? factor->getScalar(stack) : cfactor;
	if(input1)
	{
		cin1 = input1->getColor(stack);
		fin1 = input1->getScalar(stack);
	}
	else
	{
		cin1 = col1;
		fin1 = val1;
	}
	if(input2)
	{
		cin2 = input2->getColor(stack);
		fin2 = input2->getScalar(stack);
	}
	else
	{
		cin2 = col2;
		fin2 = val2;
	}
}


__END_YAFRAY

#endif // Y_BASICNODES_H
