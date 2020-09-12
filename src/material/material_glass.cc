/****************************************************************************
 *      glass.cc: a dielectric material with dispersion, two trivial mats
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "material/material_glass.h"
#include "shader/shader_node.h"
#include "common/surface.h"
#include "common/logging.h"
#include "common/spectrum.h"
#include "common/color_ramp.h"
#include "common/param.h"
#include "common/scene.h"

BEGIN_YAFARAY

GlassMaterial::GlassMaterial(float ior, Rgb filt_c, const Rgb &srcol, double disp_pow, bool fake_s, Visibility e_visibility):
		filter_color_(filt_c), specular_reflection_color_(srcol), fake_shadow_(fake_s), dispersion_power_(disp_pow)
{
	visibility_ = e_visibility;
	ior_ = ior;
	bsdf_flags_ = BsdfFlags::AllSpecular;
	if(fake_s) bsdf_flags_ |= BsdfFlags::Filter;
	tm_flags_ = fake_s ? BsdfFlags::Filter | BsdfFlags::Transmit : BsdfFlags::Specular | BsdfFlags::Transmit;
	if(disp_pow > 0.0)
	{
		disperse_ = true;
		cauchyCoefficients__(ior, disp_pow, cauchy_a_, cauchy_b_);
		bsdf_flags_ |= BsdfFlags::Dispersive;
	}

	visibility_ = e_visibility;
}

void GlassMaterial::initBsdf(const RenderState &state, SurfacePoint &sp, BsdfFlags &bsdf_types) const
{
	NodeStack stack(state.userdata_);
	if(bump_shader_) evalBump(stack, state, sp, bump_shader_);

	//eval viewindependent nodes
	auto end = all_viewindep_.end();
	for(auto iter = all_viewindep_.begin(); iter != end; ++iter)(*iter)->eval(stack, state, sp);
	bsdf_types = bsdf_flags_;
}

#define MATCHES(bits, flags) ((bits & (flags)) == (flags))

Rgb GlassMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	NodeStack stack(state.userdata_);
	if(!Material::hasFlag(s.flags_, BsdfFlags::Specular) && !(Material::hasFlag(s.flags_, bsdf_flags_ & BsdfFlags::Dispersive) && state.chromatic_))
	{
		s.pdf_ = 0.f;
		Rgb scolor = Rgb(0.f);
		float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
		applyWireFrame(scolor, wire_frame_amount, sp);
		return scolor;
	}
	Vec3 refdir, n;
	bool outside = sp.ng_ * wo > 0;
	float cos_wo_n = sp.n_ * wo;
	if(outside) n = (cos_wo_n >= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	else n = (cos_wo_n <= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	s.pdf_ = 1.f;

	// we need to sample dispersion
	if(disperse_ && state.chromatic_)
	{
		float cur_ior = ior_;

		if(ior_shader_)
		{
			cur_ior += ior_shader_->getScalar(stack);
		}

		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;

		if(ior_shader_) cauchyCoefficients__(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
		cur_ior = getIor__(state.wavelength_, cur_cauchy_a, cur_cauchy_b);

		if(refract__(n, wo, refdir, cur_ior))
		{
			float kr, kt;
			fresnel__(wo, n, cur_ior, kr, kt);
			float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(!Material::hasFlag(s.flags_, BsdfFlags::Specular) || s.s_1_ < p_kt)
			{
				wi = refdir;
				s.pdf_ = (MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect)) ? p_kt : 1.f;
				s.sampled_flags_ = BsdfFlags::Dispersive | BsdfFlags::Transmit;
				w = 1.f;
				Rgb scolor = (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_); // * (Kt/std::fabs(sp.N*wi));
				float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
				applyWireFrame(scolor, wire_frame_amount, sp);
				return scolor;
			}
			else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect))
			{
				wi = wo;
				wi.reflect(n);
				s.pdf_ = p_kr;
				s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
				w = 1.f;
				Rgb scolor = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_); // * (Kr/std::fabs(sp.N*wi));
				float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
				applyWireFrame(scolor, wire_frame_amount, sp);
				return scolor;
			}
		}
		else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect)) //total inner reflection
		{
			wi = wo;
			wi.reflect(n);
			s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
			w = 1.f;
			Rgb scolor = 1.f; //Rgb(1.f/std::fabs(sp.N*wi));
			float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
			applyWireFrame(scolor, wire_frame_amount, sp);
			return scolor;
		}
	}
	else // no dispersion calculation necessary, regardless of material settings
	{
		float cur_ior = ior_;

		if(ior_shader_)
		{
			cur_ior += ior_shader_->getScalar(stack);
		}

		if(disperse_ && state.chromatic_)
		{
			float cur_cauchy_a = cauchy_a_;
			float cur_cauchy_b = cauchy_b_;

			if(ior_shader_) cauchyCoefficients__(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
			cur_ior = getIor__(state.wavelength_, cur_cauchy_a, cur_cauchy_b);
		}

		if(refract__(n, wo, refdir, cur_ior))
		{
			float kr, kt;
			fresnel__(wo, n, cur_ior, kr, kt);
			float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(s.s_1_ < p_kt && MATCHES(s.flags_, tm_flags_))
			{
				wi = refdir;
				s.pdf_ = p_kt;
				s.sampled_flags_ = tm_flags_;
				if(s.reverse_)
				{
					s.pdf_back_ = s.pdf_; //wrong...need to calc fresnel explicitly!
					s.col_back_ = (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);//*(Kt/std::fabs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);//*(Kt/std::fabs(sp.N*wi));
				float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
				applyWireFrame(scolor, wire_frame_amount, sp);
				return scolor;
			}
			else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect)) //total inner reflection
			{
				wi = wo;
				wi.reflect(n);
				s.pdf_ = p_kr;
				s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
				if(s.reverse_)
				{
					s.pdf_back_ = s.pdf_; //wrong...need to calc fresnel explicitly!
					s.col_back_ = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);// * (Kr/std::fabs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);// * (Kr/std::fabs(sp.N*wi));
				float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
				applyWireFrame(scolor, wire_frame_amount, sp);
				return scolor;
			}
		}
		else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect))//total inner reflection
		{
			wi = wo;
			wi.reflect(n);
			s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
			//Rgb tir_col(1.f/std::fabs(sp.N*wi));
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_;
				s.col_back_ = 1.f;//tir_col;
			}
			w = 1.f;
			Rgb scolor = 1.f;//tir_col;
			float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
			applyWireFrame(scolor, wire_frame_amount, sp);
			return scolor;
		}
	}
	s.pdf_ = 0.f;
	return Rgb(0.f);
}

Rgb GlassMaterial::getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	float kr, kt;
	fresnel__(wo, n, (ior_shader_ ? ior_shader_->getScalar(stack) : ior_), kr, kt);
	Rgb result = kt * (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(result, wire_frame_amount, sp);
	return result;
}

float GlassMaterial::getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);
	float alpha = 1.0 - getTransparency(state, sp, wo).energy();
	if(alpha < 0.0f) alpha = 0.0f;

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(alpha, wire_frame_amount, sp);
	return alpha;
}

void GlassMaterial::getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
								bool &refl, bool &refr, Vec3 *const dir, Rgb *const col) const
{
	NodeStack stack(state.userdata_);
	bool outside = sp.ng_ * wo > 0;
	Vec3 n;
	float cos_wo_n = sp.n_ * wo;
	if(outside)
	{
		n = (cos_wo_n >= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	}
	else
	{
		n = (cos_wo_n <= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	}
	//	vector3d_t N = SurfacePoint::normalFaceForward(sp.Ng, sp.N, wo);
	Vec3 refdir;

	float cur_ior = ior_;

	if(ior_shader_)
	{
		cur_ior += ior_shader_->getScalar(stack);
	}

	if(disperse_ && state.chromatic_)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;

		if(ior_shader_) cauchyCoefficients__(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
		cur_ior = getIor__(state.wavelength_, cur_cauchy_a, cur_cauchy_b);
	}

	if(refract__(n, wo, refdir, cur_ior))
	{
		float kr, kt;
		fresnel__(wo, n, cur_ior, kr, kt);
		if(!state.chromatic_ || !disperse_)
		{
			col[1] = kt * (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);
			dir[1] = refdir;
			refr = true;
		}
		else refr = false; // in this case, we need to sample dispersion, i.e. not considered specular
		// accounting for fresnel reflection when leaving refractive material is a real performance
		// killer as rays keep bouncing inside objects and contribute little after few bounces, so limit we it:
		if(outside || state.raylevel_ < 3)
		{
			dir[0] = wo;
			dir[0].reflect(n);
			col[0] = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_) * kr;
			refl = true;
		}
		else refl = false;
	}
	else //total inner reflection
	{
		col[0] = mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_;
		dir[0] = wo;
		dir[0].reflect(n);
		refl = true;
		refr = false;
	}

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col, wire_frame_amount, sp);
}

float GlassMaterial::getMatIor() const
{
	return ior_;
}

Material *GlassMaterial::factory(ParamMap &params, std::list< ParamMap > &param_list, Scene &scene)
{
	double ior = 1.4;
	double filt = 0.f;
	double disp_power = 0.0;
	Rgb filt_col(1.f), absorp(1.f), sr_col(1.f);
	std::string name;
	bool fake_shad = false;
	std::string s_visibility = "normal";
	Visibility visibility = Material::Visibility::NormalVisible;
	int mat_pass_index = 0;
	bool receive_shadows = true;
	int additionaldepth = 0;
	float samplingfactor = 1.f;
	float wire_frame_amount = 0.f;           //!< Wireframe shading amount
	float wire_frame_thickness = 0.01f;      //!< Wireframe thickness
	float wire_frame_exponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
	Rgb wire_frame_color = Rgb(1.f); //!< Wireframe shading color

	params.getParam("IOR", ior);
	params.getParam("filter_color", filt_col);
	params.getParam("transmit_filter", filt);
	params.getParam("mirror_color", sr_col);
	params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);

	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);
	params.getParam("mat_pass_index",   mat_pass_index);
	params.getParam("additionaldepth",   additionaldepth);
	params.getParam("samplingfactor",   samplingfactor);

	params.getParam("wireframe_amount", wire_frame_amount);
	params.getParam("wireframe_thickness", wire_frame_thickness);
	params.getParam("wireframe_exponent", wire_frame_exponent);
	params.getParam("wireframe_color", wire_frame_color);

	if(s_visibility == "normal") visibility = Material::Visibility::NormalVisible;
	else if(s_visibility == "no_shadows") visibility = Material::Visibility::VisibleNoShadows;
	else if(s_visibility == "shadow_only") visibility = Material::Visibility::InvisibleShadowsOnly;
	else if(s_visibility == "invisible") visibility = Material::Visibility::Invisible;
	else visibility = Material::Visibility::NormalVisible;

	GlassMaterial *mat = new GlassMaterial(ior, filt * filt_col + Rgb(1.f - filt), sr_col, disp_power, fake_shad, visibility);

	mat->setMaterialIndex(mat_pass_index);
	mat->receive_shadows_ = receive_shadows;
	mat->additional_depth_ = additionaldepth;

	mat->wireframe_amount_ = wire_frame_amount;
	mat->wireframe_thickness_ = wire_frame_thickness;
	mat->wireframe_exponent_ = wire_frame_exponent;
	mat->wireframe_color_ = wire_frame_color;

	mat->setSamplingFactor(samplingfactor);

	if(params.getParam("absorption", absorp))
	{
		double dist = 1.f;
		if(absorp.r_ < 1.f || absorp.g_ < 1.f || absorp.b_ < 1.f)
		{
			//deprecated method:
			Rgb sigma(0.f);
			if(params.getParam("absorption_dist", dist))
			{
				const float maxlog = log(1e38);
				sigma.r_ = (absorp.r_ > 1e-38) ? -log(absorp.r_) : maxlog;
				sigma.g_ = (absorp.g_ > 1e-38) ? -log(absorp.g_) : maxlog;
				sigma.b_ = (absorp.b_ > 1e-38) ? -log(absorp.b_) : maxlog;
				if(dist != 0.f) sigma *= 1.f / dist;
			}
			mat->absorb_ = true;
			mat->beer_sigma_a_ = sigma;
			mat->bsdf_flags_ |= BsdfFlags::Volumetric;
			// creat volume handler (backwards compatibility)
			if(params.getParam("name", name))
			{
				ParamMap map;
				map["type"] = std::string("beer");
				map["absorption_col"] = absorp;
				map["absorption_dist"] = Parameter(dist);
				mat->vol_i_ = scene.createVolumeH(name, map);
				mat->bsdf_flags_ |= BsdfFlags::Volumetric;
			}
		}
	}

	std::vector<ShaderNode *> roots;
	std::map<std::string, ShaderNode *> node_list;

	// Prepare our node list
	node_list["mirror_color_shader"] = nullptr;
	node_list["bump_shader"] = nullptr;
	node_list["filter_color_shader"] = nullptr;
	node_list["IOR_shader"] = nullptr;
	node_list["wireframe_shader"]    = nullptr;

	if(mat->loadNodes(param_list, scene))
	{
		mat->parseNodes(params, roots, node_list);
	}
	else Y_ERROR << "Glass: loadNodes() failed!" << YENDL;

	mat->mirror_color_shader_ = node_list["mirror_color_shader"];
	mat->bump_shader_ = node_list["bump_shader"];
	mat->filter_color_shader_ = node_list["filter_color_shader"];
	mat->ior_shader_ = node_list["IOR_shader"];
	mat->wireframe_shader_    = node_list["wireframe_shader"];

	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		std::vector<ShaderNode *> color_nodes;
		if(mat->mirror_color_shader_) mat->getNodeList(mat->mirror_color_shader_, color_nodes);
		if(mat->filter_color_shader_) mat->getNodeList(mat->filter_color_shader_, color_nodes);
		if(mat->ior_shader_) mat->getNodeList(mat->ior_shader_, color_nodes);
		if(mat->wireframe_shader_)    mat->getNodeList(mat->wireframe_shader_, color_nodes);
		mat->filterNodes(color_nodes, mat->all_viewdep_, ViewDep);
		mat->filterNodes(color_nodes, mat->all_viewindep_, ViewIndep);
		if(mat->bump_shader_)
		{
			mat->getNodeList(mat->bump_shader_, mat->bump_nodes_);
		}
	}
	mat->req_mem_ = mat->req_node_mem_;
	return mat;
}

Rgb GlassMaterial::getGlossyColor(const RenderState &state) const {
	NodeStack stack(state.userdata_);
	return (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);
}

Rgb GlassMaterial::getTransColor(const RenderState &state) const {
	NodeStack stack(state.userdata_);
	if(filter_color_shader_ || filter_color_.minimum() < .99f)	return (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);
	else
	{
		Rgb tmp_col = beer_sigma_a_;
		tmp_col.clampRgb01();
		return Rgb(1.f) - tmp_col;
	}
}

Rgb GlassMaterial::getMirrorColor(const RenderState &state) const {
	NodeStack stack(state.userdata_);
	return (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);
}

Rgb MirrorMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	wi = reflectDir__(sp.n_, wo);
	s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
	w = 1.f;
	return ref_col_ * (1.f / std::fabs(sp.n_ * wi));
}

void MirrorMaterial::getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
								 bool &refl, bool &refr, Vec3 *const dir, Rgb *const col) const
{
	col[0] = ref_col_;
	col[1] = Rgb(1.0);
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	dir[0] = reflectDir__(n, wo);
	refl = true;
	refr = false;
}

Material *MirrorMaterial::factory(ParamMap &params, std::list< ParamMap > &param_list, Scene &scene)
{
	Rgb col(1.0);
	float refl = 1.0;
	params.getParam("color", col);
	params.getParam("reflect", refl);
	return new MirrorMaterial(col, refl);
}


Rgb NullMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	s.pdf_ = 0.f;
	w = 0.f;
	return Rgb(0.f);
}

Material *NullMaterial::factory(ParamMap &, std::list< ParamMap > &, Scene &)
{
	return new NullMaterial();
}

END_YAFARAY
