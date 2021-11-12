/****************************************************************************
 *      glass.cc: a dielectric material with dispersion, two trivial mats
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
#include "common/param.h"
#include "render/render_data.h"
#include "volume/volume.h"

BEGIN_YAFARAY

GlassMaterial::GlassMaterial(Logger &logger, float ior, Rgb filt_c, const Rgb &srcol, double disp_pow, bool fake_s, Visibility e_visibility):
		NodeMaterial(logger), filter_color_(filt_c), specular_reflection_color_(srcol), fake_shadow_(fake_s), dispersion_power_(disp_pow)
{
	visibility_ = e_visibility;
	ior_ = ior;
	bsdf_flags_ = BsdfFlags::AllSpecular;
	if(fake_s) bsdf_flags_ |= BsdfFlags::Filter;
	tm_flags_ = fake_s ? BsdfFlags::Filter | BsdfFlags::Transmit : BsdfFlags::Specular | BsdfFlags::Transmit;
	if(disp_pow > 0.0)
	{
		disperse_ = true;
		spectrum::cauchyCoefficients(ior, disp_pow, cauchy_a_, cauchy_b_);
		bsdf_flags_ |= BsdfFlags::Dispersive;
	}

	visibility_ = e_visibility;
}

std::unique_ptr<const MaterialData> GlassMaterial::initBsdf(SurfacePoint &sp, const Camera *camera) const
{
	std::unique_ptr<MaterialData> mat_data = createMaterialData(color_nodes_.size() + bump_nodes_.size());
	if(bump_shader_) evalBump(mat_data->node_tree_data_, sp, bump_shader_, camera);
	for(const auto &node : color_nodes_) node->eval(mat_data->node_tree_data_, sp, camera);
	return mat_data;
}

#define MATCHES(bits, flags) ((bits & (flags)) == (flags))

Rgb GlassMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	if(!s.flags_.hasAny(BsdfFlags::Specular) && !(s.flags_.hasAny(bsdf_flags_ & BsdfFlags::Dispersive) && chromatic))
	{
		s.pdf_ = 0.f;
		Rgb scolor = Rgb(0.f);
		applyWireFrame(scolor, wireframe_shader_, mat_data->node_tree_data_, sp);
		return scolor;
	}
	Vec3 refdir, n;
	const bool outside = sp.ng_ * wo > 0;
	const float cos_wo_n = sp.n_ * wo;
	if(outside) n = (cos_wo_n >= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	else n = (cos_wo_n <= 0) ? sp.n_ : (sp.n_ - (1.00001 * cos_wo_n) * wo).normalize();
	s.pdf_ = 1.f;

	// we need to sample dispersion
	if(disperse_ && chromatic)
	{
		float cur_ior = ior_;
		if(ior_shader_) cur_ior += ior_shader_->getScalar(mat_data->node_tree_data_);
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;

		if(ior_shader_) spectrum::cauchyCoefficients(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
		cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);

		if(Vec3::refract(n, wo, refdir, cur_ior))
		{
			float kr, kt;
			Vec3::fresnel(wo, n, cur_ior, kr, kt);
			const float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(!s.flags_.hasAny(BsdfFlags::Specular) || s.s_1_ < p_kt)
			{
				wi = refdir;
				s.pdf_ = (MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect)) ? p_kt : 1.f;
				s.sampled_flags_ = BsdfFlags::Dispersive | BsdfFlags::Transmit;
				w = 1.f;
				Rgb scolor = getShaderColor(filter_color_shader_, mat_data->node_tree_data_, filter_color_); // * (Kt/std::abs(sp.N*wi));
				applyWireFrame(scolor, wireframe_shader_, mat_data->node_tree_data_, sp);
				return scolor;
			}
			else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect))
			{
				wi = wo;
				wi.reflect(n);
				s.pdf_ = p_kr;
				s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
				w = 1.f;
				Rgb scolor = getShaderColor(mirror_color_shader_, mat_data->node_tree_data_, specular_reflection_color_); // * (Kr/std::abs(sp.N*wi));
				applyWireFrame(scolor, wireframe_shader_, mat_data->node_tree_data_, sp);
				return scolor;
			}
		}
		else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect)) //total inner reflection
		{
			wi = wo;
			wi.reflect(n);
			s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
			w = 1.f;
			Rgb scolor = 1.f; //Rgb(1.f/std::abs(sp.N*wi));
			applyWireFrame(scolor, wireframe_shader_, mat_data->node_tree_data_, sp);
			return scolor;
		}
	}
	else // no dispersion calculation necessary, regardless of material settings
	{
		float cur_ior = ior_;
		if(ior_shader_) cur_ior += ior_shader_->getScalar(mat_data->node_tree_data_);
		if(disperse_ && chromatic)
		{
			float cur_cauchy_a = cauchy_a_;
			float cur_cauchy_b = cauchy_b_;
			if(ior_shader_) spectrum::cauchyCoefficients(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
			cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);
		}

		if(Vec3::refract(n, wo, refdir, cur_ior))
		{
			float kr, kt;
			Vec3::fresnel(wo, n, cur_ior, kr, kt);
			float p_kr = 0.01 + 0.99 * kr, p_kt = 0.01 + 0.99 * kt;
			if(s.s_1_ < p_kt && MATCHES(s.flags_, tm_flags_))
			{
				wi = refdir;
				s.pdf_ = p_kt;
				s.sampled_flags_ = tm_flags_;
				if(s.reverse_)
				{
					s.pdf_back_ = s.pdf_; //wrong...need to calc fresnel explicitly!
					s.col_back_ = getShaderColor(filter_color_shader_, mat_data->node_tree_data_, filter_color_);//*(Kt/std::abs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = getShaderColor(filter_color_shader_, mat_data->node_tree_data_, filter_color_);//*(Kt/std::abs(sp.N*wi));
				applyWireFrame(scolor, wireframe_shader_, mat_data->node_tree_data_, sp);
				return scolor;
			}
			else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect)) //total inner reflection
			{
				wi = wo;
				wi.reflect(n);
				s.pdf_ = p_kr;
				s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
				if(s.reverse_)
				{
					s.pdf_back_ = s.pdf_; //wrong...need to calc fresnel explicitly!
					s.col_back_ = getShaderColor(mirror_color_shader_, mat_data->node_tree_data_, specular_reflection_color_);// * (Kr/std::abs(sp.N*wo));
				}
				w = 1.f;
				Rgb scolor = getShaderColor(mirror_color_shader_, mat_data->node_tree_data_, specular_reflection_color_);// * (Kr/std::abs(sp.N*wi));
				applyWireFrame(scolor, wireframe_shader_, mat_data->node_tree_data_, sp);
				return scolor;
			}
		}
		else if(MATCHES(s.flags_, BsdfFlags::Specular | BsdfFlags::Reflect))//total inner reflection
		{
			wi = wo;
			wi.reflect(n);
			s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
			//Rgb tir_col(1.f/std::abs(sp.N*wi));
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_;
				s.col_back_ = 1.f;//tir_col;
			}
			w = 1.f;
			Rgb scolor = 1.f;//tir_col;
			applyWireFrame(scolor, wireframe_shader_, mat_data->node_tree_data_, sp);
			return scolor;
		}
	}
	s.pdf_ = 0.f;
	return Rgb(0.f);
}

Rgb GlassMaterial::getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	float kr, kt;
	Vec3::fresnel(wo, n, getShaderScalar(ior_shader_, mat_data->node_tree_data_, ior_), kr, kt);
	Rgb result = kt * getShaderColor(filter_color_shader_, mat_data->node_tree_data_, filter_color_);
	applyWireFrame(result, wireframe_shader_, mat_data->node_tree_data_, sp);
	return result;
}

float GlassMaterial::getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const
{
	float alpha = 1.f - getTransparency(mat_data, sp, wo, camera).energy();
	if(alpha < 0.f) alpha = 0.f;
	applyWireFrame(alpha, wireframe_shader_, mat_data->node_tree_data_, sp);
	return alpha;
}

Specular GlassMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const
{
	const bool outside = sp.ng_ * wo > 0;
	Vec3 n;
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
	Vec3 refdir;

	float cur_ior = ior_;
	if(ior_shader_) cur_ior += ior_shader_->getScalar(mat_data->node_tree_data_);

	if(disperse_ && chromatic)
	{
		float cur_cauchy_a = cauchy_a_;
		float cur_cauchy_b = cauchy_b_;
		if(ior_shader_) spectrum::cauchyCoefficients(cur_ior, dispersion_power_, cur_cauchy_a, cur_cauchy_b);
		cur_ior = spectrum::getIor(wavelength, cur_cauchy_a, cur_cauchy_b);
	}

	Specular specular;
	if(Vec3::refract(n, wo, refdir, cur_ior))
	{
		float kr, kt;
		Vec3::fresnel(wo, n, cur_ior, kr, kt);
		if(!chromatic || !disperse_)
		{
			specular.refract_ = std::unique_ptr<DirectionColor>(new DirectionColor());
			specular.refract_->col_ = kt * getShaderColor(filter_color_shader_, mat_data->node_tree_data_, filter_color_);
			specular.refract_->dir_ = refdir;
		}
		//FIXME? If the above does not happen, in this case, we need to sample dispersion, i.e. not considered specular

		// accounting for fresnel reflection when leaving refractive material is a real performance
		// killer as rays keep bouncing inside objects and contribute little after few bounces, so limit we it:
		if(outside || ray_level < 3)
		{
			specular.reflect_ = std::unique_ptr<DirectionColor>(new DirectionColor());
			specular.reflect_->dir_ = wo;
			specular.reflect_->dir_.reflect(n);
			specular.reflect_->col_ = getShaderColor(mirror_color_shader_, mat_data->node_tree_data_, specular_reflection_color_) * kr;
		}
	}
	else //total inner reflection
	{
		specular.reflect_ = std::unique_ptr<DirectionColor>(new DirectionColor());
		specular.reflect_->col_ = getShaderColor(mirror_color_shader_, mat_data->node_tree_data_, specular_reflection_color_);
		specular.reflect_->dir_ = wo;
		specular.reflect_->dir_.reflect(n);
	}
	if(wireframe_thickness_ > 0.f && (specular.reflect_ || specular.refract_))
	{
		const float wire_frame_amount = wireframe_shader_ ? wireframe_shader_->getScalar(mat_data->node_tree_data_) * wireframe_amount_ : wireframe_amount_;
		if(wire_frame_amount > 0.f)
		{
			if(specular.reflect_) applyWireFrame(specular.reflect_->col_, wire_frame_amount, sp);
			if(specular.refract_) applyWireFrame(specular.refract_->col_, wire_frame_amount, sp);
		}
	}
	return specular;
}

float GlassMaterial::getMatIor() const
{
	return ior_;
}

std::unique_ptr<Material> GlassMaterial::factory(Logger &logger, ParamMap &params, std::list<ParamMap> &nodes_params, const Scene &scene)
{
	double ior = 1.4;
	double filt = 0.f;
	double disp_power = 0.0;
	Rgb filt_col(1.f), absorp(1.f), sr_col(1.f);
	std::string name;
	bool fake_shad = false;
	std::string s_visibility = "normal";
	int mat_pass_index = 0;
	bool receive_shadows = true;
	int additionaldepth = 0;
	float samplingfactor = 1.f;
	float wire_frame_amount = 0.f;           //!< Wireframe shading amount
	float wire_frame_thickness = 0.01f;      //!< Wireframe thickness
	float wire_frame_exponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
	Rgb wire_frame_color = Rgb(1.f); //!< Wireframe shading color

	params.getParam("IOR", ior);
	params.getParam("filter_color", filt_col);
	params.getParam("transmit_filter", filt);
	params.getParam("mirror_color", sr_col);
	params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);

	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", s_visibility);
	params.getParam("mat_pass_index", mat_pass_index);
	params.getParam("additionaldepth", additionaldepth);
	params.getParam("samplingfactor", samplingfactor);

	params.getParam("wireframe_amount", wire_frame_amount);
	params.getParam("wireframe_thickness", wire_frame_thickness);
	params.getParam("wireframe_exponent", wire_frame_exponent);
	params.getParam("wireframe_color", wire_frame_color);

	const Visibility visibility = visibility::fromString(s_visibility);

	auto mat = std::unique_ptr<GlassMaterial>(new GlassMaterial(logger, ior, filt * filt_col + Rgb(1.f - filt), sr_col, disp_power, fake_shad, visibility));

	mat->setMaterialIndex(mat_pass_index);
	mat->receive_shadows_ = receive_shadows;
	mat->additional_depth_ = additionaldepth;

	mat->wireframe_amount_ = wire_frame_amount;
	mat->wireframe_thickness_ = wire_frame_thickness;
	mat->wireframe_exponent_ = wire_frame_exponent;
	mat->wireframe_color_ = wire_frame_color;

	mat->setSamplingFactor(samplingfactor);

	if(params.getParam("absorption", absorp))
	{
		double dist = 1.f;
		if(absorp.r_ < 1.f || absorp.g_ < 1.f || absorp.b_ < 1.f)
		{
			//deprecated method:
			Rgb sigma(0.f);
			if(params.getParam("absorption_dist", dist))
			{
				const float maxlog = log(1e38);
				sigma.r_ = (absorp.r_ > 1e-38) ? -log(absorp.r_) : maxlog;
				sigma.g_ = (absorp.g_ > 1e-38) ? -log(absorp.g_) : maxlog;
				sigma.b_ = (absorp.b_ > 1e-38) ? -log(absorp.b_) : maxlog;
				if(dist != 0.f) sigma *= 1.f / dist;
			}
			mat->absorb_ = true;
			mat->beer_sigma_a_ = sigma;
			mat->bsdf_flags_ |= BsdfFlags::Volumetric;
			// creat volume handler (backwards compatibility)
			if(params.getParam("name", name))
			{
				ParamMap map;
				map["type"] = std::string("beer");
				map["absorption_col"] = absorp;
				map["absorption_dist"] = Parameter(dist);
				mat->vol_i_ = VolumeHandler::factory(logger, map, scene);
				mat->bsdf_flags_ |= BsdfFlags::Volumetric;
			}
		}
	}

	std::map<std::string, const ShaderNode *> root_nodes_map;
	// Prepare our node list
	root_nodes_map["mirror_color_shader"] = nullptr;
	root_nodes_map["bump_shader"] = nullptr;
	root_nodes_map["filter_color_shader"] = nullptr;
	root_nodes_map["IOR_shader"] = nullptr;
	root_nodes_map["wireframe_shader"] = nullptr;

	std::vector<const ShaderNode *> root_nodes_list;
	mat->nodes_map_ = mat->loadNodes(nodes_params, scene, logger);
	if(!mat->nodes_map_.empty()) mat->parseNodes(params, root_nodes_list, root_nodes_map, mat->nodes_map_, logger);

	mat->mirror_color_shader_ = root_nodes_map["mirror_color_shader"];
	mat->bump_shader_ = root_nodes_map["bump_shader"];
	mat->filter_color_shader_ = root_nodes_map["filter_color_shader"];
	mat->ior_shader_ = root_nodes_map["IOR_shader"];
	mat->wireframe_shader_ = root_nodes_map["wireframe_shader"];

	// solve nodes order
	if(!root_nodes_list.empty())
	{
		const std::vector<const ShaderNode *> nodes_sorted = mat->solveNodesOrder(root_nodes_list, mat->nodes_map_, logger);
		if(mat->mirror_color_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->mirror_color_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->filter_color_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->filter_color_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->ior_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->ior_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->wireframe_shader_)
		{
			const std::vector<const ShaderNode *> shader_nodes_list = mat->getNodeList(mat->wireframe_shader_, nodes_sorted);
			mat->color_nodes_.insert(mat->color_nodes_.end(), shader_nodes_list.begin(), shader_nodes_list.end());
		}
		if(mat->bump_shader_) mat->bump_nodes_ = mat->getNodeList(mat->bump_shader_, nodes_sorted);
	}
	return mat;
}

Rgb GlassMaterial::getGlossyColor(const NodeTreeData &node_tree_data) const
{
	return mirror_color_shader_ ? mirror_color_shader_->getColor(node_tree_data) : specular_reflection_color_;
}

Rgb GlassMaterial::getTransColor(const NodeTreeData &node_tree_data) const
{
	if(filter_color_shader_ || filter_color_.minimum() < .99f)	return (filter_color_shader_ ? filter_color_shader_->getColor(node_tree_data) : filter_color_);
	else
	{
		Rgb tmp_col = beer_sigma_a_;
		tmp_col.clampRgb01();
		return Rgb(1.f) - tmp_col;
	}
}

Rgb GlassMaterial::getMirrorColor(const NodeTreeData &node_tree_data) const
{
	return mirror_color_shader_ ? mirror_color_shader_->getColor(node_tree_data) : specular_reflection_color_;
}

Rgb MirrorMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	wi = Vec3::reflectDir(sp.n_, wo);
	s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
	w = 1.f;
	return ref_col_ * (1.f / std::abs(sp.n_ * wi));
}

Specular MirrorMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const
{
	Specular specular;
	specular.reflect_ = std::unique_ptr<DirectionColor>(new DirectionColor());
	specular.reflect_->col_ = ref_col_;
	const Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	specular.reflect_->dir_ = Vec3::reflectDir(n, wo);
	return specular;
}

std::unique_ptr<Material> MirrorMaterial::factory(Logger &logger, ParamMap &params, std::list<ParamMap> &param_list, const Scene &scene)
{
	Rgb col(1.0);
	float refl = 1.0;
	params.getParam("color", col);
	params.getParam("reflect", refl);
	return std::unique_ptr<Material>(new MirrorMaterial(logger, col, refl));
}


Rgb NullMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	s.pdf_ = 0.f;
	w = 0.f;
	return Rgb(0.f);
}

std::unique_ptr<Material> NullMaterial::factory(Logger &logger, ParamMap &, std::list<ParamMap> &, const Scene &)
{
	return std::unique_ptr<Material>(new NullMaterial(logger));
}

END_YAFARAY
