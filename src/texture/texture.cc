/****************************************************************************
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

#include "texture/texture.h"
#include "texture/texture_blend.h"
#include "texture/texture_clouds.h"
#include "texture/texture_distorted_noise.h"
#include "texture/texture_marble.h"
#include "texture/texture_musgrave.h"
#include "texture/texture_rgb_cube.h"
#include "texture/texture_voronoi.h"
#include "texture/texture_wood.h"
#include "texture/texture_image.h"
#include "common/items.h"
#include "param/param.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> Texture::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(adj_mult_factor_red_);
	PARAM_META(adj_mult_factor_green_);
	PARAM_META(adj_mult_factor_blue_);
	PARAM_META(adj_intensity_);
	PARAM_META(adj_contrast_);
	PARAM_META(adj_saturation_);
	PARAM_META(adj_hue_degrees_);
	PARAM_META(adj_clamp_);
	PARAM_META(interpolation_type_);
	PARAM_META(ramp_num_items_);
	PARAM_META(ramp_color_mode_);
	PARAM_META(ramp_interpolation_);
	PARAM_META(ramp_hue_interpolation_);
	return param_meta_map;
}

Texture::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(adj_mult_factor_red_);
	PARAM_LOAD(adj_mult_factor_green_);
	PARAM_LOAD(adj_mult_factor_blue_);
	PARAM_LOAD(adj_intensity_);
	PARAM_LOAD(adj_contrast_);
	PARAM_LOAD(adj_saturation_);
	PARAM_LOAD(adj_hue_degrees_);
	PARAM_LOAD(adj_clamp_);
	PARAM_ENUM_LOAD(interpolation_type_);
	PARAM_LOAD(ramp_num_items_);
	PARAM_ENUM_LOAD(ramp_color_mode_);
	PARAM_ENUM_LOAD(ramp_interpolation_);
	PARAM_ENUM_LOAD(ramp_hue_interpolation_);
}

ParamMap Texture::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(adj_mult_factor_red_);
	PARAM_SAVE(adj_mult_factor_green_);
	PARAM_SAVE(adj_mult_factor_blue_);
	PARAM_SAVE(adj_intensity_);
	PARAM_SAVE(adj_contrast_);
	PARAM_SAVE(adj_saturation_);
	PARAM_SAVE(adj_hue_degrees_);
	PARAM_SAVE(adj_clamp_);
	PARAM_ENUM_SAVE(interpolation_type_);
	PARAM_SAVE(ramp_num_items_);
	PARAM_ENUM_SAVE(ramp_color_mode_);
	PARAM_ENUM_SAVE(ramp_interpolation_);
	PARAM_ENUM_SAVE(ramp_hue_interpolation_);
	if(color_ramp_)
	{
		const std::vector<ColorRampItem> &ramp_items{color_ramp_->getRamp()};
		for(size_t i = 0; i < ramp_items.size(); ++i)
		{
			std::stringstream param_name;
			param_name << "ramp_item_" << i << "_color";
			param_map.setParam(param_name.str(), ramp_items[i].color());
			param_name.str("");
			param_name.clear();

			param_name << "ramp_item_" << i << "_position";
			param_map.setParam(param_name.str(), ramp_items[i].position());
			param_name.str("");
			param_name.clear();
		}
	}
	return param_map;
}

std::pair<std::unique_ptr<Texture>, ParamResult> Texture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Blend: return BlendTexture::factory(logger, scene, name, param_map);
		case Type::Clouds: return CloudsTexture::factory(logger, scene, name, param_map);
		case Type::Marble: return MarbleTexture::factory(logger, scene, name, param_map);
		case Type::Wood: return WoodTexture::factory(logger, scene, name, param_map);
		case Type::Voronoi: return VoronoiTexture::factory(logger, scene, name, param_map);
		case Type::Musgrave: return MusgraveTexture::factory(logger, scene, name, param_map);
		case Type::DistortedNoise: return DistortedNoiseTexture::factory(logger, scene, name, param_map);
		case Type::RgbCube: return RgbCubeTexture::factory(logger, scene, name, param_map);
		case Type::Image: return ImageTexture::factory(logger, scene, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

Texture::Texture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures) : params_{param_result, param_map}, textures_{textures}, logger_{logger}
{
	if(params_.ramp_num_items_ > 0)
	{
		color_ramp_ = std::make_unique<ColorRamp>(params_.ramp_color_mode_, params_.ramp_interpolation_, params_.ramp_hue_interpolation_);
		for(int i = 0; i < params_.ramp_num_items_; ++i)
		{
			std::stringstream param_name;
			Rgba color(0.f, 0.f, 0.f, 1.f);
			float position = 0.f;
			param_name << "ramp_item_" << i << "_color";
			if(param_map.getParam(param_name.str(), color) == YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE)
			{
				param_result.flags_ |= ResultFlags{YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE};
				param_result.wrong_type_params_.emplace_back(param_name.str());
			}
			param_name.str("");
			param_name.clear();

			param_name << "ramp_item_" << i << "_position";
			if(param_map.getParam(param_name.str(), position) == YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE)
			{
				param_result.flags_ |= ResultFlags{YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE};
				param_result.wrong_type_params_.emplace_back(param_name.str());
			}
			param_name.str("");
			param_name.clear();
			color_ramp_->addItem(color, position);
		}
	}
}

Uv<float> Texture::angMap(const Point3f &p)
{
	float r = p[Axis::X] * p[Axis::X] + p[Axis::Z] * p[Axis::Z];
	if(r > 0.f)
	{
		const float phi_ratio = math::div_1_by_pi<> * math::acos(p[Axis::Y]);//[0,1] range
		r = phi_ratio / math::sqrt(r);
		return {
			p[Axis::X] * r, // costheta * r * phiRatio
			p[Axis::Z] * r // sintheta * r * phiRatio
		};
	}
	else return {0.f, 0.f};
}

// slightly modified Blender's own function,
// works better than previous function which needed extra tweaks
void Texture::tubeMap(const Point3f &p, float &u, float &v)
{
	u = 0;
	v = 1 - (p[Axis::Z] + 1) * 0.5f;
	float d = p[Axis::X] * p[Axis::X] + p[Axis::Y] * p[Axis::Y];
	if(d > 0)
	{
		d = 1 / math::sqrt(d);
		u = 0.5f * (1 - (std::atan2(p[Axis::X] * d, p[Axis::Y] * d) * math::div_1_by_pi<>));
	}
}

// maps a direction to a 2d 0..1 interval
Uv<float> Texture::sphereMap(const Point3f &p)
{
	const float sqrt_r_phi = p[Axis::X] * p[Axis::X] + p[Axis::Y] * p[Axis::Y];
	const float sqrt_r_theta = sqrt_r_phi + p[Axis::Z] * p[Axis::Z];
	Uv<float> uv;
	if(sqrt_r_phi > 0.f)
	{
		float phi_ratio;
		if(p[Axis::Y] < 0.f) phi_ratio = (math::mult_pi_by_2<> - math::acos(p[Axis::X] / math::sqrt(sqrt_r_phi))) * math::div_1_by_2pi<>;
		else phi_ratio = math::acos(p[Axis::X] / math::sqrt(sqrt_r_phi)) * math::div_1_by_2pi<>;
		uv.u_ = 1.f - phi_ratio;
	}
	else uv.u_ = 0.f;
	uv.v_ = 1.f - (math::acos(p[Axis::Z] / math::sqrt(sqrt_r_theta)) * math::div_1_by_pi<>);
	return uv;
}

// maps u,v coords in the 0..1 interval to a direction
Point3f Texture::invSphereMap(const Uv<float> &uv)
{
	const float theta = uv.v_ * math::num_pi<>;
	const float phi = -(uv.u_ * math::mult_pi_by_2<>);
	const float costheta = math::cos(theta), sintheta = math::sin(theta);
	const float cosphi = math::cos(phi), sinphi = math::sin(phi);
	return {{
			sintheta * cosphi,
			sintheta * sinphi,
			-costheta
	}};
}

Rgba Texture::applyAdjustments(const Rgba &tex_col) const
{
	if(!adjustments_set_) return tex_col;
	else return applyColorAdjustments(applyIntensityContrastAdjustments(tex_col));
}

Rgba Texture::applyIntensityContrastAdjustments(const Rgba &tex_col) const
{
	if(!adjustments_set_) return tex_col;

	Rgba ret = tex_col;

	if(params_.adj_intensity_ != 1.f || params_.adj_contrast_ != 1.f)
	{
		ret.r_ = (tex_col.r_ - 0.5f) * params_.adj_contrast_ + params_.adj_intensity_ - 0.5f;
		ret.g_ = (tex_col.g_ - 0.5f) * params_.adj_contrast_ + params_.adj_intensity_ - 0.5f;
		ret.b_ = (tex_col.b_ - 0.5f) * params_.adj_contrast_ + params_.adj_intensity_ - 0.5f;
	}

	if(params_.adj_clamp_) ret.clampRgb0();

	return ret;
}

Rgba Texture::applyColorAdjustments(const Rgba &tex_col) const
{
	if(!adjustments_set_) return tex_col;

	Rgba ret = tex_col;

	if(params_.adj_mult_factor_red_ != 1.f) ret.r_ *= params_.adj_mult_factor_red_;
	if(params_.adj_mult_factor_green_ != 1.f) ret.g_ *= params_.adj_mult_factor_green_;
	if(params_.adj_mult_factor_blue_ != 1.f) ret.b_ *= params_.adj_mult_factor_blue_;

	if(params_.adj_clamp_) ret.clampRgb0();

	if(params_.adj_saturation_ != 1.f || adj_hue_radians_ != 0.f)
	{
		float h = 0.f, s = 0.f, v = 0.f;
		ret.rgbToHsv(h, s, v);
		s *= params_.adj_saturation_;
		h += adj_hue_radians_;
		if(h < 0.f) h += 6.f;
		else if(h > 6.f) h -= 6.f;
		ret.hsvToRgb(h, s, v);
		if(params_.adj_clamp_) ret.clampRgb0();
	}

	return ret;
}

float Texture::applyIntensityContrastAdjustments(float tex_float) const
{
	if(!adjustments_set_) return tex_float;

	float ret = tex_float;

	if(params_.adj_intensity_ != 1.f || params_.adj_contrast_ != 1.f)
	{
		ret = (tex_float - 0.5f) * params_.adj_contrast_ + params_.adj_intensity_ - 0.5f;
	}

	if(params_.adj_clamp_)
	{
		if(ret < 0.f) ret = 0.f;
		else if(ret > 1.f) ret = 1.f;
	}

	return ret;
}

std::string Texture::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<texture name=\"" << getName() << "\">" << std::endl;
	ss << param_map.exportMap(indent_level + 1, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level, '\t') << "</texture>" << std::endl;
	return ss.str();
}

std::string Texture::getName() const
{
	return textures_.findNameFromId(id_).first;
}

} //namespace yafaray
