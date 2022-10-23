/****************************************************************************
 *      blendmat.cc: a material that blends two material
 *      This is part of the libYafaRay package
 *      Copyright (C) 2008  Mathias Wein
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

#include "material/material_blend.h"
#include "shader/shader_node.h"
#include "geometry/surface.h"
#include "param/param.h"
#include "common/logger.h"
#include "math/interpolation.h"
#include "scene/scene.h"
#include "material/sample.h"
#include "photon/photon_sample.h"
#include "scene/scene.h"

namespace yafaray {

BlendMaterial::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(material_1_name_);
	PARAM_LOAD(material_2_name_);
	PARAM_LOAD(blend_value_);
	PARAM_SHADERS_LOAD;
}

ParamMap BlendMaterial::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(material_1_name_);
	PARAM_SAVE(material_2_name_);
	PARAM_SAVE(blend_value_);
	PARAM_SHADERS_SAVE;
	PARAM_SAVE_END;
}

ParamMap BlendMaterial::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Material>, ParamError> BlendMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	std::string mat_1_name;
	if(param_map.getParam(Params::material_1_name_meta_.name(), mat_1_name).notOk()) return {nullptr, ParamError{ResultFlags::ErrorWhileCreating}};
	const auto [mat_1_id, mat_1_error]{scene.getMaterial(mat_1_name)};
	std::string mat_2_name;
	if(param_map.getParam(Params::material_2_name_meta_.name(), mat_2_name).notOk()) return {nullptr, ParamError{ResultFlags::ErrorWhileCreating}};
	const auto [mat_2_id, mat_2_error]{scene.getMaterial(mat_2_name)};
	if(mat_1_error.hasError() || mat_2_error.hasError()) return {nullptr, ParamError{ResultFlags::ErrorWhileCreating}};

	auto material{std::make_unique<ThisClassType_t>(logger, param_error, param_map, mat_1_id, mat_2_id, scene.getMaterials())};
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
	if(param_error.notOk()) logger.logWarning(param_error.print<BlendMaterial>(name, {"type"}));
	return {std::move(material), param_error};
}

BlendMaterial::BlendMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map, size_t material_1_id, size_t material_2_id, const SceneItems<Material> &materials) :
		ParentClassType_t{logger, param_error, param_map, materials}, params_{param_error, param_map},
		material_1_id_{material_1_id}, material_2_id_{material_2_id}, materials_{materials}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	blended_ior_ = (materials_.getById(material_1_id_).first->getMatIor() + materials_.getById(material_2_id_).first->getMatIor()) * 0.5f;
	additional_depth_ = std::max(materials_.getById(material_1_id_).first->getAdditionalDepth(), materials_.getById(material_2_id_).first->getAdditionalDepth());
}

inline float BlendMaterial::getBlendVal(const NodeTreeData &node_tree_data) const
{
	const float blend_val = getShaderScalar(shaders_[ShaderNodeType::Blend], node_tree_data, params_.blend_value_);
	return blend_val;
}

std::unique_ptr<const MaterialData> BlendMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data{std::make_unique<MaterialData_t>(bsdf_flags_, color_nodes_.size() + bump_nodes_.size())};
	evalNodes(sp, color_nodes_, mat_data->node_tree_data_, camera);
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	SurfacePoint sp_1 {sp};
	mat_data->mat_1_data_ = materials_.getById(material_1_id_).first->initBsdf(sp_1, camera);
	SurfacePoint sp_2 {sp};
	mat_data->mat_2_data_ = materials_.getById(material_2_id_).first->initBsdf(sp_2, camera);
	sp = SurfacePoint{sp_1, sp_2, blend_val};
	mat_data->bsdf_flags_ = mat_data->mat_1_data_->bsdf_flags_ | mat_data->mat_2_data_->bsdf_flags_;
	//todo: bump mapping blending
	return mat_data;
}

Rgb BlendMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const Rgb col_1 = materials_.getById(material_1_id_).first->eval(mat_data_specific->mat_1_data_.get(), sp, wo, wl, bsdfs);
	const Rgb col_2 = materials_.getById(material_2_id_).first->eval(mat_data_specific->mat_2_data_.get(), sp, wo, wl, bsdfs);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	applyWireFrame(col_blend, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col_blend;
}

Rgb BlendMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	bool mat_1_sampled = false;
	bool mat_2_sampled = false;

	Rgb col_1(0.f), col_2(0.f);
	Sample s_1 = s, s_2 = s;
	Vec3f wi_1(0.f), wi_2(0.f);
	float w_1 = 0.f, w_2 = 0.f;

	s_2.pdf_ = s_1.pdf_ = s.pdf_ = 0.f;
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	if(s.flags_.has(mat_data_specific->mat_1_data_->bsdf_flags_))
	{
		col_1 = materials_.getById(material_1_id_).first->sample(mat_data_specific->mat_1_data_.get(), sp, wo, wi_1, s_1, w_1, chromatic, wavelength, camera);
		mat_1_sampled = true;
	}

	if(s.flags_.has(mat_data_specific->mat_2_data_->bsdf_flags_))
	{
		col_2 = materials_.getById(material_2_id_).first->sample(mat_data_specific->mat_2_data_.get(), sp, wo, wi_2, s_2, w_2, chromatic, wavelength, camera);
		mat_2_sampled = true;
	}

	if(mat_1_sampled && mat_2_sampled)
	{
		wi = (wi_2 + wi_1).normalize();

		s.pdf_ = math::lerp(s_1.pdf_, s_2.pdf_, blend_val);
		s.sampled_flags_ = s_1.sampled_flags_ | s_2.sampled_flags_;
		s.reverse_ = s_1.reverse_ | s_2.reverse_;
		if(s.reverse_)
		{
			s.pdf_back_ = math::lerp(s_1.pdf_back_, s_2.pdf_back_, blend_val);
			s.col_back_ = math::lerp(s_1.col_back_ * w_1, s_2.col_back_ * w_2, blend_val);
		}
		col_1 = math::lerp(col_1 * w_1, col_2 * w_2, blend_val);
		w = 1.f;//addPdf(W1, W2);
	}
	else if(mat_1_sampled)
	{
		wi = wi_1;

		s.pdf_ = s_1.pdf_;
		s.sampled_flags_ = s_1.sampled_flags_;
		s.reverse_ = s_1.reverse_;
		if(s.reverse_)
		{
			s.pdf_back_ = s_1.pdf_back_;
			s.col_back_ = s_1.col_back_;
		}
		//col1 = col1;
		w = w_1;
	}
	else if(mat_2_sampled)
	{
		wi = wi_2;

		s.pdf_ = s_2.pdf_;
		s.sampled_flags_ = s_2.sampled_flags_;
		s.reverse_ = s_2.reverse_;
		if(s.reverse_)
		{
			s.pdf_back_ = s_2.pdf_back_;
			s.col_back_ = s_2.col_back_;
		}
		col_1 = col_2;
		w = w_2;
	}
	applyWireFrame(col_1, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col_1;
}

Rgb BlendMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	Rgb col;
	if(blend_val <= 0.f) col = materials_.getById(material_1_id_).first->sample(mat_data_specific->mat_1_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
	else if(blend_val >= 1.f) col = materials_.getById(material_2_id_).first->sample(mat_data_specific->mat_2_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
	else
	{
		const Rgb col_1 = materials_.getById(material_1_id_).first->sample(mat_data_specific->mat_1_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
		const Rgb col_2 = materials_.getById(material_2_id_).first->sample(mat_data_specific->mat_2_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
		col = math::lerp(col_1, col_2, blend_val);
	}
	applyWireFrame(col, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col;
}

float BlendMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const float pdf_1 = materials_.getById(material_1_id_).first->pdf(mat_data_specific->mat_1_data_.get(), sp, wo, wi, bsdfs);
	const float pdf_2 = materials_.getById(material_2_id_).first->pdf(mat_data_specific->mat_2_data_.get(), sp, wo, wi, bsdfs);
	const float pdf_blend = math::lerp(pdf_1, pdf_2, blend_val);
	return pdf_blend;
}

Specular BlendMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	Specular specular_1 = materials_.getById(material_1_id_).first->getSpecular(ray_level, mat_data_specific->mat_1_data_.get(), sp, wo, chromatic, wavelength);
	Specular specular_2 = materials_.getById(material_2_id_).first->getSpecular(ray_level, mat_data_specific->mat_2_data_.get(), sp, wo, chromatic, wavelength);
	Specular specular_blend;
	specular_blend.reflect_ = DirectionColor::blend(std::move(specular_1.reflect_), std::move(specular_2.reflect_), blend_val);
	specular_blend.refract_ = DirectionColor::blend(std::move(specular_1.refract_), std::move(specular_2.refract_), blend_val);
	if(Material::params_.wireframe_thickness_ > 0.f && (specular_blend.reflect_ || specular_blend.refract_))
	{
		const float wire_frame_amount = shaders_[ShaderNodeType::Wireframe] ? shaders_[ShaderNodeType::Wireframe]->getScalar(mat_data->node_tree_data_) * Material::params_.wireframe_amount_ : Material::params_.wireframe_amount_;
		if(wire_frame_amount > 0.f)
		{
			if(specular_blend.reflect_) applyWireFrame(specular_blend.reflect_->col_, wire_frame_amount, sp);
			if(specular_blend.refract_) applyWireFrame(specular_blend.refract_->col_, wire_frame_amount, sp);
		}
	}
	return specular_blend;
}

float BlendMaterial::getMatIor() const
{
	return blended_ior_;
}

bool BlendMaterial::isTransparent() const
{
	return materials_.getById(material_1_id_).first->isTransparent() || materials_.getById(material_2_id_).first->isTransparent();
}

Rgb BlendMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const Rgb col_1 = materials_.getById(material_1_id_).first->getTransparency(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
	const Rgb col_2 = materials_.getById(material_2_id_).first->getTransparency(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	applyWireFrame(col_blend, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col_blend;
}

float BlendMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	float result = 1.f;
	if(isTransparent())
	{
		const float al_1 = materials_.getById(material_1_id_).first->getAlpha(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
		const float al_2 = materials_.getById(material_2_id_).first->getAlpha(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
		result = std::min(al_1, al_2);
	}
	applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return result;
}

Rgb BlendMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const Rgb col_1 = materials_.getById(material_1_id_).first->emit(mat_data_specific->mat_1_data_.get(), sp, wo);
	const Rgb col_2 = materials_.getById(material_2_id_).first->emit(mat_data_specific->mat_2_data_.get(), sp, wo);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	applyWireFrame(col_blend, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col_blend;
}

bool BlendMaterial::scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wi, Vec3f &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	const bool scattered_1 = materials_.getById(material_1_id_).first->scatterPhoton(mat_data_specific->mat_1_data_.get(), sp, wi, wo, s, chromatic, wavelength, camera);
	const Rgb col_1 = s.color_;
	const float pdf_1 = s.pdf_;
	const bool scattered_2 = materials_.getById(material_2_id_).first->scatterPhoton(mat_data_specific->mat_2_data_.get(), sp, wi, wo, s, chromatic, wavelength, camera);
	const Rgb col_2 = s.color_;
	const float pdf_2 = s.pdf_;
	s.color_ = math::lerp(col_1, col_2, blend_val);
	s.pdf_ = math::lerp(pdf_1, pdf_2, blend_val);
	return scattered_1 | scattered_2;
}

const VolumeHandler *BlendMaterial::getVolumeHandler(bool inside) const
{
	const VolumeHandler *vol_1 = materials_.getById(material_1_id_).first->getVolumeHandler(inside);
	const VolumeHandler *vol_2 = materials_.getById(material_2_id_).first->getVolumeHandler(inside);
	if(vol_1 && vol_2)
	{
		if(params_.blend_value_ <= 0.5f) return vol_1;
		else return vol_2;
	}
	else if(vol_1) return vol_1;
	else return vol_2;
}

} //namespace yafaray
