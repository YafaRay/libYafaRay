#ifndef Y_TEXTURE_H
#define Y_TEXTURE_H

#include <yafray_config.h>
#include "surface.h"
#include <core_api/color_ramp.h>

#ifdef HAVE_OPENCV
#include <opencv2/photo/photo.hpp>
#endif

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT texture_t
{
	public :
		/* indicate wether the the texture is discrete (e.g. image map) or continuous */
		virtual bool discrete() const { return false; }
		/* indicate wether the the texture is 3-dimensional. If not, or p.z (and
		   z for discrete textures) are unused on getColor and getFloat calls */
		virtual bool isThreeD() const { return true; }
		virtual bool isNormalmap() const { return false; }
		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const = 0;
		virtual colorA_t getColor(int x, int y, int z, bool from_postprocessed=false) const { return colorA_t(0.f); }
		virtual colorA_t getRawColor(const point3d_t &p, bool from_postprocessed=false) const { return getColor(p, from_postprocessed); }
		virtual colorA_t getRawColor(int x, int y, int z, bool from_postprocessed=false) const { return getColor(x, y, z, from_postprocessed); }
		virtual float getFloat(const point3d_t &p, bool from_postprocessed=false) const { return applyIntensityContrastAdjustments(getRawColor(p, from_postprocessed).col2bri()); }
		virtual float getFloat(int x, int y, int z, bool from_postprocessed=false) const { return applyIntensityContrastAdjustments(getRawColor(x, y, z, from_postprocessed).col2bri()); }
		/* gives the number of values in each dimension for discrete textures */
		virtual void resolution(int &x, int &y, int &z) const { x=0, y=0, z=0; }
		virtual void getInterpolationStep(float &step) const { step = 0.f; };
		virtual void postProcessedCreate() { };
		virtual void postProcessedBlur(float blur_factor) { };
		void setAdjustments(float intensity, float contrast, float saturation, float hue, bool clamp, float factor_red, float factor_green, float factor_blue);
		colorA_t applyAdjustments(const colorA_t & texCol) const;
		colorA_t applyIntensityContrastAdjustments(const colorA_t & texCol) const;
		float applyIntensityContrastAdjustments(float texFloat) const;
		colorA_t applyColorAdjustments(const colorA_t & texCol) const;
		void colorRampCreate(std::string modeStr, std::string interpolationStr, std::string hue_interpolationStr) { color_ramp = new color_ramp_t(modeStr, interpolationStr, hue_interpolationStr); } 
		void colorRampAddItem(colorA_t color, float position) { color_ramp->add_item(color, position); }
		virtual ~texture_t() { if(color_ramp) { delete color_ramp; color_ramp = nullptr; } }
		bool get_distance_avg_enabled() const { return distance_avg_enabled; }
		float get_distance_avg_dist_min() const { return distance_avg_dist_min; }
		float get_distance_avg_dist_max() const { return distance_avg_dist_max; }
	
	protected:
		float adj_intensity = 1.f;
		float adj_contrast = 1.f;
		float adj_saturation = 1.f;
		float adj_hue = 0.f;
		bool adj_clamp = false;
		float adj_mult_factor_red = 1.f;
		float adj_mult_factor_green = 1.f;
		float adj_mult_factor_blue = 1.f;
		bool adjustments_set = false;
		color_ramp_t * color_ramp = nullptr;

		bool distance_avg_enabled = false; //!< Distance averaging function that "blurs" a texture when it's far from the camera, to reduce noise/artifacts from far shots but keep texture details in close shots
		float distance_avg_dist_min = 0.f;	//!< Distance (camera to surface point) up to which the texture is used
		float distance_avg_dist_max = 0.f;	//!< Distance (camera to surface point) from which the single averaged texture color will be used. Between dist_min and dist_max, a progressive blend between texture and average color will be used.
};

inline void angmap(const point3d_t &p, float &u, float &v)
{
	float r = p.x*p.x + p.z*p.z;
	u = v = 0.f;
	if (r > 0.f)
	{
		float phiRatio = M_1_PI * fAcos(p.y);//[0,1] range
		r = phiRatio / fSqrt(r);
		u = p.x * r;// costheta * r * phiRatio
		v = p.z * r;// sintheta * r * phiRatio
	}
}

// slightly modified Blender's own function,
// works better than previous function which needed extra tweaks
inline void tubemap(const point3d_t &p, float &u, float &v)
{
	u = 0;
	v = 1 - (p.z + 1)*0.5;
	float d = p.x*p.x + p.y*p.y;
	if (d>0) {
		d = 1/fSqrt(d);
		u = 0.5*(1 - (atan2(p.x*d, p.y*d) *M_1_PI));
	}
}

// maps a direction to a 2d 0..1 interval
inline void spheremap(const point3d_t &p, float &u, float &v)
{
	float sqrtRPhi = p.x*p.x + p.y*p.y;
	float sqrtRTheta = sqrtRPhi + p.z*p.z;
	float phiRatio;

	u = 0.f;
	v = 0.f;

	if(sqrtRPhi > 0.f)
	{
		if(p.y < 0.f) phiRatio = (M_2PI - fAcos(p.x / fSqrt(sqrtRPhi))) * M_1_2PI;
		else		  phiRatio = fAcos(p.x / fSqrt(sqrtRPhi)) * M_1_2PI;
		u = 1.f - phiRatio;
	}

	v = 1.f - (fAcos(p.z / fSqrt(sqrtRTheta)) * M_1_PI);
}

// maps u,v coords in the 0..1 interval to a direction
inline void invSpheremap(float u, float v, vector3d_t &p)
{
	float theta = v * M_PI;
	float phi = -(u * M_2PI);
	float costheta = fCos(theta), sintheta = fSin(theta);
	float cosphi = fCos(phi), sinphi = fSin(phi);
	p.x = sintheta * cosphi;
	p.y = sintheta * sinphi;
	p.z = -costheta;
}

inline void texture_t::setAdjustments(float intensity, float contrast, float saturation, float hue, bool clamp, float factor_red, float factor_green, float factor_blue)
{
	adj_intensity = intensity;
	adj_contrast = contrast;
	adj_saturation = saturation;
	adj_hue = hue / 60.f; //The value of the hue offset parameter is in degrees, but internally the HSV hue works in "units" where each unit is 60 degrees
	adj_clamp = clamp;
	adj_mult_factor_red = factor_red;
	adj_mult_factor_green = factor_green;
	adj_mult_factor_blue = factor_blue;
	
	std::stringstream adjustments_stream;

	if(intensity != 1.f)
	{
		adjustments_stream << " intensity=" << intensity; 
		adjustments_set = true;
	}
	if(contrast != 1.f)
	{
		adjustments_stream << " contrast=" << contrast; 
		adjustments_set = true;
	}
	if(saturation != 1.f)
	{
		adjustments_stream << " saturation=" << saturation; 
		adjustments_set = true;
	}
	if(hue != 0.f)
	{
		adjustments_stream << " hue offset=" << hue << "º"; 
		adjustments_set = true;
	}
	if(factor_red != 1.f)
	{
		adjustments_stream << " factor_red=" << factor_red; 
		adjustments_set = true;
	}
	if(factor_green != 1.f)
	{
		adjustments_stream << " factor_green=" << factor_green; 
		adjustments_set = true;
	}
	if(factor_blue != 1.f)
	{
		adjustments_stream << " factor_blue=" << factor_blue; 
		adjustments_set = true;
	}
	if(clamp)
	{
		adjustments_stream << " clamping=true"; 
		adjustments_set = true;
	}

	if(adjustments_set)
	{
		Y_VERBOSE << "Texture: modified texture adjustment values:" << adjustments_stream.str() << yendl;
	}
}

inline colorA_t texture_t::applyAdjustments(const colorA_t & texCol) const
{
	if(!adjustments_set) return texCol;
	else return applyColorAdjustments(applyIntensityContrastAdjustments(texCol));
}

inline colorA_t texture_t::applyIntensityContrastAdjustments(const colorA_t & texCol) const
{
	if(!adjustments_set) return texCol;
	
	colorA_t ret = texCol;

	if(adj_intensity != 1.f || adj_contrast != 1.f)
	{
		ret.R = (texCol.R-0.5f) * adj_contrast + adj_intensity-0.5f;
		ret.G = (texCol.G-0.5f) * adj_contrast + adj_intensity-0.5f;
		ret.B = (texCol.B-0.5f) * adj_contrast + adj_intensity-0.5f;
	}

	if(adj_clamp) ret.clampRGB0();
	
	return ret;
}

inline colorA_t texture_t::applyColorAdjustments(const colorA_t & texCol) const
{
	if(!adjustments_set) return texCol;

	colorA_t ret = texCol;

	if(adj_mult_factor_red != 1.f) ret.R *= adj_mult_factor_red;
	if(adj_mult_factor_green != 1.f) ret.G *= adj_mult_factor_green;
	if(adj_mult_factor_blue != 1.f) ret.B *= adj_mult_factor_blue;

	if(adj_clamp) ret.clampRGB0();
	
	if(adj_saturation != 1.f || adj_hue != 0.f)
	{
		float h = 0.f, s = 0.f, v = 0.f;
		ret.rgb_to_hsv(h, s, v);
		s *= adj_saturation;
		h += adj_hue;
		if(h < 0.f) h += 6.f;
		else if(h > 6.f) h -= 6.f;
		ret.hsv_to_rgb(h, s, v);
		if(adj_clamp) ret.clampRGB0();
	}

	return ret;
}

inline float texture_t::applyIntensityContrastAdjustments(float texFloat) const
{
	if(!adjustments_set) return texFloat;
	
	float ret = texFloat;

	if(adj_intensity != 1.f || adj_contrast != 1.f)
	{
		ret = (texFloat-0.5f) * adj_contrast + adj_intensity-0.5f;
	}

	if(adj_clamp)
	{
		if(ret < 0.f) ret = 0.f;
		else if(ret > 1.f) ret = 1.f;
	}
	
	return ret;
}

__END_YAFRAY

#endif // Y_TEXTURE_H
