/****************************************************************************
 *      coatedglossy.cc: a glossy material with specular coating
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
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "material/material_coated_glossy.h"
#include "scene/scene.h"
#include "shader/shader_node.h"
#include "sampler/sample.h"
#include "material/material_utils_microfacet.h"
#include "common/param.h"
#include "geometry/surface.h"
#include "common/logger.h"
#include "render/render_data.h"

BEGIN_YAFARAY

/*! Coated Glossy Material.
	A Material with Phong/Anisotropic Phong microfacet base layer and a layer of
	(dielectric) perfectly specular coating. This is to simulate surfaces like
	metallic paint */

#define C_SPECULAR 	0
#define C_GLOSSY  	1
#define C_DIFFUSE 	2

CoatedGlossyMaterial::CoatedGlossyMaterial(const Rgb &col, const Rgb &dcol, const Rgb &mir_col, float mirror_strength, float reflect, float diff, float ior, float expo, bool as_diff, Visibility e_visibility):
		gloss_color_(col), diff_color_(dcol), mirror_color_(mir_col), mirror_strength_(mirror_strength), ior_(ior), exponent_(expo), reflectivity_(reflect), diffuse_(diff), as_diffuse_(as_diff)
{
	visibility_ = e_visibility;
	c_flags_[C_SPECULAR] = (BsdfFlags::Specular | BsdfFlags::Reflect);
	c_flags_[C_GLOSSY] = as_diffuse_ ? (BsdfFlags::Diffuse | BsdfFlags::Reflect) : (BsdfFlags::Glossy | BsdfFlags::Reflect);

	if(diff > 0)
	{
		c_flags_[C_DIFFUSE] = BsdfFlags::Diffuse | BsdfFlags::Reflect;
		with_diffuse_ = true;
		n_bsdf_ = 3;
	}
	else
	{
		c_flags_[C_DIFFUSE] = BsdfFlags::None;
		n_bsdf_ = 2;
	}

	oren_nayar_ = false;

	bsdf_flags_ = c_flags_[C_SPECULAR] | c_flags_[C_GLOSSY] | c_flags_[C_DIFFUSE];

	visibility_ = e_visibility;
}

void CoatedGlossyMaterial::initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const
{
	MDat *dat = (MDat *)render_data.arena_;
	dat->stack_ = (char *)render_data.arena_ + sizeof(MDat);
	NodeStack stack(dat->stack_);
	if(bump_shader_) evalBump(stack, render_data, sp, bump_shader_);

	for(const auto &node : color_nodes_) node->eval(stack, render_data, sp);
	bsdf_types = bsdf_flags_;
	dat->diffuse_ = diffuse_;
	dat->glossy_ = glossy_reflection_shader_ ? glossy_reflection_shader_->getScalar(stack) : reflectivity_;
	dat->p_diffuse_ = std::min(0.6f, 1.f - (dat->glossy_ / (dat->glossy_ + (1.f - dat->glossy_) * dat->diffuse_)));
}

void CoatedGlossyMaterial::initOrenNayar(double sigma)
{
	const double sigma_2 = sigma * sigma;
	oren_a_ = 1.0 - 0.5 * (sigma_2 / (sigma_2 + 0.33));
	oren_b_ = 0.45 * sigma_2 / (sigma_2 + 0.09);
	oren_nayar_ = true;
}

float CoatedGlossyMaterial::orenNayar(const Vec3 &wi, const Vec3 &wo, const Vec3 &n, bool use_texture_sigma, double texture_sigma) const
{
	const float cos_ti = std::max(-1.f, std::min(1.f, n * wi));
	const float cos_to = std::max(-1.f, std::min(1.f, n * wo));
	float maxcos_f = 0.f;

	if(cos_ti < 0.9999f && cos_to < 0.9999f)
	{
		const Vec3 v_1 = (wi - n * cos_ti).normalize();
		const Vec3 v_2 = (wo - n * cos_to).normalize();
		maxcos_f = std::max(0.f, v_1 * v_2);
	}

	float sin_alpha, tan_beta;

	if(cos_to >= cos_ti)
	{
		sin_alpha = math::sqrt(1.f - cos_ti * cos_ti);
		tan_beta = math::sqrt(1.f - cos_to * cos_to) / ((cos_to == 0.f) ? 1e-8f : cos_to); // white (black on windows) dots fix for oren-nayar, could happen with bad normals
	}
	else
	{
		sin_alpha = math::sqrt(1.f - cos_to * cos_to);
		tan_beta = math::sqrt(1.f - cos_ti * cos_ti) / ((cos_ti == 0.f) ? 1e-8f : cos_ti); // white (black on windows) dots fix for oren-nayar, could happen with bad normals
	}

	if(use_texture_sigma)
	{
		const double sigma_squared = texture_sigma * texture_sigma;
		const double m_oren_nayar_texture_a = 1.0 - 0.5 * (sigma_squared / (sigma_squared + 0.33));
		const double m_oren_nayar_texture_b = 0.45 * sigma_squared / (sigma_squared + 0.09);
		return std::min(1.f, std::max(0.f, (float)(m_oren_nayar_texture_a + m_oren_nayar_texture_b * maxcos_f * sin_alpha * tan_beta)));
	}
	else
	{
		return std::min(1.f, std::max(0.f, oren_a_ + oren_b_ * maxcos_f * sin_alpha * tan_beta));
	}

}

Rgb CoatedGlossyMaterial::eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval) const
{
	MDat *dat = (MDat *)render_data.arena_;
	Rgb col(0.f);
	const bool diffuse_flag = bsdfs.hasAny(BsdfFlags::Diffuse);

	if(!force_eval)	//If the flag force_eval = true then the next line will be skipped, necessary for the Glossy Direct render pass
	{
		if(!diffuse_flag || ((sp.ng_ * wi) * (sp.ng_ * wo)) < 0.f) return col;
	}

	NodeStack stack(dat->stack_);
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	float kr, kt;
	const float wi_n = std::abs(wi * n);
	const float wo_n = std::abs(wo * n);

	Vec3::fresnel(wo, n, (ior_shader_ ? ior_ + ior_shader_->getScalar(stack) : ior_), kr, kt);

	if((as_diffuse_ && diffuse_flag) || (!as_diffuse_ && bsdfs.hasAny(BsdfFlags::Glossy)))
	{
		const Vec3 h = (wo + wi).normalize(); // half-angle
		const float cos_wi_h = wi * h;
		float glossy;
		if(anisotropic_)
		{
			const Vec3 hs(h * sp.nu_, h * sp.nv_, h * n);
			glossy = kt * asAnisoD_global(hs, exp_u_, exp_v_) * schlickFresnel_global(cos_wi_h, dat->glossy_) / asDivisor_global(cos_wi_h, wo_n, wi_n);
		}
		else
		{
			glossy = kt * blinnD_global(h * n, (exponent_shader_ ? exponent_shader_->getScalar(stack) : exponent_)) * schlickFresnel_global(cos_wi_h, dat->glossy_) / asDivisor_global(cos_wi_h, wo_n, wi_n);
		}
		col = glossy * (glossy_shader_ ? glossy_shader_->getColor(stack) : gloss_color_);
	}
	if(with_diffuse_ && diffuse_flag)
	{
		Rgb add_col = dat->diffuse_ * (1.f - dat->glossy_) * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diff_color_) * kt;

		if(diffuse_reflection_shader_) add_col *= diffuse_reflection_shader_->getScalar(stack);

		if(oren_nayar_)
		{
			const double texture_sigma = (sigma_oren_shader_ ? sigma_oren_shader_->getScalar(stack) : 0.f);
			const bool use_texture_sigma = (sigma_oren_shader_ ? true : false);
			add_col *= orenNayar(wi, wo, n, use_texture_sigma, texture_sigma);
		}
		col += add_col;//diffuseReflectFresnel(wiN, woN, dat->mGlossy, dat->mDiffuse, (diffuseS ? diffuseS->getColor(stack) : diff_color), Kt) * ((orenNayar)?OrenNayar(wi, wo, N):1.f);
	}
	const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col, wire_frame_amount, sp);
	return col;
}

Rgb CoatedGlossyMaterial::sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	const MDat *dat = (MDat *)render_data.arena_;
	const NodeStack stack(dat->stack_);

	const float cos_ng_wo = sp.ng_ * wo;
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	Vec3 hs(0.f);
	s.pdf_ = 0.f;
	float kr, kt;

	Vec3::fresnel(wo, n, (ior_shader_ ? ior_ + ior_shader_->getScalar(stack) : ior_), kr, kt);

	// missing! get components
	bool use[3] = {false, false, false};
	float sum = 0.f, accum_c[3], val[3], width[3];
	int c_index[3]; // entry values: 0 := specular part, 1 := glossy part, 2:= diffuse part;
	int rc_index[3]; // reverse fmapping of cIndex, gives position of spec/glossy/diff in val/width array
	accum_c[0] = kr;
	accum_c[1] = kt * (1.f - dat->p_diffuse_);
	accum_c[2] = kt * (dat->p_diffuse_);

	int n_match = 0, pick = -1;
	for(int i = 0; i < n_bsdf_; ++i)
	{
		if((s.flags_ & c_flags_[i]) == c_flags_[i])
		{
			use[i] = true;
			width[n_match] = accum_c[i];
			c_index[n_match] = i;
			rc_index[i] = n_match;
			sum += width[n_match];
			val[n_match] = sum;
			++n_match;
		}
	}
	if(!n_match || sum < 0.00001)
	{
		wi = Vec3::reflectDir(n, wo);	//If the sampling is prematurely ended for some reason, we need to give wi a value or it will be undefinded causing unexpected problems as black dots. By default I've chosen wi to be the reflection of wo, but it's an arbitrary choice.
		return Rgb(0.f);
	}

	else if(n_match == 1) { pick = 0; width[0] = 1.f; }
	else
	{
		const float inv_sum = 1.f / sum;
		for(int i = 0; i < n_match; ++i)
		{
			val[i] *= inv_sum;
			width[i] *= inv_sum;
			if((s.s_1_ <= val[i]) && (pick < 0)) pick = i;
		}
	}
	if(pick < 0) pick = n_match - 1;
	float s_1;
	if(pick > 0) s_1 = (s.s_1_ - val[pick - 1]) / width[pick];
	else s_1 = s.s_1_ / width[pick];

	Rgb scolor(0.f);
	switch(c_index[pick])
	{
		case C_SPECULAR: // specular reflect
			wi = Vec3::reflectDir(n, wo);

			if(mirror_color_shader_) scolor = mirror_color_shader_->getColor(stack) * kr;
			else scolor = mirror_color_ * kr;//)/std::abs(N*wi);

			scolor *= (mirror_shader_ ? mirror_shader_->getScalar(stack) : mirror_strength_);

			s.pdf_ = width[pick];
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_; // mirror is symmetrical

				if(mirror_color_shader_) s.col_back_ = mirror_color_shader_->getColor(stack) * kr;
				else s.col_back_ = mirror_color_ * kr;//)/std::abs(N*wi);

				s.col_back_ *= (mirror_shader_ ? mirror_shader_->getScalar(stack) : mirror_strength_);
			}
			break;
		case C_GLOSSY: // glossy
			if(anisotropic_) asAnisoSample_global(hs, s_1, s.s_2_, exp_u_, exp_v_);
			else blinnSample_global(hs, s_1, s.s_2_, (exponent_shader_ ? exponent_shader_->getScalar(stack) : exponent_));
			break;
		case C_DIFFUSE: // lambertian
		default:
			wi = sample::cosHemisphere(n, sp.nu_, sp.nv_, s_1, s.s_2_);
			const float cos_ng_wi = sp.ng_ * wi;
			if(cos_ng_wo * cos_ng_wi < 0)
			{
				scolor = Rgb(0.f);
				float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
				applyWireFrame(scolor, wire_frame_amount, sp);
				return scolor;
			}
	}

	float wi_n = std::abs(wi * n);
	const float wo_n = std::abs(wo * n);

	if(c_index[pick] != C_SPECULAR)
	{
		// evaluate BSDFs and PDFs...
		if(use[C_GLOSSY])
		{
			float glossy;
			float cos_wo_h;
			Vec3 h(0.f);
			if(c_index[pick] != C_GLOSSY)
			{
				h = (wi + wo).normalize();
				hs = Vec3(h * sp.nu_, h * sp.nv_, h * n);
				cos_wo_h = wo * h;
			}
			else
			{
				h = hs.x_ * sp.nu_ + hs.y_ * sp.nv_ + hs.z_ * n;
				cos_wo_h = wo * h;
				if(cos_wo_h < 0.f)
				{
					h.reflect(n);
					cos_wo_h = wo * h;
				}
				// Compute incident direction by reflecting wo about H
				wi = Vec3::reflectDir(h, wo);
				const float cos_ng_wi = sp.ng_ * wi;
				if(cos_ng_wo * cos_ng_wi < 0)
				{
					scolor = Rgb(0.f);
					const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
					applyWireFrame(scolor, wire_frame_amount, sp);
					return scolor;
				}
			}

			wi_n = std::abs(wi * n);

			if(anisotropic_)
			{
				s.pdf_ += asAnisoPdf_global(hs, cos_wo_h, exp_u_, exp_v_) * width[rc_index[C_GLOSSY]];
				glossy = asAnisoD_global(hs, exp_u_, exp_v_) * schlickFresnel_global(cos_wo_h, dat->glossy_) / asDivisor_global(cos_wo_h, wo_n, wi_n);
			}
			else
			{
				float cos_hn = h * n;
				s.pdf_ += blinnPdf_global(cos_hn, cos_wo_h, (exponent_shader_ ? exponent_shader_->getScalar(stack) : exponent_)) * width[rc_index[C_GLOSSY]];
				glossy = blinnD_global(cos_hn, (exponent_shader_ ? exponent_shader_->getScalar(stack) : exponent_)) * schlickFresnel_global(cos_wo_h, dat->glossy_) / asDivisor_global(cos_wo_h, wo_n, wi_n);
			}
			scolor = glossy * kt * (glossy_shader_ ? glossy_shader_->getColor(stack) : gloss_color_);
		}

		if(use[C_DIFFUSE])
		{
			Rgb add_col = diffuseReflectFresnel_global(wi_n, wo_n, dat->glossy_, dat->diffuse_, (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diff_color_), kt);

			if(diffuse_reflection_shader_) add_col *= diffuse_reflection_shader_->getScalar(stack);

			if(oren_nayar_)
			{
				const double texture_sigma = (sigma_oren_shader_ ? sigma_oren_shader_->getScalar(stack) : 0.f);
				const bool use_texture_sigma = (sigma_oren_shader_ ? true : false);

				add_col *= orenNayar(wi, wo, n, use_texture_sigma, texture_sigma);
			}

			scolor += add_col;
			s.pdf_ += wi_n * width[rc_index[C_DIFFUSE]];
		}
		w = wi_n / (s.pdf_ * 0.99f + 0.01f);
	}
	else
	{
		w = 1.f;
	}

	s.sampled_flags_ = c_flags_[c_index[pick]];

	const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(scolor, wire_frame_amount, sp);
	return scolor;
}

float CoatedGlossyMaterial::pdf(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &flags) const
{
	const MDat *dat = (MDat *)render_data.arena_;
	const NodeStack stack(dat->stack_);
	const bool transmit = ((sp.ng_ * wo) * (sp.ng_ * wi)) < 0.f;
	if(transmit) return 0.f;
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	float pdf = 0.f;
	float kr, kt;

	Vec3::fresnel(wo, n, (ior_shader_ ? ior_ + ior_shader_->getScalar(stack) : ior_), kr, kt);

	float accum_c[3], sum = 0.f, width;
	accum_c[0] = kr;
	accum_c[1] = kt * (1.f - dat->p_diffuse_);
	accum_c[2] = kt * (dat->p_diffuse_);

	int n_match = 0;
	for(int i = 0; i < n_bsdf_; ++i)
	{
		if((flags & c_flags_[i]) == c_flags_[i])
		{
			width = accum_c[i];
			sum += width;
			if(i == C_GLOSSY)
			{
				const Vec3 h = (wi + wo).normalize();
				const float cos_wo_h = wo * h;
				const float cos_n_h = n * h;
				if(anisotropic_)
				{
					Vec3 hs(h * sp.nu_, h * sp.nv_, cos_n_h);
					pdf += asAnisoPdf_global(hs, cos_wo_h, exp_u_, exp_v_) * width;
				}
				else pdf += blinnPdf_global(cos_n_h, cos_wo_h, (exponent_shader_ ? exponent_shader_->getScalar(stack) : exponent_)) * width;
			}
			else if(i == C_DIFFUSE)
			{
				pdf += std::abs(wi * n) * width;
			}
			++n_match;
		}
	}
	if(!n_match || sum < 0.00001) return 0.f;
	return pdf / sum;
}

Material::Specular CoatedGlossyMaterial::getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Specular specular;
	const MDat *dat = (MDat *)render_data.arena_;
	const NodeStack stack(dat->stack_);
	const bool outside = sp.ng_ * wo >= 0;
	Vec3 n, ng;
	const float cos_wo_n = sp.n_ * wo;
	if(outside)
	{
		n = (cos_wo_n >= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
		ng = sp.ng_;
	}
	else
	{
		n = (cos_wo_n <= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
		ng = -sp.ng_;
	}
	float kr, kt;
	Vec3::fresnel(wo, n, (ior_shader_ ? ior_ + ior_shader_->getScalar(stack) : ior_), kr, kt);
	if(render_data.raylevel_ > 5) return specular;
	specular.reflect_.dir_ = wo;
	specular.reflect_.dir_.reflect(n);
	if(mirror_color_shader_) specular.reflect_.col_ = mirror_color_shader_->getColor(stack) * kr;
	else specular.reflect_.col_ = mirror_color_ * kr;//)/std::abs(N*wi);
	specular.reflect_.col_ *= (mirror_shader_ ? mirror_shader_->getScalar(stack) : mirror_strength_);
	const float cos_wi_ng = specular.reflect_.dir_ * ng;
	if(cos_wi_ng < 0.01)
	{
		specular.reflect_.dir_ += (0.01 - cos_wi_ng) * ng;
		specular.reflect_.dir_.normalize();
	}
	specular.reflect_.enabled_ = true;
	const float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(specular.reflect_.col_, wire_frame_amount, sp);
	return specular;
}

std::unique_ptr<Material> CoatedGlossyMaterial::factory(ParamMap &params, std::list< ParamMap > &param_list, Scene &scene)
{
	Rgb col(1.f), dcol(1.f), mir_col(1.f);
	float refl = 1.f;
	float diff = 0.f;
	float exponent = 50.f; //wild guess, do sth better
	float mirror_strength = 1.f;
	double ior = 1.4;
	bool as_diff = true;
	bool aniso = false;
	std::string name;
	std::string s_visibility = "normal";
	int mat_pass_index = 0;
	bool receive_shadows = true;
	int additionaldepth = 0;
	float samplingfactor = 1.f;
	float wire_frame_amount = 0.f;           //!< Wireframe shading amount
	float wire_frame_thickness = 0.01f;      //!< Wireframe thickness
	float wire_frame_exponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
	Rgb wire_frame_color = Rgb(1.f); //!< Wireframe shading color

	params.getParam("color", col);
	params.getParam("diffuse_color", dcol);
	params.getParam("diffuse_reflect", diff);
	params.getParam("glossy_reflect", refl);
	params.getParam("as_diffuse", as_diff);
	params.getParam("exponent", exponent);
	params.getParam("anisotropic", aniso);
	params.getParam("IOR", ior);
	params.getParam("mirror_color", mir_col);
	params.getParam("specular_reflect", mirror_strength);

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

	if(ior == 1.f) ior = 1.0000001f;

	auto mat = std::unique_ptr<CoatedGlossyMaterial>(new CoatedGlossyMaterial(col, dcol, mir_col, mirror_strength, refl, diff, ior, exponent, as_diff, visibility));

	mat->setMaterialIndex(mat_pass_index);
	mat->receive_shadows_ = receive_shadows;
	mat->additional_depth_ = additionaldepth;

	mat->wireframe_amount_ = wire_frame_amount;
	mat->wireframe_thickness_ = wire_frame_thickness;
	mat->wireframe_exponent_ = wire_frame_exponent;
	mat->wireframe_color_ = wire_frame_color;

	mat->setSamplingFactor(samplingfactor);

	if(aniso)
	{
		double e_u = 50.0, e_v = 50.0;
		params.getParam("exp_u", e_u);
		params.getParam("exp_v", e_v);
		mat->anisotropic_ = true;
		mat->exp_u_ = e_u;
		mat->exp_v_ = e_v;
	}

	if(params.getParam("diffuse_brdf", name))
	{
		if(name == "Oren-Nayar")
		{
			double sigma = 0.1;
			params.getParam("sigma", sigma);
			mat->initOrenNayar(sigma);
		}
	}

	std::vector<ShaderNode *> roots;
	std::map<std::string, ShaderNode *> node_list;

	// Prepare our node list
	node_list["diffuse_shader"] = nullptr;
	node_list["glossy_shader"] = nullptr;
	node_list["glossy_reflect_shader"] = nullptr;
	node_list["bump_shader"] = nullptr;
	node_list["exponent_shader"] = nullptr;
	node_list["wireframe_shader"]    = nullptr;
	node_list["IOR_shader"] = nullptr;
	node_list["sigma_oren_shader"]   = nullptr;
	node_list["mirror_shader"]       = nullptr;
	node_list["diffuse_refl_shader"] = nullptr;
	node_list["mirror_color_shader"] = nullptr;

	if(mat->loadNodes(param_list, scene))
	{
		mat->parseNodes(params, roots, node_list);
	}
	else Y_ERROR << "CoatedGlossy: loadNodes() failed!" << YENDL;

	mat->diffuse_shader_ = node_list["diffuse_shader"];
	mat->glossy_shader_ = node_list["glossy_shader"];
	mat->glossy_reflection_shader_ = node_list["glossy_reflect_shader"];
	mat->bump_shader_ = node_list["bump_shader"];
	mat->exponent_shader_ = node_list["exponent_shader"];
	mat->wireframe_shader_ = node_list["wireframe_shader"];
	mat->ior_shader_ = node_list["IOR_shader"];
	mat->mirror_shader_ = node_list["mirror_shader"];
	mat->sigma_oren_shader_ = node_list["sigma_oren_shader"];
	mat->diffuse_reflection_shader_  = node_list["diffuse_refl_shader"];
	mat->mirror_color_shader_  = node_list["mirror_color_shader"];

	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		if(mat->diffuse_shader_) mat->getNodeList(mat->diffuse_shader_, mat->color_nodes_);
		if(mat->glossy_shader_) mat->getNodeList(mat->glossy_shader_, mat->color_nodes_);
		if(mat->glossy_reflection_shader_) mat->getNodeList(mat->glossy_reflection_shader_, mat->color_nodes_);
		if(mat->mirror_shader_)       mat->getNodeList(mat->mirror_shader_, mat->color_nodes_);
		if(mat->sigma_oren_shader_)    mat->getNodeList(mat->sigma_oren_shader_, mat->color_nodes_);
		if(mat->ior_shader_) mat->getNodeList(mat->ior_shader_, mat->color_nodes_);
		if(mat->exponent_shader_) mat->getNodeList(mat->exponent_shader_, mat->color_nodes_);
		if(mat->wireframe_shader_)    mat->getNodeList(mat->wireframe_shader_, mat->color_nodes_);
		if(mat->diffuse_reflection_shader_)  mat->getNodeList(mat->diffuse_reflection_shader_, mat->color_nodes_);
		if(mat->mirror_color_shader_)  mat->getNodeList(mat->mirror_color_shader_, mat->color_nodes_);
		if(mat->bump_shader_) mat->getNodeList(mat->bump_shader_, mat->bump_nodes_);
	}
	mat->req_mem_ = mat->req_node_mem_ + sizeof(MDat);
	return mat;
}

Rgb CoatedGlossyMaterial::getDiffuseColor(const RenderData &render_data) const {
	MDat *dat = (MDat *)render_data.arena_;
	NodeStack stack(dat->stack_);

	if(as_diffuse_ || with_diffuse_) return (diffuse_reflection_shader_ ? diffuse_reflection_shader_->getScalar(stack) : 1.f) * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diff_color_);
	else return Rgb(0.f);
}

Rgb CoatedGlossyMaterial::getGlossyColor(const RenderData &render_data) const {
	MDat *dat = (MDat *)render_data.arena_;
	NodeStack stack(dat->stack_);

	return (glossy_reflection_shader_ ? glossy_reflection_shader_->getScalar(stack) : reflectivity_) * (glossy_shader_ ? glossy_shader_->getColor(stack) : gloss_color_);
}

Rgb CoatedGlossyMaterial::getMirrorColor(const RenderData &render_data) const {
	MDat *dat = (MDat *)render_data.arena_;
	NodeStack stack(dat->stack_);

	return (mirror_shader_ ? mirror_shader_->getScalar(stack) : mirror_strength_) * (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : mirror_color_);
}

END_YAFARAY
