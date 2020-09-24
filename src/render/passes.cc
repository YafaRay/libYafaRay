/****************************************************************************
 *      renderpasses.cc: Render Passes operations
 *      This is part of the libYafaRay package
 *      Copyright (C) 2015  David Bluecame
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

#include "color/color.h"
#include "render/passes.h"
#include "common/logging.h"

BEGIN_YAFARAY

///////////////////////////////
// --  IntPassesSettings  -- //
///////////////////////////////

IntPassesSettings::IntPassesSettings()
{
	map_name_type_["combined"] = PassCombined;
	map_name_type_["z-depth-norm"] = PassZDepthNorm;
	map_name_type_["z-depth-abs"] = PassZDepthAbs;
	map_name_type_["debug-normal-smooth"] = PassNormalSmooth;
	map_name_type_["debug-normal-geom"] = PassNormalGeom;
	map_name_type_["adv-radiance"] = PassRadiance;
	map_name_type_["debug-uv"] = PassUv;
	map_name_type_["emit"] = PassEmit;
	map_name_type_["mist"] = PassMist;
	map_name_type_["diffuse"] = PassDiffuse;
	map_name_type_["diffuse-noshadow"] = PassDiffuseNoShadow;
	map_name_type_["ao"] = PassAo;
	map_name_type_["ao-clay"] = PassAoClay;
	map_name_type_["env"] = PassEnv;
	map_name_type_["indirect"] = PassIndirectAll;
	map_name_type_["adv-indirect"] = PassIndirect;
	map_name_type_["shadow"] = PassShadow;
	map_name_type_["reflect"] = PassReflectAll;
	map_name_type_["refract"] = PassRefractAll;
	map_name_type_["adv-reflect"] = PassReflectPerfect;
	map_name_type_["adv-refract"] = PassRefractPerfect;
	map_name_type_["obj-index-abs"] = PassObjIndexAbs;
	map_name_type_["obj-index-norm"] = PassObjIndexNorm;
	map_name_type_["obj-index-auto"] = PassObjIndexAuto;
	map_name_type_["obj-index-auto-abs"] = PassObjIndexAutoAbs;
	map_name_type_["obj-index-mask"] = PassObjIndexMask;
	map_name_type_["obj-index-mask-shadow"] = PassObjIndexMaskShadow;
	map_name_type_["obj-index-mask-all"] = PassObjIndexMaskAll;
	map_name_type_["mat-index-abs"] = PassMatIndexAbs;
	map_name_type_["mat-index-norm"] = PassMatIndexNorm;
	map_name_type_["mat-index-auto"] = PassMatIndexAuto;
	map_name_type_["mat-index-auto-abs"] = PassMatIndexAutoAbs;
	map_name_type_["mat-index-mask"] = PassMatIndexMask;
	map_name_type_["mat-index-mask-shadow"] = PassMatIndexMaskShadow;
	map_name_type_["mat-index-mask-all"] = PassMatIndexMaskAll;
	map_name_type_["adv-diffuse-indirect"] = PassDiffuseIndirect;
	map_name_type_["adv-diffuse-color"] = PassDiffuseColor;
	map_name_type_["adv-glossy"] = PassGlossy;
	map_name_type_["adv-glossy-indirect"] = PassGlossyIndirect;
	map_name_type_["adv-glossy-color"] = PassGlossyColor;
	map_name_type_["adv-trans"] = PassTrans;
	map_name_type_["adv-trans-indirect"] = PassTransIndirect;
	map_name_type_["adv-trans-color"] = PassTransColor;
	map_name_type_["adv-subsurface"] = PassSubsurface;
	map_name_type_["adv-subsurface-indirect"] = PassSubsurfaceIndirect;
	map_name_type_["adv-subsurface-color"] = PassSubsurfaceColor;
	map_name_type_["debug-normal-smooth"] = PassNormalSmooth;
	map_name_type_["debug-normal-geom"] = PassNormalGeom;
	map_name_type_["debug-nu"] = PassDebugNu;
	map_name_type_["debug-nv"] = PassDebugNv;
	map_name_type_["debug-dpdu"] = PassDebugDpdu;
	map_name_type_["debug-dpdv"] = PassDebugDpdv;
	map_name_type_["debug-dsdu"] = PassDebugDsdu;
	map_name_type_["debug-dsdv"] = PassDebugDsdv;
	map_name_type_["adv-surface-integration"] = PassSurfaceIntegration;
	map_name_type_["adv-volume-integration"] = PassVolumeIntegration;
	map_name_type_["adv-volume-transmittance"] = PassVolumeTransmittance;
	map_name_type_["debug-aa-samples"] = PassAaSamples;
	map_name_type_["debug-light-estimation-light-dirac"] = PassDebugLightEstimationLightDirac;
	map_name_type_["debug-light-estimation-light-sampling"] = PassDebugLightEstimationLightSampling;
	map_name_type_["debug-light-estimation-mat-sampling"] = PassDebugLightEstimationMatSampling;
	map_name_type_["debug-wireframe"] = PassDebugWireframe;
	map_name_type_["debug-faces-edges"] = PassDebugFacesEdges;
	map_name_type_["debug-objects-edges"] = PassDebugObjectsEdges;
	map_name_type_["toon"] = PassToon;
	map_name_type_["debug-sampling-factor"] = PassDebugSamplingFactor;
	map_name_type_["debug-dp-lengths"] = PassDebugDpLengths;
	map_name_type_["debug-dpdx"] = PassDebugDpdx;
	map_name_type_["debug-dpdy"] = PassDebugDpdy;
	map_name_type_["debug-dpdxy"] = PassDebugDpdxy;
	map_name_type_["debug-dudx-dvdx"] = PassDebugDudxDvdx;
	map_name_type_["debug-dudy-dvdy"] = PassDebugDudyDvdy;
	map_name_type_["debug-dudxy-dvdxy"] = PassDebugDudxyDvdxy;

	//Generation of reverse map (pass type -> pass_string)
	for(const auto &it : map_name_type_)
	{
		map_type_name_[it.second] = it.first;
	}

	enabled_bool_ = std::vector<bool>(listAvailable().size(), false);
}

void IntPassesSettings::enable(const IntPassType &type)
{
	enabled_list_.insert(type);
	enabled_bool_.at(type) = true;

	if(type != PassCombined) Y_VERBOSE << "Render Passes: enabled internal pass: \"" << name(type) << "\" [" << (int) type << "]" << YENDL;
}

Rgba IntPassesSettings::defaultColor(const IntPassType &type) const
{
	switch(type) //Default initialization color in general is black/opaque, except for SHADOW and MASK passes where the default is black/transparent for easier masking
	{
		case PassDebugWireframe:
		case PassShadow:
		case PassObjIndexMask:
		case PassObjIndexMaskShadow:
		case PassObjIndexMaskAll:
		case PassMatIndexMask:
		case PassMatIndexMaskShadow:
		case PassMatIndexMaskAll: return Rgba(0.f, 0.f, 0.f, 0.f); break;
		default: return Rgba(0.f, 0.f, 0.f, 1.f); break;
	}
}

std::string IntPassesSettings::name(const IntPassType &type) const
{
	if(type == PassDisabled) return "disabled";
	auto it = map_type_name_.find(type);
	if(it == map_type_name_.end()) return "unknown";
	else return it->second;
}

IntPassType IntPassesSettings::type(const std::string &name) const
{
	if(name == "disabled" || name == "unknown" || name.empty()) return PassDisabled;
	auto it = map_name_type_.find(name);
	if(it == map_name_type_.end()) return PassDisabled;
	else return it->second;
}


///////////////////////////////
// --  ExtPassDefinition  -- //
///////////////////////////////

ImageType ExtPassDefinition::getImageTypeFromName(const std::string &image_type_name)
{
	if(image_type_name == "ColorAlphaWeight") return ImageType::ColorAlphaWeight;
	else if(image_type_name == "ColorAlpha") return ImageType::ColorAlpha;
	else if(image_type_name == "Color") return ImageType::Color;
	else if(image_type_name == "GrayAlphaWeight") return ImageType::GrayAlphaWeight;
	else if(image_type_name == "GrayWeight") return ImageType::GrayWeight;
	else if(image_type_name == "GrayAlpha") return ImageType::GrayAlpha;
	else if(image_type_name == "Gray") return ImageType::Gray;
	else return ImageType::Unknown;
}

int ExtPassDefinition::getImageTypeNumChannels(const ImageType &image_type)
{
	switch(image_type)
	{
		case ImageType::ColorAlphaWeight: return 5;
		case ImageType::ColorAlpha: return 4;
		case ImageType::Color:
		case ImageType::GrayAlphaWeight: return 3;
		case ImageType::GrayWeight:
		case ImageType::GrayAlpha: return 2;
		case ImageType::Gray: return 1;
		default: return 0;
	}
}

std::string ExtPassDefinition::getImageTypeName(const ImageType &image_type)
{
	switch(image_type)
	{
		case ImageType::ColorAlphaWeight: return "Color + Alpha (weighted) [5 channels]";
		case ImageType::ColorAlpha: return "Color + Alpha [4 channels]";
		case ImageType::Color: return "Color [3 channels]";
		case ImageType::GrayAlphaWeight: return "Gray + Alpha (weighted) [3 channels]";
		case ImageType::GrayWeight: return "Gray (weighted) [2 channels]";
		case ImageType::GrayAlpha: return "Gray + Alpha [2 channels]";
		case ImageType::Gray: return "Gray [1 channel]";
		default: return "unknown image type [0 channels]";
	}
}



///////////////////////////////
// --  ExtPassesSettings  -- //
///////////////////////////////

void ExtPassesSettings::extPassAdd(const std::string &ext_pass_name, IntPassType int_pass_type, const ImageType &image_type, bool save)
{
	passes_.push_back(ExtPassDefinition(ext_pass_name, int_pass_type, image_type, save));
}


////////////////////////////
// --  PassesSettings  -- //
////////////////////////////

PassesSettings::PassesSettings()
{
	extPassAdd("Combined", "combined"); //by default we will have an external/internal Combined pass
}

void PassesSettings::extPassAdd(const std::string &ext_pass_name, const std::string &int_pass_name, const ImageType &image_type)
{
	const IntPassType int_pass_type = int_passes_settings_.type(int_pass_name);
	if(int_pass_type == PassDisabled)
	{
		Y_ERROR << "Render Passes: error creating external pass \"" << ext_pass_name << "\" (linked to internal pass \"" << int_pass_name << "\") with image type: '" << ExtPassDefinition::getImageTypeName(image_type) << "'" << YENDL;
		return; //do nothing if the string cannot be found
	}

	ext_passes_settings_.extPassAdd(ext_pass_name, int_pass_type, image_type);
	int_passes_settings_.enable(int_pass_type);

	if(int_pass_type != PassCombined) Y_INFO << "Render Passes: added image pass \"" << ext_pass_name << "\"  (internal pass: \"" << int_pass_name << "\" [" << int_pass_type << "]) with image type: '" << ExtPassDefinition::getImageTypeName(image_type) << "'" << YENDL;
}

void PassesSettings::auxPassAdd(const IntPassType &int_pass_type, const ImageType &image_type)
{
	const std::string int_pass_name = intPassesSettings().name(int_pass_type);
	const std::string aux_pass_name = "aux_" + int_pass_name;

	for(const auto &it : ext_passes_settings_)
	{
		if(it.intPassType() == int_pass_type) return; //If the internal pass is already rendered into a certain external pass, the auxiliary pass is not necessary.
		else if(it.name() == aux_pass_name) return; //If the auxiliary pass already exists, do nothing.
	}

	ext_passes_settings_.extPassAdd(aux_pass_name, int_pass_type, image_type, false);
	int_passes_settings_.enable(int_pass_type);

	Y_VERBOSE << "Render Passes: auxiliary image pass \"" << aux_pass_name << "\" generated for internal pass type: \"" << int_pass_name << "\" [" << int_pass_type << "] with image type: '" << ExtPassDefinition::getImageTypeName(image_type) << "'" << YENDL;
}


void PassesSettings::auxPassesGenerate()
{
	auxPassAdd(PassDebugSamplingFactor, ImageType::Gray); //This auxiliary pass will always be needed for material-specific number of samples calculation

	for(const auto &it : intPassesSettings().listEnabled())
	{
		//If any internal pass needs an auxiliary internal pass and/or auxiliary Render pass, enable also the auxiliary passes.
		switch(it)
		{
			case PassReflectAll:
				int_passes_settings_.enable(PassReflectPerfect);
				int_passes_settings_.enable(PassGlossy);
				int_passes_settings_.enable(PassGlossyIndirect);
				break;

			case PassRefractAll:
				int_passes_settings_.enable(PassRefractPerfect);
				int_passes_settings_.enable(PassTrans);
				int_passes_settings_.enable(PassTransIndirect);
				break;

			case PassIndirectAll:
				int_passes_settings_.enable(PassIndirect);
				int_passes_settings_.enable(PassDiffuseIndirect);
				break;

			case PassObjIndexMaskAll:
				int_passes_settings_.enable(PassObjIndexMask);
				int_passes_settings_.enable(PassObjIndexMaskShadow);
				break;

			case PassMatIndexMaskAll:
				int_passes_settings_.enable(PassMatIndexMask);
				int_passes_settings_.enable(PassMatIndexMaskShadow);
				break;

			case PassDebugFacesEdges:
				auxPassAdd(PassNormalGeom, ImageType::ColorAlpha);
				auxPassAdd(PassZDepthNorm, ImageType::GrayAlpha);
				break;

			case PassDebugObjectsEdges:
				auxPassAdd(PassNormalSmooth, ImageType::ColorAlpha);
				auxPassAdd(PassZDepthNorm, ImageType::GrayAlpha);
				break;

			case PassToon:
				auxPassAdd(PassDebugObjectsEdges, ImageType::ColorAlpha);
				break;

			default:
				break;
		}
	}
}


/////////////////////
// -- IntPasses -- //
/////////////////////

IntPasses::IntPasses(const IntPassesSettings &settings): settings_(settings)
{
	//for performance, even if we don't actually use all the possible internal passes, we reserve a contiguous memory block
	passes_.resize(settings_.listAvailable().size());

	//Reset all colors in all possible render passes to default, even the ones not used, for easier debugging even at the cost of small performance drop in the start of each tile
	for(const auto &it : settings_.listAvailable())
	{
		passes_.at(it.first) = settings_.defaultColor(it.first);
	}
}

Rgba &IntPasses::operator()(const IntPassType &type)
{
	return passes_.at(type);
}

const Rgba &IntPasses::operator()(const IntPassType &type) const
{
	return passes_.at(type);
}

void IntPasses::setDefaults()
{
	for(const auto &it : settings_.listEnabled())
	{
		passes_.at(it) = settings_.defaultColor(it);
	}
}

Rgba *IntPasses::find(const IntPassType &type)
{
	if(enabled(type))
	{
		return &(passes_.at(type));
	}
	else return nullptr;
}

END_YAFARAY
