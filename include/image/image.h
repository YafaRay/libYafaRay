#pragma once
/****************************************************************************
 * 
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
 *
 */

#ifndef LIBYAFARAY_IMAGE_H
#define LIBYAFARAY_IMAGE_H

#include "color/color.h"
#include "geometry/rect.h"
#include "param/class_meta.h"
#include "common/enum.h"
#include <string>
#include <memory>

namespace yafaray {

class ParamMap;
class Scene;
class Logger;

struct DenoiseParams
{
	bool enabled_ = false;
	int hlum_ = 3;
	int hcol_ = 3;
	float mix_ = 0.8f;	//!< Mix factor between the de-noised image and the original "noisy" image to avoid banding artifacts in images with all noise removed.
};

class Image
{
	public:
		struct Type;
		inline static std::string getClassName() { return "Image"; }
		virtual Type type() const = 0;
		void setId(size_t id) { id_ = id; }
		[[nodiscard]] size_t getId() const { return id_; }
		static std::pair<std::unique_ptr<Image>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Gray, GrayAlpha, Color, ColorAlpha };
			inline static const EnumMap<ValueType_t> map_{{
					{"Gray", Gray, "Gray [1 channel]"},
					{"GrayAlpha", GrayAlpha, "Gray + Alpha [2 channels]"},
					{"Color", Color, "Color [3 channels]"},
					{"ColorAlpha", ColorAlpha, "Color + Alpha [4 channels]"},
				}};
		};
		struct Optimization : public Enum<Optimization>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Optimized, Compressed };
			inline static const EnumMap<ValueType_t> map_{{
					{"none", None, ""},
					{"optimized", Optimized, ""},
					{"compressed", Compressed, ""},
				}};
		};
		struct Position : public Enum<Position>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Top, Bottom, Left, Right, Overlay };
			inline static const EnumMap<ValueType_t> map_{{
					{"top", Top, ""},
					{"bottom", Bottom, ""},
					{"left", Left, ""},
					{"right", Right, ""},
					{"overlay", Overlay, ""},
				}};
		};
		const struct Params
		{
			Params() = default;
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(std::string, filename_, "", "filename", "File path when loading the image from a file. Leave blank when creating a new image from RAM");
			PARAM_ENUM_DECL(Type, type_, Type::None, "type", getClassName() + " type (overriden by the loaded image type if '" + filename_meta_.name() + "' is used)");
			PARAM_ENUM_DECL(ColorSpace, color_space_, ColorSpace::Srgb, "color_space", "");
			PARAM_DECL(float, gamma_, 1.f, "gamma", "");
			PARAM_ENUM_DECL(Optimization, image_optimization_, Optimization::Optimized, "image_optimization", "");
			PARAM_DECL(int, width_, 100, "width", "Image width (overriden by the loaded image type if '" + filename_meta_.name() + "' is used)");
			PARAM_DECL(int, height_, 100, "height", "Image height (overriden by the loaded image type if '" + filename_meta_.name() + "' is used)");
		} params_;
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		static std::unique_ptr<Image> factory(const Params &params);
		explicit Image(Params params) : params_{std::move(params)} { }
		virtual ~Image() = default;
		virtual Optimization getOptimization() const = 0;
		virtual Rgba getColor(const Point2i &point) const = 0;
		virtual float getFloat(const Point2i &point) const = 0;
		virtual void setColor(const Point2i &point, const Rgba &col) = 0;
		virtual void setColor(const Point2i &point, Rgba &&col) = 0;
		virtual void addColor(const Point2i &point, const Rgba &col) = 0;
		virtual void setFloat(const Point2i &point, float val) = 0;
		virtual void addFloat(const Point2i &point, float val) = 0;
		virtual void clear() = 0;
		int getWidth() const { return params_.width_; }
		int getHeight() const { return params_.height_; }
		Size2i getSize() const { return {{getWidth(), getHeight()}}; }
		std::string getTypeName() const { return getTypeNameLong(type()); }
		int getNumChannels() const { return getNumChannels(type()); }
		bool hasAlpha() const { return hasAlpha(type()); }
		bool isGrayscale() const { return isGrayscale(type()); }
		ColorSpace getColorSpace() const { return params_.color_space_; }
		float getGamma() const { return params_.gamma_; }

		static Type imageTypeWithAlpha(Type image_type);
		static std::string getTypeNameLong(const Type &image_type);
		static std::string getTypeNameShort(const Type &image_type);
		static Type getTypeFromName(const std::string &image_type_name);
		static int getNumChannels(const Type &image_type);
		static Optimization getOptimizationTypeFromName(const std::string &optimization_type_name);
		static std::string getOptimizationName(const Optimization &optimization_type);
		static bool hasAlpha(const Type &image_type);
		static bool isGrayscale(const Type &image_type);
		static Type getTypeFromSettings(bool has_alpha, bool grayscale);

		size_t id_{0};
};

} //namespace yafaray

#endif // LIBYAFARAY_IMAGE_H
