/****************************************************************************
 * 		roughglass.cc: a dielectric material with rough surface
 *      This is part of the yafray package
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
#include <materials/roughglass.h>
#include <core_api/environment.h>
#include <materials/microfacet.h>
#include <utilities/mcqmc.h>
#include <yafraycore/spectrum.h>
#include <core_api/color_ramp.h>
#include <core_api/params.h>
#include <iostream>

BEGIN_YAFRAY


RoughGlassMaterial::RoughGlassMaterial(float ior, Rgb filt_c, const Rgb &srcol, bool fake_s, float alpha, float disp_pow, Visibility e_visibility):
		filter_color_(filt_c), specular_reflection_color_(srcol), ior_(ior), a_2_(alpha * alpha), a_(alpha), fake_shadow_(fake_s), dispersion_power_(disp_pow)
{
	visibility_ = e_visibility;
	bsdf_flags_ = BsdfAllGlossy;
	if(fake_s) bsdf_flags_ |= BsdfFilter;
	if(disp_pow > 0.0)
	{
		disperse_ = true;
		cauchyCoefficients__(ior, disp_pow, cauchy_a_, cauchy_b_);
		bsdf_flags_ |= BsdfDispersive;
	}

	visibility_ = e_visibility;
}

void RoughGlassMaterial::initBsdf(const RenderState &state, SurfacePoint &sp, Bsdf_t &bsdf_types) const
{
	NodeStack stack(state.userdata_);
	if(bump_shader_) evalBump(stack, state, sp, bump_shader_);

	//eval viewindependent nodes
	auto end = all_viewindep_.end();
	for(auto iter = all_viewindep_.begin(); iter != end; ++iter)(*iter)->eval(stack, state, sp);
	bsdf_types = bsdf_flags_;
}

Rgb RoughGlassMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	NodeStack stack(state.userdata_);
	Vec3 n = FACE_FORWARD(sp.ng_, sp.n_, wo);
	bool outside = sp.ng_ * wo > 0.f;

	s.pdf_ = 1.f;

	float alpha_texture = roughness_shader_ ? roughness_shader_->getScalar(stack) + 0.001f : 0.001f;
	float alpha_2 = roughness_shader_ ? alpha_texture * alpha_texture : a_2_;
	float cos_theta, tan_theta_2;

	Vec3 h(0.f);
	ggxSample__(h, alpha_2, s.s_1_, s.s_2_);
	h = h.x_ * sp.nu_ + h.y_ * sp.nv_ + h.z_ * n;
	h.normalize();

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

	float glossy;
	float glossy_d = 0.f;
	float glossy_g = 0.f;
	float wi_n, wo_n, wi_h, wo_h;
	float jacobian = 0.f;

	cos_theta = h * n;
	float cos_theta_2 = cos_theta * cos_theta;
	tan_theta_2 = (1.f - cos_theta_2) / std::max(1.0e-8f, cos_theta_2);

	if(cos_theta > 0.f) glossy_d = ggxD__(alpha_2, cos_theta_2, tan_theta_2);

	wo_h = wo * h;
	wo_n = wo * n;

	float kr, kt;

	Rgb ret(0.f);

	if(refractMicrofacet__(((outside) ? 1.f / cur_ior : cur_ior), wo, wi, h, wo_h, wo_n, kr, kt))
	{
		if(s.s_1_ < kt && (s.flags_ & BsdfTransmit))
		{
			wi_n = wi * n;
			wi_h = wi * h;

			if((wi_h * wi_n) > 0.f && (wo_h * wo_n) > 0.f) glossy_g = ggxG__(alpha_2, wi_n, wo_n);

			float ior_wi = 1.f;
			float ior_wo = 1.f;

			if(outside) ior_wi = cur_ior;
			else ior_wo = cur_ior;

			float ht = ior_wo * wo_h + ior_wi * wi_h;
			jacobian = (ior_wi * ior_wi) / std::max(1.0e-8f, ht * ht);

			glossy = std::fabs((wo_h * wi_h) / (wi_n * wo_n)) * kt * glossy_g * glossy_d * jacobian;

			s.pdf_ = ggxPdf__(glossy_d, cos_theta, jacobian * std::fabs(wi_h));
			s.sampled_flags_ = ((disperse_ && state.chromatic_) ? BsdfDispersive : BsdfGlossy) | BsdfTransmit;

			ret = (glossy * (filter_col_shader_ ? filter_col_shader_->getColor(stack) : filter_color_));
			w = std::fabs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
		else if(s.flags_ & BsdfReflect)
		{
			reflectMicrofacet__(wo, wi, h, wo_h);

			wi_n = wi * n;
			wi_h = wi * h;

			glossy_g = ggxG__(alpha_2, wi_n, wo_n);

			jacobian = 1.f / std::max(1.0e-8f, (4.f * std::fabs(wi_h)));
			glossy = (kr * glossy_g * glossy_d) / std::max(1.0e-8f, (4.f * std::fabs(wo_n * wi_n)));

			s.pdf_ = ggxPdf__(glossy_d, cos_theta, jacobian);
			s.sampled_flags_ = BsdfGlossy | BsdfReflect;

			ret = (glossy * (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_));

			w = std::fabs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(h);
		s.sampled_flags_ = BsdfGlossy | BsdfReflect;
		ret = 1.f;
		w = 1.f;
	}

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(ret, wire_frame_amount, sp);

	return ret;
}

Rgb RoughGlassMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w) const
{
	NodeStack stack(state.userdata_);
	Vec3 n = FACE_FORWARD(sp.ng_, sp.n_, wo);
	bool outside = sp.ng_ * wo > 0.f;

	s.pdf_ = 1.f;

	float alpha_texture = roughness_shader_ ? roughness_shader_->getScalar(stack) + 0.001f : 0.001f;
	float alpha_2 = roughness_shader_ ? alpha_texture * alpha_texture : a_2_;
	float cos_theta, tan_theta_2;

	Vec3 h(0.f);
	Vec3 wi;
	ggxSample__(h, alpha_2, s.s_1_, s.s_2_);
	h = h.x_ * sp.nu_ + h.y_ * sp.nv_ + h.z_ * n;
	h.normalize();

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

	float glossy;
	float glossy_d = 0.f;
	float glossy_g = 0.f;
	float wi_n, wo_n, wi_h, wo_h;
	float jacobian = 0.f;

	cos_theta = h * n;
	float cos_theta_2 = cos_theta * cos_theta;
	tan_theta_2 = (1.f - cos_theta_2) / std::max(1.0e-8f, cos_theta_2);

	if(cos_theta > 0.f) glossy_d = ggxD__(alpha_2, cos_theta_2, tan_theta_2);

	wo_h = wo * h;
	wo_n = wo * n;

	float kr, kt;

	Rgb ret(0.f);
	s.sampled_flags_ = 0;

	if(refractMicrofacet__(((outside) ? 1.f / cur_ior : cur_ior), wo, wi, h, wo_h, wo_n, kr, kt))
	{
		if(s.flags_ & BsdfTransmit)
		{
			wi_n = wi * n;
			wi_h = wi * h;

			if((wi_h * wi_n) > 0.f && (wo_h * wo_n) > 0.f) glossy_g = ggxG__(alpha_2, wi_n, wo_n);

			float ior_wi = 1.f;
			float ior_wo = 1.f;

			if(outside) ior_wi = cur_ior;
			else ior_wo = cur_ior;

			float ht = ior_wo * wo_h + ior_wi * wi_h;
			jacobian = (ior_wi * ior_wi) / std::max(1.0e-8f, ht * ht);

			glossy = std::fabs((wo_h * wi_h) / (wi_n * wo_n)) * kt * glossy_g * glossy_d * jacobian;

			s.pdf_ = ggxPdf__(glossy_d, cos_theta, jacobian * std::fabs(wi_h));
			s.sampled_flags_ = ((disperse_ && state.chromatic_) ? BsdfDispersive : BsdfGlossy) | BsdfTransmit;

			ret = (glossy * (filter_col_shader_ ? filter_col_shader_->getColor(stack) : filter_color_));
			w[0] = std::fabs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[0] = wi;

		}
		if(s.flags_ & BsdfReflect)
		{
			reflectMicrofacet__(wo, wi, h, wo_h);

			wi_n = wi * n;
			wi_h = wi * h;

			glossy_g = ggxG__(alpha_2, wi_n, wo_n);

			jacobian = 1.f / std::max(1.0e-8f, (4.f * std::fabs(wi_h)));
			glossy = (kr * glossy_g * glossy_d) / std::max(1.0e-8f, (4.f * std::fabs(wo_n * wi_n)));

			s.pdf_ = ggxPdf__(glossy_d, cos_theta, jacobian);
			s.sampled_flags_ |= BsdfGlossy | BsdfReflect;

			tcol = (glossy * (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_));

			w[1] = std::fabs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[1] = wi;
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(h);
		s.sampled_flags_ |= BsdfGlossy | BsdfReflect;
		dir[0] = wi;
		ret = 1.f;
		w[0] = 1.f;
	}

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(ret, wire_frame_amount, sp);

	return ret;
}

Rgb RoughGlassMaterial::getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);
	Vec3 n = FACE_FORWARD(sp.ng_, sp.n_, wo);
	float kr, kt;
	fresnel__(wo, n, (ior_shader_ ? ior_shader_->getScalar(stack) : ior_), kr, kt);
	Rgb result = kt * (filter_col_shader_ ? filter_col_shader_->getColor(stack) : filter_color_);

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(result, wire_frame_amount, sp);
	return result;
}

float RoughGlassMaterial::getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);

	float alpha = std::max(0.f, std::min(1.f, 1.f - getTransparency(state, sp, wo).energy()));

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(alpha, wire_frame_amount, sp);
	return alpha;
}

float RoughGlassMaterial::getMatIor() const
{
	return ior_;
}

Material *RoughGlassMaterial::factory(ParamMap &params, std::list< ParamMap > &param_list, RenderEnvironment &render)
{
	float ior = 1.4;
	float filt = 0.f;
	float alpha = 0.5f;
	float disp_power = 0.0;
	Rgb filt_col(1.f), absorp(1.f), sr_col(1.f);
	std::string name;
	bool fake_shad = false;
	std::string s_visibility = "normal";
	Visibility visibility = NormalVisible;
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
	params.getParam("alpha", alpha);
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

	if(s_visibility == "normal") visibility = NormalVisible;
	else if(s_visibility == "no_shadows") visibility = VisibleNoShadows;
	else if(s_visibility == "shadow_only") visibility = InvisibleShadowsOnly;
	else if(s_visibility == "invisible") visibility = Invisible;
	else visibility = NormalVisible;

	alpha = std::max(1e-4f, std::min(alpha * 0.5f, 1.f));

	RoughGlassMaterial *mat = new RoughGlassMaterial(ior, filt * filt_col + Rgb(1.f - filt), sr_col, fake_shad, alpha, disp_power, visibility);

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
			mat->bsdf_flags_ |= BsdfVolumetric;
			// creat volume handler (backwards compatibility)
			if(params.getParam("name", name))
			{
				ParamMap map;
				map["type"] = std::string("beer");
				map["absorption_col"] = absorp;
				map["absorption_dist"] = Parameter(dist);
				mat->vol_i_ = render.createVolumeH(name, map);
				mat->bsdf_flags_ |= BsdfVolumetric;
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
	node_list["wireframe_shader"] = nullptr;
	node_list["roughness_shader"] = nullptr;

	if(mat->loadNodes(param_list, render))
	{
		mat->parseNodes(params, roots, node_list);
	}
	else Y_ERROR << "RoughGlass: loadNodes() failed!" << YENDL;

	mat->mirror_color_shader_ = node_list["mirror_color_shader"];
	mat->bump_shader_ = node_list["bump_shader"];
	mat->filter_col_shader_ = node_list["filter_color_shader"];
	mat->ior_shader_ = node_list["IOR_shader"];
	mat->wireframe_shader_ = node_list["wireframe_shader"];
	mat->roughness_shader_ = node_list["roughness_shader"];

	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		std::vector<ShaderNode *> color_nodes;
		if(mat->mirror_color_shader_) mat->getNodeList(mat->mirror_color_shader_, color_nodes);
		if(mat->roughness_shader_) mat->getNodeList(mat->roughness_shader_, color_nodes);
		if(mat->ior_shader_) mat->getNodeList(mat->ior_shader_, color_nodes);
		if(mat->wireframe_shader_)    mat->getNodeList(mat->wireframe_shader_, color_nodes);
		if(mat->filter_col_shader_) mat->getNodeList(mat->filter_col_shader_, color_nodes);
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

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("rough_glass", RoughGlassMaterial::factory);
	}
}

END_YAFRAY
