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
 *
 */

#ifndef YAFARAY_LAYER_H
#define YAFARAY_LAYER_H

#include "common/collection.h"
#include "image/image.h"
#include "common/layer_definitions.h"
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <array>

namespace yafaray {

class Layer final
{
	public:
		Layer() = default;
		explicit Layer(LayerDef::Type type, Image::Type image_type = Image::Type::None, Image::Type exported_image_type = Image::Type::None, std::string exported_image_name = "") : type_(type), image_type_(image_type), exported_image_type_(exported_image_type), exported_image_name_(std::move(exported_image_name)) { }
		explicit Layer(const std::string &type_name, const std::string &image_type_name = "", const std::string &exported_image_type_name = "", const std::string &exported_image_name = "") : Layer(LayerDef::getType(type_name), Image::getTypeFromName(image_type_name), Image::getTypeFromName(exported_image_type_name), exported_image_name) { }
		LayerDef::Type getType() const { return type_; }
		std::string getTypeName() const { return LayerDef::getName(type_); }
		int getNumExportedChannels() const { return Image::getNumChannels(exported_image_type_); }
		bool hasInternalImage() const { return (image_type_ != Image::Type::None); }
		bool isExported() const { return (exported_image_type_ != Image::Type::None); }
		Image::Type getImageType() const { return image_type_; }
		std::string getImageTypeName() const { return Image::getTypeNameLong(image_type_); }
		Image::Type getExportedImageType() const { return exported_image_type_; }
		std::string getExportedImageTypeNameLong() const { return Image::getTypeNameLong(exported_image_type_); }
		std::string getExportedImageTypeNameShort() const { return Image::getTypeNameShort(exported_image_type_); }
		std::string getExportedImageName() const { return exported_image_name_; }
		LayerDef::Flags getFlags() const { return LayerDef::getFlags(type_); }
		std::string print() const;

		void setType(LayerDef::Type type) { type_ = type; }
		void setImageType(Image::Type image_type) { image_type_ = image_type; }
		void setExportedImageType(Image::Type exported_image_type) { exported_image_type_ = exported_image_type; }
		void setExportedImageName(const std::string &exported_image_name) { exported_image_name_ = exported_image_name; }

		static Rgba postProcess(const Rgba &color, LayerDef::Type layer_type, ColorSpace color_space, float gamma, bool alpha_premultiply);

	private:
		LayerDef::Type type_ = LayerDef::Type::Disabled;
		Image::Type image_type_ = Image::Type::None;
		Image::Type exported_image_type_ = Image::Type::None;
		std::string exported_image_name_;
};

} //namespace yafaray

#endif // YAFARAY_LAYER_H
