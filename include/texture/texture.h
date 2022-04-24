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

#include "common/yafaray_common.h"
#include "color/color.h"
#include "color/color_ramp.h"
#include "geometry/vector.h"
#include "common/logger.h"
#include <memory>
#include <sstream>

BEGIN_YAFARAY

class ParamMap;
class Scene;
class MipMapParams;
class Logger;

enum class InterpolationType : int { None, Bilinear, Bicubic, Trilinear, Ewa };

class Texture
{
	public :
		static Texture *factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &params);
		explicit Texture(Logger &logger) : logger_(logger) { }
		virtual ~Texture() = default;

		/* indicate wether the the texture is discrete (e.g. image map) or continuous */
		virtual bool discrete() const { return false; }
		/* indicate wether the the texture is 3-dimensional. If not, or p.z (and
		   z for discrete textures) are unused on getColor and getFloat calls */
		virtual bool isThreeD() const { return true; }
		virtual bool isNormalmap() const { return false; }

		virtual Rgba getColor(const Point3 &p, const MipMapParams *mipmap_params) const { return Rgba(0.f); }
		Rgba getColor(const Point3 &p) const { return getColor(p, nullptr); }
		virtual Rgba getRawColor(const Point3 &p, const MipMapParams *mipmap_params) const { return getColor(p, mipmap_params); }
		Rgba getRawColor(const Point3 &p) const { return getRawColor(p, nullptr); }
		virtual float getFloat(const Point3 &p, const MipMapParams *mipmap_params) const { return applyIntensityContrastAdjustments(getRawColor(p, mipmap_params).col2Bri()); }
		float getFloat(const Point3 &p) const { return getFloat(p, nullptr); }

		/* gives the number of values in each dimension for discrete textures */
		virtual void resolution(int &x, int &y, int &z) const { x = 0, y = 0, z = 0; };
		virtual void generateMipMaps() {}
		void setAdjustments(float intensity, float contrast, float saturation, float hue, bool clamp, float factor_red, float factor_green, float factor_blue);
		Rgba applyAdjustments(const Rgba &tex_col) const;
		Rgba applyIntensityContrastAdjustments(const Rgba &tex_col) const;
		float applyIntensityContrastAdjustments(float tex_float) const;
		Rgba applyColorAdjustments(const Rgba &tex_col) const;
		void colorRampCreate(const std::string &mode_str, const std::string &interpolation_str, const std::string &hue_interpolation_str) { color_ramp_ = std::make_unique<ColorRamp>(mode_str, interpolation_str, hue_interpolation_str); }
		void colorRampAddItem(const Rgba &color, float position) { if(color_ramp_) color_ramp_->addItem(color, position); }
		InterpolationType getInterpolationType() const { return interpolation_type_; }
		static InterpolationType getInterpolationTypeFromName(const std::string &interpolation_type_name);
		static std::string getInterpolationTypeName(const InterpolationType &interpolation_type);
		static void angMap(const Point3 &p, float &u, float &v);
		static void tubeMap(const Point3 &p, float &u, float &v);
		static void sphereMap(const Point3 &p, float &u, float &v);
		static void invSphereMap(float u, float v, Vec3 &p);

	protected:
		static void textureReadColorRamp(const ParamMap &params, Texture *tex);
		float adj_intensity_ = 1.f;
		float adj_contrast_ = 1.f;
		float adj_saturation_ = 1.f;
		float adj_hue_ = 0.f;
		bool adj_clamp_ = false;
		float adj_mult_factor_red_ = 1.f;
		float adj_mult_factor_green_ = 1.f;
		float adj_mult_factor_blue_ = 1.f;
		bool adjustments_set_ = false;
		std::unique_ptr<ColorRamp> color_ramp_;
		InterpolationType interpolation_type_ = InterpolationType::Bilinear;
		Logger &logger_;
};

END_YAFARAY

#endif // YAFARAY_TEXTURE_H
