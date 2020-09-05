#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_TEXTURE_H
#define YAFARAY_TEXTURE_H

#include "constants.h"
#include "imagehandler/imagehandler.h"
#include "common/color_ramp.h"
#include "common/vector.h"
#include "common/logging.h"
#include <sstream>

BEGIN_YAFARAY

enum class InterpolationType : int;

class Texture
{
	public :
		static Texture *factory(ParamMap &params, RenderEnvironment &render);

		/* indicate wether the the texture is discrete (e.g. image map) or continuous */
		virtual bool discrete() const { return false; }
		/* indicate wether the the texture is 3-dimensional. If not, or p.z (and
		   z for discrete textures) are unused on getColor and getFloat calls */
		virtual bool isThreeD() const { return true; }
		virtual bool isNormalmap() const { return false; }

		virtual Rgba getColor(const Point3 &p, const MipMapParams *mipmap_params = nullptr) const { return Rgba(0.f); }
		virtual Rgba getColor(int x, int y, int z, const MipMapParams *mipmap_params = nullptr) const { return Rgba(0.f); }
		virtual Rgba getRawColor(const Point3 &p, const MipMapParams *mipmap_params = nullptr) const { return getColor(p, mipmap_params); }
		virtual Rgba getRawColor(int x, int y, int z, const MipMapParams *mipmap_params = nullptr) const { return getColor(x, y, z, mipmap_params); }

		virtual float getFloat(const Point3 &p, const MipMapParams *mipmap_params = nullptr) const { return applyIntensityContrastAdjustments(getRawColor(p, mipmap_params).col2Bri()); }
		virtual float getFloat(int x, int y, int z, const MipMapParams *mipmap_params = nullptr) const { return applyIntensityContrastAdjustments(getRawColor(x, y, z, mipmap_params).col2Bri()); }

		/* gives the number of values in each dimension for discrete textures */
		virtual void resolution(int &x, int &y, int &z) const { x = 0, y = 0, z = 0; }

		virtual void getInterpolationStep(float &step) const { step = 0.f; };
		virtual void generateMipMaps() {}
		void setAdjustments(float intensity, float contrast, float saturation, float hue, bool clamp, float factor_red, float factor_green, float factor_blue);
		Rgba applyAdjustments(const Rgba &tex_col) const;
		Rgba applyIntensityContrastAdjustments(const Rgba &tex_col) const;
		float applyIntensityContrastAdjustments(float tex_float) const;
		Rgba applyColorAdjustments(const Rgba &tex_col) const;
		void colorRampCreate(std::string mode_str, std::string interpolation_str, std::string hue_interpolation_str) { color_ramp_ = new ColorRamp(mode_str, interpolation_str, hue_interpolation_str); }
		void colorRampAddItem(Rgba color, float position) { color_ramp_->addItem(color, position); }
		virtual ~Texture() { if(color_ramp_) { delete color_ramp_; color_ramp_ = nullptr; } }
		InterpolationType getInterpolationType() const { return interpolation_type_; }

	protected:
		float adj_intensity_ = 1.f;
		float adj_contrast_ = 1.f;
		float adj_saturation_ = 1.f;
		float adj_hue_ = 0.f;
		bool adj_clamp_ = false;
		float adj_mult_factor_red_ = 1.f;
		float adj_mult_factor_green_ = 1.f;
		float adj_mult_factor_blue_ = 1.f;
		bool adjustments_set_ = false;
		ColorRamp *color_ramp_ = nullptr;
		InterpolationType interpolation_type_ = InterpolationType::Bilinear;
};

inline void angmap__(const Point3 &p, float &u, float &v)
{
	float r = p.x_ * p.x_ + p.z_ * p.z_;
	u = v = 0.f;
	if(r > 0.f)
	{
		float phi_ratio = M_1_PI * fAcos__(p.y_);//[0,1] range
		r = phi_ratio / fSqrt__(r);
		u = p.x_ * r;// costheta * r * phiRatio
		v = p.z_ * r;// sintheta * r * phiRatio
	}
}

// slightly modified Blender's own function,
// works better than previous function which needed extra tweaks
inline void tubemap__(const Point3 &p, float &u, float &v)
{
	u = 0;
	v = 1 - (p.z_ + 1) * 0.5;
	float d = p.x_ * p.x_ + p.y_ * p.y_;
	if(d > 0)
	{
		d = 1 / fSqrt__(d);
		u = 0.5 * (1 - (atan2(p.x_ * d, p.y_ * d) * M_1_PI));
	}
}

// maps a direction to a 2d 0..1 interval
inline void spheremap__(const Point3 &p, float &u, float &v)
{
	float sqrt_r_phi = p.x_ * p.x_ + p.y_ * p.y_;
	float sqrt_r_theta = sqrt_r_phi + p.z_ * p.z_;
	float phi_ratio;

	u = 0.f;
	v = 0.f;

	if(sqrt_r_phi > 0.f)
	{
		if(p.y_ < 0.f) phi_ratio = (mult_pi_by_2__ - fAcos__(p.x_ / fSqrt__(sqrt_r_phi))) * div_1_by_2pi__;
		else phi_ratio = fAcos__(p.x_ / fSqrt__(sqrt_r_phi)) * div_1_by_2pi__;
		u = 1.f - phi_ratio;
	}

	v = 1.f - (fAcos__(p.z_ / fSqrt__(sqrt_r_theta)) * M_1_PI);
}

// maps u,v coords in the 0..1 interval to a direction
inline void invSpheremap__(float u, float v, Vec3 &p)
{
	float theta = v * M_PI;
	float phi = -(u * mult_pi_by_2__);
	float costheta = fCos__(theta), sintheta = fSin__(theta);
	float cosphi = fCos__(phi), sinphi = fSin__(phi);
	p.x_ = sintheta * cosphi;
	p.y_ = sintheta * sinphi;
	p.z_ = -costheta;
}

inline void Texture::setAdjustments(float intensity, float contrast, float saturation, float hue, bool clamp, float factor_red, float factor_green, float factor_blue)
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
		Y_VERBOSE << "Texture: modified texture adjustment values:" << adjustments_stream.str() << YENDL;
	}
}

inline Rgba Texture::applyAdjustments(const Rgba &tex_col) const
{
	if(!adjustments_set_) return tex_col;
	else return applyColorAdjustments(applyIntensityContrastAdjustments(tex_col));
}

inline Rgba Texture::applyIntensityContrastAdjustments(const Rgba &tex_col) const
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

inline Rgba Texture::applyColorAdjustments(const Rgba &tex_col) const
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

inline float Texture::applyIntensityContrastAdjustments(float tex_float) const
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

END_YAFARAY

#endif // YAFARAY_TEXTURE_H
