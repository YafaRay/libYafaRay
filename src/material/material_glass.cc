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
#include "geometry/surface.h"
#include "common/logger.h"
#include "color/spectrum.h"
#include "common/param.h"
#include "render/render_data.h"

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
		cauchyCoefficients_global(ior, disp_pow, cauchy_a_, cauchy_b_);
		bsdf_flags_ |= BsdfFlags::Dispersive;
	}

	visibility_ = e_visibility;
}

void GlassMaterial::initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const
{
	NodeStack stack(render_data.arena_);
	if(bump_shader_) evalBump(stack, render_data, sp, bump_shader_);
	for(const auto &node : color_nodes_) node->eval(stack, render_data, sp);
	bsdf_types = bsdf_flags_;
}

#define MATCHES(bits, flags) ((bits & (flags)) == (flags))

Rgb GlassMaterial::sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	const NodeStack stack(render_data.arena_);
	if(!s.flags_.hasAny(BsdfFlags::Specular) && !(s.flags_.hasAny(bsdf_flags_ & BsdfFlags::Dispersive) && render_data.chromatic_))
	{
		s.pdf_ = 0.f;
		Rgb scolor = Rgb(0.f);
		const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
		applyWireFrame(scolor, wire_frame_amount, sp);
		return scolor;
	}
	Vec3 refdir, n;
	const bool outside = sp.ng_ * wo > 0;
	const float cos_wo_n = sp.n_ * wo;
	if(outside) n = (cos_wo_n >= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	else n = (cos_wo_n <= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	s.pdf_ = 1.f;

	// we need to sample dispersion
	if(disperse_ && render_data.chromatic_)
	{
		float cur_ior = ior_;
		if(ior_shader_) cur_ior += ior_shader_->getScalar(stack);
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;

		if(ior_shader_) cauchyCoefficients_global(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
		cur_ior = getIor_global(render_data.wavelength_, cur_cauchy_a, cur_cauchy_b);

		if(Vec3::refract(n, wo, refdir, cur_ior))
		{
			float kr, kt;
			Vec3::fresnel(wo, n, cur_ior, kr, kt);
			const float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(!s.flags_.hasAny(BsdfFlags::Specular) || s.s_1_ < p_kt)
			{
				wi = refdir;
				s.pdf_ = (MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect)) ? p_kt : 1.f;
				s.sampled_flags_ = BsdfFlags::Dispersive | BsdfFlags::Transmit;
				w = 1.f;
				Rgb scolor = (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_); // * (Kt/std::abs(sp.N*wi));
				const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
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
				Rgb scolor = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_); // * (Kr/std::abs(sp.N*wi));
				const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
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
			Rgb scolor = 1.f; //Rgb(1.f/std::abs(sp.N*wi));
			const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
			applyWireFrame(scolor, wire_frame_amount, sp);
			return scolor;
		}
	}
	else // no dispersion calculation necessary, regardless of material settings
	{
		float cur_ior = ior_;
		if(ior_shader_) cur_ior += ior_shader_->getScalar(stack);
		if(disperse_ && render_data.chromatic_)
		{
			float cur_cauchy_a = cauchy_a_;
			float cur_cauchy_b = cauchy_b_;
			if(ior_shader_) cauchyCoefficients_global(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
			cur_ior = getIor_global(render_data.wavelength_, cur_cauchy_a, cur_cauchy_b);
		}

		if(Vec3::refract(n, wo, refdir, cur_ior))
		{
			float kr, kt;
			Vec3::fresnel(wo, n, cur_ior, kr, kt);
			float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(s.s_1_ < p_kt && MATCHES(s.flags_, tm_flags_))
			{
				wi = refdir;
				s.pdf_ = p_kt;
				s.sampled_flags_ = tm_flags_;
				if(s.reverse_)
				{
					s.pdf_back_ = s.pdf_; //wrong...need to calc fresnel explicitly!
					s.col_back_ = (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);//*(Kt/std::abs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);//*(Kt/std::abs(sp.N*wi));
				const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
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
					s.col_back_ = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);// * (Kr/std::abs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);// * (Kr/std::abs(sp.N*wi));
				const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
				applyWireFrame(scolor, wire_frame_amount, sp);
				return scolor;
			}
		}
		else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect))//total inner reflection
		{
			wi = wo;
			wi.reflect(n);
			s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
			//Rgb tir_col(1.f/std::abs(sp.N*wi));
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_;
				s.col_back_ = 1.f;//tir_col;
			}
			w = 1.f;
			Rgb scolor = 1.f;//tir_col;
			const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
			applyWireFrame(scolor, wire_frame_amount, sp);
			return scolor;
		}
	}
	s.pdf_ = 0.f;
	return Rgb(0.f);
}

Rgb GlassMaterial::getTransparency(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	const NodeStack stack(render_data.arena_);
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	float kr, kt;
	Vec3::fresnel(wo, n, (ior_shader_ ? ior_shader_->getScalar(stack) : ior_), kr, kt);
	Rgb result = kt * (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);
	const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(result, wire_frame_amount, sp);
	return result;
}

float GlassMaterial::getAlpha(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	const NodeStack stack(render_data.arena_);
	float alpha = 1.0 - getTransparency(render_data, sp, wo).energy();
	if(alpha < 0.0f) alpha = 0.0f;
	const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(alpha, wire_frame_amount, sp);
	return alpha;
}

Material::Specular GlassMaterial::getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Material::Specular specular;
	const NodeStack stack(render_data.arena_);
	const bool outside = sp.ng_ * wo > 0;
	Vec3 n;
	const float cos_wo_n = sp.n_ * wo;
	if(outside)
	{
		n = (cos_wo_n >= 0.f) ? sp.n_ : (sp.n_ - (1.00001f * cos_wo_n) * wo).normalize();
	}
	else
	{
		n = (cos_wo_n <= 0.f) ? sp.n_ : (sp.n_ - (1.00001f * cos_wo_n) * wo).normalize();
	}
	//	vector3d_t N = SurfacePoint::normalFaceForward(sp.Ng, sp.N, wo);
	Vec3 refdir;

	float cur_ior = ior_;
	if(ior_shader_) cur_ior += ior_shader_->getScalar(stack);

	if(disperse_ && render_data.chromatic_)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;
		if(ior_shader_) cauchyCoefficients_global(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
		cur_ior = getIor_global(render_data.wavelength_, cur_cauchy_a, cur_cauchy_b);
	}

	if(Vec3::refract(n, wo, refdir, cur_ior))
	{
		float kr, kt;
		Vec3::fresnel(wo, n, cur_ior, kr, kt);
		if(!render_data.chromatic_ || !disperse_)
		{
			specular.refract_.col_ = kt * (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);
			specular.refract_.dir_ = refdir;
			specular.refract_.enabled_ = true;
		}
		//FIXME? If the above does not happen, in this case, we need to sample dispersion, i.e. not considered specular

		// accounting for fresnel reflection when leaving refractive material is a real performance
		// killer as rays keep bouncing inside objects and contribute little after few bounces, so limit we it:
		if(outside || render_data.raylevel_ < 3)
		{
			specular.reflect_.dir_ = wo;
			specular.reflect_.dir_.reflect(n);
			specular.reflect_.col_ = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_) * kr;
			specular.reflect_.enabled_ = true;
		}
	}
	else //total inner reflection
	{
		specular.reflect_.col_ = mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_;
		specular.reflect_.dir_ = wo;
		specular.reflect_.dir_.reflect(n);
		specular.reflect_.enabled_ = true;
	}

	const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(specular.reflect_.col_, wire_frame_amount, sp);
	applyWireFrame(specular.refract_.col_, wire_frame_amount, sp);
	return specular;
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

	const Visibility visibility = visibilityFromString_global(s_visibility);

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
				mat->vol_i_ = scene.createVolumeHandler(name, map);
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
		if(mat->mirror_color_shader_) mat->getNodeList(mat->mirror_color_shader_, mat->color_nodes_);
		if(mat->filter_color_shader_) mat->getNodeList(mat->filter_color_shader_, mat->color_nodes_);
		if(mat->ior_shader_) mat->getNodeList(mat->ior_shader_, mat->color_nodes_);
		if(mat->wireframe_shader_)    mat->getNodeList(mat->wireframe_shader_, mat->color_nodes_);
		if(mat->bump_shader_) mat->getNodeList(mat->bump_shader_, mat->bump_nodes_);
	}
	mat->req_mem_ = mat->req_node_mem_;
	return mat;
}

Rgb GlassMaterial::getGlossyColor(const RenderData &render_data) const {
	NodeStack stack(render_data.arena_);
	return (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);
}

Rgb GlassMaterial::getTransColor(const RenderData &render_data) const {
	NodeStack stack(render_data.arena_);
	if(filter_color_shader_ || filter_color_.minimum() < .99f)	return (filter_color_shader_ ? filter_color_shader_->getColor(stack) : filter_color_);
	else
	{
		Rgb tmp_col = beer_sigma_a_;
		tmp_col.clampRgb01();
		return Rgb(1.f) - tmp_col;
	}
}

Rgb GlassMaterial::getMirrorColor(const RenderData &render_data) const {
	NodeStack stack(render_data.arena_);
	return (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : specular_reflection_color_);
}

Rgb MirrorMaterial::sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	wi = Vec3::reflectDir(sp.n_, wo);
	s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
	w = 1.f;
	return ref_col_ * (1.f / std::abs(sp.n_ * wi));
}

Material::Specular MirrorMaterial::getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Specular specular;
	specular.reflect_.col_ = ref_col_;
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	specular.reflect_.dir_ = Vec3::reflectDir(n, wo);
	specular.reflect_.enabled_ = true;
	return specular;
}

Material *MirrorMaterial::factory(ParamMap &params, std::list< ParamMap > &param_list, Scene &scene)
{
	Rgb col(1.0);
	float refl = 1.0;
	params.getParam("color", col);
	params.getParam("reflect", refl);
	return new MirrorMaterial(col, refl);
}


Rgb NullMaterial::sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
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
