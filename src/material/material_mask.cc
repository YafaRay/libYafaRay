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

BEGIN_YAFARAY

MaskMaterial::MaskMaterial(Logger &logger, const Material *m_1, const Material *m_2, float thresh, Visibility visibility):
		NodeMaterial(logger), mat_1_(m_1), mat_2_(m_2), threshold_(thresh)
{
	visibility_ = visibility;
}

std::unique_ptr<const MaterialData> MaskMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	std::unique_ptr<MaterialData> mat_data = createMaterialData(color_nodes_.size() + bump_nodes_.size());
	evalNodes(sp, color_nodes_, mat_data->node_tree_data_, camera);
	const float val = mask_->getScalar(mat_data->node_tree_data_); //mask->getFloat(sp.P);
	MaskMaterialData *mat_data_specific = static_cast<MaskMaterialData *>(mat_data.get());
	mat_data_specific->select_mat_2_ = val > threshold_;
	if(mat_data_specific->select_mat_2_)
	{
		mat_data_specific->mat_2_data_ = mat_2_->initBsdf(sp, camera);
		mat_data->bsdf_flags_ = mat_data_specific->mat_2_data_->bsdf_flags_;
	}
	else
	{
		mat_data_specific->mat_1_data_ = mat_1_->initBsdf(sp, camera);
		mat_data->bsdf_flags_ = mat_data_specific->mat_1_data_->bsdf_flags_;
	}
	return mat_data;
}

Rgb MaskMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const
{
	const MaskMaterialData *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->eval(mat_data_specific->mat_2_data_.get(), sp, wo, wl, bsdfs);
	else col = mat_1_->eval(mat_data_specific->mat_1_data_.get(), sp, wo, wl, bsdfs);
	return col;
}

Rgb MaskMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const MaskMaterialData *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->sample(mat_data_specific->mat_2_data_.get(), sp, wo, wi, s, w, chromatic, wavelength, camera);
	else col = mat_1_->sample(mat_data_specific->mat_1_data_.get(), sp, wo, wi, s, w, chromatic, wavelength, camera);
	return col;
}

float MaskMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	float pdf;
	const MaskMaterialData *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	if(mat_data_specific->select_mat_2_) pdf = mat_2_->pdf(mat_data_specific->mat_2_data_.get(), sp, wo, wi, bsdfs);
	else pdf = mat_1_->pdf(mat_data_specific->mat_1_data_.get(), sp, wo, wi, bsdfs);
	return pdf;
}

bool MaskMaterial::isTransparent() const
{
	return mat_1_->isTransparent() || mat_2_->isTransparent();
}

Rgb MaskMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const MaskMaterialData *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	if(mat_data_specific->select_mat_2_) return mat_2_->getTransparency(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	else return mat_1_->getTransparency(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
}

Specular MaskMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const
{
	const MaskMaterialData *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Specular specular;
	if(mat_data_specific->select_mat_2_) specular = mat_2_->getSpecular(ray_level, mat_data_specific->mat_2_data_.get(), sp, wo, chromatic, wavelength);
	else specular = mat_1_->getSpecular(ray_level, mat_data_specific->mat_1_data_.get(), sp, wo, chromatic, wavelength);
	return specular;
}

Rgb MaskMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool lights_geometry_material_emit) const
{
	const MaskMaterialData *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	Rgb col;
	if(mat_data_specific->select_mat_2_) col = mat_2_->emit(mat_data_specific->mat_2_data_.get(), sp, wo, lights_geometry_material_emit);
	else col = mat_1_->emit(mat_data_specific->mat_1_data_.get(), sp, wo, lights_geometry_material_emit);
	return col;
}

float MaskMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const MaskMaterialData *mat_data_specific = static_cast<const MaskMaterialData *>(mat_data);
	float alpha;
	if(mat_data_specific->select_mat_2_) alpha = mat_2_->getAlpha(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	else alpha = mat_1_->getAlpha(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
	return alpha;
}

std::unique_ptr<Material> MaskMaterial::factory(Logger &logger, ParamMap &params, std::list<ParamMap> &nodes_params, const Scene &scene)
{
	std::string name;
	if(!params.getParam("material1", name)) return nullptr;
	const Material *m_1 = scene.getMaterial(name);
	if(!params.getParam("material2", name)) return nullptr;
	const Material *m_2 = scene.getMaterial(name);
	if(m_1 == nullptr || m_2 == nullptr) return nullptr;
	//if(! params.getParam("mask", name) ) return nullptr;
	//mask = scene.getTexture(*name);

	double thresh = 0.5;
	std::string s_visibility = "normal";
	bool receive_shadows = true;
	params.getParam("threshold", thresh);
	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);

	const Visibility visibility = visibility::fromString(s_visibility);
	auto mat = std::unique_ptr<MaskMaterial>(new MaskMaterial(logger, m_1, m_2, thresh, visibility));
	mat->receive_shadows_ = receive_shadows;

	std::vector<const ShaderNode *> root_nodes_list;
	mat->nodes_map_ = mat->loadNodes(nodes_params, scene, logger);
	if(mat->nodes_map_.empty())
	{
		return nullptr;
	}
	else
	{
		if(params.getParam("mask", name))
		{
			const auto &i = mat->nodes_map_.find(name);
			if(i != mat->nodes_map_.end()) { mat->mask_ = i->second.get(); root_nodes_list.push_back(mat->mask_); }
			else
			{
				logger.logError("MaskMat: Mask shader node '", name, "' does not exist!");
				return nullptr;
			}
		}
	}
	mat->color_nodes_ = mat->solveNodesOrder(root_nodes_list, mat->nodes_map_, logger);
	return mat;
}

END_YAFARAY
