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

#include "shader/shader_node_basic.h"

#include "camera/camera.h"
#include "common/param.h"
#include "geometry/surface.h"
#include "math/interpolation.h"
#include "render/render_data.h"
#include "shader/shader_node_layer.h"
#include "texture/texture_image.h"
#include <cmath>

BEGIN_YAFARAY

void TextureMapperNode::setup()
{
	if(tex_->discrete())
	{
		const auto [u, v, w] = tex_->resolution();
		d_u_ = 1.f / static_cast<float>(u);
		d_v_ = 1.f / static_cast<float>(v);
		if(tex_->isThreeD()) d_w_ = 1.f / static_cast<float>(w);
		else d_w_ = 0.f;
	}
	else
	{
		const float step = 0.0002f;
		d_u_ = d_v_ = d_w_ = step;
	}

	p_du_ = {d_u_, 0.f, 0.f};
	p_dv_ = {0.f, d_v_, 0.f};
	p_dw_ = {0.f, 0.f, d_w_};

	bump_str_ /= scale_.length();

	if(!tex_->isNormalmap())
		bump_str_ /= 100.0f;
}

// Map the texture to a cylinder
Point3 TextureMapperNode::tubeMap(const Point3 &p)
{
	Point3 res;
	res.y() = p.z();
	const float d = p.x() * p.x() + p.y() * p.y();
	if(d > 0.f)
	{
		res.z() = 1.f / math::sqrt(d);
		res.x() = -std::atan2(p.x(), p.y()) * math::div_1_by_pi<>;
	}
	else res.x() = res.z() = 0.f;
	return res;
}

// Map the texture to a sphere
Point3 TextureMapperNode::sphereMap(const Point3 &p)
{
	Point3 res{0.f, 0.f, 0.f};
	const float d = p.x() * p.x() + p.y() * p.y() + p.z() * p.z();
	if(d > 0.f)
	{
		res.z() = math::sqrt(d);
		if((p.x() != 0.f) && (p.y() != 0.f)) res.x() = -std::atan2(p.x(), p.y()) * math::div_1_by_pi<>;
		res.y() = 1.f - 2.f * (math::acos(p.z() / res.z()) * math::div_1_by_pi<>);
	}
	return res;
}

// Map the texture to a cube
Point3 TextureMapperNode::cubeMap(const Point3 &p, const Vec3 &n)
{
	const std::array<std::array<Axis, 3>, 3> ma {{ {Axis::Y, Axis::Z, Axis::X}, {Axis::X, Axis::Z, Axis::Y}, {Axis::X, Axis::Y, Axis::Z} }};
	// int axis = std::abs(n.x) > std::abs(n.y) ? (std::abs(n.x) > std::abs(n.z) ? 0 : 2) : (std::abs(n.y) > std::abs(n.z) ? 1 : 2);
	// no functionality changes, just more readable code
	Axis axis;
	if(std::abs(n.z()) >= std::abs(n.x()) && std::abs(n.z()) >= std::abs(n.y())) axis = Axis::Z;
	else if(std::abs(n.y()) >= std::abs(n.x()) && std::abs(n.y()) >= std::abs(n.z())) axis = Axis::Y;
	else axis = Axis::X;
	const auto axis_id = axis::getId(axis);
	return { p[ma[axis_id][0]], p[ma[axis_id][1]], p[ma[axis_id][2]] };
}

// Map the texture to a plane, but it should not be used by now as it does nothing, it's just for completeness' sake
Point3 TextureMapperNode::flatMap(const Point3 &p)
{
	return p;
}

Point3 TextureMapperNode::doMapping(const Point3 &p, const Vec3 &n) const
{
	Point3 texpt{p};
	// Texture coordinates standardized, if needed, to -1..1
	switch(coords_)
	{
		case Coords::Uv: texpt = {2.f * texpt.x() - 1.f, 2.f * texpt.y() - 1.f, texpt.z()}; break;
		default: break;
	}
	// Texture axis mapping
	const std::array<float, 4> texmap {0.f, texpt.x(), texpt.y(), texpt.z() };
	texpt.x() = texmap[map_x_];
	texpt.y() = texmap[map_y_];
	texpt.z() = texmap[map_z_];
	// Texture coordinates mapping
	switch(projection_)
	{
		case Projection::Tube: texpt = tubeMap(texpt); break;
		case Projection::Sphere: texpt = sphereMap(texpt); break;
		case Projection::Cube: texpt = cubeMap(texpt, n); break;
		case Projection::Plain: // texpt = flatmap(texpt); break;
		default: break;
	}
	// Texture scale and offset
	texpt = Point3::mult(texpt, scale_) + offset_;
	return texpt;
}

Point3 TextureMapperNode::evalUv(const SurfacePoint &sp)
{
	return { sp.u_, sp.v_, 0.f };
}

std::pair<Point3, Vec3> TextureMapperNode::getCoords(const SurfacePoint &sp, const Camera *camera) const
{
	switch(coords_)
	{
		case Coords::Uv: return { evalUv(sp), sp.ng_ };
		case Coords::Orco: return { sp.orco_p_, sp.orco_ng_ };
		case Coords::Transformed: return { mtx_ * sp.p_, mtx_ * sp.ng_ };  // apply 4x4 matrix of object for mapping also to true surface normals
		case Coords::Window: return { camera->screenproject(sp.p_), sp.ng_ };
		case Coords::Normal: {
			const auto [cam_x, cam_y, cam_z] = camera->getAxes();
			return {{sp.n_ * cam_x, -sp.n_ * cam_y, 0.f}, sp.ng_};
			}
		case Coords::Stick: // Not implemented yet use GLOB
		case Coords::Stress: // Not implemented yet use GLOB
		case Coords::Tangent: // Not implemented yet use GLOB
		case Coords::Reflect: // Not implemented yet use GLOB
		case Coords::Global: // GLOB mapped as default
		default: return { sp.p_, sp.ng_ };
	}
}

void TextureMapperNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	std::unique_ptr<const MipMapParams> mip_map_params;
	auto [texpt, ng] = getCoords(sp, camera);
	if((tex_->getInterpolationType() == InterpolationType::Trilinear || tex_->getInterpolationType() == InterpolationType::Ewa) && sp.differentials_)
	{
		const Point3 texpt_orig {texpt};
		texpt = doMapping(texpt_orig, ng);
		if(coords_ == Coords::Uv && sp.has_uv_)
		{
			float du_dx = 0.f, dv_dx = 0.f;
			float du_dy = 0.f, dv_dy = 0.f;
			sp.getUVdifferentials(du_dx, dv_dx, du_dy, dv_dy);
			const Point3 texpt_diffx{1.0e+2f * (doMapping(texpt_orig + 1.0e-2f * Point3{du_dx, dv_dx, 0.f}, ng) - texpt)};
			const Point3 texpt_diffy{1.0e+2f * (doMapping(texpt_orig + 1.0e-2f * Point3{du_dy, dv_dy, 0.f}, ng) - texpt)};
			mip_map_params = std::make_unique<const MipMapParams>(texpt_diffx.x(), texpt_diffx.y(), texpt_diffy.x(), texpt_diffy.y());
		}
	}
	else texpt = doMapping(texpt, ng);
	node_tree_data[getId()] = {
			tex_->getColor(texpt, mip_map_params.get()),
			do_scalar_ ? tex_->getFloat(texpt, mip_map_params.get()) : 0.f
	};
}

// Normal perturbation

void TextureMapperNode::evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	float du = 0.0f, dv = 0.0f;
	auto [texpt, ng] = getCoords(sp, camera);
	if(tex_->discrete() && sp.has_uv_ && coords_ == Coords::Uv)
	{
		texpt = doMapping(texpt, ng);
		Vec3 norm;

		if(tex_->isNormalmap())
		{
			// Get color from normal map texture
			const Rgba color = tex_->getRawColor(texpt);
			// Assign normal map RGB colors to vector norm
			norm = { color.getR(), color.getG(), color.getB() };
			norm = (2.f * norm) - Vec3{1.f}; //FIXME DAVID: does the Vec3 portion make sense?

			// Convert norm into shading space
			du = norm * sp.ds_du_;
			dv = norm * sp.ds_dv_;
		}
		else
		{
			const Point3 i_0{texpt - p_du_};
			const Point3 i_1{texpt + p_du_};
			const Point3 j_0{texpt - p_dv_};
			const Point3 j_1{texpt + p_dv_};
			const float dfdu = (tex_->getFloat(i_0) - tex_->getFloat(i_1)) / d_u_;
			const float dfdv = (tex_->getFloat(j_0) - tex_->getFloat(j_1)) / d_v_;

			// now we got the derivative in UV-space, but need it in shading space:
			Vec3 vec_u{sp.ds_du_};
			Vec3 vec_v{sp.ds_dv_};
			vec_u.z() = dfdu;
			vec_v.z() = dfdv;
			// now we have two vectors NU/NV/df; Solve plane equation to get 1/0/df and 0/1/df (i.e. dNUdf and dNVdf)
			norm = vec_u ^ vec_v;
		}

		norm.normalize();

		if(std::abs(norm.z()) > 1e-30f)
		{
			const float nf = 1.f / norm.z() * bump_str_; // normalizes z to 1, why?
			du = norm.x() * nf;
			dv = norm.y() * nf;
		}
		else du = dv = 0.f;
	}
	else
	{
		if(tex_->isNormalmap())
		{
			texpt = doMapping(texpt, ng);

			// Get color from normal map texture
			const Rgba color = tex_->getRawColor(texpt);

			// Assign normal map RGB colors to vector norm
			Vec3 norm { color.getR(), color.getG(), color.getB() };
			norm = (2.f * norm) - Vec3{1.f}; //FIXME DAVID: does the Vec3 portion make sense?

			// Convert norm into shading space
			du = norm * sp.ds_du_;
			dv = norm * sp.ds_dv_;

			norm.normalize();

			if(std::abs(norm.z()) > 1e-30f)
			{
				const float nf = 1.f / norm.z() * bump_str_; // normalizes z to 1, why?
				du = norm.x() * nf;
				dv = norm.y() * nf;
			}
			else du = dv = 0.f;
		}
		else
		{
			// no uv coords -> procedurals usually, this mapping only depends on NU/NV which is fairly arbitrary
			// weird things may happen when objects are rotated, i.e. incorrect bump change
			const Point3 i_0{doMapping(texpt - d_u_ * sp.nu_, ng)};
			const Point3 i_1{doMapping(texpt + d_u_ * sp.nu_, ng)};
			const Point3 j_0{doMapping(texpt - d_v_ * sp.nv_, ng)};
			const Point3 j_1{doMapping(texpt + d_v_ * sp.nv_, ng)};

			du = (tex_->getFloat(i_0) - tex_->getFloat(i_1)) / d_u_;
			dv = (tex_->getFloat(j_0) - tex_->getFloat(j_1)) / d_v_;
			du *= bump_str_;
			dv *= bump_str_;

			if(coords_ != Coords::Uv)
			{
				du = -du;
				dv = -dv;
			}
		}
	}
	node_tree_data[getId()] = { {du, dv, 0.f, 0.f}, 0.f };
}

ShaderNode * TextureMapperNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	const Texture *tex = nullptr;
	std::string texname, option;
	Coords tc = Coords::Global;
	Projection projection = Projection::Plain;
	float bump_str = 1.f;
	bool scalar = true;
	std::array<int, 3> map { 1, 2, 3 };
	Point3 offset{0.f, 0.f, 0.f}, scale{1.f, 1.f, 1.f};
	Matrix4 mtx{1.f};
	if(!params.getParam("texture", texname))
	{
		logger.logError("TextureMapper: No texture given for texture mapper!");
		return nullptr;
	}
	tex = scene.getTexture(texname);
	if(!tex)
	{
		logger.logError("TextureMapper: texture '", texname, "' does not exist!");
		return nullptr;
	}
	auto tm = new TextureMapperNode(tex);
	if(params.getParam("texco", option))
	{
		if(option == "uv") tc = Coords::Uv;
		else if(option == "global") tc = Coords::Global;
		else if(option == "orco") tc = Coords::Orco;
		else if(option == "transformed") tc = Coords::Transformed;
		else if(option == "window") tc = Coords::Window;
		else if(option == "normal") tc = Coords::Normal;
		else if(option == "reflect") tc = Coords::Reflect;
		else if(option == "stick") tc = Coords::Stick;
		else if(option == "stress") tc = Coords::Stress;
		else if(option == "tangent") tc = Coords::Tangent;
	}
	if(params.getParam("mapping", option) && tex->discrete())
	{
		if(option == "plain") projection = Projection::Plain;
		else if(option == "cube") projection = Projection::Cube;
		else if(option == "tube") projection = Projection::Tube;
		else if(option == "sphere") projection = Projection::Sphere;
	}
	params.getParam("transform", mtx);
	params.getParam("scale", scale);
	params.getParam("offset", offset);
	params.getParam("do_scalar", scalar);
	params.getParam("bump_strength", bump_str);
	params.getParam("proj_x", map[0]);
	params.getParam("proj_y", map[1]);
	params.getParam("proj_z", map[2]);
	for(int &map_entry : map) map_entry = std::min(3, std::max(0, map_entry));
	tm->coords_ = tc;
	tm->projection_ = projection;
	tm->map_x_ = map[0];
	tm->map_y_ = map[1];
	tm->map_z_ = map[2];
	tm->scale_ = scale;
	tm->offset_ = 2 * offset;	// Offset need to be doubled due to -1..1 texture standardized wich results in a 2 wide width/height
	tm->do_scalar_ = scalar;
	tm->bump_str_ = bump_str;
	tm->mtx_ = mtx;
	tm->setup();
	return tm;
}

/* ==========================================
/  The most simple node you could imagine...
/  ========================================== */

void ValueNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	node_tree_data[getId()] = { color_, value_ };
}

ShaderNode * ValueNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Rgb col(1.f);
	float alpha = 1.f;
	float val = 1.f;
	params.getParam("color", col);
	params.getParam("alpha", alpha);
	params.getParam("scalar", val);
	return new ValueNode(Rgba(col, alpha), val);
}

/* ==========================================
/  A simple mix node, could be used to derive other math nodes
/ ========================================== */

void MixNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	const float factor = node_factor_ ? node_factor_->getScalar(node_tree_data) : factor_;
	const Rgba col_1 = node_in_1_ ? node_in_1_->getColor(node_tree_data) : col_1_;
	const float f_1 = node_in_1_ ? node_in_1_->getScalar(node_tree_data) : val_1_;
	const Rgba col_2 = node_in_2_ ? node_in_2_->getColor(node_tree_data) : col_2_;
	const float f_2 = node_in_2_ ? node_in_2_->getScalar(node_tree_data) : val_2_;
	node_tree_data[getId()] = {
			math::lerp(col_1, col_2, factor),
			math::lerp(f_1, f_2, factor)
	};
}

bool MixNode::configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find)
{
	std::string name;
	if(params.getParam("input1", name))
	{
		node_in_1_ = find(name);
		if(!node_in_1_)
		{
			logger.logError("MixNode: Couldn't get input1 ", name);
			return false;
		}
	}
	else if(!params.getParam("color1", col_1_))
	{
		logger.logError("MixNode: Color1 not set");
		return false;
	}

	if(params.getParam("input2", name))
	{
		node_in_2_ = find(name);
		if(!node_in_2_)
		{
			logger.logError("MixNode: Couldn't get input2 ", name);
			return false;
		}
	}
	else if(!params.getParam("color2", col_2_))
	{
		logger.logError("MixNode: Color2 not set");
		return false;
	}

	if(params.getParam("factor", name))
	{
		node_factor_ = find(name);
		if(!node_factor_)
		{
			logger.logError("MixNode: Couldn't get factor ", name);
			return false;
		}
	}
	else if(!params.getParam("value", factor_))
	{
		logger.logError("MixNode: Value not set");
		return false;
	}

	return true;
}

std::vector<const ShaderNode *> MixNode::getDependencies() const
{
	std::vector<const ShaderNode *> dependencies;
	if(node_in_1_) dependencies.emplace_back(node_in_1_);
	if(node_in_2_) dependencies.emplace_back(node_in_2_);
	if(node_factor_) dependencies.emplace_back(node_factor_);
	return dependencies;
}

MixNode::Inputs MixNode::getInputs(const NodeTreeData &node_tree_data) const
{
	const float factor = node_factor_ ? node_factor_->getScalar(node_tree_data) : factor_;
	NodeResult in_1 = node_in_1_ ?
		NodeResult{node_in_1_->getColor(node_tree_data), node_in_1_->getScalar(node_tree_data)} :
		NodeResult{col_1_, val_1_};
	NodeResult in_2 = node_in_2_ ?
		NodeResult{node_in_2_->getColor(node_tree_data), node_in_2_->getScalar(node_tree_data)} :
		NodeResult{col_2_, val_2_};
	return {std::move(in_1), std::move(in_2), factor};
}

class AddNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			inputs.in_1_.col_ += inputs.factor_ * inputs.in_2_.col_;
			inputs.in_1_.f_ += inputs.factor_ * inputs.in_2_.f_;
			node_tree_data[getId()] = std::move(inputs.in_1_);
		}
};

class MultNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			inputs.in_1_.col_ *= math::lerp(Rgba{1.f}, inputs.in_2_.col_, inputs.factor_);
			inputs.in_1_.f_ *= math::lerp(1.f, inputs.in_2_.f_, inputs.factor_);
			node_tree_data[getId()] = std::move(inputs.in_1_);
		}
};

class SubNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			inputs.in_1_.col_ -= inputs.factor_ * inputs.in_2_.col_;
			inputs.in_1_.f_ -= inputs.factor_ * inputs.in_2_.f_;
			node_tree_data[getId()] = std::move(inputs.in_1_);
		}
};

class ScreenNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			const float factor_reversed = 1.f - inputs.factor_;
			const Rgba color { Rgba{1.f} - (Rgba{factor_reversed} + inputs.factor_ * (Rgba{1.f} - inputs.in_2_.col_)) * (Rgba{1.f} - inputs.in_1_.col_) };
			const float scalar = 1.f - (factor_reversed + inputs.factor_ * (1.f - inputs.in_2_.f_)) * (1.f - inputs.in_1_.f_);
			node_tree_data[getId()] = { color, scalar };
		}
};

class DiffNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			const auto mix_function = [](float val_1, float val_2, float factor) -> float {
				return math::lerp(val_1, std::abs(val_1 - val_2), factor);
			};
			inputs.in_1_.col_.r_ = mix_function(inputs.in_1_.col_.r_, inputs.in_2_.col_.r_, inputs.factor_);
			inputs.in_1_.col_.g_ = mix_function(inputs.in_1_.col_.g_, inputs.in_2_.col_.g_, inputs.factor_);
			inputs.in_1_.col_.b_ = mix_function(inputs.in_1_.col_.b_, inputs.in_2_.col_.b_, inputs.factor_);
			inputs.in_1_.col_.a_ = mix_function(inputs.in_1_.col_.a_, inputs.in_2_.col_.a_, inputs.factor_);
			inputs.in_1_.f_ = mix_function(inputs.in_1_.f_, inputs.in_2_.f_, inputs.factor_);
			node_tree_data[getId()] = std::move(inputs.in_1_);
		}
};

class DarkNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			inputs.in_2_.col_ *= inputs.factor_;
			if(inputs.in_2_.col_.r_ < inputs.in_1_.col_.r_) inputs.in_1_.col_.r_ = inputs.in_2_.col_.r_;
			if(inputs.in_2_.col_.g_ < inputs.in_1_.col_.g_) inputs.in_1_.col_.g_ = inputs.in_2_.col_.g_;
			if(inputs.in_2_.col_.b_ < inputs.in_1_.col_.b_) inputs.in_1_.col_.b_ = inputs.in_2_.col_.b_;
			if(inputs.in_2_.col_.a_ < inputs.in_1_.col_.a_) inputs.in_1_.col_.a_ = inputs.in_2_.col_.a_;
			inputs.in_2_.f_ *= inputs.factor_;
			if(inputs.in_2_.f_ < inputs.in_1_.f_) inputs.in_1_.f_ = inputs.in_2_.f_;
			node_tree_data[getId()] = std::move(inputs.in_1_);
		}
};

class LightNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			inputs.in_2_.col_ *= inputs.factor_;
			if(inputs.in_2_.col_.r_ > inputs.in_1_.col_.r_) inputs.in_1_.col_.r_ = inputs.in_2_.col_.r_;
			if(inputs.in_2_.col_.g_ > inputs.in_1_.col_.g_) inputs.in_1_.col_.g_ = inputs.in_2_.col_.g_;
			if(inputs.in_2_.col_.b_ > inputs.in_1_.col_.b_) inputs.in_1_.col_.b_ = inputs.in_2_.col_.b_;
			if(inputs.in_2_.col_.a_ > inputs.in_1_.col_.a_) inputs.in_1_.col_.a_ = inputs.in_2_.col_.a_;
			inputs.in_2_.f_ *= inputs.factor_;
			if(inputs.in_2_.f_ > inputs.in_1_.f_) inputs.in_1_.f_ = inputs.in_2_.f_;
			node_tree_data[getId()] = std::move(inputs.in_1_);
		}
};

class OverlayNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			Inputs inputs = getInputs(node_tree_data);
			const auto overlay_function = [](float val_1, float val_2, float factor) -> float {
				if(val_1 < 0.5f) return val_1 * ((1.f - factor) + 2.f * factor * val_2);
				else return 1.f - ((1.f - factor) + 2.f * factor * (1.f - val_2)) * (1.f - val_1);
			};
			const Rgba color {
					overlay_function(inputs.in_1_.col_.r_, inputs.in_2_.col_.r_, inputs.factor_),
					overlay_function(inputs.in_1_.col_.g_, inputs.in_2_.col_.g_, inputs.factor_),
					overlay_function(inputs.in_1_.col_.b_, inputs.in_2_.col_.b_, inputs.factor_),
					overlay_function(inputs.in_1_.col_.a_, inputs.in_2_.col_.a_, inputs.factor_)
			};
			const float scalar = overlay_function(inputs.in_1_.f_, inputs.in_2_.f_, inputs.factor_);
			node_tree_data[getId()] = {color, scalar};
		}
};

ShaderNode * MixNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	float cfactor = 0.5f;
	std::string blend_mode;
	params.getParam("cfactor", cfactor);
	params.getParam("blend_mode", blend_mode);

	if(blend_mode == "add") return new AddNode();
	else if(blend_mode == "multiply") return new MultNode();
	else if(blend_mode == "subtract") return new SubNode();
	else if(blend_mode == "screen") return new ScreenNode();
	//else if(blend_mode == "divide") return new DivNode(); //FIXME Why isn't there a DivNode?
	else if(blend_mode == "difference") return new DiffNode();
	else if(blend_mode == "darken") return new DarkNode();
	else if(blend_mode == "lighten") return new LightNode();
	else if(blend_mode == "overlay") return new OverlayNode();
	//else if(blend_mode == "mix")
	else return new MixNode(cfactor);
}

END_YAFARAY
