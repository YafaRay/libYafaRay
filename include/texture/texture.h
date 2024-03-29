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

#ifndef LIBYAFARAY_TEXTURE_H
#define LIBYAFARAY_TEXTURE_H

#include "color/color.h"
#include "color/color_ramp.h"
#include "geometry/vector.h"
#include "common/logger.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include <memory>
#include <sstream>

namespace yafaray {

class ParamMap;
class Scene;
class MipMapParams;
template <typename T> class Items;
class Logger;

struct InterpolationType : public Enum<InterpolationType>
{
	using Enum::Enum;
	enum : ValueType_t { Bilinear, Bicubic, Trilinear, Ewa, None };
	inline static const EnumMap<ValueType_t> map_{{
			{"bilinear", Bilinear, "Bilinear interpolation (recommended default)"},
			{"bicubic", Bicubic, "Bicubic interpolation (slower but better quality than bilinear)"},
			{"mipmap_trilinear", Trilinear, "For trilinear mipmaps interpolation (to avoid aliasing in far distances)"},
			{"mipmap_ewa", Ewa, "For EWA mipmaps interpolation (to avoid aliasing in far distances). Slower but better quality than trilinear"},
			{"none", None, "No interpolation, not recommended for production"},
		}};
};

class Texture
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "Texture"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<Texture>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual std::map<std::string, const ParamMeta *> getParamMetaMap() const = 0;
		[[nodiscard]] std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const;
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		Texture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures);
		virtual ~Texture() = default;
		void setId(size_t id) { id_ = id; }
		std::string getName() const;
		[[nodiscard]] size_t getId() const { return id_; }

		/* indicate wether the the texture is discrete (e.g. image map) or continuous */
		virtual bool discrete() const { return false; }
		/* indicate wether the the texture is 3-dimensional. If not, or p.z (and
		   z for discrete textures) are unused on getColor and getFloat calls */
		virtual bool isThreeD() const { return true; }
		virtual bool isNormalmap() const { return false; }

		virtual Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const { return Rgba(0.f); }
		Rgba getColor(const Point3f &p) const { return getColor(p, nullptr); }
		virtual Rgba getRawColor(const Point3f &p, const MipMapParams *mipmap_params) const { return getColor(p, mipmap_params); }
		Rgba getRawColor(const Point3f &p) const { return getRawColor(p, nullptr); }
		virtual float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const { return applyIntensityContrastAdjustments(getRawColor(p, mipmap_params).col2Bri()); }
		float getFloat(const Point3f &p) const { return getFloat(p, nullptr); }

		/* gives the number of values in each dimension for discrete textures. Returns resolution of the texture, last coordinate z/depth is for 3D textures (not currently implemented) */
		virtual std::array<int, 3> resolution() const { return {0, 0, 0}; };
		virtual void useMipMaps() {}
		virtual void updateMipMaps() {}
		Rgba applyAdjustments(const Rgba &tex_col) const;
		Rgba applyIntensityContrastAdjustments(const Rgba &tex_col) const;
		float applyIntensityContrastAdjustments(float tex_float) const;
		Rgba applyColorAdjustments(const Rgba &tex_col) const;
		InterpolationType getInterpolationType() const { return params_.interpolation_type_; }
		static Uv<float> angMap(const Point3f &p);
		static void tubeMap(const Point3f &p, float &u, float &v);
		static Uv<float> sphereMap(const Point3f &p);
		static Point3f invSphereMap(const Uv<float> &uv);

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Blend, Clouds, Marble, Wood, Voronoi, Musgrave, DistortedNoise, RgbCube, Image };
			inline static const EnumMap<ValueType_t> map_{{
					{"blend", Blend, ""},
					{"clouds", Clouds, ""},
					{"marble", Marble, ""},
					{"wood", Wood, ""},
					{"voronoi", Voronoi, ""},
					{"musgrave", Musgrave, ""},
					{"distorted_noise", DistortedNoise, ""},
					{"rgb_cube", RgbCube, ""},
					{"image", Image, ""},
				}};
		};
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float, adj_mult_factor_red_, 1.f, "adj_mult_factor_red", "");
			PARAM_DECL(float, adj_mult_factor_green_, 1.f, "adj_mult_factor_green", "");
			PARAM_DECL(float, adj_mult_factor_blue_, 1.f, "adj_mult_factor_blue", "");
			PARAM_DECL(float, adj_intensity_, 1.f, "adj_intensity", "");
			PARAM_DECL(float, adj_contrast_, 1.f, "adj_contrast", "");
			PARAM_DECL(float, adj_saturation_, 1.f, "adj_saturation", "");
			PARAM_DECL(float, adj_hue_degrees_, 0.f, "adj_hue", "");
			PARAM_DECL(bool, adj_clamp_, false, "adj_clamp", "");
			PARAM_ENUM_DECL(InterpolationType, interpolation_type_, InterpolationType::Bilinear, "interpolate", "Interpolation type (currently only used in image textures)");
			PARAM_DECL(int, ramp_num_items_, 0, "ramp_num_items", "Number of items in color ramp. Ramp disabled if this value is zero. If this is non-zero, then add also the ramp items with additional entries 'ramp_item_(num)_color (color)' and 'ramp_item_(num)_position (float)'");
			PARAM_ENUM_DECL(ColorRamp::Mode, ramp_color_mode_, ColorRamp::Mode::Rgb, "ramp_color_mode", "");
			PARAM_ENUM_DECL(ColorRamp::Interpolation, ramp_interpolation_, ColorRamp::Interpolation::Linear, "ramp_interpolation", "");
			PARAM_ENUM_DECL(ColorRamp::HueInterpolation, ramp_hue_interpolation_, ColorRamp::HueInterpolation::Near, "ramp_hue_interpolation", "");
		} params_;
		const float adj_hue_radians_{params_.adj_hue_degrees_ / 60.f};
		const bool adjustments_set_{
				CHECK_PARAM_NOT_DEFAULT(adj_mult_factor_red_) ||
				CHECK_PARAM_NOT_DEFAULT(adj_mult_factor_green_) ||
				CHECK_PARAM_NOT_DEFAULT(adj_mult_factor_blue_) ||
				CHECK_PARAM_NOT_DEFAULT(adj_intensity_) ||
				CHECK_PARAM_NOT_DEFAULT(adj_contrast_) ||
				CHECK_PARAM_NOT_DEFAULT(adj_saturation_) ||
				CHECK_PARAM_NOT_DEFAULT(adj_hue_degrees_) ||
				CHECK_PARAM_NOT_DEFAULT(adj_clamp_)
		};
		size_t id_{0};
		std::unique_ptr<ColorRamp> color_ramp_;
		const Items<Texture> &textures_;
		Logger &logger_;
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_H
