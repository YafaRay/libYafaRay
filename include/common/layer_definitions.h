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

#ifndef YAFARAY_LAYER_DEFINITIONS_H
#define YAFARAY_LAYER_DEFINITIONS_H

#include "image/image.h"
#include <string>
#include <array>
#include <map>

namespace yafaray {

class LayerDef final
{
	public:
		struct Flags : Enum<Flags, unsigned short>
		{
			using Enum::Enum;
			enum : ValueType_t { None = 0, BasicLayers = 1 << 0, DepthLayers = 1 << 1, DiffuseLayers = 1 << 2, IndexLayers = 1 << 3, DebugLayers = 1 << 4, AoLayers = 1 << 5, ToonEdgeLayers = 1 << 6 };
			inline static const EnumMap<ValueType_t> map_{{
					{"None", None, ""},
					{"BasicLayers", BasicLayers, ""},
					{"DepthLayers", DepthLayers, ""},
					{"DiffuseLayers", DiffuseLayers, ""},
					{"IndexLayers", IndexLayers, ""},
					{"DebugLayers", DebugLayers, ""},
					{"AoLayers", AoLayers, ""},
					{"ToonEdgeLayers", ToonEdgeLayers, ""},
				}};
		};
		enum Type : unsigned char
		{
			Combined = 0, //Combined should always have value 0 and be the first entry in the enum
			AaSamples,
			Ao,
			AoClay,
			BarycentricUvw,
			DebugDpLengths,
			DebugDpdu,
			DebugDpdv,
			DebugDpdx,
			DebugDpdxy,
			DebugDpdy,
			DebugDsdu,
			DebugDsdv,
			DebugDudxDvdx,
			DebugDudxyDvdxy,
			DebugDudyDvdy,
			DebugFacesEdges,
			DebugLightEstimationLightDirac,
			DebugLightEstimationLightSampling,
			DebugLightEstimationMatSampling,
			DebugNu,
			DebugNv,
			DebugObjectsEdges,
			DebugSamplingFactor,
			DebugWireframe,
			DebugObjectTime,
			Diffuse,
			DiffuseColor,
			DiffuseIndirect,
			DiffuseNoShadow,
			Disabled,
			Emit,
			Env,
			Glossy,
			GlossyColor,
			GlossyIndirect,
			Indirect,
			IndirectAll,
			MatIndexAbs,
			MatIndexAuto,
			MatIndexAutoAbs,
			MatIndexMask,
			MatIndexMaskAll,
			MatIndexMaskShadow,
			MatIndexNorm,
			Mist,
			NormalGeom,
			NormalSmooth,
			ObjIndexAbs,
			ObjIndexAuto,
			ObjIndexAutoAbs,
			ObjIndexMask,
			ObjIndexMaskAll,
			ObjIndexMaskShadow,
			ObjIndexNorm,
			Radiance,
			ReflectAll,
			ReflectPerfect,
			RefractAll,
			RefractPerfect,
			Shadow,
			Subsurface,
			SubsurfaceColor,
			SubsurfaceIndirect,
			SurfaceIntegration,
			Toon,
			Trans,
			TransColor,
			TransIndirect,
			Uv,
			VolumeIntegration,
			VolumeTransmittance,
			ZDepthAbs,
			ZDepthNorm,
			Size //Size should always be the last entry in the enum!
		};
		LayerDef(Type type, std::string name, Flags flags, Image::Type default_image_type = Image::Type{Image::Type::Color}, const Rgba &default_color = {0.f, 1.f}, bool apply_color_space = true) : type_(type), flags_(flags), name_(std::move(name)), default_color_(default_color), apply_color_space_(apply_color_space), default_image_type_(default_image_type) { }
		Type getType() const { return type_; }
		Flags getFlags() const { return flags_; }
		const std::string &getName() const { return name_; }
		const Rgba &getDefaultColor() const { return default_color_; }
		bool applyColorSpace() const { return apply_color_space_; }
		Image::Type getDefaultImageType() const { return default_image_type_; }
		static Type getType(const std::string &name);
		static Flags getFlags(Type type) { return definitions_array_[type].getFlags(); }
		static const std::string &getName(Type type) { return definitions_array_[type].getName(); }
		static const Rgba &getDefaultColor(Type type) { return definitions_array_[type].getDefaultColor(); }
		static bool applyColorSpace(Type type) { return definitions_array_[type].applyColorSpace(); }
		static Image::Type getDefaultImageType(Type type) { return definitions_array_[type].getDefaultImageType(); }

	private:
		static const LayerDef &getLayerDefinition(Type type) { return definitions_array_[type]; }
		static std::map<std::string, Type> initLayerNameMap();
		Type type_ = Type::Disabled;
		Flags flags_{Flags::None};
		std::string name_ = "disabled";
		Rgba default_color_{0.f, 1.f};
		bool apply_color_space_ = true;
		Image::Type default_image_type_{Image::Type::None};
		static const std::array<LayerDef, LayerDef::Type::Size> definitions_array_;
		static const std::map<std::string, Type> layer_name_map_; //!Dictionary name->layer
};

} //namespace yafaray

#endif //LIBYAFARAY_LAYER_DEFINITIONS_H
