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

namespace yafaray {

class TextureMapperNode final : public ShaderNode
{
	public:
		static ShaderNode *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		enum class Coords : unsigned char { Uv, Global, Orco, Transformed, Normal, Reflect, Window, Stick, Stress, Tangent };
		enum class Projection : unsigned char { Plain, Cube, Tube, Sphere };

		explicit TextureMapperNode(const Texture *texture) : tex_(texture) { }
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		void evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override { return true; };
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;

		void setup();
		std::pair<Point3, Vec3> getCoords(const SurfacePoint &sp, const Camera *camera) const;
		Point3 doMapping(const Point3 &p, const Vec3 &n) const;
		static Point3 tubeMap(const Point3 &p);
		static Point3 sphereMap(const Point3 &p);
		static Point3 cubeMap(const Point3 &p, const Vec3 &n);
		static Point3 flatMap(const Point3 &p);

		int map_x_ = 1, map_y_ = 2, map_z_ = 3; //!< axis mapping; 0:set to zero, 1:x, 2:y, 3:z
		Point3 p_du_, p_dv_, p_dw_;
		float d_u_, d_v_, d_w_;
		const Texture *tex_ = nullptr;
		Vec3 scale_;
		Vec3 offset_;
		float bump_str_ = 0.02f;
		bool do_scalar_ = true;
		Matrix4 mtx_;
		Coords coords_;
		Projection projection_;
};

class ValueNode final : public ShaderNode
{
	public:
		static ShaderNode *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		ValueNode(Rgba col, float val): color_(col), value_(val) { }
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override { return true; };

		Rgba color_;
		float value_;
};

class MixNode : public ShaderNode
{
	public:
		static ShaderNode *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	protected:
		MixNode() = default;
		struct Inputs
		{
			Inputs(NodeResult input_1, NodeResult input_2, float factor) : in_1_{std::move(input_1)}, in_2_{std::move(input_2)}, factor_{factor} { }
			NodeResult in_1_;
			NodeResult in_2_;
			float factor_;
		};
		Inputs getInputs(const NodeTreeData &node_tree_data) const;

	private:
		explicit MixNode(float mix_factor) : factor_(mix_factor) { }
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override;
		std::vector<const ShaderNode *> getDependencies() const override;

		Rgba col_1_, col_2_;
		float val_1_, val_2_;
		float factor_ = 0.f;
		const ShaderNode *node_in_1_ = nullptr;
		const ShaderNode *node_in_2_ = nullptr;
		const ShaderNode *node_factor_ = nullptr;
};

} //namespace yafaray

#endif // YAFARAY_SHADER_NODE_BASIC_H
