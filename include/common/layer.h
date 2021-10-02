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

#include "yafaray_common.h"
#include "common/collection.h"
#include "image/image.h"
#include "common/flags.h"
#include <set>
#include <map>
#include <string>
#include <vector>
#include <array>

BEGIN_YAFARAY

class Layer final
{
	public:
		enum Type : int
		{
			Disabled = -1,
			Combined = 0,
			ZDepthNorm,
			ZDepthAbs,
			NormalSmooth,
			NormalGeom,
			Uv,
			BarycentricUvw,
			Radiance,
			Emit,
			Diffuse,
			DiffuseNoShadow,
			Ao,
			AoClay,
			Env,
			Mist,
			Indirect,
			IndirectAll,
			Shadow,
			ReflectPerfect,
			RefractPerfect,
			ReflectAll,
			RefractAll,
			ObjIndexAbs,
			ObjIndexNorm,
			ObjIndexAuto,
			ObjIndexAutoAbs,
			MatIndexAbs,
			MatIndexNorm,
			MatIndexAuto,
			MatIndexAutoAbs,
			ObjIndexMask,
			ObjIndexMaskShadow,
			ObjIndexMaskAll,
			MatIndexMask,
			MatIndexMaskShadow,
			MatIndexMaskAll,
			DiffuseIndirect,
			DiffuseColor,
			Glossy,
			GlossyIndirect,
			GlossyColor,
			Trans,
			TransIndirect,
			TransColor,
			Subsurface,
			SubsurfaceIndirect,
			SubsurfaceColor,
			SurfaceIntegration,
			VolumeIntegration,
			VolumeTransmittance,
			DebugNu,
			DebugNv,
			DebugDpdu,
			DebugDpdv,
			DebugDsdu,
			DebugDsdv,
			AaSamples,
			DebugLightEstimationLightDirac,
			DebugLightEstimationLightSampling,
			DebugLightEstimationMatSampling,
			DebugWireframe,
			DebugFacesEdges,
			DebugObjectsEdges,
			Toon,
			DebugSamplingFactor,
			DebugDpLengths,
			DebugDpdx,
			DebugDpdy,
			DebugDpdxy,
			DebugDudxDvdx,
			DebugDudyDvdy,
			DebugDudxyDvdxy,
		};
		Layer() = default;
		Layer(const Type &type, const Image::Type &image_type = Image::Type::None, const Image::Type &exported_image_type = Image::Type::None, const std::string &exported_image_name = "");
		Layer(const std::string &type_name, const std::string &image_type_name = "", const std::string &exported_image_type_name = "", const std::string &exported_image_name = "");
		struct Flags : public yafaray::Flags
		{
			Flags() = default;
			Flags(unsigned int flags) : yafaray::Flags(flags) { }
			enum Enum : unsigned int { None = 0, BasicLayers = 1 << 0, DepthLayers = 1 << 1, DiffuseLayers = 1 << 2, IndexLayers = 1 << 3, DebugLayers = 1 << 4, };
		} flags_; //!< Flags to group layers and improve runtime performance
		Type getType() const { return type_; }
		std::string getTypeName() const { return getTypeName(type_); }
		int getNumExportedChannels() const { return Image::getNumChannels(exported_image_type_); }
		bool hasInternalImage() const { return (image_type_ != Image::Type::None); }
		bool isExported() const { return (exported_image_type_ != Image::Type::None); }
		Image::Type getImageType() const { return image_type_; }
		std::string getImageTypeName() const { return Image::getTypeNameLong(image_type_); }
		Image::Type getExportedImageType() const { return exported_image_type_; }
		std::string getExportedImageTypeNameLong() const { return Image::getTypeNameLong(exported_image_type_); }
		std::string getExportedImageTypeNameShort() const { return Image::getTypeNameShort(exported_image_type_); }
		std::string getExportedImageName() const { return exported_image_name_; }
		Flags getFlags() const { return getFlags(type_); }
		std::string print() const;

		void setType(const Type &type) { type_ = type; }
		void setImageType(const Image::Type &image_type) { image_type_ = image_type; }
		void setExportedImageType(const Image::Type &exported_image_type) { exported_image_type_ = exported_image_type; }
		void setExportedImageName(const std::string &exported_image_name) { exported_image_name_ = exported_image_name; }

		static std::string getTypeName(const Type &type);
		static Type getType(const std::string &type_name);
		static bool applyColorSpace(const Type &type);
		static Rgba getDefaultColor(const Type &type);
		static Image::Type getDefaultImageType(const Type &type);
		static Flags getFlags(const Type &type);
		static const std::map<Type, std::string> &getDictTypeTypeName() { return dict_type_typename_; }

	private:
		static std::map<std::string, Type> initDictTypeNamesTypes();
		static std::map<Type, std::string> initDictTypeTypeNames(const std::map<std::string, Type> &dict_typename_type);

		Type type_ = Disabled;
		Image::Type image_type_ = Image::Type::None;
		Image::Type exported_image_type_ = Image::Type::None;
		std::string exported_image_name_;

		static const std::map<std::string, Type> dict_typename_type_; //!Dictionary name->layer
		static const std::map<Type, std::string> dict_type_typename_; //!Dictionary layer->name
};

END_YAFARAY

#endif // YAFARAY_LAYER_H
