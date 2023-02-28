/****************************************************************************
 *      roughglass.cc: a dielectric material with rough surface
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
#include "material/material_rough_glass.h"
#include "shader/shader_node.h"
#include "common/logger.h"
#include "material/material_utils_microfacet.h"
#include "color/spectrum.h"
#include "param/param.h"
#include "geometry/surface.h"
#include "material/sample.h"
#include "volume/handler/volume_handler.h"
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> RoughGlassMaterial::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(ior_);
	PARAM_META(filter_color_);
	PARAM_META(transmit_filter_);
	PARAM_META(mirror_color_);
	PARAM_META(alpha_);
	PARAM_META(dispersion_power_);
	PARAM_META(fake_shadows_);
	PARAM_META(absorption_color_);
	PARAM_META(absorption_dist_);
	const auto shaders_meta_map{shadersMeta<Params, ShaderNodeType>()};
	param_meta_map.insert(shaders_meta_map.begin(), shaders_meta_map.end());
	return param_meta_map;
}

RoughGlassMaterial::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(ior_);
	PARAM_LOAD(filter_color_);
	PARAM_LOAD(transmit_filter_);
	PARAM_LOAD(mirror_color_);
	PARAM_LOAD(alpha_);
	PARAM_LOAD(dispersion_power_);
	PARAM_LOAD(fake_shadows_);
	PARAM_LOAD(absorption_color_);
	PARAM_LOAD(absorption_dist_);
}

ParamMap RoughGlassMaterial::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(ior_);
	PARAM_SAVE(filter_color_);
	PARAM_SAVE(transmit_filter_);
	PARAM_SAVE(mirror_color_);
	PARAM_SAVE(alpha_);
	PARAM_SAVE(dispersion_power_);
	PARAM_SAVE(fake_shadows_);
	PARAM_SAVE(absorption_color_);
	PARAM_SAVE(absorption_dist_);
	const auto shader_nodes_names{getShaderNodesNames<ThisClassType_t, ShaderNodeType>(shaders_, only_non_default)};
	param_map.append(shader_nodes_names);
	return param_map;
}

std::pair<std::unique_ptr<Material>, ParamResult> RoughGlassMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	param_result.merge(checkShadersParams<Params, ShaderNodeType>(param_map));
	auto material{std::make_unique<RoughGlassMaterial>(logger, param_result, param_map, scene.getMaterials())};
	if(material->params_.absorption_color_.r_ < 1.f || material->params_.absorption_color_.g_ < 1.f || material->params_.absorption_color_.b_ < 1.f)
	{
		//deprecated method:
		Rgb sigma(0.f);
		const float maxlog = math::log(1e38f);
		sigma.r_ = (material->params_.absorption_color_.r_ > 1e-38f) ? -math::log(material->params_.absorption_color_.r_) : maxlog;
		sigma.g_ = (material->params_.absorption_color_.g_ > 1e-38f) ? -math::log(material->params_.absorption_color_.g_) : maxlog;
		sigma.b_ = (material->params_.absorption_color_.b_ > 1e-38f) ? -math::log(material->params_.absorption_color_.b_) : maxlog;
		if(material->params_.absorption_dist_ != 0.f) sigma *= 1.f / material->params_.absorption_dist_;
		material->absorb_ = true;
		material->beer_sigma_a_ = sigma;
		material->bsdf_flags_ |= BsdfFlags{BsdfFlags::Volumetric};
		// creat volume handler (backwards compatibility)
		ParamMap map;
		map["type"] = std::string("beer");
		map["absorption_col"] = material->params_.absorption_color_;
		map["absorption_dist"] = Parameter(material->params_.absorption_dist_);
		material->vol_i_ = VolumeHandler::factory(logger, scene, name, map).first;
		material->bsdf_flags_ |= BsdfFlags{BsdfFlags::Volumetric};
	}
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
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + material->getAsParamMap(true).print());
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(material), param_result};
}

RoughGlassMaterial::RoughGlassMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) :
		ParentClassType_t{logger, param_result, param_map, materials}, params_{param_result, param_map}
{
	bsdf_flags_ = BsdfFlags::AllGlossy;
	if(params_.fake_shadows_) bsdf_flags_ |= BsdfFlags{BsdfFlags::Filter};
	if(params_.dispersion_power_ > 0.0)
	{
		disperse_ = true;
		std::tie(cauchy_a_, cauchy_b_) = spectrum::cauchyCoefficients(params_.ior_, params_.dispersion_power_);
		bsdf_flags_ |= BsdfFlags{BsdfFlags::Dispersive};
	}
}

std::unique_ptr<const MaterialData> RoughGlassMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data{std::make_unique<MaterialData_t>(bsdf_flags_, color_nodes_.size() + bump_nodes_.size())};
	if(shaders_[ShaderNodeType::Bump]) evalBump(mat_data->node_tree_data_, sp, shaders_[ShaderNodeType::Bump], camera);
	for(const auto &node : color_nodes_) node->eval(mat_data->node_tree_data_, sp, camera);
	return mat_data;
}

Rgb RoughGlassMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	const bool outside = sp.ng_ * wo > 0.f;
	s.pdf_ = 1.f;
	const float alpha_texture = shaders_[ShaderNodeType::Roughness] ? shaders_[ShaderNodeType::Roughness]->getScalar(mat_data->node_tree_data_) + 0.001f : 0.001f;
	const float alpha_2 = shaders_[ShaderNodeType::Roughness] ? alpha_texture * alpha_texture : a_2_;
	Vec3f h{microfacet::ggxSample(alpha_2, s.s_1_, s.s_2_)};
	h = h[Axis::X] * sp.uvn_.u_ + h[Axis::Y] * sp.uvn_.v_ + h[Axis::Z] * n;
	h.normalize();
	float cur_ior = params_.ior_;
	if(shaders_[ShaderNodeType::Ior]) cur_ior += shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);

	if(disperse_ && chromatic)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;
		if(shaders_[ShaderNodeType::Ior]) std::tie(cur_cauchy_a, cur_cauchy_b) = spectrum::cauchyCoefficients(cur_ior, params_.dispersion_power_);
		cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);
	}

	const float cos_theta = h * n;
	const float cos_theta_2 = cos_theta * cos_theta;
	const float tan_theta_2 = (1.f - cos_theta_2) / std::max(1.0e-8f, cos_theta_2);
	float glossy_d = 0.f;
	if(cos_theta > 0.f) glossy_d = microfacet::ggxD(alpha_2, cos_theta_2, tan_theta_2);
	const float wo_h = wo * h;
	const float wo_n = wo * n;

	Rgb ret(0.f);
	float kr, kt;
	if(microfacet::refract(((outside) ? 1.f / cur_ior : cur_ior), wo, wi, h, wo_h, kr, kt))
	{
		if(s.s_1_ < kt && s.flags_.has(BsdfFlags::Transmit))
		{
			const float wi_n = wi * n;
			const float wi_h = wi * h;

			float glossy_g = 0.f;
			if((wi_h * wi_n) > 0.f && (wo_h * wo_n) > 0.f) glossy_g = microfacet::ggxG(alpha_2, wi_n, wo_n);

			float ior_wi = 1.f;
			float ior_wo = 1.f;

			if(outside) ior_wi = cur_ior;
			else ior_wo = cur_ior;

			float ht = ior_wo * wo_h + ior_wi * wi_h;
			const float jacobian = (ior_wi * ior_wi) / std::max(1.0e-8f, ht * ht);
			const float glossy = std::abs((wo_h * wi_h) / (wi_n * wo_n)) * kt * glossy_g * glossy_d * jacobian;

			s.pdf_ = microfacet::ggxPdf(glossy_d, cos_theta, jacobian * std::abs(wi_h));
			s.sampled_flags_ = ((disperse_ && chromatic) ? BsdfFlags::Dispersive : BsdfFlags::Glossy) | BsdfFlags::Transmit;

			ret = glossy * getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_);
			w = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
		else if(s.flags_.has(BsdfFlags::Reflect))
		{
			microfacet::reflect(wo, wi, h);

			const float wi_n = wi * n;
			const float wi_h = wi * h;

			const float glossy_g = microfacet::ggxG(alpha_2, wi_n, wo_n);

			const float jacobian = 1.f / std::max(1.0e-8f, (4.f * std::abs(wi_h)));
			const float glossy = (kr * glossy_g * glossy_d) / std::max(1.0e-8f, (4.f * std::abs(wo_n * wi_n)));

			s.pdf_ = microfacet::ggxPdf(glossy_d, cos_theta, jacobian);
			s.sampled_flags_ = BsdfFlags::Glossy | BsdfFlags::Reflect;

			ret = glossy * getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_);

			w = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(h);
		s.sampled_flags_ = BsdfFlags::Glossy | BsdfFlags::Reflect;
		ret = Rgb{1.f};
		w = 1.f;
	}
	applyWireFrame(ret, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return ret;
}

Rgb RoughGlassMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const
{
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	const bool outside = sp.ng_ * wo > 0.f;
	s.pdf_ = 1.f;
	const float alpha_texture = shaders_[ShaderNodeType::Roughness] ? shaders_[ShaderNodeType::Roughness]->getScalar(mat_data->node_tree_data_) + 0.001f : 0.001f;
	const float alpha_2 = shaders_[ShaderNodeType::Roughness] ? alpha_texture * alpha_texture : a_2_;

	Vec3f h{microfacet::ggxSample(alpha_2, s.s_1_, s.s_2_)};
	h = h[Axis::X] * sp.uvn_.u_ + h[Axis::Y] * sp.uvn_.v_ + h[Axis::Z] * n;
	h.normalize();

	float cur_ior = params_.ior_;
	if(shaders_[ShaderNodeType::Ior]) cur_ior += shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);

	if(disperse_ && chromatic)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;
		if(shaders_[ShaderNodeType::Ior]) std::tie(cur_cauchy_a, cur_cauchy_b) = spectrum::cauchyCoefficients(cur_ior, params_.dispersion_power_);
		cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);
	}

	const float cos_theta = h * n;
	const float cos_theta_2 = cos_theta * cos_theta;
	const float tan_theta_2 = (1.f - cos_theta_2) / std::max(1.0e-8f, cos_theta_2);

	float glossy_d = 0.f;
	if(cos_theta > 0.f) glossy_d = microfacet::ggxD(alpha_2, cos_theta_2, tan_theta_2);

	const float wo_h = wo * h;
	const float wo_n = wo * n;

	Rgb ret(0.f);
	s.sampled_flags_ = BsdfFlags::None;

	float kr, kt;
	Vec3f wi;
	if(microfacet::refract(((outside) ? 1.f / cur_ior : cur_ior), wo, wi, h, wo_h, kr, kt))
	{
		if(s.flags_.has(BsdfFlags::Transmit))
		{
			const float wi_n = wi * n;
			const float wi_h = wi * h;

			float glossy_g = 0.f;
			if((wi_h * wi_n) > 0.f && (wo_h * wo_n) > 0.f) glossy_g = microfacet::ggxG(alpha_2, wi_n, wo_n);

			float ior_wi = 1.f;
			float ior_wo = 1.f;

			if(outside) ior_wi = cur_ior;
			else ior_wo = cur_ior;

			const float ht = ior_wo * wo_h + ior_wi * wi_h;
			const float jacobian = (ior_wi * ior_wi) / std::max(1.0e-8f, ht * ht);

			const float glossy = std::abs((wo_h * wi_h) / (wi_n * wo_n)) * kt * glossy_g * glossy_d * jacobian;

			s.pdf_ = microfacet::ggxPdf(glossy_d, cos_theta, jacobian * std::abs(wi_h));
			s.sampled_flags_ = ((disperse_ && chromatic) ? BsdfFlags::Dispersive : BsdfFlags::Glossy) | BsdfFlags::Transmit;

			ret = glossy * getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_);
			w[0] = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[0] = wi;

		}
		if(s.flags_.has(BsdfFlags::Reflect))
		{
			microfacet::reflect(wo, wi, h);
			const float wi_n = wi * n;
			const float wi_h = wi * h;
			const float glossy_g = microfacet::ggxG(alpha_2, wi_n, wo_n);
			const float jacobian = 1.f / std::max(1.0e-8f, (4.f * std::abs(wi_h)));
			const float glossy = (kr * glossy_g * glossy_d) / std::max(1.0e-8f, (4.f * std::abs(wo_n * wi_n)));
			s.pdf_ = microfacet::ggxPdf(glossy_d, cos_theta, jacobian);
			s.sampled_flags_ |= BsdfFlags{BsdfFlags::Glossy | BsdfFlags::Reflect};
			tcol = glossy * getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_);
			w[1] = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[1] = wi;
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(h);
		s.sampled_flags_ |= BsdfFlags{BsdfFlags::Glossy | BsdfFlags::Reflect};
		dir[0] = wi;
		ret = Rgb{1.f};
		w[0] = 1.f;
	}
	applyWireFrame(ret, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return ret;
}

Rgb RoughGlassMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	const auto [kr, kt]{Vec3f::fresnel(wo, n, getShaderScalar(shaders_[ShaderNodeType::Ior], mat_data->node_tree_data_, params_.ior_))};
	Rgb result = kt * getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_);
	applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return result;
}

float RoughGlassMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	float alpha = std::max(0.f, std::min(1.f, 1.f - getTransparency(mat_data, sp, wo, camera).energy()));
	applyWireFrame(alpha, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return alpha;
}

Rgb RoughGlassMaterial::getGlossyColor(const NodeTreeData &node_tree_data) const {
	return shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getColor(node_tree_data) : params_.mirror_color_;
}

Rgb RoughGlassMaterial::getTransColor(const NodeTreeData &node_tree_data) const {
	return shaders_[ShaderNodeType::FilterColor] ? shaders_[ShaderNodeType::FilterColor]->getColor(node_tree_data) : filter_color_;
}

Rgb RoughGlassMaterial::getMirrorColor(const NodeTreeData &node_tree_data) const {
	return shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getColor(node_tree_data) : params_.mirror_color_;
}

} //namespace yafaray
