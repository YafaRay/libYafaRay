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

#include <stack>
#include "material/material_blend.h"
#include "shader/shader_node.h"
#include "geometry/surface.h"
#include "common/param.h"
#include "common/logger.h"
#include "math/interpolation.h"
#include "render/render_data.h"

BEGIN_YAFARAY

BlendMaterial::BlendMaterial(Logger &logger, const Material *m_1, const Material *m_2, float bval, Visibility visibility):
		NodeMaterial(logger), mat_1_(m_1), mat_2_(m_2)
{
	visibility_ = visibility;
	recalc_blend_ = false;
	blend_val_ = bval;
	blended_ior_ = (mat_1_->getMatIor() + mat_2_->getMatIor()) * 0.5f;
	visibility_ = visibility;
	additional_depth_ = std::max(mat_1_->getAdditionalDepth(), mat_2_->getAdditionalDepth());
}

inline float BlendMaterial::getBlendVal(const NodeTreeData &node_tree_data) const
{
	const float blend_val = getShaderScalar(blend_shader_, node_tree_data, blend_val_);
	return blend_val;
}

std::unique_ptr<MaterialData> BlendMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	std::unique_ptr<MaterialData> mat_data = createMaterialData(color_nodes_.size() + bump_nodes_.size());
	evalNodes(sp, color_nodes_, mat_data->node_tree_data_, camera);
	const float blend_val = getBlendVal(mat_data->node_tree_data_);

	BlendMaterialData *mat_data_specific = static_cast<BlendMaterialData *>(mat_data.get());
	SurfacePoint sp_1 = sp;
	mat_data_specific->mat_1_data_ = mat_1_->initBsdf(sp_1, camera);
	SurfacePoint sp_2 = sp;
	mat_data_specific->mat_2_data_ = mat_2_->initBsdf(sp_2, camera);
	sp = SurfacePoint::blendSurfacePoints(sp_1, sp_2, blend_val);
	mat_data->bsdf_flags_ = mat_data_specific->mat_1_data_->bsdf_flags_ | mat_data_specific->mat_2_data_->bsdf_flags_;

	//todo: bump mapping blending
	return mat_data;
}

Rgb BlendMaterial::eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const Rgb col_1 = mat_1_->eval(mat_data_specific->mat_1_data_.get(), sp, wo, wl, bsdfs);
	const Rgb col_2 = mat_2_->eval(mat_data_specific->mat_2_data_.get(), sp, wo, wl, bsdfs);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	if(wireframe_thickness_ > 0.f) applyWireFrame(col_blend, wireframe_shader_, mat_data->node_tree_data_, sp);
	return col_blend;
}

Rgb BlendMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	bool mat_1_sampled = false;
	bool mat_2_sampled = false;

	Rgb col_1(0.f), col_2(0.f);
	Sample s_1 = s, s_2 = s;
	Vec3 wi_1(0.f), wi_2(0.f);
	float w_1 = 0.f, w_2 = 0.f;

	s_2.pdf_ = s_1.pdf_ = s.pdf_ = 0.f;
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	if(s.flags_.hasAny(mat_data_specific->mat_1_data_->bsdf_flags_))
	{
		col_1 = mat_1_->sample(mat_data_specific->mat_1_data_.get(), sp, wo, wi_1, s_1, w_1, chromatic, wavelength, camera);
		mat_1_sampled = true;
	}

	if(s.flags_.hasAny(mat_data_specific->mat_2_data_->bsdf_flags_))
	{
		col_2 = mat_2_->sample(mat_data_specific->mat_2_data_.get(), sp, wo, wi_2, s_2, w_2, chromatic, wavelength, camera);
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
	if(wireframe_thickness_ > 0.f) applyWireFrame(col_1, wireframe_shader_, mat_data->node_tree_data_, sp);
	return col_1;
}

Rgb BlendMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	Rgb col;
	if(blend_val <= 0.f) col = mat_1_->sample(mat_data_specific->mat_1_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
	else if(blend_val >= 1.f) col = mat_2_->sample(mat_data_specific->mat_2_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
	else
	{
		const Rgb col_1 = mat_1_->sample(mat_data_specific->mat_1_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
		const Rgb col_2 = mat_2_->sample(mat_data_specific->mat_2_data_.get(), sp, wo, dir, tcol, s, w, chromatic, wavelength);
		col = math::lerp(col_1, col_2, blend_val);
	}
	if(wireframe_thickness_ > 0.f) applyWireFrame(col, wireframe_shader_, mat_data->node_tree_data_, sp);
	return col;
}

float BlendMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const float pdf_1 = mat_1_->pdf(mat_data_specific->mat_1_data_.get(), sp, wo, wi, bsdfs);
	const float pdf_2 = mat_2_->pdf(mat_data_specific->mat_2_data_.get(), sp, wo, wi, bsdfs);
	const float pdf_blend = math::lerp(pdf_1, pdf_2, blend_val);
	return pdf_blend;
}

Specular BlendMaterial::getSpecular(int raylevel, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	Specular specular_1 = mat_1_->getSpecular(raylevel, mat_data_specific->mat_1_data_.get(), sp, wo, chromatic, wavelength);
	Specular specular_2 = mat_2_->getSpecular(raylevel, mat_data_specific->mat_2_data_.get(), sp, wo, chromatic, wavelength);
	Specular specular_blend;
	specular_blend.reflect_ = DirectionColor::blend(std::move(specular_1.reflect_), std::move(specular_2.reflect_), blend_val);
	specular_blend.refract_ = DirectionColor::blend(std::move(specular_1.refract_), std::move(specular_2.refract_), blend_val);
	if(wireframe_thickness_ > 0.f && (specular_blend.reflect_ || specular_blend.refract_))
	{
		const float wire_frame_amount = wireframe_shader_ ? wireframe_shader_->getScalar(mat_data->node_tree_data_) * wireframe_amount_ : wireframe_amount_;
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
	return mat_1_->isTransparent() || mat_2_->isTransparent();
}

Rgb BlendMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const Rgb col_1 = mat_1_->getTransparency(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
	const Rgb col_2 = mat_2_->getTransparency(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	if(wireframe_thickness_ > 0.f) applyWireFrame(col_blend, wireframe_shader_, mat_data->node_tree_data_, sp);
	return col_blend;
}

float BlendMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	float result = 1.f;
	if(isTransparent())
	{
		const float al_1 = mat_1_->getAlpha(mat_data_specific->mat_1_data_.get(), sp, wo, camera);
		const float al_2 = mat_2_->getAlpha(mat_data_specific->mat_2_data_.get(), sp, wo, camera);
		result = std::min(al_1, al_2);
	}
	if(wireframe_thickness_ > 0.f) applyWireFrame(result, wireframe_shader_, mat_data->node_tree_data_, sp);
	return result;
}

Rgb BlendMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool lights_geometry_material_emit) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const Rgb col_1 = mat_1_->emit(mat_data_specific->mat_1_data_.get(), sp, wo, lights_geometry_material_emit);
	const Rgb col_2 = mat_2_->emit(mat_data_specific->mat_2_data_.get(), sp, wo, lights_geometry_material_emit);
	Rgb col_blend = math::lerp(col_1, col_2, blend_val);
	if(wireframe_thickness_ > 0.f) applyWireFrame(col_blend, wireframe_shader_, mat_data->node_tree_data_, sp);
	return col_blend;
}

bool BlendMaterial::scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
{
	const float blend_val = getBlendVal(mat_data->node_tree_data_);
	const BlendMaterialData *mat_data_specific = static_cast<const BlendMaterialData *>(mat_data);
	const bool scattered_1 = mat_1_->scatterPhoton(mat_data_specific->mat_1_data_.get(), sp, wi, wo, s, chromatic, wavelength, camera);
	const Rgb col_1 = s.color_;
	const float pdf_1 = s.pdf_;
	const bool scattered_2 = mat_2_->scatterPhoton(mat_data_specific->mat_2_data_.get(), sp, wi, wo, s, chromatic, wavelength, camera);
	const Rgb col_2 = s.color_;
	const float pdf_2 = s.pdf_;
	s.color_ = math::lerp(col_1, col_2, blend_val);
	s.pdf_ = math::lerp(pdf_1, pdf_2, blend_val);
	return scattered_1 | scattered_2;
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

std::unique_ptr<Material> BlendMaterial::factory(Logger &logger, ParamMap &params, std::list<ParamMap> &nodes_params, const Scene &scene)
{
	std::string name;
	double blend_val = 0.5;
	std::string s_visibility = "normal";
	int mat_pass_index = 0;
	float samplingfactor = 1.f;
	bool receive_shadows = true;
	float wire_frame_amount = 0.f;           //!< Wireframe shading amount
	float wire_frame_thickness = 0.01f;      //!< Wireframe thickness
	float wire_frame_exponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
	Rgb wire_frame_color = Rgb(1.f); //!< Wireframe shading color

	if(! params.getParam("material1", name)) return nullptr;
	const Material *m_1 = scene.getMaterial(name);
	if(! params.getParam("material2", name)) return nullptr;
	const Material *m_2 = scene.getMaterial(name);
	if(!m_1 || !m_2) return nullptr;
	params.getParam("blend_value", blend_val);

	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);
	params.getParam("mat_pass_index", mat_pass_index);
	params.getParam("samplingfactor", samplingfactor);

	params.getParam("wireframe_amount", wire_frame_amount);
	params.getParam("wireframe_thickness", wire_frame_thickness);
	params.getParam("wireframe_exponent", wire_frame_exponent);
	params.getParam("wireframe_color", wire_frame_color);

	const Visibility visibility = visibility::fromString(s_visibility);

	auto mat = std::unique_ptr<BlendMaterial>(new BlendMaterial(logger, m_1, m_2, blend_val, visibility));

	mat->setMaterialIndex(mat_pass_index);
	mat->receive_shadows_ = receive_shadows;
	mat->wireframe_amount_ = wire_frame_amount;
	mat->wireframe_thickness_ = wire_frame_thickness;
	mat->wireframe_exponent_ = wire_frame_exponent;
	mat->wireframe_color_ = wire_frame_color;
	mat->setSamplingFactor(samplingfactor);

	std::vector<const ShaderNode *> root_nodes_list;
	mat->nodes_map_ = mat->loadNodes(nodes_params, scene, logger);
	if(!mat->nodes_map_.empty())
	{
		if(params.getParam("mask", name))
		{
			const auto i = mat->nodes_map_.find(name);
			if(i != mat->nodes_map_.end())
			{
				mat->blend_shader_ = i->second.get();
				mat->recalc_blend_ = true;
				root_nodes_list.push_back(mat->blend_shader_);
			}
			else
			{
				logger.logError("Blend: Blend shader node '", name, "' does not exist!");
				return nullptr;
			}
		}
		mat->color_nodes_ = mat->solveNodesOrder(root_nodes_list, mat->nodes_map_, logger);
	}
	return mat;
}

END_YAFARAY
