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
#include "shader/shader_node_layer.h"
#include "camera/camera.h"
#include "common/param.h"
#include "geometry/surface.h"
#include "texture/texture_image.h"
#include "render/render_data.h"

BEGIN_YAFARAY

void TextureMapperNode::setup()
{
	if(tex_->discrete())
	{
		int u, v, w;
		tex_->resolution(u, v, w);
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
	res.y_ = p.z_;
	const float d = p.x_ * p.x_ + p.y_ * p.y_;
	if(d > 0.f)
	{
		res.z_ = 1.f / math::sqrt(d);
		res.x_ = -atan2(p.x_, p.y_) * math::div_1_by_pi;
	}
	else res.x_ = res.z_ = 0.f;
	return res;
}

// Map the texture to a sphere
Point3 TextureMapperNode::sphereMap(const Point3 &p)
{
	Point3 res{0.f, 0.f, 0.f};
	const float d = p.x_ * p.x_ + p.y_ * p.y_ + p.z_ * p.z_;
	if(d > 0.f)
	{
		res.z_ = math::sqrt(d);
		if((p.x_ != 0.f) && (p.y_ != 0.f)) res.x_ = -atan2(p.x_, p.y_) * math::div_1_by_pi;
		res.y_ = 1.f - 2.f * (math::acos(p.z_ / res.z_) * math::div_1_by_pi);
	}
	return res;
}

// Map the texture to a cube
Point3 TextureMapperNode::cubeMap(const Point3 &p, const Vec3 &n)
{
	const std::array<std::array<int, 3>, 3> ma {{ {1, 2, 0}, {0, 2, 1}, {0, 1, 2} }};
	// int axis = std::abs(n.x) > std::abs(n.y) ? (std::abs(n.x) > std::abs(n.z) ? 0 : 2) : (std::abs(n.y) > std::abs(n.z) ? 1 : 2);
	// no functionality changes, just more readable code
	int axis;
	if(std::abs(n.z_) >= std::abs(n.x_) && std::abs(n.z_) >= std::abs(n.y_)) axis = 2;
	else if(std::abs(n.y_) >= std::abs(n.x_) && std::abs(n.y_) >= std::abs(n.z_)) axis = 1;
	else axis = 0;
	return { p[ma[axis][0]], p[ma[axis][1]], p[ma[axis][2]] };
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
		case Uv: texpt = {2.f * texpt.x_ - 1.f, 2.f * texpt.y_ - 1.f, texpt.z_}; break;
		default: break;
	}
	// Texture axis mapping
	const std::array<float, 4> texmap { 0.f, texpt.x_, texpt.y_, texpt.z_ };
	texpt.x_ = texmap[map_x_];
	texpt.y_ = texmap[map_y_];
	texpt.z_ = texmap[map_z_];
	// Texture coordinates mapping
	switch(projection_)
	{
		case Tube: texpt = tubeMap(texpt); break;
		case Sphere: texpt = sphereMap(texpt); break;
		case Cube: texpt = cubeMap(texpt, n); break;
		case Plain: // texpt = flatmap(texpt); break;
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

void TextureMapperNode::getCoords(Point3 &texpt, Vec3 &ng, const SurfacePoint &sp, const Camera *camera) const
{
	switch(coords_)
	{
		case Uv: texpt = evalUv(sp); ng = sp.ng_; break;
		case Orco: texpt = sp.orco_p_; ng = sp.orco_ng_; break;
		case Transformed: texpt = mtx_ * sp.p_; ng = mtx_ * sp.ng_; break;  // apply 4x4 matrix of object for mapping also to true surface normals
		case Window: texpt = camera->screenproject(sp.p_); ng = sp.ng_; break;
		case Normal:
			{
				Vec3 camx, camy, camz;
				camera->getAxis(camx, camy, camz);
				texpt = {sp.n_ * camx, -sp.n_ * camy, 0.f};
				ng = sp.ng_;
			}
			break;
		case Stick: // Not implemented yet use GLOB
		case Stress: // Not implemented yet use GLOB
		case Tangent: // Not implemented yet use GLOB
		case Reflect: // Not implemented yet use GLOB
		case Global: // GLOB mapped as default
		default:
			texpt = sp.p_; ng = sp.ng_;
			break;
	}
}

void TextureMapperNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	Point3 texpt{0.f, 0.f, 0.f};
	Vec3 ng(0.f);
	std::unique_ptr<const MipMapParams> mip_map_params;

	if((tex_->getInterpolationType() == InterpolationType::Trilinear || tex_->getInterpolationType() == InterpolationType::Ewa) && sp.differentials_)
	{
		getCoords(texpt, ng, sp, camera);
		const Point3 texptorig{texpt};
		texpt = doMapping(texptorig, ng);
		if(coords_ == Uv && sp.has_uv_)
		{
			float du_dx = 0.f, dv_dx = 0.f;
			float du_dy = 0.f, dv_dy = 0.f;
			sp.getUVdifferentials(du_dx, dv_dx, du_dy, dv_dy);
			const Point3 texpt_diffx{1.0e+2f * (doMapping(texptorig + 1.0e-2f * Point3{du_dx, dv_dx, 0.f}, ng) - texpt)};
			const Point3 texpt_diffy{1.0e+2f * (doMapping(texptorig + 1.0e-2f * Point3{du_dy, dv_dy, 0.f}, ng) - texpt)};
			mip_map_params = std::unique_ptr<const MipMapParams>(new MipMapParams(texpt_diffx.x_, texpt_diffx.y_, texpt_diffy.x_, texpt_diffy.y_));
		}
	}
	else
	{
		getCoords(texpt, ng, sp, camera);
		texpt = doMapping(texpt, ng);
	}

	node_tree_data[getId()] = NodeResult(tex_->getColor(texpt, mip_map_params.get()), (do_scalar_) ? tex_->getFloat(texpt, mip_map_params.get()) : 0.f);
}

// Normal perturbation

void TextureMapperNode::evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	Point3 texpt{0.f, 0.f, 0.f};
	Vec3 ng(0.f);
	float du = 0.0f, dv = 0.0f;

	getCoords(texpt, ng, sp, camera);

	if(tex_->discrete() && sp.has_uv_ && coords_ == Uv)
	{
		texpt = doMapping(texpt, ng);
		Vec3 norm(0.f);

		if(tex_->isNormalmap())
		{
			// Get color from normal map texture
			const Rgba color = tex_->getRawColor(texpt);

			// Assign normal map RGB colors to vector norm
			norm.x_ = color.getR();
			norm.y_ = color.getG();
			norm.z_ = color.getB();
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
			vec_u.z_ = dfdu;
			vec_v.z_ = dfdv;
			// now we have two vectors NU/NV/df; Solve plane equation to get 1/0/df and 0/1/df (i.e. dNUdf and dNVdf)
			norm = vec_u ^ vec_v;
		}

		norm.normalize();

		if(std::abs(norm.z_) > 1e-30f)
		{
			const float nf = 1.f / norm.z_ * bump_str_; // normalizes z to 1, why?
			du = norm.x_ * nf;
			dv = norm.y_ * nf;
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

			if(std::abs(norm.z_) > 1e-30f)
			{
				const float nf = 1.0 / norm.z_ * bump_str_; // normalizes z to 1, why?
				du = norm.x_ * nf;
				dv = norm.y_ * nf;
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

			if(coords_ != Uv)
			{
				du = -du;
				dv = -dv;
			}
		}
	}

	node_tree_data[getId()] = NodeResult(Rgba(du, dv, 0.f, 0.f), 0.f);
}

ShaderNode * TextureMapperNode::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	const Texture *tex = nullptr;
	std::string texname, option;
	Coords tc = Global;
	Projection projection = Plain;
	float bump_str = 1.f;
	bool scalar = true;
	int map[3] = {1, 2, 3 };
	Point3 offset{0.f, 0.f, 0.f}, scale{1.f, 1.f, 1.f};
	Matrix4 mtx(1);
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
		if(option == "uv") tc = Uv;
		else if(option == "global") tc = Global;
		else if(option == "orco") tc = Orco;
		else if(option == "transformed") tc = Transformed;
		else if(option == "window") tc = Window;
		else if(option == "normal") tc = Normal;
		else if(option == "reflect") tc = Reflect;
		else if(option == "stick") tc = Stick;
		else if(option == "stress") tc = Stress;
		else if(option == "tangent") tc = Tangent;
	}
	if(params.getParam("mapping", option) && tex->discrete())
	{
		if(option == "plain") projection = Plain;
		else if(option == "cube") projection = Cube;
		else if(option == "tube") projection = Tube;
		else if(option == "sphere") projection = Sphere;
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
	node_tree_data[getId()] = NodeResult(color_, value_);
}

ShaderNode * ValueNode::factory(Logger &logger, const ParamMap &params, const Scene &scene)
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
	const float f_2 = factor_ ? factor_->getScalar(node_tree_data) : cfactor_;
	const float f_1 = 1.f - f_2;
	float fin_1, fin_2;
	Rgba cin_1, cin_2;
	if(input_1_)
	{
		cin_1 = input_1_->getColor(node_tree_data);
		fin_1 = input_1_->getScalar(node_tree_data);
	}
	else
	{
		cin_1 = col_1_;
		fin_1 = val_1_;
	}
	if(input_2_)
	{
		cin_2 = input_2_->getColor(node_tree_data);
		fin_2 = input_2_->getScalar(node_tree_data);
	}
	else
	{
		cin_2 = col_2_;
		fin_2 = val_2_;
	}

	const Rgba color { f_1 * cin_1 + f_2 * cin_2 };
	const float scalar { f_1 * fin_1 + f_2 * fin_2 };
	node_tree_data[getId()] = NodeResult(color, scalar);
}

bool MixNode::configInputs(Logger &logger, const ParamMap &params, const NodeFinder &find)
{
	std::string name;
	if(params.getParam("input1", name))
	{
		input_1_ = find(name);
		if(!input_1_)
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
		input_2_ = find(name);
		if(!input_2_)
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
		factor_ = find(name);
		if(!factor_)
		{
			logger.logError("MixNode: Couldn't get factor ", name);
			return false;
		}
	}
	else if(!params.getParam("value", cfactor_))
	{
		logger.logError("MixNode: Value not set");
		return false;
	}

	return true;
}

std::vector<const ShaderNode *> MixNode::getDependencies() const
{
	std::vector<const ShaderNode *> dependencies;
	if(input_1_) dependencies.push_back(input_1_);
	if(input_2_) dependencies.push_back(input_2_);
	if(factor_) dependencies.push_back(factor_);
	return dependencies;
}

void MixNode::getInputs(const NodeTreeData &node_tree_data, Rgba &cin_1, Rgba &cin_2, float &fin_1, float &fin_2, float &f_2) const
{
	f_2 = factor_ ? factor_->getScalar(node_tree_data) : cfactor_;
	if(input_1_)
	{
		cin_1 = input_1_->getColor(node_tree_data);
		fin_1 = input_1_->getScalar(node_tree_data);
	}
	else
	{
		cin_1 = col_1_;
		fin_1 = val_1_;
	}
	if(input_2_)
	{
		cin_2 = input_2_->getColor(node_tree_data);
		fin_2 = input_2_->getScalar(node_tree_data);
	}
	else
	{
		cin_2 = col_2_;
		fin_2 = val_2_;
	}
}

class AddNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_1 += f_2 * cin_2;
			fin_1 += f_2 * fin_2;
			node_tree_data[getId()] = NodeResult(cin_1, fin_1);
		}
};

class MultNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			cin_1 *= Rgba(f_1) + f_2 * cin_2;
			fin_2 *= f_1 + f_2 * fin_2;
			node_tree_data[getId()] = NodeResult(cin_1, fin_1);
		}
};

class SubNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_1 -= f_2 * cin_2;
			fin_1 -= f_2 * fin_2;
			node_tree_data[getId()] = NodeResult(cin_1, fin_1);
		}
};

class ScreenNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			const Rgba color { Rgba{1.f} - (Rgba{f_1} + f_2 * (Rgba{1.f} - cin_2)) * (Rgba{1.f} - cin_1) };
			const float scalar = 1.f - (f_1 + f_2 * (1.f - fin_2)) * (1.f - fin_1);
			node_tree_data[getId()] = NodeResult(color, scalar);
		}
};

class DiffNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			cin_1.r_ = f_1 * cin_1.r_ + f_2 * std::abs(cin_1.r_ - cin_2.r_);
			cin_1.g_ = f_1 * cin_1.g_ + f_2 * std::abs(cin_1.g_ - cin_2.g_);
			cin_1.b_ = f_1 * cin_1.b_ + f_2 * std::abs(cin_1.b_ - cin_2.b_);
			cin_1.a_ = f_1 * cin_1.a_ + f_2 * std::abs(cin_1.a_ - cin_2.a_);
			fin_1   = f_1 * fin_1 + f_2 * std::abs(fin_1 - fin_2);
			node_tree_data[getId()] = NodeResult(cin_1, fin_1);
		}
};

class DarkNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_2 *= f_2;
			if(cin_2.r_ < cin_1.r_) cin_1.r_ = cin_2.r_;
			if(cin_2.g_ < cin_1.g_) cin_1.g_ = cin_2.g_;
			if(cin_2.b_ < cin_1.b_) cin_1.b_ = cin_2.b_;
			if(cin_2.a_ < cin_1.a_) cin_1.a_ = cin_2.a_;
			fin_2 *= f_2;
			if(fin_2 < fin_1) fin_1 = fin_2;
			node_tree_data[getId()] = NodeResult(cin_1, fin_1);
		}
};

class LightNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_2 *= f_2;
			if(cin_2.r_ > cin_1.r_) cin_1.r_ = cin_2.r_;
			if(cin_2.g_ > cin_1.g_) cin_1.g_ = cin_2.g_;
			if(cin_2.b_ > cin_1.b_) cin_1.b_ = cin_2.b_;
			if(cin_2.a_ > cin_1.a_) cin_1.a_ = cin_2.a_;
			fin_2 *= f_2;
			if(fin_2 > fin_1) fin_1 = fin_2;
			node_tree_data[getId()] = NodeResult(cin_1, fin_1);
		}
};

class OverlayNode: public MixNode
{
	public:
		void eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const override
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(node_tree_data, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			const Rgba color {
				(cin_1.r_ < 0.5f) ? cin_1.r_ * (f_1 + 2.f * f_2 * cin_2.r_) : 1.f - (f_1 + 2.f * f_2 * (1.f - cin_2.r_)) * (1.f - cin_1.r_),
				(cin_1.g_ < 0.5f) ? cin_1.g_ * (f_1 + 2.f * f_2 * cin_2.g_) : 1.f - (f_1 + 2.f * f_2 * (1.f - cin_2.g_)) * (1.f - cin_1.g_),
				(cin_1.b_ < 0.5f) ? cin_1.b_ * (f_1 + 2.f * f_2 * cin_2.b_) : 1.f - (f_1 + 2.f * f_2 * (1.f - cin_2.b_)) * (1.f - cin_1.b_),
				(cin_1.a_ < 0.5f) ? cin_1.a_ * (f_1 + 2.f * f_2 * cin_2.a_) : 1.f - (f_1 + 2.f * f_2 * (1.f - cin_2.a_)) * (1.f - cin_1.a_)
			};
			const float scalar = (fin_1 < 0.5f) ? fin_1 * (f_1 + 2.f * f_2 * fin_2) : 1.f - (f_1 + 2.f * f_2 * (1.f - fin_2)) * (1.f - fin_1);
			node_tree_data[getId()] = NodeResult(color, scalar);
		}
};


ShaderNode * MixNode::factory(Logger &logger, const ParamMap &params, const Scene &scene)
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
