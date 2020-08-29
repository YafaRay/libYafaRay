#pragma once

#ifndef YAFARAY_LAYERNODE_H
#define YAFARAY_LAYERNODE_H

#include <core_api/shader.h>
#include <core_api/texture.h>
#include <core_api/environment.h>

BEGIN_YAFRAY

// texture flag
#define TXF_RGBTOINT    1
#define TXF_STENCIL     2
#define TXF_NEGATIVE    4
#define TXF_ALPHAMIX    8

class LayerNode: public ShaderNode
{
	public:
		LayerNode(unsigned tflag, float col_fac, float var_fac, float def_val, Rgba def_col, MixModes mmod);
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const;
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const;
		virtual void evalDerivative(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const;
		virtual bool isViewDependant() const;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find);
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;
		virtual bool getDependencies(std::vector<const ShaderNode *> &dep) const;
		static ShaderNode *factory(const ParamMap &params, RenderEnvironment &render);
	protected:
		const ShaderNode *input_, *upper_layer_;
		unsigned int texflag_;
		float colfac_;
		float valfac_;
		float default_val_, upper_val_;
		Rgba default_col_, upper_col_;
		MixModes mode_;
		bool do_color_, do_scalar_, color_input_, use_alpha_;
};



END_YAFRAY

#endif // YAFARAY_LAYERNODE_H
