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

#include "material/material_shiny_diffuse.h"
#include "param/param.h"
#include "sampler/sample.h"
#include "shader/shader_node.h"
#include "geometry/surface.h"
#include "common/logger.h"
#include "material/sample.h"
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> ShinyDiffuseMaterial::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(diffuse_color_);
	PARAM_META(mirror_color_);
	PARAM_META(transparency_);
	PARAM_META(translucency_);
	PARAM_META(diffuse_reflect_);
	PARAM_META(specular_reflect_);
	PARAM_META(emit_);
	PARAM_META(fresnel_effect_);
	PARAM_META(ior_);
	PARAM_META(transmit_filter_);
	PARAM_META(diffuse_brdf_);
	PARAM_META(sigma_);
	const auto shaders_meta_map{shadersMeta<Params, ShaderNodeType>()};
	param_meta_map.insert(shaders_meta_map.begin(), shaders_meta_map.end());
	return param_meta_map;
}

ShinyDiffuseMaterial::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(diffuse_color_);
	PARAM_LOAD(mirror_color_);
	PARAM_LOAD(transparency_);
	PARAM_LOAD(translucency_);
	PARAM_LOAD(diffuse_reflect_);
	PARAM_LOAD(specular_reflect_);
	PARAM_LOAD(emit_);
	PARAM_LOAD(fresnel_effect_);
	PARAM_LOAD(ior_);
	PARAM_LOAD(transmit_filter_);
	PARAM_ENUM_LOAD(diffuse_brdf_);
	PARAM_LOAD(sigma_);
}

ParamMap ShinyDiffuseMaterial::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(diffuse_color_);
	PARAM_SAVE(mirror_color_);
	PARAM_SAVE(transparency_);
	PARAM_SAVE(translucency_);
	PARAM_SAVE(diffuse_reflect_);
	PARAM_SAVE(specular_reflect_);
	PARAM_SAVE(emit_);
	PARAM_SAVE(fresnel_effect_);
	PARAM_SAVE(ior_);
	PARAM_SAVE(transmit_filter_);
	PARAM_ENUM_SAVE(diffuse_brdf_);
	PARAM_SAVE(sigma_);
	const auto shader_nodes_names{getShaderNodesNames<ThisClassType_t, ShaderNodeType>(shaders_, only_non_default)};
	param_map.append(shader_nodes_names);
	return param_map;
}

std::pair<std::unique_ptr<Material>, ParamResult> ShinyDiffuseMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	param_result.merge(checkShadersParams<Params, ShaderNodeType>(param_map));
	auto material{std::make_unique<ShinyDiffuseMaterial>(logger, param_result, param_map, scene.getMaterials())};
	material->nodes_map_ = loadNodes(nodes_param_maps, scene, logger);
	std::map<std::string, const ShaderNode *> root_nodes_map;
	// Prepare our node list
	for(size_t shader_index = 0; shader_index < material->shaders_.size(); ++shader_index)
	{
		root_nodes_map[ShaderNodeType{shader_index}.print()] = nullptr;
	}
	std::vector<const ShaderNode *> root_nodes_list;
	if(!material->nodes_map_.empty()) parseNodes(param_map, root_nodes_list, root_nodes_map, material->nodes_map_, logger);
	for(size_t shader_index = 0; shader_index < material->shaders_.size(); ++shader_index)
	{
		material->shaders_[shader_index] = root_nodes_map[ShaderNodeType{shader_index}.print()];
	}
	// solve nodes order
	if(!root_nodes_list.empty())
	{
		const auto nodes_sorted{solveNodesOrder(root_nodes_list, material->nodes_map_, logger)};
		for(size_t shader_index = 0; shader_index < material->shaders_.size(); ++shader_index)
		{
			if(material->shaders_[shader_index])
			{
				if(ShaderNodeType{shader_index}.isBump())
				{
					material->bump_nodes_ = getNodeList(material->shaders_[shader_index], nodes_sorted);
				}
				else
				{
					const auto shader_nodes_list{getNodeList(material->shaders_[shader_index], nodes_sorted)};
					material->color_nodes_.insert(material->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
				}
			}
		}
	}
	material->config();
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + material->getAsParamMap(true).print());
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(material), param_result};
}

ShinyDiffuseMaterial::ShinyDiffuseMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) :
		ParentClassType_t{logger, param_result, param_map, materials}, params_{param_result, param_map}
{
	if(params_.emit_ > 0.f) bsdf_flags_ |= BsdfFlags{BsdfFlags::Emit};
	if(params_.diffuse_brdf_ == DiffuseBrdf::OrenNayar) initOrenNayar(params_.sigma_);
}

/*! ATTENTION! You *MUST* call this function before using the material, no matter
    if you want to use shaderNodes or not!
*/
void ShinyDiffuseMaterial::config()
{
	n_bsdf_ = 0;
	float acc = 1.f;
	if(params_.specular_reflect_ > 0.00001f || shaders_[ShaderNodeType::MirrorColor])
	{
		is_mirror_ = true;
		if(shaders_[ShaderNodeType::MirrorColor]) components_view_independent_[0] = true;
		else if(!params_.fresnel_effect_) acc = 1.f - params_.specular_reflect_;
		bsdf_flags_ |= BsdfFlags{BsdfFlags::Specular | BsdfFlags::Reflect};
		c_flags_[n_bsdf_] = BsdfFlags::Specular | BsdfFlags::Reflect;
		c_index_[n_bsdf_] = 0;
		++n_bsdf_;
	}
	if(params_.transparency_ * acc > 0.00001f || shaders_[ShaderNodeType::Transparency])
	{
		is_transparent_ = true;
		if(shaders_[ShaderNodeType::Transparency]) components_view_independent_[1] = true;
		else acc *= 1.f - params_.transparency_;
		bsdf_flags_ |= BsdfFlags{BsdfFlags::Transmit | BsdfFlags::Filter};
		c_flags_[n_bsdf_] = BsdfFlags::Transmit | BsdfFlags::Filter;
		c_index_[n_bsdf_] = 1;
		++n_bsdf_;
	}
	if(params_.translucency_ * acc > 0.00001f || shaders_[ShaderNodeType::Translucency])
	{
		is_translucent_ = true;
		if(shaders_[ShaderNodeType::Translucency]) components_view_independent_[2] = true;
		else acc *= 1.f - params_.transparency_;
		bsdf_flags_ |= BsdfFlags{BsdfFlags::Diffuse | BsdfFlags::Transmit};
		c_flags_[n_bsdf_] = BsdfFlags::Diffuse | BsdfFlags::Transmit;
		c_index_[n_bsdf_] = 2;
		++n_bsdf_;
	}
	if(params_.diffuse_reflect_ * acc > 0.00001f)
	{
		is_diffuse_ = true;
		if(shaders_[ShaderNodeType::Diffuse]) components_view_independent_[3] = true;
		bsdf_flags_ |= BsdfFlags{BsdfFlags::Diffuse | BsdfFlags::Reflect};
		c_flags_[n_bsdf_] = BsdfFlags::Diffuse | BsdfFlags::Reflect;
		c_index_[n_bsdf_] = 3;
		++n_bsdf_;
	}
}

// component should be initialized with mMirrorStrength, mTransparencyStrength, mTranslucencyStrength, mDiffuseStrength
// since values for which useNode is false do not get touched, so it can be applied
// twice, for view-independent (initBSDF) and view-dependent (sample/eval) nodes

std::array<float, 4> ShinyDiffuseMaterial::getComponents(const std::array<bool, 4> &use_nodes, const NodeTreeData &node_tree_data) const
{
	std::array<float, 4> components;
	if(is_mirror_) components[0] = use_nodes[0] ? shaders_[ShaderNodeType::MirrorColor]->getScalar(node_tree_data) : params_.specular_reflect_;
	else components[0] = 0.f;
	if(is_transparent_) components[1] = use_nodes[1] ? shaders_[ShaderNodeType::Transparency]->getScalar(node_tree_data) : params_.transparency_;
	else components[1] = 0.f;
	if(is_translucent_) components[2] = use_nodes[2] ? shaders_[ShaderNodeType::Translucency]->getScalar(node_tree_data) : params_.translucency_;
	else components[2] = 0.f;
	if(is_diffuse_) components[3] = params_.diffuse_reflect_;
	else components[3] = 0.f;
	return components;
}

float ShinyDiffuseMaterial::getFresnelKr(const Vec3f &wo, const Vec3f &n, float current_ior_squared) const
{
	if(params_.fresnel_effect_)
	{
		const Vec3f N{((wo * n) < 0.f) ? -n : n};
		const float c = wo * N;
		float g = current_ior_squared + c * c - 1.f;
		if(g < 0.f) g = 0.f;
		else g = math::sqrt(g);
		const float aux = c * (g + c);
		return ((0.5f * (g - c) * (g - c)) / ((g + c) * (g + c))) * (1.f + ((aux - 1) * (aux - 1)) / ((aux + 1) * (aux + 1)));
	}
	else return 1.f;
}

// calculate the absolute value of scattering components from the "normalized"
// fractions which are between 0 (no scattering) and 1 (scatter all remaining light)
// Kr is an optional reflection multiplier (e.g. from Fresnel)
std::array<float, 4> ShinyDiffuseMaterial::accumulate(const std::array<float, 4> &components, float kr)
{
	std::array<float, 4> accum;
	accum[0] = components[0] * kr;
	float acc = 1.f - accum[0];
	accum[1] = components[1] * acc;
	acc *= 1.f - components[1];
	accum[2] = components[2] * acc;
	acc *= 1.f - components[2];
	accum[3] = components[3] * acc;
	return accum;
}

std::unique_ptr<const MaterialData> ShinyDiffuseMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data{std::make_unique<MaterialData_t>(bsdf_flags_, color_nodes_.size() + bump_nodes_.size())};
	if(shaders_[ShaderNodeType::Bump]) evalBump(mat_data->node_tree_data_, sp, shaders_[ShaderNodeType::Bump], camera);
	for(const auto &node : color_nodes_) node->eval(mat_data->node_tree_data_, sp, camera);
	mat_data->components_ = getComponents(components_view_independent_, mat_data->node_tree_data_);
	return mat_data;
}

/** Initialize Oren Nayar reflectance.
 *  Initialize Oren Nayar A and B coefficient.
 *  @param  sigma Roughness of the surface
 */
void ShinyDiffuseMaterial::initOrenNayar(double sigma)
{
	const double sigma_squared = sigma * sigma;
	oren_nayar_a_ = 1.0 - 0.5 * (sigma_squared / (sigma_squared + 0.33));
	oren_nayar_b_ = 0.45 * sigma_squared / (sigma_squared + 0.09);
}

/** Calculate Oren Nayar reflectance.
 *  Calculate Oren Nayar reflectance for a given reflection.
 *  @param  wi Reflected ray direction
 *  @param  wo Incident ray direction
 *  @param  n  Surface normal
 *  @note   http://en.wikipedia.org/wiki/Oren-Nayar_reflectance_model
 */
float ShinyDiffuseMaterial::orenNayar(const Vec3f &wi, const Vec3f &wo, const Vec3f &n, bool use_texture_sigma, double texture_sigma) const
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
	else return std::min(1.f, std::max(0.f, oren_nayar_a_ + oren_nayar_b_ * maxcos_f * sin_alpha * tan_beta));
}

Rgb ShinyDiffuseMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const
{
	const float cos_ng_wo = sp.ng_ * wo;
	const float cos_ng_wl = sp.ng_ * wl;
	// face forward:
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	if(!bsdfs.has(bsdf_flags_ & BsdfFlags::Diffuse)) return Rgb{0.f};

	float cur_ior_squared;
	if(shaders_[ShaderNodeType::Ior])
	{
		cur_ior_squared = params_.ior_ + shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = ior_squared_;

	const float kr = getFresnelKr(wo, n, cur_ior_squared);

	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const float m_t = (1.f - kr * mat_data_specific->components_[0]) * (1.f - mat_data_specific->components_[1]);

	const bool transmit = (cos_ng_wo * cos_ng_wl) < 0.f;

	if(transmit) // light comes from opposite side of surface
	{
		if(is_translucent_) return mat_data_specific->components_[2] * m_t * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_);
	}

	if(n * wl < 0.0 && !Material::params_.flat_material_) return Rgb{0.f};
	float m_d = m_t * (1.f - mat_data_specific->components_[2]) * mat_data_specific->components_[3];

	if(params_.diffuse_brdf_ == DiffuseBrdf::OrenNayar)
	{
		const double texture_sigma = getShaderScalar(shaders_[ShaderNodeType::SigmaOrenNayar], mat_data->node_tree_data_, 0.f);
		const bool use_texture_sigma = static_cast<bool>(shaders_[ShaderNodeType::SigmaOrenNayar]);
		if(params_.diffuse_brdf_ == DiffuseBrdf::OrenNayar) m_d *= orenNayar(wo, wl, n, use_texture_sigma, texture_sigma);
	}

	if(shaders_[ShaderNodeType::DiffuseReflect]) m_d *= shaders_[ShaderNodeType::DiffuseReflect]->getScalar(mat_data->node_tree_data_);

	Rgb result = m_d * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_);
	applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return result;
}

Rgb ShinyDiffuseMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const
{
	Rgb result = (shaders_[ShaderNodeType::Diffuse] ? shaders_[ShaderNodeType::Diffuse]->getColor(mat_data->node_tree_data_) * params_.emit_ : emit_color_);
	applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return result;
}

Rgb ShinyDiffuseMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const float cos_ng_wo = sp.ng_ * wo;
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};

	float cur_ior_squared;
	if(shaders_[ShaderNodeType::Ior])
	{
		cur_ior_squared = params_.ior_ + shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = ior_squared_;

	const float kr = getFresnelKr(wo, n, cur_ior_squared);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const std::array<float, 4> accum_c = accumulate(mat_data_specific->components_, kr);

	float sum = 0.f;
	std::array<float, 4> val, width;
	std::array<BsdfFlags, 4> choice{BsdfFlags::None, BsdfFlags::None, BsdfFlags::None, BsdfFlags::None};
	int n_match = 0, pick = -1;
	for(int i = 0; i < n_bsdf_; ++i)
	{
		if((s.flags_ & c_flags_[i]) == c_flags_[i])
		{
			width[n_match] = accum_c[c_index_[i]];
			sum += width[n_match];
			choice[n_match] = c_flags_[i];
			val[n_match] = sum;
			++n_match;
		}
	}
	if(!n_match || sum < 0.00001) { s.sampled_flags_ = BsdfFlags::None; s.pdf_ = 0.f; return Rgb{1.f}; }
	const float inv_sum = 1.f / sum;
	for(int i = 0; i < n_match; ++i)
	{
		val[i] *= inv_sum;
		width[i] *= inv_sum;
		if((s.s_1_ <= val[i]) && (pick < 0)) pick = i;
	}
	if(pick < 0) pick = n_match - 1;
	float s_1;
	if(pick > 0) s_1 = (s.s_1_ - val[pick - 1]) / width[pick];
	else s_1 = s.s_1_ / width[pick];

	Rgb scolor(0.f);
	switch(choice[pick].value())
	{
		case BsdfFlags::SpecularReflect:
			wi = Vec3f::reflectDir(n, wo);
			s.pdf_ = width[pick];
			scolor = getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_) * (accum_c[0]);
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_;
				s.col_back_ = scolor / std::max(std::abs(sp.n_ * wo), 1.0e-6f);
			}
			scolor *= 1.f / std::max(std::abs(sp.n_ * wi), 1.0e-6f);
			break;
		case BsdfFlags::SpecularTransmit:
			wi = -wo;
			scolor = accum_c[1] * (params_.transmit_filter_ * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_) + Rgb(1.f - params_.transmit_filter_));
			if(std::abs(wi * n) /*cos_n*/ < 1e-6) s.pdf_ = 0.f;
			else s.pdf_ = width[pick];
			break;
		case BsdfFlags::Translucency:
			wi = sample::cosHemisphere(-n, sp.uvn_, s_1, s.s_2_);
			if(cos_ng_wo * (sp.ng_ * wi) /* cos_ng_wi */ < 0) scolor = accum_c[2] * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_);
			s.pdf_ = std::abs(wi * n) * width[pick]; break;
		case BsdfFlags::DiffuseReflect:
		default:
			wi = sample::cosHemisphere(n, sp.uvn_, s_1, s.s_2_);
			if(cos_ng_wo * (sp.ng_ * wi) /* cos_ng_wi */ > 0) scolor = accum_c[3] * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_);

			if(params_.diffuse_brdf_ == DiffuseBrdf::OrenNayar)
			{
				const double texture_sigma = getShaderScalar(shaders_[ShaderNodeType::SigmaOrenNayar], mat_data->node_tree_data_, 0.f);
				const bool use_texture_sigma = static_cast<bool>(shaders_[ShaderNodeType::SigmaOrenNayar]);
				scolor *= orenNayar(wo, wi, n, use_texture_sigma, texture_sigma);
			}
			s.pdf_ = std::abs(wi * n) * width[pick]; break;
	}
	s.sampled_flags_ = choice[pick];
	w = std::abs(wi * sp.n_) / (s.pdf_ * 0.99f + 0.01f);

	const float alpha = getAlpha(mat_data, sp, wo, camera);
	w = w * (alpha) + 1.f * (1.f - alpha);

	applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return scolor;
}

float ShinyDiffuseMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const
{
	if(!bsdfs.has(BsdfFlags::Diffuse)) return 0.f;

	float pdf = 0.f;
	const float cos_ng_wo = sp.ng_ * wo;
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};

	float cur_ior_squared;
	if(shaders_[ShaderNodeType::Ior])
	{
		cur_ior_squared = params_.ior_ + shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = ior_squared_;

	const float kr = getFresnelKr(wo, n, cur_ior_squared);

	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const std::array<float, 4> accum_c = accumulate(mat_data_specific->components_, kr);
	float sum = 0.f, width;
	int n_match = 0;
	for(int i = 0; i < n_bsdf_; ++i)
	{
		if(bsdfs.has(c_flags_[i]))
		{
			width = accum_c[c_index_[i]];
			sum += width;
			switch(c_flags_[i].value())
			{
				case BsdfFlags::Diffuse | BsdfFlags::Transmit:  // translucency (diffuse transmitt)
					if(cos_ng_wo * (sp.ng_ * wi) /*cos_ng_wi*/ < 0) pdf += std::abs(wi * n) * width;
					break;
				case BsdfFlags::Diffuse | BsdfFlags::Reflect:  // lambertian
					pdf += std::abs(wi * n) * width;
					break;
				default:
					break;
			}
			++n_match;
		}
	}
	if(!n_match || sum < 0.00001) return 0.f;
	return pdf / sum;
}


/** Perfect specular reflection.
 *  Calculate perfect specular reflection and refraction from the material for
 *  a given surface point \a sp and a given incident ray direction \a wo
 *  @param  chromatic_data Render state
 *  @param  sp Surface point
 *  @param  wo Incident ray direction
 *  @param  do_reflect Boolean value which is true if you have a reflection, false otherwise
 *  @param  do_refract Boolean value which is true if you have a refraction, false otherwise
 *  @param  wi Array of two vectors to record reflected ray direction (wi[0]) and refracted ray direction (wi[1])
 *  @param  col Array of two colors to record reflected ray color (col[0]) and refracted ray color (col[1])
 */
Specular ShinyDiffuseMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const
{
	Specular specular;
	const bool backface = wo * sp.ng_ < 0.f;
	const Vec3f n {backface ? -sp.n_ : sp.n_};
	const Vec3f ng{backface ? -sp.ng_ : sp.ng_};
	float cur_ior_squared;
	if(shaders_[ShaderNodeType::Ior])
	{
		cur_ior_squared = params_.ior_ + shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = ior_squared_;
	const float kr = getFresnelKr(wo, n, cur_ior_squared);
	if(is_transparent_)
	{
		specular.refract_ = std::make_unique<DirectionColor>();
		specular.refract_->dir_ = -wo;
		const Rgb tcol = params_.transmit_filter_ * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_) + Rgb(1.f - params_.transmit_filter_);
		const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
		specular.refract_->col_ = (1.f - mat_data_specific->components_[0] * kr) * mat_data_specific->components_[1] * tcol;
		applyWireFrame(specular.refract_->col_, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	}
	if(is_mirror_)
	{
		specular.reflect_ = std::make_unique<DirectionColor>();
		//logger_.logWarning(sp.N << " | " << N);
		specular.reflect_->dir_ = wo;
		specular.reflect_->dir_.reflect(n);
		const float cos_wi_ng = specular.reflect_->dir_ * ng;
		if(cos_wi_ng < 0.01f)
		{
			specular.reflect_->dir_ += (0.01f - cos_wi_ng) * ng;
			specular.reflect_->dir_.normalize();
		}
		const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
		specular.reflect_->col_ = getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_) * (mat_data_specific->components_[0] * kr);
		applyWireFrame(specular.reflect_->col_, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	}
	return specular;
}

Rgb ShinyDiffuseMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	if(!is_transparent_) return Rgb{0.f};
	float accum = 1.f;
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};

	float cur_ior_squared;
	if(shaders_[ShaderNodeType::Ior])
	{
		cur_ior_squared = params_.ior_ + shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = ior_squared_;

	const float kr = getFresnelKr(wo, n, cur_ior_squared);

	if(is_mirror_) accum = 1.f - kr * getShaderScalar(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.specular_reflect_);
	if(is_transparent_) //uhm...should actually be true if this function gets called anyway...
	{
		accum *= shaders_[ShaderNodeType::Transparency] ? shaders_[ShaderNodeType::Transparency]->getScalar(mat_data->node_tree_data_) * accum : params_.transparency_ * accum;
	}
	const Rgb tcol = params_.transmit_filter_ * getShaderColor(shaders_[ShaderNodeType::Diffuse], mat_data->node_tree_data_, params_.diffuse_color_) + Rgb(1.f - params_.transmit_filter_);
	Rgb result = accum * tcol;
	applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return result;
}

float ShinyDiffuseMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	if(is_transparent_)
	{
		const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
		float cur_ior_squared;
		if(shaders_[ShaderNodeType::Ior])
		{
			cur_ior_squared = params_.ior_ + shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
			cur_ior_squared *= cur_ior_squared;
		}
		else cur_ior_squared = ior_squared_;
		const float kr = getFresnelKr(wo, n, cur_ior_squared);
		const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
		const float refl = (1.f - mat_data_specific->components_[0] * kr) * mat_data_specific->components_[1];
		float result = 1.f - refl;
		applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
		return result;
	}
	else return 1.f;
}

Rgb ShinyDiffuseMaterial::getDiffuseColor(const NodeTreeData &node_tree_data) const
{
	if(is_diffuse_) return (shaders_[ShaderNodeType::DiffuseReflect] ? shaders_[ShaderNodeType::DiffuseReflect]->getScalar(node_tree_data) : params_.diffuse_reflect_) * (shaders_[ShaderNodeType::Diffuse] ? shaders_[ShaderNodeType::Diffuse]->getColor(node_tree_data) : params_.diffuse_color_);
	else return Rgb{0.f};
}

Rgb ShinyDiffuseMaterial::getGlossyColor(const NodeTreeData &node_tree_data) const
{
	if(is_mirror_) return (shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getScalar(node_tree_data) : params_.specular_reflect_) * (shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getColor(node_tree_data) : params_.mirror_color_);
	else return Rgb{0.f};
}

Rgb ShinyDiffuseMaterial::getTransColor(const NodeTreeData &node_tree_data) const
{
	if(is_transparent_) return (shaders_[ShaderNodeType::Transparency] ? shaders_[ShaderNodeType::Transparency]->getScalar(node_tree_data) : params_.transparency_) * (shaders_[ShaderNodeType::Diffuse] ? shaders_[ShaderNodeType::Diffuse]->getColor(node_tree_data) : params_.diffuse_color_);
	else return Rgb{0.f};
}

Rgb ShinyDiffuseMaterial::getMirrorColor(const NodeTreeData &node_tree_data) const
{
	if(is_mirror_) return (shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getScalar(node_tree_data) : params_.specular_reflect_) * (shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getColor(node_tree_data) : params_.mirror_color_);
	else return Rgb{0.f};
}

Rgb ShinyDiffuseMaterial::getSubSurfaceColor(const NodeTreeData &node_tree_data) const
{
	if(is_translucent_) return (shaders_[ShaderNodeType::Translucency] ? shaders_[ShaderNodeType::Translucency]->getScalar(node_tree_data) : params_.translucency_) * (shaders_[ShaderNodeType::Diffuse] ? shaders_[ShaderNodeType::Diffuse]->getColor(node_tree_data) : params_.diffuse_color_);
	else return Rgb{0.f};
}

} //namespace yafaray
