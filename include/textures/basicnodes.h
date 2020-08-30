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

#ifndef YAFARAY_BASICNODES_H
#define YAFARAY_BASICNODES_H

#include <core_api/shader.h>
#include <core_api/texture.h>
#include <core_api/environment.h>
#include <core_api/matrix4.h>

BEGIN_YAFRAY

class TextureMapperNode final : public ShaderNode
{
	public:
		static ShaderNode *factory(const ParamMap &params, RenderEnvironment &render);

	private:
		enum Coords : int { Uv, Glob, Orco, Tran, Nor, Refl, Win, Stick, Stress, Tan };
		enum Projection : int { Plain = 0, Cube, Tube, Sphere };

		TextureMapperNode(const Texture *texture);
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const override;
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const override;
		virtual void evalDerivative(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const override;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) override { return true; };
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;

		void setup();
		void getCoords(Point3 &texpt, Vec3 &ng, const SurfacePoint &sp, const RenderState &state) const;
		Point3 doMapping(const Point3 &p, const Vec3 &n) const ;

		Coords coords_;
		Projection projection_;
		int map_x_, map_y_, map_z_; //!< axis mapping; 0:set to zero, 1:x, 2:y, 3:z
		Point3 p_du_, p_dv_, p_dw_;
		float d_u_, d_v_, d_w_, d_uv_;
		const Texture *tex_ = nullptr;
		Vec3 scale_;
		Vec3 offset_;
		float bump_str_;
		bool do_scalar_;
		Matrix4 mtx_;
};

class ValueNode final : public ShaderNode
{
	public:
		static ShaderNode *factory(const ParamMap &params, RenderEnvironment &render);

	private:
		ValueNode(Rgba col, float val): color_(col), value_(val) {}
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const override;
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const override;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) override { return true; };

		Rgba color_;
		float value_;
};

class MixNode : public ShaderNode
{
	public:
		static ShaderNode *factory(const ParamMap &params, RenderEnvironment &render);

	protected:
		MixNode();
		void getInputs(NodeStack &stack, Rgba &cin_1, Rgba &cin_2, float &fin_1, float &fin_2, float &f_2) const;

	private:
		MixNode(float val);
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp) const override;
		virtual void eval(NodeStack &stack, const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const override;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) override;
		virtual bool getDependencies(std::vector<const ShaderNode *> &dep) const override;

		Rgba col_1_, col_2_;
		float val_1_, val_2_, cfactor_;
		const ShaderNode *input_1_ = nullptr;
		const ShaderNode *input_2_ = nullptr;
		const ShaderNode *factor_ = nullptr;
};

inline void MixNode::getInputs(NodeStack &stack, Rgba &cin_1, Rgba &cin_2, float &fin_1, float &fin_2, float &f_2) const
{
	f_2 = (factor_) ? factor_->getScalar(stack) : cfactor_;
	if(input_1_)
	{
		cin_1 = input_1_->getColor(stack);
		fin_1 = input_1_->getScalar(stack);
	}
	else
	{
		cin_1 = col_1_;
		fin_1 = val_1_;
	}
	if(input_2_)
	{
		cin_2 = input_2_->getColor(stack);
		fin_2 = input_2_->getScalar(stack);
	}
	else
	{
		cin_2 = col_2_;
		fin_2 = val_2_;
	}
}


END_YAFRAY

#endif // YAFARAY_BASICNODES_H
