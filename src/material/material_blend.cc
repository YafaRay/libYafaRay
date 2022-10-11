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
	ParamMap result{Material::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Material *, ParamError> BlendMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	std::string mat1_name;
	if(param_map.getParam(Params::material_1_name_meta_.name(), mat1_name).notOk()) return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	const std::unique_ptr<const Material> *m_1 = scene.getMaterial(mat1_name);
	std::string mat2_name;
	if(param_map.getParam(Params::material_2_name_meta_.name(), mat2_name).notOk()) return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	const std::unique_ptr<const Material> *m_2 = scene.getMaterial(mat2_name);
	if(!m_1 || !m_2) return {nullptr, {ParamError::Flags::ErrorWhileCreating}};

	auto mat = new BlendMaterial(logger, param_error, param_map, m_1, m_2);
	mat->nodes_map_ = NodeMaterial::loadNodes(nodes_param_maps, scene, logger);
	std::map<std::string, const ShaderNode *> root_nodes_map;
	// Prepare our node list
	for(size_t shader_index = 0; shader_index < mat->shaders_.size(); ++shader_index)
	{
		root_nodes_map[ShaderNodeType{static_cast<unsigned char>(shader_index)}.print()] = nullptr;
	}
	std::vector<const ShaderNode *> root_nodes_list;
	if(!mat->nodes_map_.empty()) NodeMaterial::parseNodes(param_map, root_nodes_list, root_nodes_map, mat->nodes_map_, logger);
	for(size_t shader_index = 0; shader_index < mat->shaders_.size(); ++shader_index)
	{
		mat->shaders_[shader_index] = root_nodes_map[ShaderNodeType{static_cast<unsigned char>(shader_index)}.print()];
	}
	// solve nodes order
	if(!root_nodes_list.empty())
	{
		const std::vector<const ShaderNode *> nodes_sorted = NodeMaterial::solveNodesOrder(root_nodes_list, mat->nodes_map_, logger);
		for(size_t shader_index = 0; shader_index < mat->shaders_.size(); ++shader_index)
		{
			if(mat->shaders_[shader_index])
			{
				if(ShaderNodeType{static_cast<unsigned char>(shader_index)}.isBump())
				{
					mat->bump_nodes_ = NodeMaterial::getNodeList(mat->shaders_[shader_index], nodes_sorted);
				}
				else
				{
					const std::vector<const ShaderNode *> shader_nodes_list = NodeMaterial::getNodeList(mat->shaders_[shader_index], nodes_sorted);
					mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
				}
			}
		}
	}
	if(param_error.notOk()) logger.logWarning(param_error.print<BlendMaterial>(name, {"type"}));
	return {mat, param_error};
}

BlendMaterial::BlendMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map, const std::unique_ptr<const Material> *material_1, const std::unique_ptr<const Material> *material_2) :
		NodeMaterial{logger, param_error, param_map}, params_{param_error, param_map},
		mat_1_{material_1}, mat_2_{material_2}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	blended_ior_ = (mat_1_->get()->getMatIor() + mat_2_->get()->getMatIor()) * 0.5f;
	additional_depth_ = std::max(mat_1_->get()->getAdditionalDepth(), mat_2_->get()->getAdditionalDepth());
}

inline float BlendMaterial::getBlendVal(const NodeTreeData &node_tree_data) const
{
	const float blend_val = getShaderScalar(shaders_[ShaderNodeType::Blend], node_tree_data, params_.blend_value_);
	return blend_val;
}

const MaterialData * BlendMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data = new BlendMaterialData(bsdf_flags_, color_nodes_.size() + bump_nodes_.size());
	evalNodes(sp, color_nodes_, mat_data->node_tree_data_, camera);
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	SurfacePoint sp_1 {sp};
	mat_data->mat_1_data_ = std::unique_ptr<const MaterialData>(mat_1_->get()->initBsdf(sp_1, camera));
	SurfacePoint sp_2 {sp};
	mat_data->mat_2_data_ = std::unique_ptr<const MaterialData>(mat_2_->get()->initBsdf(sp_2, camera));
	sp = SurfacePoint{sp_1, sp_2, blend_val};
	mat_data->bsdf_flags_ = mat_data->mat_1_data_->bsdf_flags_ | mat_data->mat_2_data_->bsdf_flags_;
	//todo: bump mapping blending
	return mat_data;
}

Rgb BlendMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const Rgb col_1 = mat_1_->get()->eval(mat_data_specific->mat_1_data_.get(), sp, wo, wl, bsdfs);
	const Rgb col_2 = mat_2_->get()->eval(mat_data_specific->mat_2_data_.get(), sp, wo, wl, bsdfs);
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
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	if(s.flags_.has(mat_data_specific->mat_1_data_->bsdf_flags_))
	{
		col_1 = mat_1_->get()->sample(mat_data_specific->mat_1_data_.get(), sp, wo, wi_1, s_1, w_1, chromatic, wavelength, camera);
		mat_1_sampled = true;
	}

	if(s.flags_.has(mat_data_specific->mat_2_data_->bsdf_flags_))
	{
		col_2 = mat_2_->get()->sample(mat_data_specific->mat_2_data_.get(), sp, wo, wi_2, s_2, w_2, chromatic, wavelength, camera);
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
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	Rgb col;
	if(blend_val <= 0.f) col = mat_1_->get()->sample(mat_data_specific->mat_1_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
	else if(blend_val >= 1.f) col = mat_2_->get()->sample(mat_data_specific->mat_2_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
	else
	{
		const Rgb col_1 = mat_1_->get()->sample(mat_data_specific->mat_1_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
		const Rgb col_2 = mat_2_->get()->sample(mat_data_specific->mat_2_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
		col = math::lerp(col_1, col_2, blend_val);
	}
	applyWireFrame(col, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col;
}

float BlendMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const float pdf_1 = mat_1_->get()->pdf(mat_data_specific->mat_1_data_.get(), sp, wo, wi, bsdfs);
	const float pdf_2 = mat_2_->get()->pdf(mat_data_specific->mat_2_data_.get(), sp, wo, wi, bsdfs);
	const float pdf_blend = math::lerp(pdf_1, pdf_2, blend_val);
	return pdf_blend;
}

Specular BlendMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	Specular specular_1 = mat_1_->get()->getSpecular(ray_level, mat_data_specific->mat_1_data_.get(), sp, wo, chromatic, wavelength);
	Specular specular_2 = mat_2_->get()->getSpecular(ray_level, mat_data_specific->mat_2_data_.get(), sp, wo, chromatic, wavelength);
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
	return mat_1_->get()->isTransparent() || mat_2_->get()->isTransparent();
}

Rgb BlendMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const Rgb col_1 = mat_1_->get()->getTransparency(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
	const Rgb col_2 = mat_2_->get()->getTransparency(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	applyWireFrame(col_blend, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col_blend;
}

float BlendMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	float result = 1.f;
	if(isTransparent())
	{
		const float al_1 = mat_1_->get()->getAlpha(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
		const float al_2 = mat_2_->get()->getAlpha(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
		result = std::min(al_1, al_2);
	}
	applyWireFrame(result, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return result;
}

Rgb BlendMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const Rgb col_1 = mat_1_->get()->emit(mat_data_specific->mat_1_data_.get(), sp, wo);
	const Rgb col_2 = mat_2_->get()->emit(mat_data_specific->mat_2_data_.get(), sp, wo);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	applyWireFrame(col_blend, shaders_[ShaderNodeType::Wireframe], mat_data->node_tree_data_, sp);
	return col_blend;
}

bool BlendMaterial::scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wi, Vec3f &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const auto *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const bool scattered_1 = mat_1_->get()->scatterPhoton(mat_data_specific->mat_1_data_.get(), sp, wi, wo, s, chromatic, wavelength, camera);
	const Rgb col_1 = s.color_;
	const float pdf_1 = s.pdf_;
	const bool scattered_2 = mat_2_->get()->scatterPhoton(mat_data_specific->mat_2_data_.get(), sp, wi, wo, s, chromatic, wavelength, camera);
	const Rgb col_2 = s.color_;
	const float pdf_2 = s.pdf_;
	s.color_ = math::lerp(col_1, col_2, blend_val);
	s.pdf_ = math::lerp(pdf_1, pdf_2, blend_val);
	return scattered_1 | scattered_2;
}

const VolumeHandler *BlendMaterial::getVolumeHandler(bool inside) const
{
	const VolumeHandler *vol_1 = mat_1_->get()->getVolumeHandler(inside);
	const VolumeHandler *vol_2 = mat_2_->get()->getVolumeHandler(inside);
	if(vol_1 && vol_2)
	{
		if(params_.blend_value_ <= 0.5f) return vol_1;
		else return vol_2;
	}
	else if(vol_1) return vol_1;
	else return vol_2;
}

} //namespace yafaray
