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

#ifndef YAFARAY_LAYERS_H
#define YAFARAY_LAYERS_H

#include "constants.h"
#include "common/collection.h"
#include "image/image.h"
#include <set>
#include <map>
#include <string>
#include <vector>

BEGIN_YAFARAY

class Rgba;

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
		Layer(const Type &type, const Image::Type &image_type = Image::Type::None, const Image::Type &exported_image_type = Image::Type::None);
		Layer(const std::string &type_name, const std::string &image_type_name = "", const std::string &exported_image_type_name = "");
		Type getType() const { return type_; }
		std::string getTypeName() const { return getTypeName(type_); }
		int getNumExportedChannels() const { return Image::getNumChannels(exported_image_type_); }
		bool hasInternalImage() const { return (image_type_ != Image::Type::None); }
		bool isExported() const { return (exported_image_type_ != Image::Type::None); }
		Image::Type getImageType() const { return image_type_; }
		std::string getImageTypeName() const { return Image::getTypeName(image_type_); }
		Image::Type getExportedImageType() const { return exported_image_type_; }
		std::string getExportedImageTypeName() const { return Image::getTypeName(exported_image_type_); }

		void setType(const Type &type) { type_ = type; }
		void setImageType(const Image::Type &image_type) { image_type_ = image_type; }
		void setExportedImageType(const Image::Type &exported_image_type) { exported_image_type_ = exported_image_type; }

		static std::string getTypeName(const Type &type);
		static Type getType(const std::string &type_name);
		static bool applyColorSpace(const Type &type);
		static Rgba getDefaultColor(const Type &type);
		static Image::Type getDefaultImageType(const Type &type);
		static const std::map<Type, std::string> &getMapTypeTypeName() { return map_type_typename_; }

	private:
		Type type_ = Disabled;
		Image::Type image_type_ = Image::Type::None;
		Image::Type exported_image_type_ = Image::Type::None;

		static std::map<Type, std::string> map_type_typename_; //!Dictionary available layers layer->name
		static std::map<std::string, Type> map_typename_type_; //!Reverse dictionary name->layer
};

struct MaskParams
{
	float obj_index_ = 0.f; //!Object Index used for masking in/out in the Mask Render Layers
	float mat_index_ = 0.f; //!Material Index used for masking in/out in the Mask Render Layers
	bool invert_ = false; //!False=mask in, True=mask out
	bool only_ = false; //!False=rendered image is masked, True=only the mask is shown without rendered image
};

struct EdgeToonParams //Options for Edge detection and Toon Render Layers
{
	int thickness_ = 2; //!Thickness of the edges used in the Object Edge and Toon Render Layers
	float threshold_ = 0.3f; //!Threshold for the edge detection process used in the Object Edge and Toon Render Layers
	float smoothness_ = 0.75f; //!Smoothness (blur) of the edges used in the Object Edge and Toon Render Layers
	std::array<float, 3> toon_color_ = {0.f, 0.f, 0.f}; //!Color of the edges used in the Toon Render Layers.
	//Using array<float, 3> to avoid including color.h header dependency
	float toon_pre_smooth_ = 3.f; //!Toon effect: smoothness applied to the original image
	float toon_quantization_ = 0.1f; //!Toon effect: color Quantization applied to the original image
	float toon_post_smooth_ = 3.f; //!Toon effect: smoothness applied after Quantization
	int face_thickness_ = 1; //!Thickness of the edges used in the Faces Edge Render Layers
	float face_threshold_ = 0.01f; //!Threshold for the edge detection process used in the Faces Edge Render Layers
	float face_smoothness_ = 0.5f; //!Smoothness (blur) of the edges used in the Faces Edge Render Layers
};

	class Layers final : public Collection<Layer::Type, Layer>
{
	public:
		bool isDefined(const Layer::Type &type) const;
		bool isDefinedAny(const std::vector<Layer::Type> &types) const;
		Layer::Type highestDefined() const { if(!items_.empty()) return items_.rbegin()->first; else return Layer::Combined; };
		const Layers getLayersWithImages() const;
		const Layers getLayersWithExportedImages() const;

		const MaskParams &getMaskParams() const { return mask_params_; }
		void setMaskParams(const MaskParams &mask_params) { mask_params_ = mask_params; }
		const EdgeToonParams &getEdgeToonParams() const { return edge_toon_params_; }
		void setEdgeToonParams(const EdgeToonParams &edge_toon_params) { edge_toon_params_ = edge_toon_params; }

		static const std::map<Layer::Type, std::string> &listAvailable() { return Layer::getMapTypeTypeName(); }

	private:
		MaskParams mask_params_;
		EdgeToonParams edge_toon_params_;
};

inline bool Layers::isDefined(const Layer::Type &type) const
{
	if(type == Layer::Combined) return true;
	else if(type == Layer::Disabled) return false;
	else return find(type);
}

END_YAFARAY

#endif // YAFARAY_LAYERS_H
