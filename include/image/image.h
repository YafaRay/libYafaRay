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

#ifndef YAFARAY_IMAGE_H
#define YAFARAY_IMAGE_H

#include "common/yafaray_common.h"
#include "color/color.h"
#include <string>
#include <memory>

BEGIN_YAFARAY

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
		enum class Type : unsigned char { None, Gray, GrayAlpha, Color, ColorAlpha };
		enum class Optimization : unsigned char { None, Optimized, Compressed };
		enum class Position : unsigned char { None, Top, Bottom, Left, Right, Overlay };
		static Image *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static Image *factory(Logger &logger, int width, int height, const Type &type, const Optimization &optimization);
		virtual ~Image() = default;

		virtual Type getType() const = 0;
		virtual Optimization getOptimization() const = 0;
		virtual Rgba getColor(int x, int y) const = 0;
		virtual float getFloat(int x, int y) const = 0;
		virtual void setColor(int x, int y, const Rgba &col) = 0;
		virtual void setColor(int x, int y, Rgba &&col) = 0;
		virtual void addColor(int x, int y, const Rgba &col) = 0;
		virtual void setFloat(int x, int y, float val) = 0;
		virtual void addFloat(int x, int y, float val) = 0;
		virtual void clear() = 0;

		int getWidth() const { return width_; }
		int getHeight() const { return height_; }
		std::string getTypeName() const { return getTypeNameLong(getType()); }
		int getNumChannels() const { return getNumChannels(getType()); }
		bool hasAlpha() const { return hasAlpha(getType()); }
		bool isGrayscale() const { return isGrayscale(getType()); }
		ColorSpace getColorSpace() const { return color_space_; }
		float getGamma() const { return gamma_; }

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

	protected:
		Image(int width, int height) : width_(width), height_(height) { }
		int width_ = 0;
		int height_ = 0;
		float gamma_ = 1.f;
		ColorSpace color_space_ = ColorSpace::RawManualGamma;
};

END_YAFARAY

#endif // YAFARAY_IMAGE_H
