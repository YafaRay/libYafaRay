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
#include "geometry/object_geom.h"
#include "camera/camera.h"
#include "common/param.h"
#include "geometry/surface.h"
#include "texture/texture_image.h"
#include "render/render_data.h"

BEGIN_YAFARAY

TextureMapperNode::TextureMapperNode(const Texture *texture): tex_(texture), bump_str_(0.02), do_scalar_(true)
{
	map_x_ = 1; map_y_ = 2, map_z_ = 3;
}

void TextureMapperNode::setup()
{
	if(tex_->discrete())
	{
		int u, v, w;
		tex_->resolution(u, v, w);
		d_u_ = 1.f / (float)u;
		d_v_ = 1.f / (float)v;
		if(tex_->isThreeD()) d_w_ = 1.f / (float)w;
		else d_w_ = 0.f;
	}
	else
	{
		float step = 0.0002f;
		d_u_ = d_v_ = d_w_ = step;
	}

	p_du_ = Point3(d_u_, 0, 0);
	p_dv_ = Point3(0, d_v_, 0);
	p_dw_ = Point3(0, 0, d_w_);

	bump_str_ /= scale_.length();

	if(!tex_->isNormalmap())
		bump_str_ /= 100.0f;
}

// Map the texture to a cylinder
inline Point3 tubemap__(const Point3 &p)
{
	Point3 res;
	res.y_ = p.z_;
	float d = p.x_ * p.x_ + p.y_ * p.y_;
	if(d > 0)
	{
		res.z_ = 1.0 / math::sqrt(d);
		res.x_ = -atan2(p.x_, p.y_) * M_1_PI;
	}
	else res.x_ = res.z_ = 0;
	return res;
}

// Map the texture to a sphere
inline Point3 spheremap__(const Point3 &p)
{
	Point3 res(0.f);
	float d = p.x_ * p.x_ + p.y_ * p.y_ + p.z_ * p.z_;
	if(d > 0)
	{
		res.z_ = math::sqrt(d);
		if((p.x_ != 0) && (p.y_ != 0)) res.x_ = -atan2(p.x_, p.y_) * M_1_PI;
		res.y_ = 1.0f - 2.0f * (math::acos(p.z_ / res.z_) * M_1_PI);
	}
	return res;
}

// Map the texture to a cube
inline Point3 cubemap__(const Point3 &p, const Vec3 &n)
{
	const int ma[3][3] = { {1, 2, 0}, {0, 2, 1}, {0, 1, 2} };
	// int axis = std::abs(n.x) > std::abs(n.y) ? (std::abs(n.x) > std::abs(n.z) ? 0 : 2) : (std::abs(n.y) > std::abs(n.z) ? 1 : 2);
	// no functionality changes, just more readable code

	int axis;

	if(std::abs(n.z_) >= std::abs(n.x_) && std::abs(n.z_) >= std::abs(n.y_))
	{
		axis = 2;
	}
	else if(std::abs(n.y_) >= std::abs(n.x_) && std::abs(n.y_) >= std::abs(n.z_))
	{
		axis = 1;
	}
	else
	{
		axis = 0;
	}

	return Point3(p[ma[axis][0]], p[ma[axis][1]], p[ma[axis][2]]);
}

// Map the texture to a plane but it should not be used by now as it does nothing, it's just for completeness sake
inline Point3 flatmap__(const Point3 &p)
{
	return p;
}

Point3 TextureMapperNode::doMapping(const Point3 &p, const Vec3 &n) const
{
	Point3 texpt(p);
	// Texture coordinates standardized, if needed, to -1..1
	switch(coords_)
	{
		case Uv: texpt = Point3(2.0f * texpt.x_ - 1.0f, 2.0f * texpt.y_ - 1.0f, texpt.z_); break;
		default: break;
	}
	// Texture axis mapping
	float texmap[4] = {0, texpt.x_, texpt.y_, texpt.z_};
	texpt.x_ = texmap[map_x_];
	texpt.y_ = texmap[map_y_];
	texpt.z_ = texmap[map_z_];
	// Texture coordinates mapping
	switch(projection_)
	{
		case Tube:	texpt = tubemap__(texpt); break;
		case Sphere: texpt = spheremap__(texpt); break;
		case Cube:	texpt = cubemap__(texpt, n); break;
		case Plain:	// texpt = flatmap(texpt); break;
		default: break;
	}
	// Texture scale and offset
	texpt = mult__(texpt, scale_) + offset_;

	return texpt;
}

Point3 evalUv__(const SurfacePoint &sp)
{
	return Point3(sp.u_, sp.v_, 0.f);
}

void TextureMapperNode::getCoords(Point3 &texpt, Vec3 &ng, const SurfacePoint &sp, const RenderData &render_data) const
{
	switch(coords_)
	{
		case Uv: texpt = evalUv__(sp); ng = sp.ng_; break;
		case Orco:	texpt = sp.orco_p_; ng = sp.orco_ng_; break;
		case Tran:	texpt = mtx_ * sp.p_; ng = mtx_ * sp.ng_; break;  // apply 4x4 matrix of object for mapping also to true surface normals
		case Win:	texpt = render_data.cam_->screenproject(sp.p_); ng = sp.ng_; break;
		case Nor:
		{
			Vec3 camx, camy, camz;
			render_data.cam_->getAxis(camx, camy, camz);
			texpt = Point3(sp.n_ * camx, -sp.n_ * camy, 0); ng = sp.ng_;
			break;
		}
		case Stick:	// Not implemented yet use GLOB
		case Stress:// Not implemented yet use GLOB
		case Tan:	// Not implemented yet use GLOB
		case Refl:	// Not implemented yet use GLOB
		case Glob:	// GLOB mapped as default
		default:		texpt = sp.p_; ng = sp.ng_;
			break;
	}
}


void TextureMapperNode::eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
{
	Point3 texpt(0.f);
	Vec3 ng(0.f);
	MipMapParams *mip_map_params = nullptr;

	if((tex_->getInterpolationType() == InterpolationType::Trilinear || tex_->getInterpolationType() == InterpolationType::Ewa) && sp.ray_ && sp.ray_->has_differentials_)
	{
		SpDifferentials sp_diff(sp, *(sp.ray_));

		getCoords(texpt, ng, sp, render_data);

		Point3 texptorig = texpt;

		texpt = doMapping(texptorig, ng);

		if(coords_ == Uv && sp.has_uv_)
		{
			float du_dx = 0.f, dv_dx = 0.f;
			float du_dy = 0.f, dv_dy = 0.f;
			sp_diff.getUVdifferentials(du_dx, dv_dx, du_dy, dv_dy);

			Point3 texpt_diffx = 1.0e+2f * (doMapping(texptorig + 1.0e-2f * Point3(du_dx, dv_dx, 0.f), ng) - texpt);
			Point3 texpt_diffy = 1.0e+2f * (doMapping(texptorig + 1.0e-2f * Point3(du_dy, dv_dy, 0.f), ng) - texpt);

			mip_map_params = new MipMapParams(texpt_diffx.x_, texpt_diffx.y_, texpt_diffy.x_, texpt_diffy.y_);
		}
	}
	else
	{
		getCoords(texpt, ng, sp, render_data);
		texpt = doMapping(texpt, ng);
	}

	stack[this->getId()] = NodeResult(tex_->getColor(texpt, mip_map_params), (do_scalar_) ? tex_->getFloat(texpt, mip_map_params) : 0.f);

	if(mip_map_params)
	{
		delete mip_map_params;
		mip_map_params = nullptr;
	}
}

// Basically you shouldn't call this anyway, but for the sake of consistency, redirect:
void TextureMapperNode::eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const
{
	eval(stack, render_data, sp);
}

// Normal perturbation

void TextureMapperNode::evalDerivative(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
{
	Point3 texpt(0.f);
	Vec3 ng(0.f);
	float du = 0.0f, dv = 0.0f;

	getCoords(texpt, ng, sp, render_data);

	if(tex_->discrete() && sp.has_uv_ && coords_ == Uv)
	{
		texpt = doMapping(texpt, ng);
		Rgba color(0.f);
		Vec3 norm(0.f);

		if(tex_->isNormalmap())
		{
			// Get color from normal map texture
			color = tex_->getRawColor(texpt);

			// Assign normal map RGB colors to vector norm
			norm.x_ = color.getR();
			norm.y_ = color.getG();
			norm.z_ = color.getB();
			norm = (2.f * norm) - 1.f;

			// Convert norm into shading space
			du = norm * sp.ds_du_;
			dv = norm * sp.ds_dv_;
		}
		else
		{
			Point3 i_0 = (texpt - p_du_);
			Point3 i_1 = (texpt + p_du_);
			Point3 j_0 = (texpt - p_dv_);
			Point3 j_1 = (texpt + p_dv_);
			float dfdu = (tex_->getFloat(i_0) - tex_->getFloat(i_1)) / d_u_;
			float dfdv = (tex_->getFloat(j_0) - tex_->getFloat(j_1)) / d_v_;

			// now we got the derivative in UV-space, but need it in shading space:
			Vec3 vec_u = sp.ds_du_;
			Vec3 vec_v = sp.ds_dv_;
			vec_u.z_ = dfdu;
			vec_v.z_ = dfdv;
			// now we have two vectors NU/NV/df; Solve plane equation to get 1/0/df and 0/1/df (i.e. dNUdf and dNVdf)
			norm = vec_u ^ vec_v;
		}

		norm.normalize();

		if(std::abs(norm.z_) > 1e-30f)
		{
			float nf = 1.0 / norm.z_ * bump_str_; // normalizes z to 1, why?
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
			Rgba color(0.f);
			Vec3 norm(0.f);

			// Get color from normal map texture
			color = tex_->getRawColor(texpt);

			// Assign normal map RGB colors to vector norm
			norm.x_ = color.getR();
			norm.y_ = color.getG();
			norm.z_ = color.getB();
			norm = (2.f * norm) - 1.f;

			// Convert norm into shading space
			du = norm * sp.ds_du_;
			dv = norm * sp.ds_dv_;

			norm.normalize();

			if(std::abs(norm.z_) > 1e-30f)
			{
				float nf = 1.0 / norm.z_ * bump_str_; // normalizes z to 1, why?
				du = norm.x_ * nf;
				dv = norm.y_ * nf;
			}
			else du = dv = 0.f;
		}
		else
		{
			// no uv coords -> procedurals usually, this mapping only depends on NU/NV which is fairly arbitrary
			// weird things may happen when objects are rotated, i.e. incorrect bump change
			Point3 i_0 = doMapping(texpt - d_u_ * sp.nu_, ng);
			Point3 i_1 = doMapping(texpt + d_u_ * sp.nu_, ng);
			Point3 j_0 = doMapping(texpt - d_v_ * sp.nv_, ng);
			Point3 j_1 = doMapping(texpt + d_v_ * sp.nv_, ng);

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

	stack[this->getId()] = NodeResult(Rgba(du, dv, 0.f, 0.f), 0.f);
}

ShaderNode *TextureMapperNode::factory(const ParamMap &params, const Scene &scene)
{
	const Texture *tex = nullptr;
	std::string texname, option;
	Coords tc = Glob;
	Projection projection = Plain;
	float bump_str = 1.f;
	bool scalar = true;
	int map[3] = { 1, 2, 3 };
	Point3 offset(0.f), scale(1.f);
	Matrix4 mtx(1);
	if(!params.getParam("texture", texname))
	{
		Y_ERROR << "TextureMapper: No texture given for texture mapper!" << YENDL;
		return nullptr;
	}
	tex = scene.getTexture(texname);
	if(!tex)
	{
		Y_ERROR << "TextureMapper: texture '" << texname << "' does not exist!" << YENDL;
		return nullptr;
	}
	TextureMapperNode *tm = new TextureMapperNode(tex);
	if(params.getParam("texco", option))
	{
		if(option == "uv") tc = Uv;
		else if(option == "global") tc = Glob;
		else if(option == "orco") tc = Orco;
		else if(option == "transformed") tc = Tran;
		else if(option == "window") tc = Win;
		else if(option == "normal") tc = Nor;
		else if(option == "reflect") tc = Refl;
		else if(option == "stick") tc = Stick;
		else if(option == "stress") tc = Stress;
		else if(option == "tangent") tc = Tan;
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
	for(int i = 0; i < 3; ++i) map[i] = std::min(3, std::max(0, map[i]));
	tm->coords_ = tc;
	tm->projection_ = projection;
	tm->map_x_ = map[0];
	tm->map_y_ = map[1];
	tm->map_z_ = map[2];
	tm->scale_ = Vec3(scale);
	tm->offset_ = Vec3(2 * offset);	// Offset need to be doubled due to -1..1 texture standardized wich results in a 2 wide width/height
	tm->do_scalar_ = scalar;
	tm->bump_str_ = bump_str;
	tm->mtx_ = mtx;
	tm->setup();
	return tm;
}

/* ==========================================
/  The most simple node you could imagine...
/  ========================================== */

void ValueNode::eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
{
	stack[this->getId()] = NodeResult(color_, value_);
}

void ValueNode::eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const
{
	stack[this->getId()] = NodeResult(color_, value_);
}

ShaderNode *ValueNode::factory(const ParamMap &params, const Scene &scene)
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

MixNode::MixNode(): cfactor_(0.f), input_1_(0), input_2_(0), factor_(0)
{}

MixNode::MixNode(float val): cfactor_(val), input_1_(0), input_2_(0), factor_(0)
{}

void MixNode::eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
{
	float f_2 = (factor_) ? factor_->getScalar(stack) : cfactor_;
	float f_1 = 1.f - f_2, fin_1, fin_2;
	Rgba cin_1, cin_2;
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

	Rgba color = f_1 * cin_1 + f_2 * cin_2;
	float   scalar = f_1 * fin_1 + f_2 * fin_2;
	stack[this->getId()] = NodeResult(color, scalar);
}

void MixNode::eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi) const
{
	eval(stack, render_data, sp);
}

bool MixNode::configInputs(const ParamMap &params, const NodeFinder &find)
{
	std::string name;
	if(params.getParam("input1", name))
	{
		input_1_ = find(name);
		if(!input_1_)
		{
			Y_ERROR << "MixNode: Couldn't get input1 " << name << YENDL;
			return false;
		}
	}
	else if(!params.getParam("color1", col_1_))
	{
		Y_ERROR << "MixNode: Color1 not set" << YENDL;
		return false;
	}

	if(params.getParam("input2", name))
	{
		input_2_ = find(name);
		if(!input_2_)
		{
			Y_ERROR << "MixNode: Couldn't get input2 " << name << YENDL;
			return false;
		}
	}
	else if(!params.getParam("color2", col_2_))
	{
		Y_ERROR << "MixNode: Color2 not set" << YENDL;
		return false;
	}

	if(params.getParam("factor", name))
	{
		factor_ = find(name);
		if(!factor_)
		{
			Y_ERROR << "MixNode: Couldn't get factor " << name << YENDL;
			return false;
		}
	}
	else if(!params.getParam("value", cfactor_))
	{
		Y_ERROR << "MixNode: Value not set" << YENDL;
		return false;
	}

	return true;
}

bool MixNode::getDependencies(std::vector<const ShaderNode *> &dep) const
{
	if(input_1_) dep.push_back(input_1_);
	if(input_2_) dep.push_back(input_2_);
	if(factor_) dep.push_back(factor_);
	return !dep.empty();
}

void MixNode::getInputs(NodeStack &stack, Rgba &cin_1, Rgba &cin_2, float &fin_1, float &fin_2, float &f_2) const
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

class AddNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_1 += f_2 * cin_2;
			fin_1 += f_2 * fin_2;
			stack[this->getId()] = NodeResult(cin_1, fin_1);
		}
};

class MultNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			cin_1 *= Rgba(f_1) + f_2 * cin_2;
			fin_2 *= f_1 + f_2 * fin_2;
			stack[this->getId()] = NodeResult(cin_1, fin_1);
		}
};

class SubNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_1 -= f_2 * cin_2;
			fin_1 -= f_2 * fin_2;
			stack[this->getId()] = NodeResult(cin_1, fin_1);
		}
};

class ScreenNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			Rgba color = Rgba(1.f) - (Rgba(f_1) + f_2 * (1.f - cin_2)) * (1.f - cin_1);
			float scalar   = 1.0 - (f_1 + f_2 * (1.f - fin_2)) * (1.f - fin_1);
			stack[this->getId()] = NodeResult(color, scalar);
		}
};

class DiffNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			cin_1.r_ = f_1 * cin_1.r_ + f_2 * std::abs(cin_1.r_ - cin_2.r_);
			cin_1.g_ = f_1 * cin_1.g_ + f_2 * std::abs(cin_1.g_ - cin_2.g_);
			cin_1.b_ = f_1 * cin_1.b_ + f_2 * std::abs(cin_1.b_ - cin_2.b_);
			cin_1.a_ = f_1 * cin_1.a_ + f_2 * std::abs(cin_1.a_ - cin_2.a_);
			fin_1   = f_1 * fin_1 + f_2 * std::abs(fin_1 - fin_2);
			stack[this->getId()] = NodeResult(cin_1, fin_1);
		}
};

class DarkNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_2 *= f_2;
			if(cin_2.r_ < cin_1.r_) cin_1.r_ = cin_2.r_;
			if(cin_2.g_ < cin_1.g_) cin_1.g_ = cin_2.g_;
			if(cin_2.b_ < cin_1.b_) cin_1.b_ = cin_2.b_;
			if(cin_2.a_ < cin_1.a_) cin_1.a_ = cin_2.a_;
			fin_2 *= f_2;
			if(fin_2 < fin_1) fin_1 = fin_2;
			stack[this->getId()] = NodeResult(cin_1, fin_1);
		}
};

class LightNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);

			cin_2 *= f_2;
			if(cin_2.r_ > cin_1.r_) cin_1.r_ = cin_2.r_;
			if(cin_2.g_ > cin_1.g_) cin_1.g_ = cin_2.g_;
			if(cin_2.b_ > cin_1.b_) cin_1.b_ = cin_2.b_;
			if(cin_2.a_ > cin_1.a_) cin_1.a_ = cin_2.a_;
			fin_2 *= f_2;
			if(fin_2 > fin_1) fin_1 = fin_2;
			stack[this->getId()] = NodeResult(cin_1, fin_1);
		}
};

class OverlayNode: public MixNode
{
	public:
		virtual void eval(NodeStack &stack, const RenderData &render_data, const SurfacePoint &sp) const
		{
			float f_1, f_2, fin_1, fin_2;
			Rgba cin_1, cin_2;
			getInputs(stack, cin_1, cin_2, fin_1, fin_2, f_2);
			f_1 = 1.f - f_2;

			Rgba color;
			color.r_ = (cin_1.r_ < 0.5f) ? cin_1.r_ * (f_1 + 2.0f * f_2 * cin_2.r_) : 1.0 - (f_1 + 2.0f * f_2 * (1.0 - cin_2.r_)) * (1.0 - cin_1.r_);
			color.g_ = (cin_1.g_ < 0.5f) ? cin_1.g_ * (f_1 + 2.0f * f_2 * cin_2.g_) : 1.0 - (f_1 + 2.0f * f_2 * (1.0 - cin_2.g_)) * (1.0 - cin_1.g_);
			color.b_ = (cin_1.b_ < 0.5f) ? cin_1.b_ * (f_1 + 2.0f * f_2 * cin_2.b_) : 1.0 - (f_1 + 2.0f * f_2 * (1.0 - cin_2.b_)) * (1.0 - cin_1.b_);
			color.a_ = (cin_1.a_ < 0.5f) ? cin_1.a_ * (f_1 + 2.0f * f_2 * cin_2.a_) : 1.0 - (f_1 + 2.0f * f_2 * (1.0 - cin_2.a_)) * (1.0 - cin_1.a_);
			float scalar = (fin_1 < 0.5f) ? fin_1 * (f_1 + 2.0f * f_2 * fin_2) : 1.0 - (f_1 + 2.0f * f_2 * (1.0 - fin_2)) * (1.0 - fin_1);
			stack[this->getId()] = NodeResult(color, scalar);
		}
};


ShaderNode *MixNode::factory(const ParamMap &params, const Scene &scene)
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
