/****************************************************************************
 *      material_glass.cc: a dielectric material with dispersion, two trivial mats
 *
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
#include "param/param.h"
#include "material/sample.h"
#include "volume/handler/volume_handler.h"

namespace yafaray {

GlassMaterial::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(ior_);
	PARAM_LOAD(filter_color_);
	PARAM_LOAD(transmit_filter_);
	PARAM_LOAD(mirror_color_);
	PARAM_LOAD(dispersion_power_);
	PARAM_LOAD(fake_shadows_);
	PARAM_LOAD(absorption_color_);
	PARAM_LOAD(absorption_dist_);
	PARAM_SHADERS_LOAD;
}

ParamMap GlassMaterial::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(ior_);
	PARAM_SAVE(filter_color_);
	PARAM_SAVE(transmit_filter_);
	PARAM_SAVE(mirror_color_);
	PARAM_SAVE(dispersion_power_);
	PARAM_SAVE(fake_shadows_);
	PARAM_SAVE(absorption_color_);
	PARAM_SAVE(absorption_dist_);
	PARAM_SHADERS_SAVE;
	PARAM_SAVE_END;
}

ParamMap GlassMaterial::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Material>, ParamError> GlassMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto material{std::make_unique<ThisClassType_t>(logger, param_error, param_map)};
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
		material->vol_i_ = std::unique_ptr<VolumeHandler>(VolumeHandler::factory(logger, scene, name, map).first);
		material->bsdf_flags_ |= BsdfFlags{BsdfFlags::Volumetric};
	}
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
	if(param_error.notOk()) logger.logWarning(param_error.print<GlassMaterial>(name, {"type"}));
	return {std::move(material), param_error};
}

GlassMaterial::GlassMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	bsdf_flags_ = BsdfFlags::AllSpecular;
	if(params_.fake_shadows_) bsdf_flags_ |= BsdfFlags{BsdfFlags::Filter};
	transmit_flags_ = params_.fake_shadows_ ? BsdfFlags::Filter | BsdfFlags::Transmit : BsdfFlags::Specular | BsdfFlags::Transmit;
	if(params_.dispersion_power_ > 0.0)
	{
		disperse_ = true;
		std::tie(cauchy_a_, cauchy_b_) = spectrum::cauchyCoefficients(params_.ior_, params_.dispersion_power_);
		bsdf_flags_ |= BsdfFlags{BsdfFlags::Dispersive};
	}
}

const MaterialData * GlassMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data = new GlassMaterialData(bsdf_flags_, color_nodes_.size() + bump_nodes_.size());
	if(shaders_[ShaderNodeType::Bump]) evalBump(mat_data->node_tree_data_, sp, shaders_[ShaderNodeType::Bump], camera);
	for(const auto &node : color_nodes_) node->eval(mat_data->node_tree_data_, sp, camera);
	return mat_data;
}

Rgb GlassMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	if(!s.flags_.has(BsdfFlags::Specular) && !(s.flags_.has(bsdf_flags_ & BsdfFlags::Dispersive) && chromatic))
	{
		s.pdf_ = 0.f;
		Rgb scolor = Rgb(0.f);
		applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
		return scolor;
	}
	Vec3f refdir, n;
	const bool outside = sp.ng_ * wo > 0;
	const float cos_wo_n = sp.n_ * wo;
	if(outside) n = (cos_wo_n >= 0) ? sp.n_ : (sp.n_ - (1.00001f * cos_wo_n) * wo).normalize();
	else n = (cos_wo_n <= 0) ? sp.n_ : (sp.n_ - (1.00001f * cos_wo_n) * wo).normalize();
	s.pdf_ = 1.f;

	// we need to sample dispersion
	if(disperse_ && chromatic)
	{
		float cur_ior = params_.ior_;
		if(shaders_[ShaderNodeType::Ior]) cur_ior += shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;

		if(shaders_[ShaderNodeType::Ior]) std::tie(cur_cauchy_a, cur_cauchy_b) = spectrum::cauchyCoefficients(cur_ior, params_.dispersion_power_);
		cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);

		if(Vec3f::refract(n, wo, refdir, cur_ior))
		{
			const auto [kr, kt]{Vec3f::fresnel(wo, n, cur_ior)};
			const float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(!s.flags_.has(BsdfFlags::Specular) || s.s_1_ < p_kt)
			{
				wi = refdir;
				s.pdf_ = s.flags_.has(BsdfFlags::Specular | BsdfFlags::Reflect) ? p_kt : 1.f;
				s.sampled_flags_ = BsdfFlags::Dispersive | BsdfFlags::Transmit;
				w = 1.f;
				Rgb scolor = getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_); // * (Kt/std::abs(sp.N*wi));
				applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
				return scolor;
			}
			else if(s.flags_.has(BsdfFlags::Specular | BsdfFlags::Reflect))
			{
				wi = wo;
				wi.reflect(n);
				s.pdf_ = p_kr;
				s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
				w = 1.f;
				Rgb scolor = getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_); // * (Kr/std::abs(sp.N*wi));
				applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
				return scolor;
			}
		}
		else if(s.flags_.has(BsdfFlags::Specular | BsdfFlags::Reflect)) //total inner reflection
		{
			wi = wo;
			wi.reflect(n);
			s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
			w = 1.f;
			Rgb scolor{1.f}; //Rgb(1.f/std::abs(sp.N*wi));
			applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
			return scolor;
		}
	}
	else // no dispersion calculation necessary, regardless of material settings
	{
		float cur_ior = params_.ior_;
		if(shaders_[ShaderNodeType::Ior]) cur_ior += shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);
		if(disperse_ && chromatic)
		{
			float cur_cauchy_a = cauchy_a_;
			float cur_cauchy_b = cauchy_b_;
			if(shaders_[ShaderNodeType::Ior]) std::tie(cur_cauchy_a, cur_cauchy_b) = spectrum::cauchyCoefficients(cur_ior, params_.dispersion_power_);
			cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);
		}

		if(Vec3f::refract(n, wo, refdir, cur_ior))
		{
			const auto [kr, kt]{Vec3f::fresnel(wo, n, cur_ior)};
			float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(s.s_1_ < p_kt && s.flags_.has(transmit_flags_))
			{
				wi = refdir;
				s.pdf_ = p_kt;
				s.sampled_flags_ = transmit_flags_;
				if(s.reverse_)
				{
					s.pdf_back_ = s.pdf_; //wrong...need to calc fresnel explicitly!
					s.col_back_ = getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_);//*(Kt/std::abs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_);//*(Kt/std::abs(sp.N*wi));
				applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
				return scolor;
			}
			else if(s.flags_.has(BsdfFlags::Specular | BsdfFlags::Reflect)) //total inner reflection
			{
				wi = wo;
				wi.reflect(n);
				s.pdf_ = p_kr;
				s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
				if(s.reverse_)
				{
					s.pdf_back_ = s.pdf_; //wrong...need to calc fresnel explicitly!
					s.col_back_ = getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_);// * (Kr/std::abs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_);// * (Kr/std::abs(sp.N*wi));
				applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
				return scolor;
			}
		}
		else if(s.flags_.has(BsdfFlags::Specular | BsdfFlags::Reflect))//total inner reflection
		{
			wi = wo;
			wi.reflect(n);
			s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
			//Rgb tir_col(1.f/std::abs(sp.N*wi));
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_;
				s.col_back_ = Rgb{1.f};//tir_col;
			}
			w = 1.f;
			Rgb scolor{1.f};//tir_col;
			applyWireFrame(scolor, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
			return scolor;
		}
	}
	s.pdf_ = 0.f;
	return Rgb{0.f};
}

Rgb GlassMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	const auto [kr, kt]{Vec3f::fresnel(wo, n, getShaderScalar(shaders_[ShaderNodeType::Ior], mat_data->node_tree_data_, params_.ior_))};
	Rgb result = kt * getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_);
	applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return result;
}

float GlassMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	float alpha = 1.f - getTransparency(mat_data, sp, wo, camera).energy();
	if(alpha < 0.f) alpha = 0.f;
	applyWireFrame(alpha, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return alpha;
}

Specular GlassMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const
{
	const bool outside = sp.ng_ * wo > 0;
	Vec3f n;
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
	Vec3f refdir;

	float cur_ior = params_.ior_;
	if(shaders_[ShaderNodeType::Ior]) cur_ior += shaders_[ShaderNodeType::Ior]->getScalar(mat_data->node_tree_data_);

	if(disperse_ && chromatic)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;
		if(shaders_[ShaderNodeType::Ior]) std::tie(cur_cauchy_a, cur_cauchy_b) = spectrum::cauchyCoefficients(cur_ior, params_.dispersion_power_);
		cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);
	}

	Specular specular;
	if(Vec3f::refract(n, wo, refdir, cur_ior))
	{
		const auto [kr, kt]{Vec3f::fresnel(wo, n, cur_ior)};
		if(!chromatic || !disperse_)
		{
			specular.refract_ = std::make_unique<DirectionColor>();
			specular.refract_->col_ = kt * getShaderColor(shaders_[ShaderNodeType::FilterColor], mat_data->node_tree_data_, filter_color_);
			specular.refract_->dir_ = refdir;
		}
		//FIXME? If the above does not happen, in this case, we need to sample dispersion, i.e. not considered specular

		// accounting for fresnel reflection when leaving refractive material is a real performance
		// killer as rays keep bouncing inside objects and contribute little after few bounces, so limit we it:
		if(outside || ray_level < 3)
		{
			specular.reflect_ = std::make_unique<DirectionColor>();
			specular.reflect_->dir_ = wo;
			specular.reflect_->dir_.reflect(n);
			specular.reflect_->col_ = getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_) * kr;
		}
	}
	else //total inner reflection
	{
		specular.reflect_ = std::make_unique<DirectionColor>();
		specular.reflect_->col_ = getShaderColor(shaders_[ShaderNodeType::MirrorColor], mat_data->node_tree_data_, params_.mirror_color_);
		specular.reflect_->dir_ = wo;
		specular.reflect_->dir_.reflect(n);
	}
	if(Material::params_.wireframe_thickness_ > 0.f && (specular.reflect_ || specular.refract_))
	{
		const float wire_frame_amount = shaders_[ShaderNodeType::Wireframe] ? shaders_[ShaderNodeType::Wireframe]->getScalar(mat_data->node_tree_data_) * Material::params_.wireframe_amount_ : Material::params_.wireframe_amount_;
		if(wire_frame_amount > 0.f)
		{
			if(specular.reflect_) applyWireFrame(specular.reflect_->col_, wire_frame_amount, sp);
			if(specular.refract_) applyWireFrame(specular.refract_->col_, wire_frame_amount, sp);
		}
	}
	return specular;
}

Rgb GlassMaterial::getGlossyColor(const NodeTreeData &node_tree_data) const
{
	return shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getColor(node_tree_data) : params_.mirror_color_;
}

Rgb GlassMaterial::getTransColor(const NodeTreeData &node_tree_data) const
{
	if(shaders_[ShaderNodeType::FilterColor] || filter_color_.minimum() < .99f)	return (shaders_[ShaderNodeType::FilterColor] ? shaders_[ShaderNodeType::FilterColor]->getColor(node_tree_data) : filter_color_);
	else
	{
		Rgb tmp_col = beer_sigma_a_;
		tmp_col.clampRgb01();
		return Rgb(1.f) - tmp_col;
	}
}

Rgb GlassMaterial::getMirrorColor(const NodeTreeData &node_tree_data) const
{
	return shaders_[ShaderNodeType::MirrorColor] ? shaders_[ShaderNodeType::MirrorColor]->getColor(node_tree_data) : params_.mirror_color_;
}

} //namespace yafaray
