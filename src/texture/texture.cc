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

#include <cmath>
#include "texture/texture_basic.h"
#include "texture/texture_image.h"
#include "common/param.h"

BEGIN_YAFARAY

Texture * Texture::factory(Logger &logger, ParamMap &params, Scene &scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Texture");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	if(type == "blend") return BlendTexture::factory(logger, params, scene);
	else if(type == "clouds") return CloudsTexture::factory(logger, params, scene);
	else if(type == "marble") return MarbleTexture::factory(logger, params, scene);
	else if(type == "wood") return WoodTexture::factory(logger, params, scene);
	else if(type == "voronoi") return VoronoiTexture::factory(logger, params, scene);
	else if(type == "musgrave") return MusgraveTexture::factory(logger, params, scene);
	else if(type == "distorted_noise") return DistortedNoiseTexture::factory(logger, params, scene);
	else if(type == "rgb_cube") return RgbCubeTexture::factory(logger, params, scene);
	else if(type == "image") return ImageTexture::factory(logger, params, scene);
	else return nullptr;
}

InterpolationType Texture::getInterpolationTypeFromName(const std::string &interpolation_type_name)
{
	// interpolation type, bilinear default
	if(interpolation_type_name == "none") return InterpolationType::None;
	else if(interpolation_type_name == "bicubic") return InterpolationType::Bicubic;
	else if(interpolation_type_name == "mipmap_trilinear") return InterpolationType::Trilinear;
	else if(interpolation_type_name == "mipmap_ewa") return InterpolationType::Ewa;
	else return InterpolationType::Bilinear;
}

std::string Texture::getInterpolationTypeName(const InterpolationType &interpolation_type)
{
	// interpolation type, bilinear default
	switch(interpolation_type)
	{
		case InterpolationType::None: return "none";
		case InterpolationType::Bilinear: return "bilinear";
		case InterpolationType::Bicubic: return "bicubic";
		case InterpolationType::Trilinear: return "mipmap_trilinear";
		case InterpolationType::Ewa: return "mipmap_ewa";
		default: return "bilinear";
	}
}


void Texture::angMap(const Point3 &p, float &u, float &v)
{
	float r = p.x_ * p.x_ + p.z_ * p.z_;
	u = v = 0.f;
	if(r > 0.f)
	{
		const float phi_ratio = math::div_1_by_pi * math::acos(p.y_);//[0,1] range
		r = phi_ratio / math::sqrt(r);
		u = p.x_ * r;// costheta * r * phiRatio
		v = p.z_ * r;// sintheta * r * phiRatio
	}
}

// slightly modified Blender's own function,
// works better than previous function which needed extra tweaks
void Texture::tubeMap(const Point3 &p, float &u, float &v)
{
	u = 0;
	v = 1 - (p.z_ + 1) * 0.5f;
	float d = p.x_ * p.x_ + p.y_ * p.y_;
	if(d > 0)
	{
		d = 1 / math::sqrt(d);
		u = 0.5f * (1 - (std::atan2(p.x_ * d, p.y_ * d) * math::div_1_by_pi));
	}
}

// maps a direction to a 2d 0..1 interval
void Texture::sphereMap(const Point3 &p, float &u, float &v)
{
	const float sqrt_r_phi = p.x_ * p.x_ + p.y_ * p.y_;
	const float sqrt_r_theta = sqrt_r_phi + p.z_ * p.z_;
	float phi_ratio;

	u = 0.f;
	v = 0.f;

	if(sqrt_r_phi > 0.f)
	{
		if(p.y_ < 0.f) phi_ratio = (math::mult_pi_by_2 - math::acos(p.x_ / math::sqrt(sqrt_r_phi))) * math::div_1_by_2pi;
		else phi_ratio = math::acos(p.x_ / math::sqrt(sqrt_r_phi)) * math::div_1_by_2pi;
		u = 1.f - phi_ratio;
	}

	v = 1.f - (math::acos(p.z_ / math::sqrt(sqrt_r_theta)) * math::div_1_by_pi);
}

// maps u,v coords in the 0..1 interval to a direction
void Texture::invSphereMap(float u, float v, Vec3 &p)
{
	const float theta = v * math::num_pi;
	const float phi = -(u * math::mult_pi_by_2);
	const float costheta = math::cos(theta), sintheta = math::sin(theta);
	const float cosphi = math::cos(phi), sinphi = math::sin(phi);
	p.x_ = sintheta * cosphi;
	p.y_ = sintheta * sinphi;
	p.z_ = -costheta;
}

void Texture::setAdjustments(float intensity, float contrast, float saturation, float hue, bool clamp, float factor_red, float factor_green, float factor_blue)
{
	adj_intensity_ = intensity;
	adj_contrast_ = contrast;
	adj_saturation_ = saturation;
	adj_hue_ = hue / 60.f; //The value of the hue offset parameter is in degrees, but internally the HSV hue works in "units" where each unit is 60 degrees
	adj_clamp_ = clamp;
	adj_mult_factor_red_ = factor_red;
	adj_mult_factor_green_ = factor_green;
	adj_mult_factor_blue_ = factor_blue;

	std::stringstream adjustments_stream;

	if(intensity != 1.f)
	{
		adjustments_stream << " intensity=" << intensity;
		adjustments_set_ = true;
	}
	if(contrast != 1.f)
	{
		adjustments_stream << " contrast=" << contrast;
		adjustments_set_ = true;
	}
	if(saturation != 1.f)
	{
		adjustments_stream << " saturation=" << saturation;
		adjustments_set_ = true;
	}
	if(hue != 0.f)
	{
		adjustments_stream << " hue offset=" << hue << "º";
		adjustments_set_ = true;
	}
	if(factor_red != 1.f)
	{
		adjustments_stream << " factor_red=" << factor_red;
		adjustments_set_ = true;
	}
	if(factor_green != 1.f)
	{
		adjustments_stream << " factor_green=" << factor_green;
		adjustments_set_ = true;
	}
	if(factor_blue != 1.f)
	{
		adjustments_stream << " factor_blue=" << factor_blue;
		adjustments_set_ = true;
	}
	if(clamp)
	{
		adjustments_stream << " clamping=true";
		adjustments_set_ = true;
	}

	if(adjustments_set_)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Texture: modified texture adjustment values:", adjustments_stream.str());
	}
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

	if(adj_intensity_ != 1.f || adj_contrast_ != 1.f)
	{
		ret.r_ = (tex_col.r_ - 0.5f) * adj_contrast_ + adj_intensity_ - 0.5f;
		ret.g_ = (tex_col.g_ - 0.5f) * adj_contrast_ + adj_intensity_ - 0.5f;
		ret.b_ = (tex_col.b_ - 0.5f) * adj_contrast_ + adj_intensity_ - 0.5f;
	}

	if(adj_clamp_) ret.clampRgb0();

	return ret;
}

Rgba Texture::applyColorAdjustments(const Rgba &tex_col) const
{
	if(!adjustments_set_) return tex_col;

	Rgba ret = tex_col;

	if(adj_mult_factor_red_ != 1.f) ret.r_ *= adj_mult_factor_red_;
	if(adj_mult_factor_green_ != 1.f) ret.g_ *= adj_mult_factor_green_;
	if(adj_mult_factor_blue_ != 1.f) ret.b_ *= adj_mult_factor_blue_;

	if(adj_clamp_) ret.clampRgb0();

	if(adj_saturation_ != 1.f || adj_hue_ != 0.f)
	{
		float h = 0.f, s = 0.f, v = 0.f;
		ret.rgbToHsv(h, s, v);
		s *= adj_saturation_;
		h += adj_hue_;
		if(h < 0.f) h += 6.f;
		else if(h > 6.f) h -= 6.f;
		ret.hsvToRgb(h, s, v);
		if(adj_clamp_) ret.clampRgb0();
	}

	return ret;
}

float Texture::applyIntensityContrastAdjustments(float tex_float) const
{
	if(!adjustments_set_) return tex_float;

	float ret = tex_float;

	if(adj_intensity_ != 1.f || adj_contrast_ != 1.f)
	{
		ret = (tex_float - 0.5f) * adj_contrast_ + adj_intensity_ - 0.5f;
	}

	if(adj_clamp_)
	{
		if(ret < 0.f) ret = 0.f;
		else if(ret > 1.f) ret = 1.f;
	}

	return ret;
}

void Texture::textureReadColorRamp(const ParamMap &params, Texture *tex)
{
	std::string mode_str, interpolation_str, hue_interpolation_str;
	int ramp_num_items = 0;
	params.getParam("ramp_color_mode", mode_str);
	params.getParam("ramp_hue_interpolation", hue_interpolation_str);
	params.getParam("ramp_interpolation", interpolation_str);
	params.getParam("ramp_num_items", ramp_num_items);

	if(ramp_num_items > 0)
	{
		tex->colorRampCreate(mode_str, interpolation_str, hue_interpolation_str);

		for(int i = 0; i < ramp_num_items; ++i)
		{
			std::stringstream param_name;
			Rgba color(0.f, 0.f, 0.f, 1.f);
			float alpha = 1.f;
			float position = 0.f;
			param_name << "ramp_item_" << i << "_color";
			params.getParam(param_name.str(), color);
			param_name.str("");
			param_name.clear();

			param_name << "ramp_item_" << i << "_alpha";
			params.getParam(param_name.str(), alpha);
			param_name.str("");
			param_name.clear();
			color.a_ = alpha;

			param_name << "ramp_item_" << i << "_position";
			params.getParam(param_name.str(), position);
			param_name.str("");
			param_name.clear();
			tex->colorRampAddItem(color, position);
		}
	}
}

END_YAFARAY
