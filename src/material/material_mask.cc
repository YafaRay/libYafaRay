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

MaskMaterial::MaskMaterial(const Material *m_1, const Material *m_2, float thresh, Visibility visibility):
		mat_1_(m_1), mat_2_(m_2), threshold_(thresh)
{
	visibility_ = visibility;
	bsdf_flags_ = mat_1_->getFlags() | mat_2_->getFlags();
}

#define PTR_ADD(ptr,sz) ((char*)ptr+(sz))
void MaskMaterial::initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const
{
	NodeStack stack(render_data.arena_);
	evalNodes(render_data, sp, color_nodes_, stack);
	const float val = mask_->getScalar(stack); //mask->getFloat(sp.P);
	const bool mv = val > threshold_;
	*(bool *)render_data.arena_ = mv;
	render_data.arena_ = PTR_ADD(render_data.arena_, sizeof(bool));
	if(mv) mat_2_->initBsdf(render_data, sp, bsdf_types);
	else mat_1_->initBsdf(render_data, sp, bsdf_types);
	render_data.arena_ = PTR_ADD(render_data.arena_, -sizeof(bool));
}

Rgb MaskMaterial::eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval) const
{
	const bool mv = *(bool *)render_data.arena_;
	Rgb col;
	render_data.arena_ = PTR_ADD(render_data.arena_, sizeof(bool));
	if(mv) col = mat_2_->eval(render_data, sp, wo, wi, bsdfs);
	else   col = mat_1_->eval(render_data, sp, wo, wi, bsdfs);
	render_data.arena_ = PTR_ADD(render_data.arena_, -sizeof(bool));
	return col;
}

Rgb MaskMaterial::sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	const bool mv = *(bool *)render_data.arena_;
	Rgb col;
	render_data.arena_ = PTR_ADD(render_data.arena_, sizeof(bool));
	if(mv) col = mat_2_->sample(render_data, sp, wo, wi, s, w);
	else   col = mat_1_->sample(render_data, sp, wo, wi, s, w);
	render_data.arena_ = PTR_ADD(render_data.arena_, -sizeof(bool));
	return col;
}

float MaskMaterial::pdf(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	const bool mv = *(bool *)render_data.arena_;
	float pdf;
	render_data.arena_ = PTR_ADD(render_data.arena_, sizeof(bool));
	if(mv) pdf = mat_2_->pdf(render_data, sp, wo, wi, bsdfs);
	else   pdf = mat_1_->pdf(render_data, sp, wo, wi, bsdfs);
	render_data.arena_ = PTR_ADD(render_data.arena_, -sizeof(bool));
	return pdf;
}

bool MaskMaterial::isTransparent() const
{
	return mat_1_->isTransparent() || mat_2_->isTransparent();
}

Rgb MaskMaterial::getTransparency(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(render_data.arena_);
	evalNodes(render_data, sp, color_nodes_, stack);
	float val = mask_->getScalar(stack);
	bool mv = val > 0.5;
	if(mv) return mat_2_->getTransparency(render_data, sp, wo);
	else   return mat_1_->getTransparency(render_data, sp, wo);
}

Material::Specular MaskMaterial::getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Specular specular;
	const bool mv = *(bool *)render_data.arena_;
	render_data.arena_ = PTR_ADD(render_data.arena_, sizeof(bool));
	if(mv) specular = mat_2_->getSpecular(render_data, sp, wo);
	else specular = mat_1_->getSpecular(render_data, sp, wo);
	render_data.arena_ = PTR_ADD(render_data.arena_, -sizeof(bool));
	return specular;
}

Rgb MaskMaterial::emit(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	const bool mv = *(bool *)render_data.arena_;
	Rgb col;
	render_data.arena_ = PTR_ADD(render_data.arena_, sizeof(bool));
	if(mv) col = mat_2_->emit(render_data, sp, wo);
	else   col = mat_1_->emit(render_data, sp, wo);
	render_data.arena_ = PTR_ADD(render_data.arena_, -sizeof(bool));
	return col;
}

float MaskMaterial::getAlpha(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	const bool mv = *(bool *)render_data.arena_;
	float alpha;
	render_data.arena_ = PTR_ADD(render_data.arena_, sizeof(bool));
	if(mv) alpha = mat_2_->getAlpha(render_data, sp, wo);
	else   alpha = mat_1_->getAlpha(render_data, sp, wo);
	render_data.arena_ = PTR_ADD(render_data.arena_, -sizeof(bool));
	return alpha;
}

std::unique_ptr<Material> MaskMaterial::factory(ParamMap &params, std::list< ParamMap > &eparams, const Scene &scene)
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

	const Visibility visibility = visibilityFromString_global(s_visibility);
	auto mat = std::unique_ptr<MaskMaterial>(new MaskMaterial(m_1, m_2, thresh, visibility));
	mat->receive_shadows_ = receive_shadows;

	std::vector<ShaderNode *> roots;
	if(mat->loadNodes(eparams, scene))
	{
		if(params.getParam("mask", name))
		{
			auto i = mat->shaders_table_.find(name);
			if(i != mat->shaders_table_.end()) { mat->mask_ = i->second.get(); roots.push_back(mat->mask_); }
			else
			{
				Y_ERROR << "MaskMat: Mask shader node '" << name << "' does not exist!" << YENDL;
				return nullptr;
			}
		}
	}
	else
	{
		Y_ERROR << "MaskMat: loadNodes() failed!" << YENDL;
		return nullptr;
	}
	mat->solveNodesOrder(roots);
	size_t input_req = std::max(m_1->getReqMem(), m_2->getReqMem());
	mat->req_mem_ = std::max(mat->req_node_mem_, sizeof(bool) + input_req);
	return mat;
}

END_YAFARAY
