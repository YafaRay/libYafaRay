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

#include "common/color.h"
#include "common/renderpasses.h"
#include "common/logging.h"

BEGIN_YAFARAY


////////////////////////////
// --  renderPasses_t  -- //
////////////////////////////

RenderPasses::RenderPasses(): index_ext_passes_(std::vector<int>(PassExtTotalPasses, -1)), index_int_passes_(std::vector<int>(PassIntTotalPasses, -1)) //Creation of the external and internal passes indexes, initially all -1 (disabled)
{
	generatePassMaps();

	extPassAdd("Combined", "combined");	//by default we will have an external/internal Combined pass
}

int RenderPasses::extPassesSize() const
{
	return ext_passes_.size();
}

int RenderPasses::auxPassesSize() const
{
	return aux_passes_.size();
}

int RenderPasses::intPassesSize() const
{
	return int_passes_.size();
}

void RenderPasses::generatePassMaps()
{
	//External Render passes - mapping String and External Pass Type
	//IMPORTANT: the external strings MUST MATCH the pass property names in Blender. These must also match the property names in Blender-Exporter without the "pass_" prefix.
	ext_pass_map_string_int_["Combined"] = PassExtCombined;
	ext_pass_map_string_int_["Depth"] = PassExtZDepth;	//From here, specific ext.passes for Blender Exporter
	ext_pass_map_string_int_["Vector"] = PassExtVector;
	ext_pass_map_string_int_["Normal"] = PassExtNormal;
	ext_pass_map_string_int_["UV"] = PassExtUv;
	ext_pass_map_string_int_["Color"] = PassExtColor;
	ext_pass_map_string_int_["Emit"] = PassExtEmit;
	ext_pass_map_string_int_["Mist"] = PassExtMist;
	ext_pass_map_string_int_["Diffuse"] = PassExtDiffuse;
	ext_pass_map_string_int_["Spec"] = PassExtSpecular;
	ext_pass_map_string_int_["AO"] = PassExtAo;
	ext_pass_map_string_int_["Env"] = PassExtEnv;
	ext_pass_map_string_int_["Indirect"] = PassExtIndirect;
	ext_pass_map_string_int_["Shadow"] = PassExtShadow;
	ext_pass_map_string_int_["Reflect"] = PassExtReflect;
	ext_pass_map_string_int_["Refract"] = PassExtRefract;
	ext_pass_map_string_int_["IndexOB"] = PassExtObjIndex;
	ext_pass_map_string_int_["IndexMA"] = PassExtMatIndex;
	ext_pass_map_string_int_["DiffDir"] = PassExtDiffuseDirect;
	ext_pass_map_string_int_["DiffInd"] = PassExtDiffuseIndirect;
	ext_pass_map_string_int_["DiffCol"] = PassExtDiffuseColor;
	ext_pass_map_string_int_["GlossDir"] = PassExtGlossyDirect;
	ext_pass_map_string_int_["GlossInd"] = PassExtGlossyIndirect;
	ext_pass_map_string_int_["GlossCol"] = PassExtGlossyColor;
	ext_pass_map_string_int_["TransDir"] = PassExtTransDirect;
	ext_pass_map_string_int_["TransInd"] = PassExtTransIndirect;
	ext_pass_map_string_int_["TransCol"] = PassExtTransColor;
	ext_pass_map_string_int_["SubsurfaceDir"] = PassExtSubsurfaceDirect;
	ext_pass_map_string_int_["SubsurfaceInd"] = PassExtSubsurfaceIndirect;
	ext_pass_map_string_int_["SubsurfaceCol"] = PassExtSubsurfaceColor;
	ext_pass_map_string_int_["RenderPass_1"] = PassExt1;	//From here, generic ext.passes for other exporters and plugins
	ext_pass_map_string_int_["RenderPass_2"] = PassExt2;
	ext_pass_map_string_int_["RenderPass_3"] = PassExt3;
	ext_pass_map_string_int_["RenderPass_4"] = PassExt4;
	ext_pass_map_string_int_["RenderPass_5"] = PassExt5;
	ext_pass_map_string_int_["RenderPass_6"] = PassExt6;
	ext_pass_map_string_int_["RenderPass_7"] = PassExt7;
	ext_pass_map_string_int_["RenderPass_8"] = PassExt8;
	ext_pass_map_string_int_["RenderPass_9"] = PassExt9;
	ext_pass_map_string_int_["RenderPass_10"] = PassExt10;
	ext_pass_map_string_int_["RenderPass_11"] = PassExt11;
	ext_pass_map_string_int_["RenderPass_12"] = PassExt12;
	ext_pass_map_string_int_["RenderPass_13"] = PassExt13;
	ext_pass_map_string_int_["RenderPass_14"] = PassExt14;
	ext_pass_map_string_int_["RenderPass_15"] = PassExt15;
	ext_pass_map_string_int_["RenderPass_16"] = PassExt16;
	ext_pass_map_string_int_["RenderPass_17"] = PassExt17;
	ext_pass_map_string_int_["RenderPass_18"] = PassExt18;
	ext_pass_map_string_int_["RenderPass_19"] = PassExt19;
	ext_pass_map_string_int_["RenderPass_20"] = PassExt20;
	ext_pass_map_string_int_["RenderPass_21"] = PassExt21;
	ext_pass_map_string_int_["RenderPass_22"] = PassExt22;
	ext_pass_map_string_int_["RenderPass_23"] = PassExt23;
	ext_pass_map_string_int_["RenderPass_24"] = PassExt24;
	ext_pass_map_string_int_["RenderPass_25"] = PassExt25;
	ext_pass_map_string_int_["RenderPass_26"] = PassExt26;
	ext_pass_map_string_int_["RenderPass_27"] = PassExt27;
	ext_pass_map_string_int_["RenderPass_28"] = PassExt28;
	ext_pass_map_string_int_["RenderPass_29"] = PassExt29;
	ext_pass_map_string_int_["RenderPass_30"] = PassExt30;
	ext_pass_map_string_int_["RenderPass_31"] = PassExt31;
	ext_pass_map_string_int_["RenderPass_32"] = PassExt32;

	//Generation of reverse map (pass type -> pass_string)
	for(auto it = ext_pass_map_string_int_.begin(); it != ext_pass_map_string_int_.end(); ++it)
	{
		ext_pass_map_int_string_[it->second] = it->first;
	}

	//Internal YafaRay Render passes - mapping String and Internal YafaRay Render passes
	//IMPORTANT: the internal strings MUST MATCH the valid values for the pass properties in Blender Exporter
	int_pass_map_string_int_["disabled"] = PassIntDisabled;
	int_pass_map_string_int_["combined"] = PassIntCombined;
	int_pass_map_string_int_["z-depth-norm"] = PassIntZDepthNorm;
	int_pass_map_string_int_["z-depth-abs"] = PassIntZDepthAbs;
	int_pass_map_string_int_["debug-normal-smooth"] = PassIntNormalSmooth;
	int_pass_map_string_int_["debug-normal-geom"] = PassIntNormalGeom;
	int_pass_map_string_int_["adv-radiance"] = PassIntRadiance;
	int_pass_map_string_int_["debug-uv"] = PassIntUv;
	int_pass_map_string_int_["emit"] = PassIntEmit;
	int_pass_map_string_int_["mist"] = PassIntMist;
	int_pass_map_string_int_["diffuse"] = PassIntDiffuse;
	int_pass_map_string_int_["diffuse-noshadow"] = PassIntDiffuseNoShadow;
	int_pass_map_string_int_["ao"] = PassIntAo;
	int_pass_map_string_int_["ao-clay"] = PassIntAoClay;
	int_pass_map_string_int_["env"] = PassIntEnv;
	int_pass_map_string_int_["indirect"] = PassIntIndirectAll;
	int_pass_map_string_int_["adv-indirect"] = PassIntIndirect;
	int_pass_map_string_int_["shadow"] = PassIntShadow;
	int_pass_map_string_int_["reflect"] = PassIntReflectAll;
	int_pass_map_string_int_["refract"] = PassIntRefractAll;
	int_pass_map_string_int_["adv-reflect"] = PassIntReflectPerfect;
	int_pass_map_string_int_["adv-refract"] = PassIntRefractPerfect;
	int_pass_map_string_int_["obj-index-abs"] = PassIntObjIndexAbs;
	int_pass_map_string_int_["obj-index-norm"] = PassIntObjIndexNorm;
	int_pass_map_string_int_["obj-index-auto"] = PassIntObjIndexAuto;
	int_pass_map_string_int_["obj-index-auto-abs"] = PassIntObjIndexAutoAbs;
	int_pass_map_string_int_["obj-index-mask"] = PassIntObjIndexMask;
	int_pass_map_string_int_["obj-index-mask-shadow"] = PassIntObjIndexMaskShadow;
	int_pass_map_string_int_["obj-index-mask-all"] = PassIntObjIndexMaskAll;
	int_pass_map_string_int_["mat-index-abs"] = PassIntMatIndexAbs;
	int_pass_map_string_int_["mat-index-norm"] = PassIntMatIndexNorm;
	int_pass_map_string_int_["mat-index-auto"] = PassIntMatIndexAuto;
	int_pass_map_string_int_["mat-index-auto-abs"] = PassIntMatIndexAutoAbs;
	int_pass_map_string_int_["mat-index-mask"] = PassIntMatIndexMask;
	int_pass_map_string_int_["mat-index-mask-shadow"] = PassIntMatIndexMaskShadow;
	int_pass_map_string_int_["mat-index-mask-all"] = PassIntMatIndexMaskAll;
	int_pass_map_string_int_["adv-diffuse-indirect"] = PassIntDiffuseIndirect;
	int_pass_map_string_int_["adv-diffuse-color"] = PassIntDiffuseColor;
	int_pass_map_string_int_["adv-glossy"] = PassIntGlossy;
	int_pass_map_string_int_["adv-glossy-indirect"] = PassIntGlossyIndirect;
	int_pass_map_string_int_["adv-glossy-color"] = PassIntGlossyColor;
	int_pass_map_string_int_["adv-trans"] = PassIntTrans;
	int_pass_map_string_int_["adv-trans-indirect"] = PassIntTransIndirect;
	int_pass_map_string_int_["adv-trans-color"] = PassIntTransColor;
	int_pass_map_string_int_["adv-subsurface"] = PassIntSubsurface;
	int_pass_map_string_int_["adv-subsurface-indirect"] = PassIntSubsurfaceIndirect;
	int_pass_map_string_int_["adv-subsurface-color"] = PassIntSubsurfaceColor;
	int_pass_map_string_int_["debug-normal-smooth"] = PassIntNormalSmooth;
	int_pass_map_string_int_["debug-normal-geom"] = PassIntNormalGeom;
	int_pass_map_string_int_["debug-nu"] = PassIntDebugNu;
	int_pass_map_string_int_["debug-nv"] = PassIntDebugNv;
	int_pass_map_string_int_["debug-dpdu"] = PassIntDebugDpdu;
	int_pass_map_string_int_["debug-dpdv"] = PassIntDebugDpdv;
	int_pass_map_string_int_["debug-dsdu"] = PassIntDebugDsdu;
	int_pass_map_string_int_["debug-dsdv"] = PassIntDebugDsdv;
	int_pass_map_string_int_["adv-surface-integration"] = PassIntSurfaceIntegration;
	int_pass_map_string_int_["adv-volume-integration"] = PassIntVolumeIntegration;
	int_pass_map_string_int_["adv-volume-transmittance"] = PassIntVolumeTransmittance;
	int_pass_map_string_int_["debug-aa-samples"] = PassIntAaSamples;
	int_pass_map_string_int_["debug-light-estimation-light-dirac"] = PassIntDebugLightEstimationLightDirac;
	int_pass_map_string_int_["debug-light-estimation-light-sampling"] = PassIntDebugLightEstimationLightSampling;
	int_pass_map_string_int_["debug-light-estimation-mat-sampling"] = PassIntDebugLightEstimationMatSampling;
	int_pass_map_string_int_["debug-wireframe"] = PassIntDebugWireframe;
	int_pass_map_string_int_["debug-faces-edges"] = PassIntDebugFacesEdges;
	int_pass_map_string_int_["debug-objects-edges"] = PassIntDebugObjectsEdges;
	int_pass_map_string_int_["toon"] = PassIntToon;
	int_pass_map_string_int_["debug-sampling-factor"] = PassIntDebugSamplingFactor;
	int_pass_map_string_int_["debug-dp-lengths"] = PassIntDebugDpLengths;
	int_pass_map_string_int_["debug-dpdx"] = PassIntDebugDpdx;
	int_pass_map_string_int_["debug-dpdy"] = PassIntDebugDpdy;
	int_pass_map_string_int_["debug-dpdxy"] = PassIntDebugDpdxy;
	int_pass_map_string_int_["debug-dudx-dvdx"] = PassIntDebugDudxDvdx;
	int_pass_map_string_int_["debug-dudy-dvdy"] = PassIntDebugDudyDvdy;
	int_pass_map_string_int_["debug-dudxy-dvdxy"] = PassIntDebugDudxyDvdxy;

	//Generation of reverse map (pass type -> pass_string)
	for(auto it = int_pass_map_string_int_.begin(); it != int_pass_map_string_int_.end(); ++it)
	{
		int_pass_map_int_string_[it->second] = it->first;
	}
}

void RenderPasses::extPassAdd(const std::string &s_external_pass, const std::string &s_internal_pass)
{
	ExtPassTypes ext_pass_type = extPassTypeFromString(s_external_pass);
	if(ext_pass_type == PassExtDisabled)
	{
		Y_ERROR << "Render Passes: error creating external pass \"" << s_external_pass << "\" (linked to internal pass \"" << s_internal_pass << "\")" << YENDL;
		return;
	}

	IntPassTypes int_pass_type = intPassTypeFromString(s_internal_pass);
	if(int_pass_type == PassIntDisabled)
	{
		Y_ERROR << "Render Passes: error creating internal pass \"" << s_internal_pass << "\" (linked to external pass \"" << s_external_pass << "\")" << YENDL;
		return;
	}

	if(index_ext_passes_.at(ext_pass_type) != -1)
	{
		//Y_VERBOSE << "Render Passes: external pass type \"" << sExternalPass << "\" already exists, skipping." << YENDL;
		return;
	}

	ext_passes_.push_back(ExtPass(ext_pass_type, int_pass_type));
	index_ext_passes_.at(ext_pass_type) = ext_passes_.end() - ext_passes_.begin() - 1;	//Each external index entry represents one of the possible external passes types and will have the (sequence) index of the external pass actually using that index

	if(s_external_pass != "Combined") Y_INFO << "Render Passes: added pass \"" << s_external_pass << "\" [" << ext_pass_type << "]  (internal pass: \"" << s_internal_pass << "\" [" << int_pass_type << "])" << YENDL;

	intPassAdd(int_pass_type);
}

void RenderPasses::auxPassAdd(IntPassTypes int_pass_type)
{
	if(int_pass_type == PassIntDisabled) return;

	for(int idx = 0; idx < extPassesSize(); ++idx)
	{
		if(intPassTypeFromExtPassIndex(idx) == int_pass_type) return;		//If the internal pass is already rendered into a certain external pass, the auxiliary pass is not necessary.
	}

	for(int idx = 0; idx < auxPassesSize(); ++idx)
	{
		if(intPassTypeFromAuxPassIndex(idx) == int_pass_type) return;		//If the auxiliary pass already exists, do nothing.
	}

	aux_passes_.push_back(AuxPass(int_pass_type));

	intPassAdd(int_pass_type);

	Y_VERBOSE << "Render Passes: auxiliary Render pass generated for internal pass type: \"" << intPassTypeStringFromType(int_pass_type) << "\" [" << int_pass_type << "]" << YENDL;
}

void RenderPasses::intPassAdd(IntPassTypes int_pass_type)
{
	//if(std::binary_search(intPasses.begin(), intPasses.end(), intPassType))
	if(index_int_passes_.at(int_pass_type) != -1)
	{
		//Y_VERBOSE << "Render Passes: internal pass \"" << intPassTypeStringFromType(intPassType) << "\" [" << intPassType << "] already exists, skipping..." << YENDL;
		return;
	}
	int_passes_.push_back(int_pass_type);
	//std::sort(intPasses.begin(), intPasses.end());
	index_int_passes_.at(int_pass_type) = int_passes_.end() - int_passes_.begin() - 1;	//Each internal index entry represents one of the possible internal passes types and will have the (sequence) index of the internal pass actually using that index

	if(int_pass_type != PassIntCombined) Y_VERBOSE << "Render Passes: created internal pass: \"" << intPassTypeStringFromType(int_pass_type) << "\" [" << int_pass_type << "]" << YENDL;
}

void RenderPasses::auxPassesGenerate()
{
	auxPassAdd(PassIntDebugSamplingFactor);	//This auxiliary pass will always be needed for material-specific number of samples calculation

	for(size_t idx = 1; idx < int_passes_.size(); ++idx)
	{
		//If any internal pass needs an auxiliary internal pass and/or auxiliary Render pass, enable also the auxiliary passes.
		switch(int_passes_.at(idx))
		{
			case PassIntReflectAll:
				intPassAdd(PassIntReflectPerfect);
				intPassAdd(PassIntGlossy);
				intPassAdd(PassIntGlossyIndirect);
				break;

			case PassIntRefractAll:
				intPassAdd(PassIntRefractPerfect);
				intPassAdd(PassIntTrans);
				intPassAdd(PassIntTransIndirect);
				break;

			case PassIntIndirectAll:
				intPassAdd(PassIntIndirect);
				intPassAdd(PassIntDiffuseIndirect);
				break;

			case PassIntObjIndexMaskAll:
				intPassAdd(PassIntObjIndexMask);
				intPassAdd(PassIntObjIndexMaskShadow);
				break;

			case PassIntMatIndexMaskAll:
				intPassAdd(PassIntMatIndexMask);
				intPassAdd(PassIntMatIndexMaskShadow);
				break;

			case PassIntDebugFacesEdges:
				auxPassAdd(PassIntNormalGeom);
				auxPassAdd(PassIntZDepthNorm);
				break;

			case PassIntDebugObjectsEdges:
				auxPassAdd(PassIntNormalSmooth);
				auxPassAdd(PassIntZDepthNorm);
				break;

			case PassIntToon:
				auxPassAdd(PassIntDebugObjectsEdges);
				break;

			default:
				break;
		}
	}
}

ExtPassTypes RenderPasses::extPassTypeFromIndex(int ext_pass_index) const { return ext_passes_.at(ext_pass_index).ext_pass_type_; }
IntPassTypes RenderPasses::intPassTypeFromIndex(int int_pass_index) const { return int_passes_.at(int_pass_index); }

std::string RenderPasses::extPassTypeStringFromIndex(int ext_pass_index) const
{
	auto map_iterator = ext_pass_map_int_string_.find(ext_passes_.at(ext_pass_index).ext_pass_type_);
	if(map_iterator == ext_pass_map_int_string_.end()) return "not found";
	else return map_iterator->second;
}

std::string RenderPasses::extPassTypeStringFromType(ExtPassTypes ext_pass_type) const
{
	auto map_iterator = ext_pass_map_int_string_.find(ext_pass_type);
	if(map_iterator == ext_pass_map_int_string_.end()) return "not found";
	else return map_iterator->second;
}

std::string RenderPasses::intPassTypeStringFromType(IntPassTypes int_pass_type) const
{
	auto map_iterator = int_pass_map_int_string_.find(int_pass_type);
	if(map_iterator == int_pass_map_int_string_.end()) return "not found";
	else return map_iterator->second;
}

ExtPassTypes RenderPasses::extPassTypeFromString(std::string ext_pass_type_string) const
{
	auto map_iterator = ext_pass_map_string_int_.find(ext_pass_type_string);
	if(map_iterator == ext_pass_map_string_int_.end()) return PassExtDisabled;	//PASS_EXT_DISABLED is returned if the string cannot be found
	else return map_iterator->second;
}

IntPassTypes RenderPasses::intPassTypeFromString(std::string int_pass_type_string) const
{
	auto map_iterator = int_pass_map_string_int_.find(int_pass_type_string);
	if(map_iterator == int_pass_map_string_int_.end()) return PassIntDisabled;	//PASS_INT_DISABLED is returned if the string cannot be found
	else return map_iterator->second;
}

ExternalPassTileTypes RenderPasses::tileType(int ext_pass_index) const { return ext_passes_.at(ext_pass_index).tile_type_; }

IntPassTypes RenderPasses::intPassTypeFromExtPassIndex(int ext_pass_index) const
{
	if(extPassesSize() > ext_pass_index) return ext_passes_.at(ext_pass_index).int_pass_type_;
	else return PassIntDisabled;
}

IntPassTypes RenderPasses::intPassTypeFromAuxPassIndex(int aux_pass_index) const
{
	if(auxPassesSize() > aux_pass_index) return aux_passes_.at(aux_pass_index).int_pass_type_;
	else return PassIntDisabled;
}

int RenderPasses::extPassIndexFromType(ExtPassTypes ext_pass_type) const
{
	return index_ext_passes_.at(ext_pass_type);
}

int RenderPasses::intPassIndexFromType(IntPassTypes int_pass_type) const
{
	return index_int_passes_.at(int_pass_type);
}

void RenderPasses::setPassMaskObjIndex(float new_obj_index) { pass_mask_obj_index_ = new_obj_index; }
void RenderPasses::setPassMaskMatIndex(float new_mat_index) { pass_mask_mat_index_ = new_mat_index; }
void RenderPasses::setPassMaskInvert(bool mask_invert) { pass_mask_invert_ = mask_invert; }
void RenderPasses::setPassMaskOnly(bool mask_only) { pass_mask_only_ = mask_only; }



////////////////////////////
// --    extPass_t     -- //
////////////////////////////

ExtPass::ExtPass(ExtPassTypes ext_pass_type, IntPassTypes int_pass_type):
		ext_pass_type_(ext_pass_type), int_pass_type_(int_pass_type)
{
	switch(ext_pass_type)  //These are the tyle types needed for Blender
	{
		case PassExtCombined: tile_type_ = PassExtTile4Rgba;		break;
		case PassExtZDepth: tile_type_ = PassExtTile1Grayscale;	break;
		case PassExtVector: tile_type_ = PassExtTile4Rgba;		break;
		case PassExtColor: tile_type_ = PassExtTile4Rgba;		break;
		case PassExtMist: tile_type_ = PassExtTile1Grayscale;	break;
		case PassExtObjIndex: tile_type_ = PassExtTile1Grayscale;	break;
		case PassExtMatIndex: tile_type_ = PassExtTile1Grayscale;	break;
		default: tile_type_ = PassExtTile3Rgb;			break;
	}
}



////////////////////////////
// --    auxPass_t     -- //
////////////////////////////

AuxPass::AuxPass(IntPassTypes int_pass_type):
		int_pass_type_(int_pass_type)
{
	//empty
}



/////////////////////////
// -- colorPasses_t -- //
/////////////////////////

ColorPasses::ColorPasses(const RenderPasses *render_passes): pass_definitions_(render_passes)
{
	//for performance, even if we don't actually use all the possible internal passes, we reserve a contiguous memory block
	col_vector_.reserve(pass_definitions_->int_passes_.size());
	for(auto it = pass_definitions_->int_passes_.begin(); it != pass_definitions_->int_passes_.end(); ++it)
	{
		col_vector_.push_back(initColor(pass_definitions_->intPassTypeFromIndex(it - pass_definitions_->int_passes_.begin())));
	}
}

bool ColorPasses::enabled(IntPassTypes int_pass_type) const
{
	if(pass_definitions_->intPassIndexFromType(int_pass_type) == -1) return false;
	else return true;
}

IntPassTypes ColorPasses::intPassTypeFromIndex(int int_pass_index) const
{
	return pass_definitions_->intPassTypeFromIndex(int_pass_index);
}

Rgba &ColorPasses::color(IntPassTypes int_pass_type)
{
	return col_vector_.at(pass_definitions_->intPassIndexFromType(int_pass_type));
}

Rgba &ColorPasses::color(int int_pass_index)
{
	return col_vector_.at(int_pass_index);
}

Rgba &ColorPasses::operator()(IntPassTypes int_pass_type)
{
	return color(int_pass_type);
}

Rgba &ColorPasses::operator()(int int_pass_index)
{
	return color(int_pass_index);
}

void ColorPasses::resetColors()
{
	for(auto it = col_vector_.begin(); it != col_vector_.end(); ++it)
	{
		*it = initColor(intPassTypeFromIndex(it - col_vector_.begin()));
	}
}

Rgba ColorPasses::initColor(IntPassTypes int_pass_type)
{
	switch(int_pass_type)    //Default initialization color in general is black/opaque, except for SHADOW and MASK passes where the default is black/transparent for easier masking
	{
		case PassIntDebugWireframe:
		case PassIntShadow:
		case PassIntObjIndexMask:
		case PassIntObjIndexMaskShadow:
		case PassIntObjIndexMaskAll:
		case PassIntMatIndexMask:
		case PassIntMatIndexMaskShadow:
		case PassIntMatIndexMaskAll: return Rgba(0.f, 0.f, 0.f, 0.f); break;
		default: return Rgba(0.f, 0.f, 0.f, 1.f); break;
	}
}

void ColorPasses::multiplyColors(float factor)
{
	for(auto it = col_vector_.begin(); it != col_vector_.end(); ++it)
	{
		*it *= factor;
	}
}

Rgba ColorPasses::probeSet(const IntPassTypes &int_pass_type, const Rgba &rendered_color, const bool &condition /*= true */)
{
	if(condition && enabled(int_pass_type)) color(pass_definitions_->intPassIndexFromType(int_pass_type)) = rendered_color;

	return rendered_color;
}

Rgba ColorPasses::probeSet(const IntPassTypes &int_pass_type, const ColorPasses &color_passes, const bool &condition /*= true */)
{
	if(condition && enabled(int_pass_type) && color_passes.enabled(int_pass_type))
	{
		int int_pass_index = pass_definitions_->intPassIndexFromType(int_pass_type);
		col_vector_.at(int_pass_index) = color_passes.col_vector_.at(int_pass_index);
		return color_passes.col_vector_.at(int_pass_index);
	}
	else return Rgba(0.f);
}

Rgba ColorPasses::probeAdd(const IntPassTypes &int_pass_type, const Rgba &rendered_color, const bool &condition /*= true */)
{
	if(condition && enabled(int_pass_type)) color(pass_definitions_->intPassIndexFromType(int_pass_type)) += rendered_color;

	return rendered_color;
}

Rgba ColorPasses::probeAdd(const IntPassTypes &int_pass_type, const ColorPasses &color_passes, const bool &condition /*= true */)
{
	if(condition && enabled(int_pass_type) && color_passes.enabled(int_pass_type))
	{
		int int_pass_index = pass_definitions_->intPassIndexFromType(int_pass_type);
		col_vector_.at(int_pass_index) += color_passes.col_vector_.at(int_pass_index);
		return  color_passes.col_vector_.at(int_pass_index);
	}
	else return Rgba(0.f);
}

Rgba ColorPasses::probeMult(const IntPassTypes &int_pass_type, const Rgba &rendered_color, const bool &condition /*= true */)
{
	if(condition && enabled(int_pass_type)) color(pass_definitions_->intPassIndexFromType(int_pass_type)) *= rendered_color;

	return rendered_color;
}

Rgba ColorPasses::probeMult(const IntPassTypes &int_pass_type, const ColorPasses &color_passes, const bool &condition /*= true */)
{
	if(condition && enabled(int_pass_type) && color_passes.enabled(int_pass_type))
	{
		int int_pass_index = pass_definitions_->intPassIndexFromType(int_pass_type);
		col_vector_.at(int_pass_index) *= color_passes.col_vector_.at(int_pass_index);
		return color_passes.col_vector_.at(int_pass_index);
	}
	else return Rgba(0.f);
}

ColorPasses &ColorPasses::operator *= (float f)
{
	for(auto it = col_vector_.begin(); it != col_vector_.end(); ++it)
	{
		*it *= f;
	}
	return *this;
}

ColorPasses &ColorPasses::operator *= (const Rgb &a)
{
	for(auto it = col_vector_.begin(); it != col_vector_.end(); ++it)
	{
		*it *= a;
	}
	return *this;
}

ColorPasses &ColorPasses::operator *= (const Rgba &a)
{
	for(auto it = col_vector_.begin(); it != col_vector_.end(); ++it)
	{
		*it *= a;
	}
	return *this;
}

ColorPasses &ColorPasses::operator += (const ColorPasses &a)
{
	for(auto it = col_vector_.begin(); it != col_vector_.end(); ++it)
	{
		*it += a.col_vector_.at(it - col_vector_.begin());
	}
	return *this;
}

float ColorPasses::getPassMaskObjIndex() const { return pass_definitions_->pass_mask_obj_index_; }
float ColorPasses::getPassMaskMatIndex() const { return pass_definitions_->pass_mask_mat_index_; }
bool ColorPasses::getPassMaskInvert() const { return pass_definitions_->pass_mask_invert_; }
bool ColorPasses::getPassMaskOnly() const { return pass_definitions_->pass_mask_only_; }



int ColorPasses::size() const { return col_vector_.size(); }


END_YAFARAY
