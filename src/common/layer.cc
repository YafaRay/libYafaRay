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

#include "common/layers.h"
#include "color/color.h"
#include "common/param.h"
#include "common/logger.h"
#include <sstream>

BEGIN_YAFARAY

const std::map<std::string, Layer::Type> Layer::dict_typename_type_ = initDictTypeNamesTypes();
const std::map<Layer::Type, std::string> Layer::dict_type_typename_ = initDictTypeTypeNames(dict_typename_type_);

Layer::Layer(const std::string &type_name, const std::string &image_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name) : Layer(getType(type_name), Image::getTypeFromName(image_type_name), Image::getTypeFromName(exported_image_type_name), exported_image_name)
{
	//EMPTY
}

Layer::Layer(const Type &type, Image::Type image_type, Image::Type exported_image_type, const std::string &exported_image_name) : type_(type), image_type_(image_type), exported_image_type_(exported_image_type), exported_image_name_(exported_image_name)
{
	//EMPTY
}

std::map<std::string, Layer::Type> Layer::initDictTypeNamesTypes()
{
	std::map<std::string, Layer::Type> dict_typename_type;

	dict_typename_type["combined"] = Layer::Combined;
	dict_typename_type["z-depth-norm"] = Layer::ZDepthNorm;
	dict_typename_type["z-depth-abs"] = Layer::ZDepthAbs;
	dict_typename_type["debug-normal-smooth"] = Layer::NormalSmooth;
	dict_typename_type["debug-normal-geom"] = Layer::NormalGeom;
	dict_typename_type["adv-radiance"] = Layer::Radiance;
	dict_typename_type["debug-uv"] = Layer::Uv;
	dict_typename_type["debug-barycentric-uvw"] = Layer::BarycentricUvw;
	dict_typename_type["emit"] = Layer::Emit;
	dict_typename_type["mist"] = Layer::Mist;
	dict_typename_type["diffuse"] = Layer::Diffuse;
	dict_typename_type["diffuse-noshadow"] = Layer::DiffuseNoShadow;
	dict_typename_type["ao"] = Layer::Ao;
	dict_typename_type["ao-clay"] = Layer::AoClay;
	dict_typename_type["env"] = Layer::Env;
	dict_typename_type["indirect"] = Layer::IndirectAll;
	dict_typename_type["adv-indirect"] = Layer::Indirect;
	dict_typename_type["shadow"] = Layer::Shadow;
	dict_typename_type["reflect"] = Layer::ReflectAll;
	dict_typename_type["refract"] = Layer::RefractAll;
	dict_typename_type["adv-reflect"] = Layer::ReflectPerfect;
	dict_typename_type["adv-refract"] = Layer::RefractPerfect;
	dict_typename_type["obj-index-abs"] = Layer::ObjIndexAbs;
	dict_typename_type["obj-index-norm"] = Layer::ObjIndexNorm;
	dict_typename_type["obj-index-auto"] = Layer::ObjIndexAuto;
	dict_typename_type["obj-index-auto-abs"] = Layer::ObjIndexAutoAbs;
	dict_typename_type["obj-index-mask"] = Layer::ObjIndexMask;
	dict_typename_type["obj-index-mask-shadow"] = Layer::ObjIndexMaskShadow;
	dict_typename_type["obj-index-mask-all"] = Layer::ObjIndexMaskAll;
	dict_typename_type["mat-index-abs"] = Layer::MatIndexAbs;
	dict_typename_type["mat-index-norm"] = Layer::MatIndexNorm;
	dict_typename_type["mat-index-auto"] = Layer::MatIndexAuto;
	dict_typename_type["mat-index-auto-abs"] = Layer::MatIndexAutoAbs;
	dict_typename_type["mat-index-mask"] = Layer::MatIndexMask;
	dict_typename_type["mat-index-mask-shadow"] = Layer::MatIndexMaskShadow;
	dict_typename_type["mat-index-mask-all"] = Layer::MatIndexMaskAll;
	dict_typename_type["adv-diffuse-indirect"] = Layer::DiffuseIndirect;
	dict_typename_type["adv-diffuse-color"] = Layer::DiffuseColor;
	dict_typename_type["adv-glossy"] = Layer::Glossy;
	dict_typename_type["adv-glossy-indirect"] = Layer::GlossyIndirect;
	dict_typename_type["adv-glossy-color"] = Layer::GlossyColor;
	dict_typename_type["adv-trans"] = Layer::Trans;
	dict_typename_type["adv-trans-indirect"] = Layer::TransIndirect;
	dict_typename_type["adv-trans-color"] = Layer::TransColor;
	dict_typename_type["adv-subsurface"] = Layer::Subsurface;
	dict_typename_type["adv-subsurface-indirect"] = Layer::SubsurfaceIndirect;
	dict_typename_type["adv-subsurface-color"] = Layer::SubsurfaceColor;
	dict_typename_type["debug-normal-smooth"] = Layer::NormalSmooth;
	dict_typename_type["debug-normal-geom"] = Layer::NormalGeom;
	dict_typename_type["debug-nu"] = Layer::DebugNu;
	dict_typename_type["debug-nv"] = Layer::DebugNv;
	dict_typename_type["debug-dpdu"] = Layer::DebugDpdu;
	dict_typename_type["debug-dpdv"] = Layer::DebugDpdv;
	dict_typename_type["debug-dsdu"] = Layer::DebugDsdu;
	dict_typename_type["debug-dsdv"] = Layer::DebugDsdv;
	dict_typename_type["adv-surface-integration"] = Layer::SurfaceIntegration;
	dict_typename_type["adv-volume-integration"] = Layer::VolumeIntegration;
	dict_typename_type["adv-volume-transmittance"] = Layer::VolumeTransmittance;
	dict_typename_type["debug-aa-samples"] = Layer::AaSamples;
	dict_typename_type["debug-light-estimation-light-dirac"] = Layer::DebugLightEstimationLightDirac;
	dict_typename_type["debug-light-estimation-light-sampling"] = Layer::DebugLightEstimationLightSampling;
	dict_typename_type["debug-light-estimation-mat-sampling"] = Layer::DebugLightEstimationMatSampling;
	dict_typename_type["debug-wireframe"] = Layer::DebugWireframe;
	dict_typename_type["debug-faces-edges"] = Layer::DebugFacesEdges;
	dict_typename_type["debug-objects-edges"] = Layer::DebugObjectsEdges;
	dict_typename_type["toon"] = Layer::Toon;
	dict_typename_type["debug-sampling-factor"] = Layer::DebugSamplingFactor;
	dict_typename_type["debug-dp-lengths"] = Layer::DebugDpLengths;
	dict_typename_type["debug-dpdx"] = Layer::DebugDpdx;
	dict_typename_type["debug-dpdy"] = Layer::DebugDpdy;
	dict_typename_type["debug-dpdxy"] = Layer::DebugDpdxy;
	dict_typename_type["debug-dudx-dvdx"] = Layer::DebugDudxDvdx;
	dict_typename_type["debug-dudy-dvdy"] = Layer::DebugDudyDvdy;
	dict_typename_type["debug-dudxy-dvdxy"] = Layer::DebugDudxyDvdxy;
	return dict_typename_type;
}

std::map<Layer::Type, std::string> Layer::initDictTypeTypeNames(const std::map<std::string, Layer::Type> &dict_typename_type)
{
	std::map<Layer::Type, std::string> dict_type_typename;
	for(const auto &typename_type : dict_typename_type)
	{
		dict_type_typename[typename_type.second] = typename_type.first;
	}
	return dict_type_typename;
}

Rgba Layer::getDefaultColor(const Type &type)
{
	switch(type) //Default initialization color in general is black/opaque, except for SHADOW and MASK passes where the default is black/transparent for easier masking
	{
		case DebugWireframe:
		case Shadow:
		case ObjIndexMask:
		case ObjIndexMaskShadow:
		case ObjIndexMaskAll:
		case MatIndexMask:
		case MatIndexMaskShadow:
		case MatIndexMaskAll: return Rgba(0.f, 0.f, 0.f, 0.f);
		default: return Rgba(0.f, 0.f, 0.f, 1.f);
	}
}

Image::Type Layer::getDefaultImageType(const Type &type)
{
	switch(type) //Default image type for images created to render each layer type
	{
		case ZDepthNorm:
		case ZDepthAbs:
		case Mist:
		case Shadow: return Image::Type::GrayAlpha;

		case AoClay:
		case ObjIndexAbs:
		case ObjIndexNorm:
		case MatIndexAbs:
		case MatIndexNorm:
		case VolumeTransmittance: return Image::Type::Gray;

		case ObjIndexMask:
		case ObjIndexMaskShadow:
		case ObjIndexMaskAll:
		case MatIndexMask:
		case MatIndexMaskShadow:
		case MatIndexMaskAll:
		case DebugWireframe:
		case Combined: return Image::Type::ColorAlpha;

		default: return Image::Type::Color;
	}
}

std::string Layer::getTypeName(const Type &type)
{
	if(type == Disabled) return "disabled";
	const auto it = dict_type_typename_.find(type);
	if(it == dict_type_typename_.end()) return "unknown";
	else return it->second;
}

Layer::Type Layer::getType(const std::string &name)
{
	if(name == "disabled" || name == "unknown" || name.empty()) return Disabled;
	const auto it = dict_typename_type_.find(name);
	if(it == dict_typename_type_.end()) return Disabled;
	else return it->second;
}

bool Layer::applyColorSpace(const Type &type)
{
	switch(type)
	{
		case Disabled:
		case ZDepthAbs:
		case ObjIndexAbs:
		case ObjIndexAutoAbs:
		case MatIndexAbs:
		case MatIndexAutoAbs:
		case AaSamples:
		case DebugSamplingFactor: return false;
		default: return true;
	}
}

Layer::Flags Layer::getFlags(const Type &type)
{
	switch(type) //Default image type for images created to render each layer type
	{
		case Disabled:
		case Combined:
		case AaSamples: return Layer::Flags::None;

		case ZDepthNorm:
		case ZDepthAbs:
		case Mist: return Layer::Flags::DepthLayers;

		case Diffuse:
		case DiffuseNoShadow:
		case DiffuseIndirect:
		case DiffuseColor: return Layer::Flags::BasicLayers | Layer::Flags::DiffuseLayers;

		case ObjIndexAbs:
		case ObjIndexNorm:
		case ObjIndexAuto:
		case ObjIndexAutoAbs:
		case MatIndexAbs:
		case MatIndexNorm:
		case MatIndexAuto:
		case MatIndexAutoAbs:
		case ObjIndexMask:
		case ObjIndexMaskShadow:
		case ObjIndexMaskAll:
		case MatIndexMask:
		case MatIndexMaskShadow:
		case MatIndexMaskAll: return Layer::Flags::IndexLayers;

		case NormalSmooth:
		case NormalGeom:
		case Uv:
		case BarycentricUvw:
		case DebugNu:
		case DebugNv:
		case DebugDpdu:
		case DebugDpdv:
		case DebugDsdu:
		case DebugDsdv:
		case DebugLightEstimationLightDirac:
		case DebugLightEstimationLightSampling:
		case DebugLightEstimationMatSampling:
		case DebugWireframe:
		case DebugFacesEdges:
		case DebugObjectsEdges:
		case DebugSamplingFactor:
		case DebugDpLengths:
		case DebugDpdx:
		case DebugDpdy:
		case DebugDpdxy:
		case DebugDudxDvdx:
		case DebugDudyDvdy:
		case DebugDudxyDvdxy: return Layer::Flags::DebugLayers;

		default: return Layer::Flags::BasicLayers;
	}
}

Rgba Layer::postProcess(const Rgba &color, Layer::Type layer_type, ColorSpace color_space, float gamma, bool alpha_premultiply)
{
	Rgba result{color};
	result.clampRgb0();
	if(Layer::applyColorSpace(layer_type)) result.colorSpaceFromLinearRgb(color_space, gamma);
	if(alpha_premultiply) result.alphaPremultiply();

	//To make sure we don't have any weird Alpha values outside the range [0.f, +1.f]
	if(result.a_ < 0.f) result.a_ = 0.f;
	else if(result.a_ > 1.f) result.a_ = 1.f;

	return result;
}

std::string Layer::print() const
{
	std::stringstream ss;
	ss << "Layer '" << getTypeName() << (isExported() ? "' (exported)" : "' (internal)");
	if(hasInternalImage()) ss << " with internal image type: '" << getImageTypeName() << "'";
	if(isExported())
	{
		ss << " with exported image type: '" << getExportedImageTypeNameLong() << "'";
		if(!getExportedImageName().empty()) ss << " and name: '" << getExportedImageName() << "'";
	}
	return ss.str();
}

END_YAFARAY
