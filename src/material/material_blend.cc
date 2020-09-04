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
#include "utility/util_sample.h"
#include "common/environment.h"
#include "common/surface.h"
#include "common/param.h"
#include "common/scene.h"

BEGIN_YAFARAY

#define PTR_ADD(ptr,sz) ((char*)ptr+(sz))

#define SUM_COLORS(c1, c2) ((c1) + (c2))
#define ADD_COLORS(c1, c2, v1, v2) SUM_COLORS(c1*v1, c2*v2)

#define ADD_PDF(p1, p2) (p1*ival + p2*val)

BlendMaterial::BlendMaterial(const Material *m_1, const Material *m_2, float bval, Visibility visibility):
		mat_1_(m_1), mat_2_(m_2)
{
	visibility_ = visibility;
	bsdf_flags_ = mat_1_->getFlags() | mat_2_->getFlags();
	mmem_1_ = mat_1_->getReqMem();
	recalc_blend_ = false;
	blend_val_ = bval;
	blended_ior_ = (mat_1_->getMatIor() + mat_2_->getMatIor()) * 0.5f;
	visibility_ = visibility;
	additional_depth_ = std::max(mat_1_->getAdditionalDepth(), mat_2_->getAdditionalDepth());
}

BlendMaterial::~BlendMaterial()
{
	mat_1_flags_ = BsdfFlags::None;
	mat_2_flags_ = BsdfFlags::None;
}

inline void BlendMaterial::getBlendVal(const RenderState &state, const SurfacePoint &sp, float &val, float &ival) const
{

	if(recalc_blend_)
	{
		void *old_dat = state.userdata_;

		NodeStack stack(state.userdata_);
		evalNodes(state, sp, all_sorted_, stack);
		val = blend_s_->getScalar(stack);
		state.userdata_ = old_dat;
	}
	else
	{
		val = blend_val_;
	}

	ival = std::min(1.f, std::max(0.f, 1.f - val));
}

void BlendMaterial::initBsdf(const RenderState &state, SurfacePoint &sp, BsdfFlags &bsdf_types) const
{
	void *old_udat = state.userdata_;

	bsdf_types = BsdfFlags::None;


	float alpha, inv_alpha;
	getBlendVal(state, sp, alpha, inv_alpha);


	SurfacePoint sp_0 = sp;

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	mat_1_->initBsdf(state, sp_0, mat_1_flags_);

	SurfacePoint sp_1 = sp;

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	mat_2_->initBsdf(state, sp_1, mat_2_flags_);

	sp = SurfacePoint::blendSurfacePoints(sp_0, sp_1, inv_alpha);

	bsdf_types = mat_1_flags_ | mat_2_flags_;

	//todo: bump mapping blending
	state.userdata_ = old_udat;
}

Rgb BlendMaterial::eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const
{
	NodeStack stack(state.userdata_);

	float val, ival;
	getBlendVal(state, sp, val, ival);

	Rgb col_1(1.f), col_2(1.f);
	void *old_udat = state.userdata_;

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	col_1 = mat_1_->eval(state, sp, wo, wl, bsdfs);

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	col_2 = mat_2_->eval(state, sp, wo, wl, bsdfs);

	state.userdata_ = old_udat;

	col_1 = ADD_COLORS(col_1, col_2, ival, val);

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col_1, wire_frame_amount, sp);
	return col_1;
}

Rgb BlendMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	NodeStack stack(state.userdata_);

	float val, ival;
	getBlendVal(state, sp, val, ival);

	bool mat_1_sampled = false;
	bool mat_2_sampled = false;

	Rgb col1(0.f), col2(0.f);
	Sample s1 = s, s2 = s;
	Vec3 wi_1(0.f), wi_2(0.f);
	float w_1 = 0.f, w_2 = 0.f;
	void *old_udat = state.userdata_;

	s2.pdf_ = s1.pdf_ = s.pdf_ = 0.f;

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	if(Material::hasFlag(s.flags_, mat_1_flags_))
	{
		col1 = mat_1_->sample(state, sp, wo, wi_1, s1, w_1);
		mat_1_sampled = true;
	}

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	if(Material::hasFlag(s.flags_, mat_2_flags_))
	{
		col2 = mat_2_->sample(state, sp, wo, wi_2, s2, w_2);
		mat_2_sampled = true;
	}

	if(mat_1_sampled && mat_2_sampled)
	{
		wi = (wi_2 + wi_1).normalize();

		s.pdf_ = ADD_PDF(s1.pdf_, s2.pdf_);
		s.sampled_flags_ = s1.sampled_flags_ | s2.sampled_flags_;
		s.reverse_ = s1.reverse_ | s2.reverse_;
		if(s.reverse_)
		{
			s.pdf_back_ = ADD_PDF(s1.pdf_back_, s2.pdf_back_);
			s.col_back_ = ADD_COLORS((s1.col_back_ * w_1), (s2.col_back_ * w_2), ival, val);
		}
		col1 = ADD_COLORS((col1 * w_1), (col2 * w_2), ival, val);
		w = 1.f;//addPdf(W1, W2);
	}
	else if(mat_1_sampled && !mat_2_sampled)
	{
		wi = wi_1;

		s.pdf_ = s1.pdf_;
		s.sampled_flags_ = s1.sampled_flags_;
		s.reverse_ = s1.reverse_;
		if(s.reverse_)
		{
			s.pdf_back_ = s1.pdf_back_;
			s.col_back_ = s1.col_back_;
		}
		//col1 = col1;
		w = w_1;
	}
	else if(!mat_1_sampled && mat_2_sampled)
	{
		wi = wi_2;

		s.pdf_ = s2.pdf_;
		s.sampled_flags_ = s2.sampled_flags_;
		s.reverse_ = s2.reverse_;
		if(s.reverse_)
		{
			s.pdf_back_ = s2.pdf_back_;
			s.col_back_ = s2.col_back_;
		}
		col1 = col2;
		w = w_2;
	}

	state.userdata_ = old_udat;

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col1, wire_frame_amount, sp);
	return col1;
}

Rgb BlendMaterial::sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w) const
{
	NodeStack stack(state.userdata_);

	float val, ival;
	getBlendVal(state, sp, val, ival);

	void *old_udat = state.userdata_;

	Rgb col;

	if(val <= 0.f) col = mat_1_->sample(state, sp, wo, dir, tcol, s, w);
	else if(val >= 1.f) col = mat_2_->sample(state, sp, wo, dir, tcol, s, w);
	else col = mat_1_->sample(state, sp, wo, dir, tcol, s, w) * ival + mat_2_->sample(state, sp, wo, dir, tcol, s, w) * val;

	state.userdata_ = old_udat;

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col, wire_frame_amount, sp);
	return col;
}


float BlendMaterial::pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);

	float pdf_1 = 0.f, pdf_2 = 0.f;
	void *old_udat = state.userdata_;

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	pdf_1 = mat_1_->pdf(state, sp, wo, wi, bsdfs);

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	pdf_2 = mat_2_->pdf(state, sp, wo, wi, bsdfs);

	state.userdata_ = old_udat;

	pdf_1 = ADD_PDF(pdf_1, pdf_2);
	return pdf_1;
}

void BlendMaterial::getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
								bool &reflect, bool &refract, Vec3 *const dir, Rgb *const col) const
{
	NodeStack stack(state.userdata_);

	float val, ival;
	getBlendVal(state, sp, val, ival);

	void *old_udat = state.userdata_;

	reflect = false;
	refract = false;

	bool m_1_reflect = false, m_1_refract = false;

	Vec3 m_1_dir[2];
	Rgb m_1_col[2];

	m_1_col[0] = Rgb(0.f);
	m_1_col[1] = Rgb(0.f);
	m_1_dir[0] = Vec3(0.f);
	m_1_dir[1] = Vec3(0.f);

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	mat_1_->getSpecular(state, sp, wo, m_1_reflect, m_1_refract, m_1_dir, m_1_col);

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	mat_2_->getSpecular(state, sp, wo, reflect, refract, dir, col);

	state.userdata_ = old_udat;

	if(reflect && m_1_reflect)
	{
		col[0] = ADD_COLORS(m_1_col[0], col[0], ival, val);
		dir[0] = (dir[0] + m_1_dir[0]).normalize();
	}
	else if(m_1_reflect)
	{
		col[0] = m_1_col[0] * ival;
		dir[0] = m_1_dir[0];
	}
	else
	{
		col[0] = col[0] * val;
	}

	if(refract && m_1_refract)
	{
		col[1] = ADD_COLORS(m_1_col[1], col[1], ival, val);
		dir[1] = (dir[1] + m_1_dir[1]).normalize();
	}
	else if(m_1_refract)
	{
		col[1] = m_1_col[1] * ival;
		dir[1] = m_1_dir[1];
	}
	else
	{
		col[1] = col[1] * val;
	}

	refract = refract || m_1_refract;
	reflect = reflect || m_1_reflect;

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col, wire_frame_amount, sp);
}

float BlendMaterial::getMatIor() const
{
	return blended_ior_;
}

bool BlendMaterial::isTransparent() const
{
	return mat_1_->isTransparent() || mat_2_->isTransparent();
}

Rgb BlendMaterial::getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);

	float val, ival;
	getBlendVal(state, sp, val, ival);

	Rgb col_1(1.f), col_2(1.f);

	void *old_udat = state.userdata_;

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	col_1 = mat_1_->getTransparency(state, sp, wo);

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	col_2 = mat_2_->getTransparency(state, sp, wo);

	col_1 = ADD_COLORS(col_1, col_2, ival, val);

	state.userdata_ = old_udat;

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col_1, wire_frame_amount, sp);
	return col_1;
}

float BlendMaterial::getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);

	if(isTransparent())
	{
		float val, ival;
		getBlendVal(state, sp, val, ival);

		float al_1 = 1.f, al_2 = 1.f;

		void *old_udat = state.userdata_;

		state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
		al_1 = mat_1_->getAlpha(state, sp, wo);

		state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
		al_2 = mat_2_->getAlpha(state, sp, wo);

		al_1 = std::min(al_1, al_2);

		state.userdata_ = old_udat;

		float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
		applyWireFrame(al_1, wire_frame_amount, sp);
		return al_1;
	}

	float result = 1.0;
	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(result, wire_frame_amount, sp);
	return result;
}

Rgb BlendMaterial::emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const
{
	NodeStack stack(state.userdata_);

	float val, ival;
	getBlendVal(state, sp, val, ival);

	Rgb col_1(0.0), col_2(0.0);
	void *old_udat = state.userdata_;

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	col_1 = mat_1_->emit(state, sp, wo);

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	col_2 = mat_2_->emit(state, sp, wo);

	col_1 = ADD_COLORS(col_1, col_2, ival, val);

	state.userdata_ = old_udat;

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col_1, wire_frame_amount, sp);
	return col_1;
}

bool BlendMaterial::scatterPhoton(const RenderState &state, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s) const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);

	void *old_udat = state.userdata_;
	bool ret = false;

	Rgb col_1(0.f), col_2(0.f);
	float pdf_1 = 0.f, pdf_2 = 0.f;

	state.userdata_ = PTR_ADD(state.userdata_, req_mem_);
	ret = ret || mat_1_->scatterPhoton(state, sp, wi, wo, s);
	col_1 = s.color_;
	pdf_1 = s.pdf_;

	state.userdata_ = PTR_ADD(state.userdata_, mmem_1_);
	ret = ret || mat_2_->scatterPhoton(state, sp, wi, wo, s);
	col_2 = s.color_;
	pdf_2 = s.pdf_;

	s.color_ = ADD_COLORS(col_1, col_2, ival, val);
	s.pdf_ = ADD_PDF(pdf_1, pdf_2);

	state.userdata_ = old_udat;
	return ret;
}

const VolumeHandler *BlendMaterial::getVolumeHandler(bool inside) const
{
	const VolumeHandler *vol_1 = mat_1_->getVolumeHandler(inside);
	const VolumeHandler *vol_2 = mat_2_->getVolumeHandler(inside);

	if(vol_1 && vol_2)
	{
		if(blend_val_ <= 0.5f) return vol_1;
		else return vol_2;
	}
	else if(vol_1) return vol_1;
	else return vol_2;
}

Material *BlendMaterial::factory(ParamMap &params, std::list<ParamMap> &eparams, RenderEnvironment &env)
{
	std::string name;
	const Material *m_1 = nullptr, *m_2 = nullptr;
	double blend_val = 0.5;
	std::string s_visibility = "normal";
	Visibility visibility = Material::Visibility::NormalVisible;
	int mat_pass_index = 0;
	float samplingfactor = 1.f;
	bool receive_shadows = true;
	float wire_frame_amount = 0.f;           //!< Wireframe shading amount
	float wire_frame_thickness = 0.01f;      //!< Wireframe thickness
	float wire_frame_exponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
	Rgb wire_frame_color = Rgb(1.f); //!< Wireframe shading color

	if(! params.getParam("material1", name)) return nullptr;
	m_1 = env.getMaterial(name);
	if(! params.getParam("material2", name)) return nullptr;
	m_2 = env.getMaterial(name);
	params.getParam("blend_value", blend_val);

	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);
	params.getParam("mat_pass_index",   mat_pass_index);
	params.getParam("samplingfactor",   samplingfactor);

	params.getParam("wireframe_amount", wire_frame_amount);
	params.getParam("wireframe_thickness", wire_frame_thickness);
	params.getParam("wireframe_exponent", wire_frame_exponent);
	params.getParam("wireframe_color", wire_frame_color);

	if(s_visibility == "normal") visibility = Material::Visibility::NormalVisible;
	else if(s_visibility == "no_shadows") visibility = Material::Visibility::VisibleNoShadows;
	else if(s_visibility == "shadow_only") visibility = Material::Visibility::InvisibleShadowsOnly;
	else if(s_visibility == "invisible") visibility = Material::Visibility::Invisible;
	else visibility = Material::Visibility::NormalVisible;

	if(m_1 == nullptr || m_2 == nullptr) return nullptr;

	BlendMaterial *mat = new BlendMaterial(m_1, m_2, blend_val, visibility);

	mat->setMaterialIndex(mat_pass_index);
	mat->receive_shadows_ = receive_shadows;

	mat->wireframe_amount_ = wire_frame_amount;
	mat->wireframe_thickness_ = wire_frame_thickness;
	mat->wireframe_exponent_ = wire_frame_exponent;
	mat->wireframe_color_ = wire_frame_color;

	mat->setSamplingFactor(samplingfactor);

	std::vector<ShaderNode *> roots;
	if(mat->loadNodes(eparams, env))
	{
		if(params.getParam("mask", name))
		{
			auto i = mat->m_shaders_table_.find(name);
			if(i != mat->m_shaders_table_.end())
			{
				mat->blend_s_ = i->second;
				mat->recalc_blend_ = true;
				roots.push_back(mat->blend_s_);
			}
			else
			{
				Y_ERROR << "Blend: Blend shader node '" << name << "' does not exist!" << YENDL;
				delete mat;
				return nullptr;
			}
		}
	}
	else
	{
		Y_ERROR << "Blend: loadNodes() failed!" << YENDL;
		delete mat;
		return nullptr;
	}
	mat->solveNodesOrder(roots);
	mat->req_mem_ = sizeof(bool) + mat->req_node_mem_;
	return mat;
}

END_YAFARAY
