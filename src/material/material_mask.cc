/****************************************************************************
 *      maskmat.cc: a material that combines 2 materials with a mask
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

#include "material/material_mask.h"
#include "shader/shader_node.h"
#include "texture/texture.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

MaskMaterial::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(material_1_name_);
	PARAM_LOAD(material_2_name_);
	PARAM_LOAD(threshold_);
	PARAM_SHADERS_LOAD;
}

ParamMap MaskMaterial::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(material_1_name_);
	PARAM_SAVE(material_2_name_);
	PARAM_SAVE(threshold_);
	PARAM_SHADERS_SAVE;
	PARAM_SAVE_END;
}

ParamMap MaskMaterial::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Material>, ParamError> MaskMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	std::string mat1_name;
	if(param_map.getParam(Params::material_1_name_meta_.name(), mat1_name).notOk()) return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
	const std::unique_ptr<const Material> *m_1 = scene.getMaterial(mat1_name);
	std::string mat2_name;
	if(param_map.getParam(Params::material_2_name_meta_.name(), mat2_name).notOk()) return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
	const std::unique_ptr<const Material> *m_2 = scene.getMaterial(mat2_name);
	if(!m_1 || !m_2) return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};

	auto material{std::make_unique<ThisClassType_t>(logger, param_error, param_map, m_1, m_2)};

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
	if(param_error.notOk()) logger.logWarning(param_error.print<MaskMaterial>(name, {"type"}));
	return {std::move(material), param_error};
}

MaskMaterial::MaskMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map, const std::unique_ptr<const Material> *material_1, const std::unique_ptr<const Material> *material_2) :
		ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map},
		mat_1_{material_1}, mat_2_{material_2}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

std::unique_ptr<const MaterialData> MaskMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data{std::make_unique<MaterialData_t>(bsdf_flags_, color_nodes_.size() + bump_nodes_.size())};
	evalNodes(sp, color_nodes_, mat_data->node_tree_data_, camera);
	const float val = shaders_[ShaderNodeType::Mask]->getScalar(mat_data->node_tree_data_); //mask->getFloat(sp.P);
	mat_data->select_mat_2_ = val > params_.threshold_;
	if(mat_data->select_mat_2_)
	{
		mat_data->mat_2_data_ = mat_2_->get()->initBsdf(sp, camera);
		mat_data->bsdf_flags_ = mat_data->mat_2_data_->bsdf_flags_;
	}
	else
	{
		mat_data->mat_1_data_ = mat_1_->get()->initBsdf(sp, camera);
		mat_data->bsdf_flags_ = mat_data->mat_1_data_->bsdf_flags_;
	}
	return mat_data;
}

Rgb MaskMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const
{
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->get()->eval(mat_data_specific->mat_2_data_.get(), sp, wo, wl, bsdfs);
	else col = mat_1_->get()->eval(mat_data_specific->mat_1_data_.get(), sp, wo, wl, bsdfs);
	return col;
}

Rgb MaskMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->get()->sample(mat_data_specific->mat_2_data_.get(), sp, wo, wi, s, w, chromatic, wavelength, camera);
	else col = mat_1_->get()->sample(mat_data_specific->mat_1_data_.get(), sp, wo, wi, s, w, chromatic, wavelength, camera);
	return col;
}

float MaskMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const
{
	float pdf;
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	if(mat_data_specific->select_mat_2_) pdf = mat_2_->get()->pdf(mat_data_specific->mat_2_data_.get(), sp, wo, wi, bsdfs);
	else pdf = mat_1_->get()->pdf(mat_data_specific->mat_1_data_.get(), sp, wo, wi, bsdfs);
	return pdf;
}

bool MaskMaterial::isTransparent() const
{
	return mat_1_->get()->isTransparent() || mat_2_->get()->isTransparent();
}

Rgb MaskMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	if(mat_data_specific->select_mat_2_) return mat_2_->get()->getTransparency(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	else return mat_1_->get()->getTransparency(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
}

Specular MaskMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const
{
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	Specular specular;
	if(mat_data_specific->select_mat_2_) specular = mat_2_->get()->getSpecular(ray_level, mat_data_specific->mat_2_data_.get(), sp, wo, chromatic, wavelength);
	else specular = mat_1_->get()->getSpecular(ray_level, mat_data_specific->mat_1_data_.get(), sp, wo, chromatic, wavelength);
	return specular;
}

Rgb MaskMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const
{
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->get()->emit(mat_data_specific->mat_2_data_.get(), sp, wo);
	else col = mat_1_->get()->emit(mat_data_specific->mat_1_data_.get(), sp, wo);
	return col;
}

float MaskMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const MaterialData_t *>(mat_data);
	float alpha;
	if(mat_data_specific->select_mat_2_) alpha = mat_2_->get()->getAlpha(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	else alpha = mat_1_->get()->getAlpha(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
	return alpha;
}

} //namespace yafaray
