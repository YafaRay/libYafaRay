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
#include "param/param.h"
#include "geometry/surface.h"
#include "common/logger.h"
#include "material/sample.h"
#include "scene/scene.h"

namespace yafaray {

/*! Coated Glossy Material.
	A Material with Phong/Anisotropic Phong microfacet base layer and a layer of
	(dielectric) perfectly specular coating. This is to simulate surfaces like
	metallic paint */

CoatedGlossyMaterial::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(glossy_color_);
	PARAM_LOAD(diffuse_color_);
	PARAM_LOAD(diffuse_reflect_);
	PARAM_LOAD(glossy_reflect_);
	PARAM_LOAD(as_diffuse_);
	PARAM_LOAD(exponent_);
	PARAM_LOAD(anisotropic_);
	PARAM_LOAD(ior_);
	PARAM_LOAD(mirror_color_);
	PARAM_LOAD(specular_reflect_);
	PARAM_LOAD(exp_u_);
	PARAM_LOAD(exp_v_);
	PARAM_ENUM_LOAD(diffuse_brdf_);
	PARAM_LOAD(sigma_);
	PARAM_SHADERS_LOAD;
}

ParamMap CoatedGlossyMaterial::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(glossy_color_);
	PARAM_SAVE(diffuse_color_);
	PARAM_SAVE(diffuse_reflect_);
	PARAM_SAVE(glossy_reflect_);
	PARAM_SAVE(as_diffuse_);
	PARAM_SAVE(exponent_);
	PARAM_SAVE(anisotropic_);
	PARAM_SAVE(ior_);
	PARAM_SAVE(mirror_color_);
	PARAM_SAVE(specular_reflect_);
	PARAM_SAVE(exp_u_);
	PARAM_SAVE(exp_v_);
	PARAM_ENUM_SAVE(diffuse_brdf_);
	PARAM_SHADERS_SAVE;
	PARAM_SAVE_END;
}

ParamMap CoatedGlossyMaterial::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Material>, ParamResult> CoatedGlossyMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	auto material{std::make_unique<ThisClassType_t>(logger, param_result, param_map, scene.getMaterials())};
	material->nodes_map_ = NodeMaterial::loadNodes(nodes_param_maps, scene, logger);
	std::map<std::string, const ShaderNode *> root_nodes_map;
	// Prepare our node list
	for(size_t shader_index = 0; shader_index < material->shaders_.size(); ++shader_index)
	{
		root_nodes_map[ShaderNodeType{static_cast<unsigned char>(shader_index)}.print()] = nullptr;
	}
	std::vector<const ShaderNode *> root_nodes_list;
	if(!material->nodes_map_.empty()) NodeMaterial::parseNodes(param_map, root_nodes_list, root_nodes_map, material->nodes_map_, logger);
	for(size_t shader_index = 0; shader_index < material->shaders_.size(); ++shader_index)
	{
		material->shaders_[shader_index] = root_nodes_map[ShaderNodeType{static_cast<unsigned char>(shader_index)}.print()];
	}
	// solve nodes order
	if(!root_nodes_list.empty())
	{
		const std::vector<const ShaderNode *> nodes_sorted = NodeMaterial::solveNodesOrder(root_nodes_list, material->nodes_map_, logger);
		for(size_t shader_index = 0; shader_index < material->shaders_.size(); ++shader_index)
		{
			if(material->shaders_[shader_index])
			{
				if(ShaderNodeType{static_cast<unsigned char>(shader_index)}.isBump())
				{
					material->bump_nodes_ = NodeMaterial::getNodeList(material->shaders_[shader_index], nodes_sorted);
				}
				else
				{
					const std::vector<const ShaderNode *> shader_nodes_list = NodeMaterial::getNodeList(material->shaders_[shader_index], nodes_sorted);
					material->color_nodes_.insert(material->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
				}
			}
		}
	}
	if(param_result.notOk()) logger.logWarning(param_result.print<CoatedGlossyMaterial>(name, {"type"}));
	return {std::move(material), param_result};
}

CoatedGlossyMaterial::CoatedGlossyMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) :
		ParentClassType_t{logger, param_result, param_map, materials}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	c_flags_[BsdfComponent::ComponentSpecular] = (BsdfFlags::Specular | BsdfFlags::Reflect);
	c_flags_[BsdfComponent::ComponentGlossy] = params_.as_diffuse_ ? (BsdfFlags::Diffuse | BsdfFlags::Reflect) : (BsdfFlags::Glossy | BsdfFlags::Reflect);

	if(params_.diffuse_reflect_ > 0)
	{
		c_flags_[BsdfComponent::ComponentDiffuse] = BsdfFlags::Diffuse | BsdfFlags::Reflect;
		with_diffuse_ = true;
		n_bsdf_ = 3;
	}
	else
	{
		c_flags_[BsdfComponent::ComponentDiffuse] = BsdfFlags::None;
		n_bsdf_ = 2;
	}
	bsdf_flags_ = c_flags_[BsdfComponent::ComponentSpecular] | c_flags_[BsdfComponent::ComponentGlossy] | c_flags_[BsdfComponent::ComponentDiffuse];
	if(params_.diffuse_brdf_ == DiffuseBrdf::OrenNayar) initOrenNayar(params_.sigma_);
}

std::unique_ptr<const MaterialData> CoatedGlossyMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data{std::make_unique<MaterialData_t>(bsdf_flags_, color_nodes_.size() + bump_nodes_.size())};
	if(shaders_[ShaderNodeType::Bump]) evalBump(mat_data->node_tree_data_, sp, shaders_[ShaderNodeType::Bump], camera);
	for(const auto &node : color_nodes_) node->eval(mat_data->node_tree_data_, sp, camera);
	mat_data->diffuse_ = params_.diffuse_reflect_;
	mat_data->glossy_ = getShaderScalar(shaders_[ShaderNodeType::GlossyReflect], mat_data->node_tree_data_, params_.glossy_reflect_);
	mat_data->p_diffuse_ = std::min(0.6f, 1.f - (mat_data->glossy_ / (mat_data->glossy_ + (1.f - mat_data->glossy_) * mat_data->diffuse_)));
	return mat_data;
}

void CoatedGlossyMaterial::initOrenNayar(double sigma)
{
	const double sigma_2 = sigma * sigma;
	oren_a_ = 1.0 - 0.5 * (sigma_2 / (sigma_2 + 0.33));
	oren_b_ = 0.45 * sigma_2 / (sigma_2 + 0.09);
}

float CoatedGlossyMaterial::orenNayar(const Vec3f &wi, const Vec3f &wo, const Vec3f &n, bool use_texture_sigma, double texture_sigma) const
{
	const float cos_ti = std::max(-1.f, std::min(1.f, n * wi));
	const float cos_to = std::max(-1.f, std::min(1.f, n * wo));
	float maxcos_f = 0.f;

	if(cos_ti < 0.9999f && cos_to < 0.9999f)
	{
		const Vec3f v_1{(wi - n * cos_ti).normalize()};
		const Vec3f v_2{(wo - n * cos_to).normalize()};
		maxcos_f = std::max(0.f, v_1 * v_2);
	}

	float sin_alpha, tan_beta;

	if(cos_to >= cos_ti)
	{
		sin_alpha = math::sqrt(1.f - cos_ti * cos_ti);
		tan_beta = math::sqrt(1.f - cos_to * cos_to) / ((cos_to == 0.f) ? 1e-8f : cos_to); // white (black on Windows) dots fix for oren-nayar, could happen with bad normals
	}
	else
	{
		sin_alpha = math::sqrt(1.f - cos_to * cos_to);
		tan_beta = math::sqrt(1.f - cos_ti * cos_ti) / ((cos_ti == 0.f) ? 1e-8f : cos_ti); // white (black on Windows) dots fix for oren-nayar, could happen with bad normals
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

Rgb CoatedGlossyMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const
{
	Rgb col(0.f);
	const bool diffuse_flag = bsdfs.has(BsdfFlags::Diffuse);

	if(!force_eval)	//If the flag force_eval = true then the next line will be skipped, necessary for the Glossy Direct render pass
	{
		if(!diffuse_flag || ((sp.ng_ * wl) * (sp.ng_ * wo)) < 0.f) return col;
	}
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	const float wi_n = std::abs(wl * n);
	const float wo_n = std::abs(wo * n);
	const auto [kr, kt]{Vec3f::fresnel(wo, n, getShaderScalar(shaders_[ShaderNodeType::Ior], mat_data->node_tree_data_, ior_))};

	if((params_.as_diffuse_ && diffuse_flag) || (!params_.as_diffuse_ && bsdfs.has(BsdfFlags::Glossy)))
	{
		const Vec3f h{(wo + wl).normalize()}; // half-angle
		const float cos_wi_h = wl * h;
		float glossy;
		if(params_.anisotropic_)
		{
			const Vec3f hs{{h * sp.uvn_.u_, h * sp.uvn_.v_, h * n}};
			const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
			glossy = kt * microfacet::asAnisoD(hs, params_.exp_u_, params_.exp_v_) * microfacet::schlickFresnel(cos_wi_h, mat_data_specific->glossy_) / microfacet::asDivisor(cos_wi_h, wo_n, wi_n);
		}
		else
		{
			const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
			glossy = kt * microfacet::blinnD(h * n, getShaderScalar(shaders_[ShaderNodeType::Exponent], mat_data->node_tree_data_, params_.exponent_)) * microfacet::schlickFresnel(cos_wi_h, mat_data_specific->glossy_) / microfacet::asDivisor(cos_wi_h, wo_n, wi_n);
		}
		col = glossy * getShaderColor(shaders_[ShaderNodeType::Glossy], mat_data->node_tree_data_, params_.glossy_color_);
	}
	if(with_diffuse_ && diffuse_flag)
	{
		const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
		Rgb add_col = mat_data_specific->diffuse_ * (1.f - mat_data_specific->glossy_) * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_) * kt;

		if(shaders_[ShaderNodeType::DiffuseReflect]) add_col *= shaders_[ShaderNodeType::DiffuseReflect]->getScalar(mat_data->node_tree_data_);

		if(params_.diffuse_brdf_ == DiffuseBrdf::OrenNayar)
		{
			const double texture_sigma = getShaderScalar(shaders_[ShaderNodeType::SigmaOrenNayar], mat_data->node_tree_data_, 0.f);
			const bool use_texture_sigma = static_cast<bool>(shaders_[ShaderNodeType::SigmaOrenNayar]);
			add_col *= orenNayar(wl, wo, n, use_texture_sigma, texture_sigma);
		}
		col += add_col;//diffuseReflectFresnel(wiN, woN, mat_data_specific->mGlossy, mat_data_specific->mDiffuse, (diffuseS ? diffuseS->getColor(mat_data->node_tree_data_) : diff_color), Kt) * ((orenNayar)?OrenNayar(wi, wo, N):1.f);
	}
	applyWireFrame(col, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col;
}

Rgb CoatedGlossyMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const float cos_ng_wo = sp.ng_ * wo;
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	Vec3f hs(0.f);
	s.pdf_ = 0.f;
	const auto [kr, kt]{Vec3f::fresnel(wo, n, getShaderScalar(shaders_[ShaderNodeType::Ior], mat_data->node_tree_data_, ior_))};
	// missing! get components
	bool use[3] = {false, false, false};
	float sum = 0.f, accum_c[3], val[3], width[3];
	int c_index[3]; // entry values: 0 := specular part, 1 := glossy part, 2:= diffuse part;
	int rc_index[3]; // reverse fmapping of cIndex, gives position of spec/glossy/diff in val/width array
	accum_c[0] = kr;
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	accum_c[1] = kt * (1.f - mat_data_specific->p_diffuse_);
	accum_c[2] = kt * (mat_data_specific->p_diffuse_);

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
		wi = Vec3f::reflectDir(n, wo);	//If the sampling is prematurely ended for some reason we need to give wi a value, or it will be undefinded causing unexpected problems as black dots. By default, I've chosen wi to be the reflection of wo, but it's an arbitrary choice.
		return Rgb{0.f};
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
		case BsdfComponent::ComponentSpecular: // specular reflect
			wi = Vec3f::reflectDir(n, wo);

			if(shaders_[ShaderNodeType::MirrorColor]) scolor = shaders_[ShaderNodeType::MirrorColor]->getColor(mat_data->node_tree_data_) * kr;
			else scolor = params_.mirror_color_ * kr;//)/std::abs(N*wi);

			scolor *= getShaderScalar(shaders_[ShaderNodeType::Mirror], mat_data->node_tree_data_, params_.specular_reflect_);

			s.pdf_ = width[pick];
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_; // mirror is symmetrical

				if(shaders_[ShaderNodeType::MirrorColor]) s.col_back_ = shaders_[ShaderNodeType::MirrorColor]->getColor(mat_data->node_tree_data_) * kr;
				else s.col_back_ = params_.mirror_color_ * kr;//)/std::abs(N*wi);

				s.col_back_ *= getShaderScalar(shaders_[ShaderNodeType::Mirror], mat_data->node_tree_data_, params_.specular_reflect_);
			}
			break;
		case BsdfComponent::ComponentGlossy: // glossy
			if(params_.anisotropic_) hs = microfacet::asAnisoSample(s_1, s.s_2_, params_.exp_u_, params_.exp_v_);
			else hs = microfacet::blinnSample(s_1, s.s_2_, getShaderScalar(shaders_[ShaderNodeType::Exponent], mat_data->node_tree_data_, params_.exponent_));
			break;
		case BsdfComponent::ComponentDiffuse: // lambertian
		default:
			wi = sample::cosHemisphere(n, sp.uvn_, s_1, s.s_2_);
			const float cos_ng_wi = sp.ng_ * wi;
			if(cos_ng_wo * cos_ng_wi < 0)
			{
				scolor = Rgb(0.f);
				applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
				return scolor;
			}
	}

	float wi_n = std::abs(wi * n);
	const float wo_n = std::abs(wo * n);

	if(c_index[pick] != BsdfComponent::ComponentSpecular)
	{
		// evaluate BSDFs and PDFs...
		if(use[BsdfComponent::ComponentGlossy])
		{
			float glossy;
			float cos_wo_h;
			Vec3f h(0.f);
			if(c_index[pick] != BsdfComponent::ComponentGlossy)
			{
				h = (wi + wo).normalize();
				hs = {{h * sp.uvn_.u_, h * sp.uvn_.v_, h * n}};
				cos_wo_h = wo * h;
			}
			else
			{
				h = hs[Axis::X] * sp.uvn_.u_ + hs[Axis::Y] * sp.uvn_.v_ + hs[Axis::Z] * n;
				cos_wo_h = wo * h;
				if(cos_wo_h < 0.f)
				{
					h.reflect(n);
					cos_wo_h = wo * h;
				}
				// Compute incident direction by reflecting wo about H
				wi = Vec3f::reflectDir(h, wo);
				const float cos_ng_wi = sp.ng_ * wi;
				if(cos_ng_wo * cos_ng_wi < 0)
				{
					scolor = Rgb(0.f);
					applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
					return scolor;
				}
			}

			wi_n = std::abs(wi * n);

			if(params_.anisotropic_)
			{
				s.pdf_ += microfacet::asAnisoPdf(hs, cos_wo_h, params_.exp_u_, params_.exp_v_) * width[rc_index[BsdfComponent::ComponentGlossy]];
				glossy = microfacet::asAnisoD(hs, params_.exp_u_, params_.exp_v_) * microfacet::schlickFresnel(cos_wo_h, mat_data_specific->glossy_) / microfacet::asDivisor(cos_wo_h, wo_n, wi_n);
			}
			else
			{
				float cos_hn = h * n;
				s.pdf_ += microfacet::blinnPdf(cos_hn, cos_wo_h, getShaderScalar(shaders_[ShaderNodeType::Exponent], mat_data->node_tree_data_, params_.exponent_)) * width[rc_index[BsdfComponent::ComponentGlossy]];
				glossy = microfacet::blinnD(cos_hn, getShaderScalar(shaders_[ShaderNodeType::Exponent], mat_data->node_tree_data_, params_.exponent_)) * microfacet::schlickFresnel(cos_wo_h, mat_data_specific->glossy_) / microfacet::asDivisor(cos_wo_h, wo_n, wi_n);
			}
			scolor = glossy * kt * getShaderColor(shaders_[ShaderNodeType::Glossy], mat_data->node_tree_data_, params_.glossy_color_);
		}

		if(use[BsdfComponent::ComponentDiffuse])
		{
			Rgb add_col = microfacet::diffuseReflectFresnel(wi_n, wo_n, mat_data_specific->glossy_, mat_data_specific->diffuse_, getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_), kt);

			if(shaders_[ShaderNodeType::DiffuseReflect]) add_col *= shaders_[ShaderNodeType::DiffuseReflect]->getScalar(mat_data->node_tree_data_);

			if(params_.diffuse_brdf_ == DiffuseBrdf::OrenNayar)
			{
				const double texture_sigma = getShaderScalar(shaders_[ShaderNodeType::SigmaOrenNayar], mat_data->node_tree_data_, 0.f);
				const bool use_texture_sigma = static_cast<bool>(shaders_[ShaderNodeType::SigmaOrenNayar]);

				add_col *= orenNayar(wi, wo, n, use_texture_sigma, texture_sigma);
			}

			scolor += add_col;
			s.pdf_ += wi_n * width[rc_index[BsdfComponent::ComponentDiffuse]];
		}
		w = wi_n / (s.pdf_ * 0.99f + 0.01f);
	}
	else
	{
		w = 1.f;
	}

	s.sampled_flags_ = c_flags_[c_index[pick]];
	applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return scolor;
}

float CoatedGlossyMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags flags) const
{
	const bool transmit = ((sp.ng_ * wo) * (sp.ng_ * wi)) < 0.f;
	if(transmit) return 0.f;
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	float pdf = 0.f;
	const auto [kr, kt]{Vec3f::fresnel(wo, n, getShaderScalar(shaders_[ShaderNodeType::Ior], mat_data->node_tree_data_, ior_))};

	float accum_c[3], sum = 0.f, width;
	accum_c[0] = kr;
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	accum_c[1] = kt * (1.f - mat_data_specific->p_diffuse_);
	accum_c[2] = kt * (mat_data_specific->p_diffuse_);

	int n_match = 0;
	for(int i = 0; i < n_bsdf_; ++i)
	{
		if((flags & c_flags_[i]) == c_flags_[i])
		{
			width = accum_c[i];
			sum += width;
			if(i == BsdfComponent::ComponentGlossy)
			{
				const Vec3f h{(wi + wo).normalize()};
				const float cos_wo_h = wo * h;
				const float cos_n_h = n * h;
				if(params_.anisotropic_)
				{
					Vec3f hs{{h * sp.uvn_.u_, h * sp.uvn_.v_, cos_n_h}};
					pdf += microfacet::asAnisoPdf(hs, cos_wo_h, params_.exp_u_, params_.exp_v_) * width;
				}
				else pdf += microfacet::blinnPdf(cos_n_h, cos_wo_h, getShaderScalar(shaders_[ShaderNodeType::Exponent], mat_data->node_tree_data_, params_.exponent_)) * width;
			}
			else if(i == BsdfComponent::ComponentDiffuse)
			{
				pdf += std::abs(wi * n) * width;
			}
			++n_match;
		}
	}
	if(!n_match || sum < 0.00001) return 0.f;
	return pdf / sum;
}

Specular CoatedGlossyMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const
{
	const bool outside = sp.ng_ * wo >= 0;
	Vec3f n, ng;
	const float cos_wo_n = sp.n_ * wo;
	if(outside)
	{
		n = (cos_wo_n >= 0) ? sp.n_ : (sp.n_ - (1.00001f * cos_wo_n) * wo).normalize();
		ng = sp.ng_;
	}
	else
	{
		n = (cos_wo_n <= 0) ? sp.n_ : (sp.n_ - (1.00001f * cos_wo_n) * wo).normalize();
		ng = -sp.ng_;
	}
	const auto [kr, kt]{Vec3f::fresnel(wo, n, getShaderScalar(shaders_[ShaderNodeType::Ior], mat_data->node_tree_data_, ior_))};
	Specular specular;
	if(ray_level > 5) return specular;
	specular.reflect_ = std::make_unique<DirectionColor>();
	specular.reflect_->dir_ = wo;
	specular.reflect_->dir_.reflect(n);
	if(shaders_[ShaderNodeType::MirrorColor]) specular.reflect_->col_ = shaders_[ShaderNodeType::MirrorColor]->getColor(mat_data->node_tree_data_) * kr;
	else specular.reflect_->col_ = params_.mirror_color_ * kr;//)/std::abs(N*wi);
	specular.reflect_->col_ *= getShaderScalar(shaders_[ShaderNodeType::Mirror], mat_data->node_tree_data_, params_.specular_reflect_);
	const float cos_wi_ng = specular.reflect_->dir_ * ng;
	if(cos_wi_ng < 0.01f)
	{
		specular.reflect_->dir_ += (0.01f - cos_wi_ng) * ng;
		specular.reflect_->dir_.normalize();
	}
	applyWireFrame(specular.reflect_->col_, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return specular;
}

Rgb CoatedGlossyMaterial::getDiffuseColor(const NodeTreeData &node_tree_data) const
{
	if(params_.as_diffuse_ || with_diffuse_) return (shaders_[ShaderNodeType::DiffuseReflect] ? shaders_[ShaderNodeType::DiffuseReflect]->getScalar(node_tree_data) : 1.f) * (shaders_[ShaderNodeType::Diffuse] ? shaders_[ShaderNodeType::Diffuse]->getColor(node_tree_data) : params_.diffuse_color_);
	else return Rgb{0.f};
}

Rgb CoatedGlossyMaterial::getGlossyColor(const NodeTreeData &node_tree_data) const
{
	return (shaders_[ShaderNodeType::GlossyReflect] ? shaders_[ShaderNodeType::GlossyReflect]->getScalar(node_tree_data) : params_.glossy_reflect_) * (shaders_[ShaderNodeType::Glossy] ? shaders_[ShaderNodeType::Glossy]->getColor(node_tree_data) : params_.glossy_color_);
}

Rgb CoatedGlossyMaterial::getMirrorColor(const NodeTreeData &node_tree_data) const
{
	return (shaders_[ShaderNodeType::Mirror] ? shaders_[ShaderNodeType::Mirror]->getScalar(node_tree_data) : params_.specular_reflect_) * (shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getColor(node_tree_data) : params_.mirror_color_);
}

} //namespace yafaray
