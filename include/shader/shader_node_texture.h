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

#ifndef LIBYAFARAY_SHADER_NODE_TEXTURE_H
#define LIBYAFARAY_SHADER_NODE_TEXTURE_H

#include "shader/shader_node.h"
#include "texture/texture.h"
#include "geometry/matrix.h"

namespace yafaray {

class TextureMapperNode final : public ShaderNode
{
		using ThisClassType_t = TextureMapperNode; using ParentClassType_t = ShaderNode;

	public:
		inline static std::string getClassName() { return "TextureMapperNode"; }
		static std::pair<std::unique_ptr<ShaderNode>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		explicit TextureMapperNode(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Texture *texture);

	private:
		struct Coords : Enum<Coords>
		{
			using Enum::Enum;
			enum : ValueType_t { Uv, Global, Orco, Transformed, Normal, Reflect, Window, Stick, Stress, Tangent };
			inline static const EnumMap<ValueType_t> map_{{
					{"uv", Uv, ""},
					{"global", Global, ""},
					{"orco", Orco, ""},
					{"transformed", Transformed, ""},
					{"window", Window, ""},
					{"normal", Normal, ""},
					{"reflect", Reflect, ""},
					{"stick", Stick, ""},
					{"stress", Stress, ""},
					{"tangent", Tangent, ""},
				}};
		};
		struct Projection : Enum<Projection>
		{
			using Enum::Enum;
			enum : ValueType_t { Plain, Cube, Tube, Sphere };
			inline static const EnumMap<ValueType_t> map_{{
					{"plain", Plain, ""},
					{"cube", Cube, ""},
					{"tube", Tube, ""},
					{"sphere", Sphere, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Texture; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(std::string, texture_, "", "texture", "");
			PARAM_DECL(Matrix4f, transform_, Matrix4f{1.f}, "transform", "");
			PARAM_DECL(Vec3f, scale_, Vec3f{1.f}, "scale", "");
			PARAM_DECL(Vec3f, offset_, Vec3f{0.f}, "offset", "");
			PARAM_DECL(bool , do_scalar_, true, "do_scalar", "");
			PARAM_DECL(float, bump_strength_, 1.f, "bump_strength", "");
			PARAM_DECL(int, proj_x_, 1, "proj_x", "axis mapping; 0:set to zero, 1:x, 2:y, 3:z");
			PARAM_DECL(int, proj_y_, 2, "proj_y", "axis mapping; 0:set to zero, 1:x, 2:y, 3:z");
			PARAM_DECL(int, proj_z_, 3, "proj_z", "axis mapping; 0:set to zero, 1:x, 2:y, 3:z");
			PARAM_ENUM_DECL(Coords, texco_, Coords::Global, "texco", "");
			PARAM_ENUM_DECL(Projection, mapping_, Projection::Plain, "mapping", "");
		} params_;
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		void evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override;
		bool configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find) override { return true; };
		//virtual void getDerivative(const surfacePoint_t &sp, float &du, float &dv) const;
		std::pair<Point3f, Vec3f> getCoords(const SurfacePoint &sp, const Camera *camera) const;
		Point3f doMapping(const Point3f &p, const Vec3f &n) const;
		static Point3f tubeMap(const Point3f &p);
		static Point3f sphereMap(const Point3f &p);
		static Point3f cubeMap(const Point3f &p, const Vec3f &n);
		static Point3f flatMap(const Point3f &p);

		const int map_x_{std::max(0, std::min(3, params_.proj_x_))};
		const int map_y_{std::max(0, std::min(3, params_.proj_y_))};
		const int map_z_{std::max(0, std::min(3, params_.proj_z_))};
		Point3f p_du_, p_dv_, p_dw_;
		float d_u_, d_v_, d_w_;
		const Texture *tex_ = nullptr;
		float bump_strength_{params_.bump_strength_};
		const Vec3f offset_{2.f * params_.offset_}; //Offset need to be doubled due to -1..1 texture standardized which results in a 2 wide width/height
};

} //namespace yafaray

#endif // LIBYAFARAY_SHADER_NODE_TEXTURE_H
