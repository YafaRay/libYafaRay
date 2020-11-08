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

std::map<std::string, Layer::Type> Layer::map_typename_type_ = initMapTypeNamesTypes();
std::map<Layer::Type, std::string> Layer::map_type_typename_ = initMapTypeTypeNames(map_typename_type_);

Layer::Layer(const std::string &type_name, const std::string &image_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name) : Layer(getType(type_name), Image::getTypeFromName(image_type_name), Image::getTypeFromName(exported_image_type_name), exported_image_name)
{
	//EMPTY
}

Layer::Layer(const Type &type, const Image::Type &image_type, const Image::Type &exported_image_type, const std::string &exported_image_name) : type_(type), image_type_(image_type), exported_image_type_(exported_image_type), exported_image_name_(exported_image_name)
{
	//EMPTY
}

std::map<std::string, Layer::Type> Layer::initMapTypeNamesTypes()
{
	std::map<std::string, Layer::Type> map_typename_type;

	map_typename_type["combined"] = Layer::Combined;
	map_typename_type["z-depth-norm"] = Layer::ZDepthNorm;
	map_typename_type["z-depth-abs"] = Layer::ZDepthAbs;
	map_typename_type["debug-normal-smooth"] = Layer::NormalSmooth;
	map_typename_type["debug-normal-geom"] = Layer::NormalGeom;
	map_typename_type["adv-radiance"] = Layer::Radiance;
	map_typename_type["debug-uv"] = Layer::Uv;
	map_typename_type["emit"] = Layer::Emit;
	map_typename_type["mist"] = Layer::Mist;
	map_typename_type["diffuse"] = Layer::Diffuse;
	map_typename_type["diffuse-noshadow"] = Layer::DiffuseNoShadow;
	map_typename_type["ao"] = Layer::Ao;
	map_typename_type["ao-clay"] = Layer::AoClay;
	map_typename_type["env"] = Layer::Env;
	map_typename_type["indirect"] = Layer::IndirectAll;
	map_typename_type["adv-indirect"] = Layer::Indirect;
	map_typename_type["shadow"] = Layer::Shadow;
	map_typename_type["reflect"] = Layer::ReflectAll;
	map_typename_type["refract"] = Layer::RefractAll;
	map_typename_type["adv-reflect"] = Layer::ReflectPerfect;
	map_typename_type["adv-refract"] = Layer::RefractPerfect;
	map_typename_type["obj-index-abs"] = Layer::ObjIndexAbs;
	map_typename_type["obj-index-norm"] = Layer::ObjIndexNorm;
	map_typename_type["obj-index-auto"] = Layer::ObjIndexAuto;
	map_typename_type["obj-index-auto-abs"] = Layer::ObjIndexAutoAbs;
	map_typename_type["obj-index-mask"] = Layer::ObjIndexMask;
	map_typename_type["obj-index-mask-shadow"] = Layer::ObjIndexMaskShadow;
	map_typename_type["obj-index-mask-all"] = Layer::ObjIndexMaskAll;
	map_typename_type["mat-index-abs"] = Layer::MatIndexAbs;
	map_typename_type["mat-index-norm"] = Layer::MatIndexNorm;
	map_typename_type["mat-index-auto"] = Layer::MatIndexAuto;
	map_typename_type["mat-index-auto-abs"] = Layer::MatIndexAutoAbs;
	map_typename_type["mat-index-mask"] = Layer::MatIndexMask;
	map_typename_type["mat-index-mask-shadow"] = Layer::MatIndexMaskShadow;
	map_typename_type["mat-index-mask-all"] = Layer::MatIndexMaskAll;
	map_typename_type["adv-diffuse-indirect"] = Layer::DiffuseIndirect;
	map_typename_type["adv-diffuse-color"] = Layer::DiffuseColor;
	map_typename_type["adv-glossy"] = Layer::Glossy;
	map_typename_type["adv-glossy-indirect"] = Layer::GlossyIndirect;
	map_typename_type["adv-glossy-color"] = Layer::GlossyColor;
	map_typename_type["adv-trans"] = Layer::Trans;
	map_typename_type["adv-trans-indirect"] = Layer::TransIndirect;
	map_typename_type["adv-trans-color"] = Layer::TransColor;
	map_typename_type["adv-subsurface"] = Layer::Subsurface;
	map_typename_type["adv-subsurface-indirect"] = Layer::SubsurfaceIndirect;
	map_typename_type["adv-subsurface-color"] = Layer::SubsurfaceColor;
	map_typename_type["debug-normal-smooth"] = Layer::NormalSmooth;
	map_typename_type["debug-normal-geom"] = Layer::NormalGeom;
	map_typename_type["debug-nu"] = Layer::DebugNu;
	map_typename_type["debug-nv"] = Layer::DebugNv;
	map_typename_type["debug-dpdu"] = Layer::DebugDpdu;
	map_typename_type["debug-dpdv"] = Layer::DebugDpdv;
	map_typename_type["debug-dsdu"] = Layer::DebugDsdu;
	map_typename_type["debug-dsdv"] = Layer::DebugDsdv;
	map_typename_type["adv-surface-integration"] = Layer::SurfaceIntegration;
	map_typename_type["adv-volume-integration"] = Layer::VolumeIntegration;
	map_typename_type["adv-volume-transmittance"] = Layer::VolumeTransmittance;
	map_typename_type["debug-aa-samples"] = Layer::AaSamples;
	map_typename_type["debug-light-estimation-light-dirac"] = Layer::DebugLightEstimationLightDirac;
	map_typename_type["debug-light-estimation-light-sampling"] = Layer::DebugLightEstimationLightSampling;
	map_typename_type["debug-light-estimation-mat-sampling"] = Layer::DebugLightEstimationMatSampling;
	map_typename_type["debug-wireframe"] = Layer::DebugWireframe;
	map_typename_type["debug-faces-edges"] = Layer::DebugFacesEdges;
	map_typename_type["debug-objects-edges"] = Layer::DebugObjectsEdges;
	map_typename_type["toon"] = Layer::Toon;
	map_typename_type["debug-sampling-factor"] = Layer::DebugSamplingFactor;
	map_typename_type["debug-dp-lengths"] = Layer::DebugDpLengths;
	map_typename_type["debug-dpdx"] = Layer::DebugDpdx;
	map_typename_type["debug-dpdy"] = Layer::DebugDpdy;
	map_typename_type["debug-dpdxy"] = Layer::DebugDpdxy;
	map_typename_type["debug-dudx-dvdx"] = Layer::DebugDudxDvdx;
	map_typename_type["debug-dudy-dvdy"] = Layer::DebugDudyDvdy;
	map_typename_type["debug-dudxy-dvdxy"] = Layer::DebugDudxyDvdxy;
	return map_typename_type;
}

std::map<Layer::Type, std::string> Layer::initMapTypeTypeNames(const std::map<std::string, Layer::Type> &map_typename_type)
{
	if(map_typename_type.empty())
	{
		Y_ERROR << "Layer::initMapTypeTypeNames: map_typename_type, cannot initialize layers, exiting..." << YENDL;
		exit(-1);
	}
	std::map<Layer::Type, std::string> map_type_typename;
	for(const auto &it : map_typename_type)
	{
		map_type_typename[it.second] = it.first;
	}
	return map_type_typename;
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
		case MatIndexMaskAll: return Image::Type::ColorAlpha;

		case Combined: return Image::Type::ColorAlphaWeight;

		default: return Image::Type::Color;
	}
}

std::string Layer::getTypeName(const Type &type)
{
	if(type == Disabled) return "disabled";
	const auto it = map_type_typename_.find(type);
	if(it == map_type_typename_.end()) return "unknown";
	else return it->second;
}

Layer::Type Layer::getType(const std::string &name)
{
	if(name == "disabled" || name == "unknown" || name.empty()) return Disabled;
	const auto it = map_typename_type_.find(name);
	if(it == map_typename_type_.end()) return Disabled;
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

std::string Layer::print() const
{
	std::stringstream ss;
	ss << "Layer '" << getTypeName() << (isExported() ? "' (exported)" : "' (internal)");
	if(hasInternalImage()) ss << " with internal image type: '" << getImageTypeName() << "'";
	if(isExported())
	{
		ss << " with exported image type: '" << getExportedImageTypeName() << "'";
		if(!getExportedImageName().empty()) ss << " and name: '" << getExportedImageName() << "'";
	}
	return ss.str();
}

bool Layers::isDefinedAny(const std::vector<Layer::Type> &types) const
{
	for(const auto &type : types)
	{
		if(isDefined(type)) return true;
	}
	return false;
}

const Layers Layers::getLayersWithImages() const
{
	Layers result;
	for(const auto &it : items_)
	{
		if(it.second.hasInternalImage()) result.set(it.first, it.second);
	}
	return result;
}

const Layers Layers::getLayersWithExportedImages() const
{
	Layers result;
	for(const auto &it : items_)
	{
		if(it.second.isExported()) result.set(it.first, it.second);
	}
	return result;
}

END_YAFARAY
