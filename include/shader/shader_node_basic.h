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

#ifndef YAFARAY_SHADER_NODE_BASIC_H
#define YAFARAY_SHADER_NODE_BASIC_H

#include "shader/shader_node.h"
#include "texture/texture.h"
#include "geometry/matrix4.h"

BEGIN_YAFARAY

class TextureMapperNode final : public ShaderNode
{
	public:
		static ShaderNode *factory(const ParamMap &params, const Scene &scene);

	private:
		enum Coords : int { Uv, Global, Orco, Transformed, Normal, Reflect, Window, Stick, Stress, Tangent };
		enum Projection : int { Plain = 0, Cube, Tube, Sphere };

		TextureMapperNode(const Texture *texture) : tex_(texture) { }
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const override;
		virtual void evalDerivative(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const override;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) override { return true; };
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;

		void setup();
		void getCoords(Point3 &texpt, Vec3 &ng, const SurfacePoint &sp, const RenderData &render_data) const;
		Point3 doMapping(const Point3 &p, const Vec3 &n) const ;

		Coords coords_;
		Projection projection_;
		int map_x_ = 1, map_y_ = 2, map_z_ = 3; //!< axis mapping; 0:set to zero, 1:x, 2:y, 3:z
		Point3 p_du_, p_dv_, p_dw_;
		float d_u_, d_v_, d_w_, d_uv_;
		const Texture *tex_ = nullptr;
		Vec3 scale_;
		Vec3 offset_;
		float bump_str_ = 0.02f;
		bool do_scalar_ = true;
		Matrix4 mtx_;
};

class ValueNode final : public ShaderNode
{
	public:
		static ShaderNode *factory(const ParamMap &params, const Scene &scene);

	private:
		ValueNode(Rgba col, float val): color_(col), value_(val) {}
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const override;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) override { return true; };

		Rgba color_;
		float value_;
};

class MixNode : public ShaderNode
{
	public:
		static ShaderNode *factory(const ParamMap &params, const Scene &scene);

	protected:
		MixNode();
		void getInputs(NodeStack &stack, Rgba &cin_1, Rgba &cin_2, float &fin_1, float &fin_2, float &f_2) const;

	private:
		MixNode(float val);
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const override;
		virtual bool configInputs(const ParamMap &params, const NodeFinder &find) override;
		virtual bool getDependencies(std::vector<const ShaderNode *> &dep) const override;

		Rgba col_1_, col_2_;
		float val_1_, val_2_, cfactor_;
		const ShaderNode *input_1_ = nullptr;
		const ShaderNode *input_2_ = nullptr;
		const ShaderNode *factor_ = nullptr;
};

END_YAFARAY

#endif // YAFARAY_SHADER_NODE_BASIC_H
