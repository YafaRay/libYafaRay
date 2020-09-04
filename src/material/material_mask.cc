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
#include "common/environment.h"
#include "common/param.h"


BEGIN_YAFARAY

MaskMaterial::MaskMaterial(const Material *m_1, const Material *m_2, float thresh, Visibility visibility):
		mat_1_(m_1), mat_2_(m_2), threshold_(thresh)
{
	visibility_ = visibility;
	bsdf_flags_ = mat_1_->getFlags() | mat_2_->getFlags();

	visibility_ = visibility;
}

#define PTR_ADD(ptr,sz) ((char*)ptr+(sz))
void MaskMaterial::initBsdf(const RenderState &state, SurfacePoint &sp, BsdfFlags &bsdf_types) const
{
	NodeStack stack(state.userdata_);
	evalNodes(state, sp, all_nodes_, stack);
	float val = mask_->getScalar(stack); //mask->getFloat(sp.P);
	bool mv = val > threshold_;
	*(bool *)state.userdata_ = mv;
	state.userdata_ = PTR_ADD(state.userdata_, sizeof(bool));
	if(mv) mat_2_->initBsdf(state, sp, bsdf_types);
	else mat_1_->initBsdf(state, sp, bsdf_types);
	state.userdata_ = PTR_ADD(state.userdata_, -sizeof(bool));
}

Rgb MaskMaterial::eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs, bool force_eval) const
{
	bool mv = *(bool *)state.userdata_;
	Rgb col;
	state.userdata_ = PTR_ADD(state.userdata_, sizeof(bool));
	if(mv) col = mat_2_->eval(state, sp, wo, wi, bsdfs);
	else   col = mat_1_->eval(state, sp, wo, wi, bsdfs);
	state.userdata_ = PTR_ADD(state.userdata_, -sizeof(bool));
	return col;
}

Rgb MaskMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	bool mv = *(bool *)state.userdata_;
	Rgb col;
	state.userdata_ = PTR_ADD(state.userdata_, sizeof(bool));
	if(mv) col = mat_2_->sample(state, sp, wo, wi, s, w);
	else   col = mat_1_->sample(state, sp, wo, wi, s, w);
	state.userdata_ = PTR_ADD(state.userdata_, -sizeof(bool));
	return col;
}

float MaskMaterial::pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	bool mv = *(bool *)state.userdata_;
	float pdf;
	state.userdata_ = PTR_ADD(state.userdata_, sizeof(bool));
	if(mv) pdf = mat_2_->pdf(state, sp, wo, wi, bsdfs);
	else   pdf = mat_1_->pdf(state, sp, wo, wi, bsdfs);
	state.userdata_ = PTR_ADD(state.userdata_, -sizeof(bool));
	return pdf;
}

bool MaskMaterial::isTransparent() const
{
	return mat_1_->isTransparent() || mat_2_->isTransparent();
}

Rgb MaskMaterial::getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);
	evalNodes(state, sp, all_nodes_, stack);
	float val = mask_->getScalar(stack);
	bool mv = val > 0.5;
	if(mv) return mat_2_->getTransparency(state, sp, wo);
	else   return mat_1_->getTransparency(state, sp, wo);
}

void MaskMaterial::getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
							   bool &reflect, bool &refract, Vec3 *const dir, Rgb *const col) const
{
	bool mv = *(bool *)state.userdata_;
	state.userdata_ = PTR_ADD(state.userdata_, sizeof(bool));
	if(mv) mat_2_->getSpecular(state, sp, wo, reflect, refract, dir, col);
	else   mat_1_->getSpecular(state, sp, wo, reflect, refract, dir, col);
	state.userdata_ = PTR_ADD(state.userdata_, -sizeof(bool));
}

Rgb MaskMaterial::emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	bool mv = *(bool *)state.userdata_;
	Rgb col;
	state.userdata_ = PTR_ADD(state.userdata_, sizeof(bool));
	if(mv) col = mat_2_->emit(state, sp, wo);
	else   col = mat_1_->emit(state, sp, wo);
	state.userdata_ = PTR_ADD(state.userdata_, -sizeof(bool));
	return col;
}

float MaskMaterial::getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	bool mv = *(bool *)state.userdata_;
	float alpha;
	state.userdata_ = PTR_ADD(state.userdata_, sizeof(bool));
	if(mv) alpha = mat_2_->getAlpha(state, sp, wo);
	else   alpha = mat_1_->getAlpha(state, sp, wo);
	state.userdata_ = PTR_ADD(state.userdata_, -sizeof(bool));
	return alpha;
}

Material *MaskMaterial::factory(ParamMap &params, std::list< ParamMap > &eparams, RenderEnvironment &env)
{
	std::string name;
	const Material *m_1 = nullptr, *m_2 = nullptr;
	double thresh = 0.5;
	std::string s_visibility = "normal";
	Visibility visibility = Material::Visibility::NormalVisible;
	bool receive_shadows = true;

	params.getParam("threshold", thresh);
	if(! params.getParam("material1", name)) return nullptr;
	m_1 = env.getMaterial(name);
	if(! params.getParam("material2", name)) return nullptr;
	m_2 = env.getMaterial(name);
	//if(! params.getParam("mask", name) ) return nullptr;
	//mask = env.getTexture(*name);

	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);

	if(s_visibility == "normal") visibility = Material::Visibility::NormalVisible;
	else if(s_visibility == "no_shadows") visibility = Material::Visibility::VisibleNoShadows;
	else if(s_visibility == "shadow_only") visibility = Material::Visibility::InvisibleShadowsOnly;
	else if(s_visibility == "invisible") visibility = Material::Visibility::Invisible;
	else visibility = Material::Visibility::NormalVisible;

	if(m_1 == nullptr || m_2 == nullptr) return nullptr;

	MaskMaterial *mat = new MaskMaterial(m_1, m_2, thresh, visibility);

	mat->receive_shadows_ = receive_shadows;

	std::vector<ShaderNode *> roots;
	if(mat->loadNodes(eparams, env))
	{
		if(params.getParam("mask", name))
		{
			auto i = mat->m_shaders_table_.find(name);
			if(i != mat->m_shaders_table_.end()) { mat->mask_ = i->second; roots.push_back(mat->mask_); }
			else
			{
				Y_ERROR << "MaskMat: Mask shader node '" << name << "' does not exist!" << YENDL;
				delete mat;
				return nullptr;
			}
		}
	}
	else
	{
		Y_ERROR << "MaskMat: loadNodes() failed!" << YENDL;
		delete mat;
		return nullptr;
	}
	mat->solveNodesOrder(roots);
	size_t input_req = std::max(m_1->getReqMem(), m_2->getReqMem());
	mat->req_mem_ = std::max(mat->req_node_mem_, sizeof(bool) + input_req);
	return mat;
}

END_YAFARAY
