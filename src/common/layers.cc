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

BEGIN_YAFARAY

std::map<Layer::Type, std::string> Layer::map_type_typename_ { };
std::map<std::string, Layer::Type> Layer::map_typename_type_ { };

Layer::Layer(const std::string &type_name, const std::string &image_type_name, const std::string &exported_image_type_name) : Layer(getType(type_name), Image::getTypeFromName(image_type_name), Image::getTypeFromName(exported_image_type_name)) { }

Layer::Layer(const Type &type, const Image::Type &image_type, const Image::Type &exported_image_type) : type_(type), image_type_(image_type), exported_image_type_(exported_image_type)
{
	//The "type-typename" translation maps are static and global to the class, only to be initialized once
	//Only initialize if one of the maps is empty
	if(map_typename_type_.empty() || map_type_typename_.empty())
	{
		map_typename_type_.clear();
		map_type_typename_.clear();

		map_typename_type_["combined"] = Combined;
		map_typename_type_["z-depth-norm"] = ZDepthNorm;
		map_typename_type_["z-depth-abs"] = ZDepthAbs;
		map_typename_type_["debug-normal-smooth"] = NormalSmooth;
		map_typename_type_["debug-normal-geom"] = NormalGeom;
		map_typename_type_["adv-radiance"] = Radiance;
		map_typename_type_["debug-uv"] = Uv;
		map_typename_type_["emit"] = Emit;
		map_typename_type_["mist"] = Mist;
		map_typename_type_["diffuse"] = Diffuse;
		map_typename_type_["diffuse-noshadow"] = DiffuseNoShadow;
		map_typename_type_["ao"] = Ao;
		map_typename_type_["ao-clay"] = AoClay;
		map_typename_type_["env"] = Env;
		map_typename_type_["indirect"] = IndirectAll;
		map_typename_type_["adv-indirect"] = Indirect;
		map_typename_type_["shadow"] = Shadow;
		map_typename_type_["reflect"] = ReflectAll;
		map_typename_type_["refract"] = RefractAll;
		map_typename_type_["adv-reflect"] = ReflectPerfect;
		map_typename_type_["adv-refract"] = RefractPerfect;
		map_typename_type_["obj-index-abs"] = ObjIndexAbs;
		map_typename_type_["obj-index-norm"] = ObjIndexNorm;
		map_typename_type_["obj-index-auto"] = ObjIndexAuto;
		map_typename_type_["obj-index-auto-abs"] = ObjIndexAutoAbs;
		map_typename_type_["obj-index-mask"] = ObjIndexMask;
		map_typename_type_["obj-index-mask-shadow"] = ObjIndexMaskShadow;
		map_typename_type_["obj-index-mask-all"] = ObjIndexMaskAll;
		map_typename_type_["mat-index-abs"] = MatIndexAbs;
		map_typename_type_["mat-index-norm"] = MatIndexNorm;
		map_typename_type_["mat-index-auto"] = MatIndexAuto;
		map_typename_type_["mat-index-auto-abs"] = MatIndexAutoAbs;
		map_typename_type_["mat-index-mask"] = MatIndexMask;
		map_typename_type_["mat-index-mask-shadow"] = MatIndexMaskShadow;
		map_typename_type_["mat-index-mask-all"] = MatIndexMaskAll;
		map_typename_type_["adv-diffuse-indirect"] = DiffuseIndirect;
		map_typename_type_["adv-diffuse-color"] = DiffuseColor;
		map_typename_type_["adv-glossy"] = Glossy;
		map_typename_type_["adv-glossy-indirect"] = GlossyIndirect;
		map_typename_type_["adv-glossy-color"] = GlossyColor;
		map_typename_type_["adv-trans"] = Trans;
		map_typename_type_["adv-trans-indirect"] = TransIndirect;
		map_typename_type_["adv-trans-color"] = TransColor;
		map_typename_type_["adv-subsurface"] = Subsurface;
		map_typename_type_["adv-subsurface-indirect"] = SubsurfaceIndirect;
		map_typename_type_["adv-subsurface-color"] = SubsurfaceColor;
		map_typename_type_["debug-normal-smooth"] = NormalSmooth;
		map_typename_type_["debug-normal-geom"] = NormalGeom;
		map_typename_type_["debug-nu"] = DebugNu;
		map_typename_type_["debug-nv"] = DebugNv;
		map_typename_type_["debug-dpdu"] = DebugDpdu;
		map_typename_type_["debug-dpdv"] = DebugDpdv;
		map_typename_type_["debug-dsdu"] = DebugDsdu;
		map_typename_type_["debug-dsdv"] = DebugDsdv;
		map_typename_type_["adv-surface-integration"] = SurfaceIntegration;
		map_typename_type_["adv-volume-integration"] = VolumeIntegration;
		map_typename_type_["adv-volume-transmittance"] = VolumeTransmittance;
		map_typename_type_["debug-aa-samples"] = AaSamples;
		map_typename_type_["debug-light-estimation-light-dirac"] = DebugLightEstimationLightDirac;
		map_typename_type_["debug-light-estimation-light-sampling"] = DebugLightEstimationLightSampling;
		map_typename_type_["debug-light-estimation-mat-sampling"] = DebugLightEstimationMatSampling;
		map_typename_type_["debug-wireframe"] = DebugWireframe;
		map_typename_type_["debug-faces-edges"] = DebugFacesEdges;
		map_typename_type_["debug-objects-edges"] = DebugObjectsEdges;
		map_typename_type_["toon"] = Toon;
		map_typename_type_["debug-sampling-factor"] = DebugSamplingFactor;
		map_typename_type_["debug-dp-lengths"] = DebugDpLengths;
		map_typename_type_["debug-dpdx"] = DebugDpdx;
		map_typename_type_["debug-dpdy"] = DebugDpdy;
		map_typename_type_["debug-dpdxy"] = DebugDpdxy;
		map_typename_type_["debug-dudx-dvdx"] = DebugDudxDvdx;
		map_typename_type_["debug-dudy-dvdy"] = DebugDudyDvdy;
		map_typename_type_["debug-dudxy-dvdxy"] = DebugDudxyDvdxy;

		//Generation of reverse map (pass layer -> pass_string)
		for(const auto &it : map_typename_type_)
		{
			map_type_typename_[it.second] = it.first;
		}
	}
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
		case AoClay:
		case Mist:
		case Shadow:
		case ObjIndexAbs:
		case ObjIndexNorm:
		case MatIndexAbs:
		case MatIndexNorm:
		case VolumeTransmittance: return Image::Type::Gray;

		case Combined:
		case ObjIndexMask:
		case ObjIndexMaskShadow:
		case ObjIndexMaskAll:
		case MatIndexMask:
		case MatIndexMaskShadow:
		case MatIndexMaskAll: return Image::Type::ColorAlpha;

		default: return Image::Type::Color;
	}
}

std::string Layer::getTypeName(const Type &type)
{
	if(type == Disabled) return "disabled";
	auto it = map_type_typename_.find(type);
	if(it == map_type_typename_.end()) return "unknown";
	else return it->second;
}

Layer::Type Layer::getType(const std::string &name)
{
	if(name == "disabled" || name == "unknown" || name.empty()) return Disabled;
	auto it = map_typename_type_.find(name);
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

Layers::Layers()
{
	defined_bool_ = std::vector<bool>(listAvailable().size(), false);
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
