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
#include "common/param.h"
#include "render/render_data.h"

namespace yafaray {

MaskMaterial::MaskMaterial(Logger &logger, const std::unique_ptr<const Material> *material_1, const std::unique_ptr<const Material> *material_2, float thresh, Visibility visibility):
		NodeMaterial(logger), mat_1_(material_1), mat_2_(material_2), threshold_(thresh)
{
	visibility_ = visibility;
}

const MaterialData * MaskMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	auto mat_data = new MaskMaterialData(bsdf_flags_, color_nodes_.size() + bump_nodes_.size());
	evalNodes(sp, color_nodes_, mat_data->node_tree_data_, camera);
	const float val = mask_->getScalar(mat_data->node_tree_data_); //mask->getFloat(sp.P);
	mat_data->select_mat_2_ = val > threshold_;
	if(mat_data->select_mat_2_)
	{
		mat_data->mat_2_data_ = std::unique_ptr<const MaterialData>(mat_2_->get()->initBsdf(sp, camera));
		mat_data->bsdf_flags_ = mat_data->mat_2_data_->bsdf_flags_;
	}
	else
	{
		mat_data->mat_1_data_ = std::unique_ptr<const MaterialData>(mat_1_->get()->initBsdf(sp, camera));
		mat_data->bsdf_flags_ = mat_data->mat_1_data_->bsdf_flags_;
	}
	return mat_data;
}

Rgb MaskMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, BsdfFlags bsdfs, bool force_eval) const
{
	const auto *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->get()->eval(mat_data_specific->mat_2_data_.get(), sp, wo, wl, bsdfs);
	else col = mat_1_->get()->eval(mat_data_specific->mat_1_data_.get(), sp, wo, wl, bsdfs);
	return col;
}

Rgb MaskMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->get()->sample(mat_data_specific->mat_2_data_.get(), sp, wo, wi, s, w, chromatic, wavelength, camera);
	else col = mat_1_->get()->sample(mat_data_specific->mat_1_data_.get(), sp, wo, wi, s, w, chromatic, wavelength, camera);
	return col;
}

float MaskMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, BsdfFlags bsdfs) const
{
	float pdf;
	const auto *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	if(mat_data_specific->select_mat_2_) pdf = mat_2_->get()->pdf(mat_data_specific->mat_2_data_.get(), sp, wo, wi, bsdfs);
	else pdf = mat_1_->get()->pdf(mat_data_specific->mat_1_data_.get(), sp, wo, wi, bsdfs);
	return pdf;
}

bool MaskMaterial::isTransparent() const
{
	return mat_1_->get()->isTransparent() || mat_2_->get()->isTransparent();
}

Rgb MaskMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	if(mat_data_specific->select_mat_2_) return mat_2_->get()->getTransparency(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	else return mat_1_->get()->getTransparency(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
}

Specular MaskMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const
{
	const auto *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Specular specular;
	if(mat_data_specific->select_mat_2_) specular = mat_2_->get()->getSpecular(ray_level, mat_data_specific->mat_2_data_.get(), sp, wo, chromatic, wavelength);
	else specular = mat_1_->get()->getSpecular(ray_level, mat_data_specific->mat_1_data_.get(), sp, wo, chromatic, wavelength);
	return specular;
}

Rgb MaskMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	const auto *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->get()->emit(mat_data_specific->mat_2_data_.get(), sp, wo);
	else col = mat_1_->get()->emit(mat_data_specific->mat_1_data_.get(), sp, wo);
	return col;
}

float MaskMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const auto *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	float alpha;
	if(mat_data_specific->select_mat_2_) alpha = mat_2_->get()->getAlpha(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	else alpha = mat_1_->get()->getAlpha(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
	return alpha;
}

Material *MaskMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params)
{
	std::string mat1_name;
	if(!params.getParam("material1", mat1_name)) return nullptr;
	const std::unique_ptr<const Material> *material_1 = scene.getMaterial(mat1_name);
	std::string mat2_name;
	if(!params.getParam("material2", mat2_name)) return nullptr;
	const std::unique_ptr<const Material> *material_2 = scene.getMaterial(mat2_name);
	if(material_1 == nullptr || material_2 == nullptr) return nullptr;
	//if(! params.getParam("mask", name) ) return nullptr;
	//mask = scene.getTexture(*name);

	double thresh = 0.5;
	std::string s_visibility = "normal";
	bool receive_shadows = true;
	params.getParam("threshold", thresh);
	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);

	const Visibility visibility = visibility::fromString(s_visibility);
	auto mat = new MaskMaterial(logger, material_1, material_2, thresh, visibility);
	mat->receive_shadows_ = receive_shadows;

	std::vector<const ShaderNode *> root_nodes_list;
	mat->nodes_map_ = NodeMaterial::loadNodes(nodes_params, scene, logger);
	if(mat->nodes_map_.empty())
	{
		return nullptr;
	}
	else
	{
		std::string mask;
		if(params.getParam("mask", mask))
		{
			const auto &i = mat->nodes_map_.find(mask);
			if(i != mat->nodes_map_.end()) { mat->mask_ = i->second.get(); root_nodes_list.emplace_back(mat->mask_); }
			else
			{
				logger.logError("MaskMat: Mask shader node '", mask, "' does not exist!");
				return nullptr;
			}
		}
	}
	mat->color_nodes_ = NodeMaterial::solveNodesOrder(root_nodes_list, mat->nodes_map_, logger);
	return mat;
}

} //namespace yafaray
