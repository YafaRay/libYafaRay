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
#include "common/param.h"
#include "geometry/surface.h"
#include "render/render_data.h"
#include "volume/volume.h"

BEGIN_YAFARAY

RoughGlassMaterial::RoughGlassMaterial(Logger &logger, float ior, Rgb filt_c, const Rgb &srcol, bool fake_s, float alpha, float disp_pow, Visibility e_visibility):
		NodeMaterial(logger), filter_color_(filt_c), specular_reflection_color_(srcol), ior_(ior), a_2_(alpha * alpha), fake_shadow_(fake_s), dispersion_power_(disp_pow)
{
	visibility_ = e_visibility;
	bsdf_flags_ = BsdfFlags::AllGlossy;
	if(fake_s) bsdf_flags_ |= BsdfFlags::Filter;
	if(disp_pow > 0.0)
	{
		disperse_ = true;
		spectrum::cauchyCoefficients(ior, disp_pow, cauchy_a_, cauchy_b_);
		bsdf_flags_ |= BsdfFlags::Dispersive;
	}
}

std::unique_ptr<const MaterialData> RoughGlassMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	std::unique_ptr<MaterialData> mat_data = createMaterialData(color_nodes_.size() + bump_nodes_.size());
	if(bump_shader_) evalBump(mat_data->node_tree_data_, sp, bump_shader_, camera);
	for(const auto &node : color_nodes_) node->eval(mat_data->node_tree_data_, sp, camera);
	return mat_data;
}

Rgb RoughGlassMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	const bool outside = sp.ng_ * wo > 0.f;
	s.pdf_ = 1.f;
	const float alpha_texture = roughness_shader_ ? roughness_shader_->getScalar(mat_data->node_tree_data_) + 0.001f : 0.001f;
	const float alpha_2 = roughness_shader_ ? alpha_texture * alpha_texture : a_2_;
	Vec3 h = microfacet::ggxSample(alpha_2, s.s_1_, s.s_2_);
	h = h.x_ * sp.nu_ + h.y_ * sp.nv_ + h.z_ * n;
	h.normalize();
	float cur_ior = ior_;
	if(ior_shader_) cur_ior += ior_shader_->getScalar(mat_data->node_tree_data_);

	if(disperse_ && chromatic)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;
		if(ior_shader_) spectrum::cauchyCoefficients(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
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
		if(s.s_1_ < kt && s.flags_.hasAny(BsdfFlags::Transmit))
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

			ret = glossy * getShaderColor(filter_col_shader_, mat_data->node_tree_data_, filter_color_);
			w = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
		else if(s.flags_.hasAny(BsdfFlags::Reflect))
		{
			microfacet::reflect(wo, wi, h);

			const float wi_n = wi * n;
			const float wi_h = wi * h;

			const float glossy_g = microfacet::ggxG(alpha_2, wi_n, wo_n);

			const float jacobian = 1.f / std::max(1.0e-8f, (4.f * std::abs(wi_h)));
			const float glossy = (kr * glossy_g * glossy_d) / std::max(1.0e-8f, (4.f * std::abs(wo_n * wi_n)));

			s.pdf_ = microfacet::ggxPdf(glossy_d, cos_theta, jacobian);
			s.sampled_flags_ = BsdfFlags::Glossy | BsdfFlags::Reflect;

			ret = glossy * getShaderColor(mirror_color_shader_, mat_data->node_tree_data_, specular_reflection_color_);

			w = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(h);
		s.sampled_flags_ = BsdfFlags::Glossy | BsdfFlags::Reflect;
		ret = 1.f;
		w = 1.f;
	}
	applyWireFrame(ret, wireframe_shader_, mat_data->node_tree_data_, sp);
	return ret;
}

Rgb RoughGlassMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const
{
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	const bool outside = sp.ng_ * wo > 0.f;
	s.pdf_ = 1.f;
	const float alpha_texture = roughness_shader_ ? roughness_shader_->getScalar(mat_data->node_tree_data_) + 0.001f : 0.001f;
	const float alpha_2 = roughness_shader_ ? alpha_texture * alpha_texture : a_2_;

	Vec3 h = microfacet::ggxSample(alpha_2, s.s_1_, s.s_2_);
	h = h.x_ * sp.nu_ + h.y_ * sp.nv_ + h.z_ * n;
	h.normalize();

	float cur_ior = ior_;
	if(ior_shader_) cur_ior += ior_shader_->getScalar(mat_data->node_tree_data_);

	if(disperse_ && chromatic)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;
		if(ior_shader_) spectrum::cauchyCoefficients(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
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
	Vec3 wi;
	if(microfacet::refract(((outside) ? 1.f / cur_ior : cur_ior), wo, wi, h, wo_h, kr, kt))
	{
		if(s.flags_.hasAny(BsdfFlags::Transmit))
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

			ret = glossy * getShaderColor(filter_col_shader_, mat_data->node_tree_data_, filter_color_);
			w[0] = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[0] = wi;

		}
		if(s.flags_.hasAny(BsdfFlags::Reflect))
		{
			microfacet::reflect(wo, wi, h);
			const float wi_n = wi * n;
			const float wi_h = wi * h;
			const float glossy_g = microfacet::ggxG(alpha_2, wi_n, wo_n);
			const float jacobian = 1.f / std::max(1.0e-8f, (4.f * std::abs(wi_h)));
			const float glossy = (kr * glossy_g * glossy_d) / std::max(1.0e-8f, (4.f * std::abs(wo_n * wi_n)));
			s.pdf_ = microfacet::ggxPdf(glossy_d, cos_theta, jacobian);
			s.sampled_flags_ |= BsdfFlags::Glossy | BsdfFlags::Reflect;
			tcol = glossy * getShaderColor(mirror_color_shader_, mat_data->node_tree_data_, specular_reflection_color_);
			w[1] = std::abs(wi_n) / std::max(0.1f, s.pdf_); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[1] = wi;
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(h);
		s.sampled_flags_ |= BsdfFlags::Glossy | BsdfFlags::Reflect;
		dir[0] = wi;
		ret = 1.f;
		w[0] = 1.f;
	}
	applyWireFrame(ret, wireframe_shader_, mat_data->node_tree_data_, sp);
	return ret;
}

Rgb RoughGlassMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	float kr, kt;
	Vec3::fresnel(wo, n, getShaderScalar(ior_shader_, mat_data->node_tree_data_, ior_), kr, kt);
	Rgb result = kt * getShaderColor(filter_col_shader_, mat_data->node_tree_data_, filter_color_);
	applyWireFrame(result, wireframe_shader_, mat_data->node_tree_data_, sp);
	return result;
}

float RoughGlassMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	float alpha = std::max(0.f, std::min(1.f, 1.f - getTransparency(mat_data, sp, wo, camera).energy()));
	applyWireFrame(alpha, wireframe_shader_, mat_data->node_tree_data_, sp);
	return alpha;
}

float RoughGlassMaterial::getMatIor() const
{
	return ior_;
}

std::unique_ptr<Material> RoughGlassMaterial::factory(Logger &logger, ParamMap &params, std::list<ParamMap> &nodes_params, const Scene &scene)
{
	float ior = 1.4;
	float filt = 0.f;
	float alpha = 0.5f;
	float disp_power = 0.0;
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
	params.getParam("alpha", alpha);
	params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);

	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);
	params.getParam("mat_pass_index", mat_pass_index);
	params.getParam("additionaldepth", additionaldepth);
	params.getParam("samplingfactor", samplingfactor);

	params.getParam("wireframe_amount", wire_frame_amount);
	params.getParam("wireframe_thickness", wire_frame_thickness);
	params.getParam("wireframe_exponent", wire_frame_exponent);
	params.getParam("wireframe_color", wire_frame_color);

	const Visibility visibility = visibility::fromString(s_visibility);

	alpha = std::max(1e-4f, std::min(alpha * 0.5f, 1.f));

	auto mat = std::unique_ptr<RoughGlassMaterial>(new RoughGlassMaterial(logger, ior, filt * filt_col + Rgb(1.f - filt), sr_col, fake_shad, alpha, disp_power, visibility));

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
				mat->vol_i_ = VolumeHandler::factory(logger, map, scene);
				mat->bsdf_flags_ |= BsdfFlags::Volumetric;
			}
		}
	}

	std::map<std::string, const ShaderNode *> root_nodes_map;
	// Prepare our node list
	root_nodes_map["mirror_color_shader"] = nullptr;
	root_nodes_map["bump_shader"] = nullptr;
	root_nodes_map["filter_color_shader"] = nullptr;
	root_nodes_map["IOR_shader"] = nullptr;
	root_nodes_map["wireframe_shader"] = nullptr;
	root_nodes_map["roughness_shader"] = nullptr;

	std::vector<const ShaderNode *> root_nodes_list;
	mat->nodes_map_ = mat->loadNodes(nodes_params, scene, logger);
	if(!mat->nodes_map_.empty()) mat->parseNodes(params, root_nodes_list, root_nodes_map, mat->nodes_map_, logger);

	mat->mirror_color_shader_ = root_nodes_map["mirror_color_shader"];
	mat->bump_shader_ = root_nodes_map["bump_shader"];
	mat->filter_col_shader_ = root_nodes_map["filter_color_shader"];
	mat->ior_shader_ = root_nodes_map["IOR_shader"];
	mat->wireframe_shader_ = root_nodes_map["wireframe_shader"];
	mat->roughness_shader_ = root_nodes_map["roughness_shader"];

	// solve nodes order
	if(!root_nodes_list.empty())
	{
		const std::vector<const ShaderNode *> nodes_sorted = mat->solveNodesOrder(root_nodes_list, mat->nodes_map_, logger);
		if(mat->mirror_color_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->mirror_color_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->roughness_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->roughness_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->ior_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->ior_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->wireframe_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->wireframe_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->filter_col_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->filter_col_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->bump_shader_) mat->bump_nodes_ = mat->getNodeList(mat->bump_shader_, nodes_sorted);
	}
	return mat;
}

Rgb RoughGlassMaterial::getGlossyColor(const NodeTreeData &node_tree_data) const {
	return mirror_color_shader_ ? mirror_color_shader_->getColor(node_tree_data) : specular_reflection_color_;
}

Rgb RoughGlassMaterial::getTransColor(const NodeTreeData &node_tree_data) const {
	return filter_col_shader_ ? filter_col_shader_->getColor(node_tree_data) : filter_color_;
}

Rgb RoughGlassMaterial::getMirrorColor(const NodeTreeData &node_tree_data) const {
	return mirror_color_shader_ ? mirror_color_shader_->getColor(node_tree_data) : specular_reflection_color_;
}

END_YAFARAY
