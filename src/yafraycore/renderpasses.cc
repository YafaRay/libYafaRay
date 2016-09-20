/****************************************************************************
 *      renderpasses.cc: Render Passes operations
 *      This is part of the yafaray package
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

#include <core_api/color.h>
#include <core_api/renderpasses.h>

__BEGIN_YAFRAY


////////////////////////////
// --  renderPasses_t  -- //
////////////////////////////

renderPasses_t::renderPasses_t():indexExtPasses(std::vector<int>(PASS_EXT_TOTAL_PASSES, -1)), indexIntPasses(std::vector<int>(PASS_INT_TOTAL_PASSES, -1)) //Creation of the external and internal passes indexes, initially all -1 (disabled)
{ 
	generate_pass_maps();
	
	extPass_add("Combined", "combined");	//by default we will have an external/internal Combined pass
}

int renderPasses_t::extPassesSize() const
{
	return extPasses.size();
}

int renderPasses_t::auxPassesSize() const
{
	return auxPasses.size();
}

int renderPasses_t::intPassesSize() const
{
	return intPasses.size();
}

void renderPasses_t::generate_pass_maps()
{
	//External Render passes - mapping String and External Pass Type
	//IMPORTANT: the external strings MUST MATCH the pass property names in Blender. These must also match the property names in Blender-Exporter without the "pass_" prefix. 
	extPassMapStringInt["Combined"] = PASS_EXT_COMBINED;
	extPassMapStringInt["Depth"] = PASS_EXT_Z_DEPTH;	//From here, specific ext.passes for Blender Exporter
	extPassMapStringInt["Vector"] = PASS_EXT_VECTOR;
	extPassMapStringInt["Normal"] = PASS_EXT_NORMAL;
	extPassMapStringInt["UV"] = PASS_EXT_UV;
	extPassMapStringInt["Color"] = PASS_EXT_COLOR;
	extPassMapStringInt["Emit"] = PASS_EXT_EMIT;
	extPassMapStringInt["Mist"] = PASS_EXT_MIST;
	extPassMapStringInt["Diffuse"] = PASS_EXT_DIFFUSE;
	extPassMapStringInt["Spec"] = PASS_EXT_SPECULAR;
	extPassMapStringInt["AO"] = PASS_EXT_AO;
	extPassMapStringInt["Env"] = PASS_EXT_ENV;
	extPassMapStringInt["Indirect"] = PASS_EXT_INDIRECT;
	extPassMapStringInt["Shadow"] = PASS_EXT_SHADOW;
	extPassMapStringInt["Reflect"] = PASS_EXT_REFLECT;
	extPassMapStringInt["Refract"] = PASS_EXT_REFRACT;
	extPassMapStringInt["IndexOB"] = PASS_EXT_OBJ_INDEX;
	extPassMapStringInt["IndexMA"] = PASS_EXT_MAT_INDEX;
	extPassMapStringInt["DiffDir"] = PASS_EXT_DIFFUSE_DIRECT;
	extPassMapStringInt["DiffInd"] = PASS_EXT_DIFFUSE_INDIRECT;
	extPassMapStringInt["DiffCol"] = PASS_EXT_DIFFUSE_COLOR;
	extPassMapStringInt["GlossDir"] = PASS_EXT_GLOSSY_DIRECT;
	extPassMapStringInt["GlossInd"] = PASS_EXT_GLOSSY_INDIRECT;
	extPassMapStringInt["GlossCol"] = PASS_EXT_GLOSSY_COLOR;
	extPassMapStringInt["TransDir"] = PASS_EXT_TRANS_DIRECT;
	extPassMapStringInt["TransInd"] = PASS_EXT_TRANS_INDIRECT;
	extPassMapStringInt["TransCol"] = PASS_EXT_TRANS_COLOR;
	extPassMapStringInt["SubsurfaceDir"] = PASS_EXT_SUBSURFACE_DIRECT;
	extPassMapStringInt["SubsurfaceInd"] = PASS_EXT_SUBSURFACE_INDIRECT;
	extPassMapStringInt["SubsurfaceCol"] = PASS_EXT_SUBSURFACE_COLOR;
	extPassMapStringInt["RenderPass_1"] = PASS_EXT_1;	//From here, generic ext.passes for other exporters and plugins
	extPassMapStringInt["RenderPass_2"] = PASS_EXT_2;
	extPassMapStringInt["RenderPass_3"] = PASS_EXT_3;
	extPassMapStringInt["RenderPass_4"] = PASS_EXT_4;
	extPassMapStringInt["RenderPass_5"] = PASS_EXT_5;
	extPassMapStringInt["RenderPass_6"] = PASS_EXT_6;
	extPassMapStringInt["RenderPass_7"] = PASS_EXT_7;
	extPassMapStringInt["RenderPass_8"] = PASS_EXT_8;
	extPassMapStringInt["RenderPass_9"] = PASS_EXT_9;
	extPassMapStringInt["RenderPass_10"] = PASS_EXT_10;
	extPassMapStringInt["RenderPass_11"] = PASS_EXT_11;
	extPassMapStringInt["RenderPass_12"] = PASS_EXT_12;
	extPassMapStringInt["RenderPass_13"] = PASS_EXT_13;
	extPassMapStringInt["RenderPass_14"] = PASS_EXT_14;
	extPassMapStringInt["RenderPass_15"] = PASS_EXT_15;
	extPassMapStringInt["RenderPass_16"] = PASS_EXT_16;
	extPassMapStringInt["RenderPass_17"] = PASS_EXT_17;
	extPassMapStringInt["RenderPass_18"] = PASS_EXT_18;
	extPassMapStringInt["RenderPass_19"] = PASS_EXT_19;
	extPassMapStringInt["RenderPass_20"] = PASS_EXT_20;
	extPassMapStringInt["RenderPass_21"] = PASS_EXT_21;
	extPassMapStringInt["RenderPass_22"] = PASS_EXT_22;
	extPassMapStringInt["RenderPass_23"] = PASS_EXT_23;
	extPassMapStringInt["RenderPass_24"] = PASS_EXT_24;
	extPassMapStringInt["RenderPass_25"] = PASS_EXT_25;
	extPassMapStringInt["RenderPass_26"] = PASS_EXT_26;
	extPassMapStringInt["RenderPass_27"] = PASS_EXT_27;
	extPassMapStringInt["RenderPass_28"] = PASS_EXT_28;
	extPassMapStringInt["RenderPass_29"] = PASS_EXT_29;
	extPassMapStringInt["RenderPass_30"] = PASS_EXT_30;
	extPassMapStringInt["RenderPass_31"] = PASS_EXT_31;
	extPassMapStringInt["RenderPass_32"] = PASS_EXT_32;
	
	//Generation of reverse map (pass type -> pass_string)
	for(auto it = extPassMapStringInt.begin(); it != extPassMapStringInt.end(); ++it)
	{
		extPassMapIntString[it->second] = it->first;
	}

	//Internal YafaRay Render passes - mapping String and Internal YafaRay Render passes
	//IMPORTANT: the internal strings MUST MATCH the valid values for the pass properties in Blender Exporter
	intPassMapStringInt["disabled"] = PASS_INT_DISABLED;
	intPassMapStringInt["combined"] = PASS_INT_COMBINED;
	intPassMapStringInt["z-depth-norm"] = PASS_INT_Z_DEPTH_NORM;
	intPassMapStringInt["z-depth-abs"] = PASS_INT_Z_DEPTH_ABS;
	intPassMapStringInt["debug-normal-smooth"] = PASS_INT_NORMAL_SMOOTH;
	intPassMapStringInt["debug-normal-geom"] = PASS_INT_NORMAL_GEOM;
	intPassMapStringInt["adv-radiance"] = PASS_INT_RADIANCE;
	intPassMapStringInt["debug-uv"] = PASS_INT_UV;
	intPassMapStringInt["emit"] = PASS_INT_EMIT;
	intPassMapStringInt["mist"] = PASS_INT_MIST;
	intPassMapStringInt["diffuse"] = PASS_INT_DIFFUSE;
	intPassMapStringInt["diffuse-noshadow"] = PASS_INT_DIFFUSE_NO_SHADOW;
	intPassMapStringInt["ao"] = PASS_INT_AO;
	intPassMapStringInt["ao-clay"] = PASS_INT_AO_CLAY;
	intPassMapStringInt["env"] = PASS_INT_ENV;
	intPassMapStringInt["indirect"] = PASS_INT_INDIRECT_ALL;
	intPassMapStringInt["adv-indirect"] = PASS_INT_INDIRECT;
	intPassMapStringInt["shadow"] = PASS_INT_SHADOW;
	intPassMapStringInt["reflect"] = PASS_INT_REFLECT_ALL;
	intPassMapStringInt["refract"] = PASS_INT_REFRACT_ALL;
	intPassMapStringInt["adv-reflect"] = PASS_INT_REFLECT_PERFECT;
	intPassMapStringInt["adv-refract"] = PASS_INT_REFRACT_PERFECT;
	intPassMapStringInt["obj-index-abs"] = PASS_INT_OBJ_INDEX_ABS;
	intPassMapStringInt["obj-index-norm"] = PASS_INT_OBJ_INDEX_NORM;
	intPassMapStringInt["obj-index-auto"] = PASS_INT_OBJ_INDEX_AUTO;
	intPassMapStringInt["obj-index-auto-abs"] = PASS_INT_OBJ_INDEX_AUTO_ABS;
	intPassMapStringInt["obj-index-mask"] = PASS_INT_OBJ_INDEX_MASK;
	intPassMapStringInt["obj-index-mask-shadow"] = PASS_INT_OBJ_INDEX_MASK_SHADOW;
	intPassMapStringInt["obj-index-mask-all"] = PASS_INT_OBJ_INDEX_MASK_ALL;
	intPassMapStringInt["mat-index-abs"] = PASS_INT_MAT_INDEX_ABS;
	intPassMapStringInt["mat-index-norm"] = PASS_INT_MAT_INDEX_NORM;
	intPassMapStringInt["mat-index-auto"] = PASS_INT_MAT_INDEX_AUTO;
	intPassMapStringInt["mat-index-auto-abs"] = PASS_INT_MAT_INDEX_AUTO_ABS;
	intPassMapStringInt["mat-index-mask"] = PASS_INT_MAT_INDEX_MASK;
	intPassMapStringInt["mat-index-mask-shadow"] = PASS_INT_MAT_INDEX_MASK_SHADOW;
	intPassMapStringInt["mat-index-mask-all"] = PASS_INT_MAT_INDEX_MASK_ALL;
	intPassMapStringInt["adv-diffuse-indirect"] = PASS_INT_DIFFUSE_INDIRECT;
	intPassMapStringInt["adv-diffuse-color"] = PASS_INT_DIFFUSE_COLOR;
	intPassMapStringInt["adv-glossy"] = PASS_INT_GLOSSY;
	intPassMapStringInt["adv-glossy-indirect"] = PASS_INT_GLOSSY_INDIRECT;
	intPassMapStringInt["adv-glossy-color"] = PASS_INT_GLOSSY_COLOR;
	intPassMapStringInt["adv-trans"] = PASS_INT_TRANS;
	intPassMapStringInt["adv-trans-indirect"] = PASS_INT_TRANS_INDIRECT;
	intPassMapStringInt["adv-trans-color"] = PASS_INT_TRANS_COLOR;
	intPassMapStringInt["adv-subsurface"] = PASS_INT_SUBSURFACE;
	intPassMapStringInt["adv-subsurface-indirect"] = PASS_INT_SUBSURFACE_INDIRECT;
	intPassMapStringInt["adv-subsurface-color"] = PASS_INT_SUBSURFACE_COLOR;
	intPassMapStringInt["debug-normal-smooth"] = PASS_INT_NORMAL_SMOOTH;
	intPassMapStringInt["debug-normal-geom"] = PASS_INT_NORMAL_GEOM;
	intPassMapStringInt["debug-nu"] = PASS_INT_DEBUG_NU;
	intPassMapStringInt["debug-nv"] = PASS_INT_DEBUG_NV;
	intPassMapStringInt["debug-dpdu"] = PASS_INT_DEBUG_DPDU;
	intPassMapStringInt["debug-dpdv"] = PASS_INT_DEBUG_DPDV;
	intPassMapStringInt["debug-dsdu"] = PASS_INT_DEBUG_DSDU;
	intPassMapStringInt["debug-dsdv"] = PASS_INT_DEBUG_DSDV;
	intPassMapStringInt["adv-surface-integration"] = PASS_INT_SURFACE_INTEGRATION;
	intPassMapStringInt["adv-volume-integration"] = PASS_INT_VOLUME_INTEGRATION;
	intPassMapStringInt["adv-volume-transmittance"] = PASS_INT_VOLUME_TRANSMITTANCE;
	intPassMapStringInt["debug-aa-samples"] = PASS_INT_AA_SAMPLES;
	intPassMapStringInt["debug-light-estimation-light-dirac"] = PASS_INT_DEBUG_LIGHT_ESTIMATION_LIGHT_DIRAC;
	intPassMapStringInt["debug-light-estimation-light-sampling"] = PASS_INT_DEBUG_LIGHT_ESTIMATION_LIGHT_SAMPLING;
	intPassMapStringInt["debug-light-estimation-mat-sampling"] = PASS_INT_DEBUG_LIGHT_ESTIMATION_MAT_SAMPLING;
	intPassMapStringInt["debug-wireframe"] = PASS_INT_DEBUG_WIREFRAME;
	intPassMapStringInt["debug-faces-edges"] = PASS_INT_DEBUG_FACES_EDGES;
	intPassMapStringInt["debug-objects-edges"] = PASS_INT_DEBUG_OBJECTS_EDGES;
	intPassMapStringInt["toon"] = PASS_INT_TOON;
	intPassMapStringInt["debug-sampling-factor"] = PASS_INT_DEBUG_SAMPLING_FACTOR;

	//Generation of reverse map (pass type -> pass_string)
	for(auto it = intPassMapStringInt.begin(); it != intPassMapStringInt.end(); ++it)
	{
		intPassMapIntString[it->second] = it->first;
	}
}

void renderPasses_t::extPass_add(const std::string& sExternalPass, const std::string& sInternalPass)
{
	extPassTypes_t extPassType = extPassTypeFromString(sExternalPass);	
	if(extPassType == PASS_EXT_DISABLED)
	{
		Y_ERROR << "Render Passes: error creating external pass \"" << sExternalPass << "\" (linked to internal pass \"" << sInternalPass << "\")" << yendl;
		return;
	}
	
	intPassTypes_t intPassType = intPassTypeFromString(sInternalPass);	
	if(intPassType == PASS_INT_DISABLED)
	{
		Y_ERROR << "Render Passes: error creating internal pass \"" << sInternalPass << "\" (linked to external pass \"" << sExternalPass << "\")" << yendl;
		return;
	}

	if(indexExtPasses.at(extPassType) != -1)
	{
		//Y_VERBOSE << "Render Passes: external pass type \"" << sExternalPass << "\" already exists, skipping." << yendl;
		return;
	}
	
	extPasses.push_back(extPass_t(extPassType, intPassType));
	indexExtPasses.at(extPassType) = extPasses.end() - extPasses.begin() - 1;	//Each external index entry represents one of the possible external passes types and will have the (sequence) index of the external pass actually using that index 

	if(sExternalPass != "Combined") Y_INFO << "Render Passes: added pass \"" << sExternalPass << "\" [" << extPassType << "]  (internal pass: \"" << sInternalPass << "\" [" << intPassType << "])" << yendl;
    
    intPass_add(intPassType);
}

void renderPasses_t::auxPass_add(intPassTypes_t intPassType)
{	
	if(intPassType == PASS_INT_DISABLED) return;

	for(int idx = 0; idx < extPassesSize(); ++idx)
	{
		if(intPassTypeFromExtPassIndex(idx) == intPassType) return;		//If the internal pass is already rendered into a certain external pass, the auxiliary pass is not necessary.
	}
	
	for(int idx = 0; idx < auxPassesSize(); ++idx)
	{
		if(intPassTypeFromAuxPassIndex(idx) == intPassType) return;		//If the auxiliary pass already exists, do nothing.
	}

	auxPasses.push_back(auxPass_t(intPassType));
    
    intPass_add(intPassType);
    
    Y_VERBOSE << "Render Passes: auxiliary Render pass generated for internal pass type: \"" << intPassTypeStringFromType(intPassType) << "\" [" << intPassType << "]" << yendl;
}

void renderPasses_t::intPass_add(intPassTypes_t intPassType)
{
	//if(std::binary_search(intPasses.begin(), intPasses.end(), intPassType))
	if(indexIntPasses.at(intPassType) != -1)
	{
		//Y_VERBOSE << "Render Passes: internal pass \"" << intPassTypeStringFromType(intPassType) << "\" [" << intPassType << "] already exists, skipping..." << yendl;
		return;
	}
	intPasses.push_back(intPassType);
	//std::sort(intPasses.begin(), intPasses.end());
	indexIntPasses.at(intPassType) = intPasses.end() - intPasses.begin() - 1;	//Each internal index entry represents one of the possible internal passes types and will have the (sequence) index of the internal pass actually using that index 
	
	if(intPassType != PASS_INT_COMBINED) Y_VERBOSE << "Render Passes: created internal pass: \"" << intPassTypeStringFromType(intPassType) << "\" [" << intPassType << "]" << yendl;
}

void renderPasses_t::auxPasses_generate()
{
	auxPass_add(PASS_INT_DEBUG_SAMPLING_FACTOR);	//This auxiliary pass will always be needed for material-specific number of samples calculation
	
	for(size_t idx=1; idx < intPasses.size(); ++idx)
	{
		//If any internal pass needs an auxiliary internal pass and/or auxiliary Render pass, enable also the auxiliary passes.
		switch(intPasses.at(idx))
		{
			case PASS_INT_REFLECT_ALL:
				intPass_add(PASS_INT_REFLECT_PERFECT);
				intPass_add(PASS_INT_GLOSSY);
				intPass_add(PASS_INT_GLOSSY_INDIRECT);
				break;
				
			case PASS_INT_REFRACT_ALL:
				intPass_add(PASS_INT_REFRACT_PERFECT);
				intPass_add(PASS_INT_TRANS);
				intPass_add(PASS_INT_TRANS_INDIRECT);
				break;

			case PASS_INT_INDIRECT_ALL:
				intPass_add(PASS_INT_INDIRECT);
				intPass_add(PASS_INT_DIFFUSE_INDIRECT);
				break;

			case PASS_INT_OBJ_INDEX_MASK_ALL:
				intPass_add(PASS_INT_OBJ_INDEX_MASK);
				intPass_add(PASS_INT_OBJ_INDEX_MASK_SHADOW);
				break;

			case PASS_INT_MAT_INDEX_MASK_ALL:
				intPass_add(PASS_INT_MAT_INDEX_MASK);
				intPass_add(PASS_INT_MAT_INDEX_MASK_SHADOW);
				break;
			
			case PASS_INT_DEBUG_FACES_EDGES:
				auxPass_add(PASS_INT_NORMAL_GEOM);
				auxPass_add(PASS_INT_Z_DEPTH_NORM);
				break;
			
			case PASS_INT_DEBUG_OBJECTS_EDGES:
				auxPass_add(PASS_INT_NORMAL_SMOOTH);
				auxPass_add(PASS_INT_Z_DEPTH_NORM);
				break;

			case PASS_INT_TOON:
				auxPass_add(PASS_INT_DEBUG_OBJECTS_EDGES);
				break;

			default:
				break;
		}
	}
}
        
extPassTypes_t renderPasses_t::extPassTypeFromIndex(int extPassIndex) const { return extPasses.at(extPassIndex).extPassType; }
intPassTypes_t renderPasses_t::intPassTypeFromIndex(int intPassIndex) const { return intPasses.at(intPassIndex); }
	
std::string renderPasses_t::extPassTypeStringFromIndex(int extPassIndex) const
{
	auto map_iterator = extPassMapIntString.find(extPasses.at(extPassIndex).extPassType);
	if(map_iterator == extPassMapIntString.end()) return "not found";
	else return map_iterator->second;
}

std::string renderPasses_t::extPassTypeStringFromType(extPassTypes_t extPassType) const
{
	auto map_iterator = extPassMapIntString.find(extPassType);
	if(map_iterator == extPassMapIntString.end()) return "not found";
	else return map_iterator->second;
}

std::string renderPasses_t::intPassTypeStringFromType(intPassTypes_t intPassType) const
{
	auto map_iterator = intPassMapIntString.find(intPassType);
	if(map_iterator == intPassMapIntString.end()) return "not found";
	else return map_iterator->second;
}

extPassTypes_t renderPasses_t::extPassTypeFromString(std::string extPassTypeString) const
{
	auto map_iterator = extPassMapStringInt.find(extPassTypeString);
	if(map_iterator == extPassMapStringInt.end()) return PASS_EXT_DISABLED;	//PASS_EXT_DISABLED is returned if the string cannot be found
	else return map_iterator->second;
}

intPassTypes_t renderPasses_t::intPassTypeFromString(std::string intPassTypeString) const
{
	auto map_iterator = intPassMapStringInt.find(intPassTypeString);
	if(map_iterator == intPassMapStringInt.end()) return PASS_INT_DISABLED;	//PASS_INT_DISABLED is returned if the string cannot be found
	else return map_iterator->second;
}
        
externalPassTileTypes_t renderPasses_t::tileType(int extPassIndex) const { return extPasses.at(extPassIndex).tileType; }

intPassTypes_t renderPasses_t::intPassTypeFromExtPassIndex(int extPassIndex) const
{
    if(extPassesSize() > extPassIndex) return extPasses.at(extPassIndex).intPassType;
    else return PASS_INT_DISABLED;
}

intPassTypes_t renderPasses_t::intPassTypeFromAuxPassIndex(int auxPassIndex) const
{
    if(auxPassesSize() > auxPassIndex) return auxPasses.at(auxPassIndex).intPassType;
    else return PASS_INT_DISABLED;
}

int renderPasses_t::extPassIndexFromType(extPassTypes_t extPassType) const
{
	return indexExtPasses.at(extPassType);
}

int renderPasses_t::intPassIndexFromType(intPassTypes_t intPassType) const
{
	return indexIntPasses.at(intPassType);
}

void renderPasses_t::set_pass_mask_obj_index(float new_obj_index) { pass_mask_obj_index = new_obj_index; }
void renderPasses_t::set_pass_mask_mat_index(float new_mat_index) { pass_mask_mat_index = new_mat_index; }
void renderPasses_t::set_pass_mask_invert(bool mask_invert) { pass_mask_invert = mask_invert; }
void renderPasses_t::set_pass_mask_only(bool mask_only) { pass_mask_only = mask_only; }



////////////////////////////
// --    extPass_t     -- //
////////////////////////////

extPass_t::extPass_t(extPassTypes_t extPassType, intPassTypes_t intPassType):
			extPassType(extPassType), intPassType(intPassType)
{ 
	switch(extPassType)  //These are the tyle types needed for Blender
	{
		case PASS_EXT_COMBINED:		tileType = PASS_EXT_TILE_4_RGBA;		break;
		case PASS_EXT_Z_DEPTH:		tileType = PASS_EXT_TILE_1_GRAYSCALE;	break;
		case PASS_EXT_VECTOR:		tileType = PASS_EXT_TILE_4_RGBA;		break;
		case PASS_EXT_COLOR:		tileType = PASS_EXT_TILE_4_RGBA;		break;
		case PASS_EXT_MIST:			tileType = PASS_EXT_TILE_1_GRAYSCALE;	break;
		case PASS_EXT_OBJ_INDEX:	tileType = PASS_EXT_TILE_1_GRAYSCALE;	break;
		case PASS_EXT_MAT_INDEX:	tileType = PASS_EXT_TILE_1_GRAYSCALE;	break;
		default: 					tileType = PASS_EXT_TILE_3_RGB;			break;
	}
}



////////////////////////////
// --    auxPass_t     -- //
////////////////////////////

auxPass_t::auxPass_t(intPassTypes_t intPassType):
			intPassType(intPassType)
{ 
	//empty
}



/////////////////////////
// -- colorPasses_t -- //
/////////////////////////

colorPasses_t::colorPasses_t(const renderPasses_t *renderPasses):passDefinitions(renderPasses)
{
	//for performance, even if we don't actually use all the possible internal passes, we reserve a contiguous memory block
	colVector.reserve(passDefinitions->intPasses.size());
	for(auto it = passDefinitions->intPasses.begin(); it != passDefinitions->intPasses.end(); ++it)
	{
		colVector.push_back(init_color(passDefinitions->intPassTypeFromIndex(it - passDefinitions->intPasses.begin())));
	}
}
        
bool colorPasses_t::enabled(intPassTypes_t intPassType) const
{
	if(passDefinitions->intPassIndexFromType(intPassType) == -1) return false;
	else return true;
}

intPassTypes_t colorPasses_t::intPassTypeFromIndex(int intPassIndex) const
{
	return passDefinitions->intPassTypeFromIndex(intPassIndex);
}

colorA_t& colorPasses_t::color(intPassTypes_t intPassType)
{
	return colVector.at(passDefinitions->intPassIndexFromType(intPassType));
}
                
colorA_t& colorPasses_t::color(int intPassIndex)
{
	return colVector.at(intPassIndex);
}
                
colorA_t& colorPasses_t::operator()(intPassTypes_t intPassType)
{
	return color(intPassType);
}

colorA_t& colorPasses_t::operator()(int intPassIndex)
{
	return color(intPassIndex);
}

void colorPasses_t::reset_colors()
{
	for(auto it = colVector.begin(); it != colVector.end(); ++it)
	{
		*it = init_color(intPassTypeFromIndex(it - colVector.begin()));
	}
}
        
colorA_t colorPasses_t::init_color(intPassTypes_t intPassType)
{
	switch(intPassType)    //Default initialization color in general is black/opaque, except for SHADOW and MASK passes where the default is black/transparent for easier masking
	{
		case PASS_INT_DEBUG_WIREFRAME:
		case PASS_INT_SHADOW:
		case PASS_INT_OBJ_INDEX_MASK:
		case PASS_INT_OBJ_INDEX_MASK_SHADOW:
		case PASS_INT_OBJ_INDEX_MASK_ALL:
		case PASS_INT_MAT_INDEX_MASK:
		case PASS_INT_MAT_INDEX_MASK_SHADOW:
		case PASS_INT_MAT_INDEX_MASK_ALL: return colorA_t(0.f, 0.f, 0.f, 0.f); break;
		default: return colorA_t(0.f, 0.f, 0.f, 1.f); break;
	}            
}

void colorPasses_t::multiply_colors(float factor)
{
	for(auto it = colVector.begin(); it != colVector.end(); ++it)
	{
		*it *= factor;
	}
}

colorA_t colorPasses_t::probe_set(const intPassTypes_t& intPassType, const colorA_t& renderedColor, const bool& condition /*= true */)
{
	if(condition && enabled(intPassType)) color(passDefinitions->intPassIndexFromType(intPassType)) = renderedColor;
	
	return renderedColor;
}

colorA_t colorPasses_t::probe_set(const intPassTypes_t& intPassType, const colorPasses_t& colorPasses, const bool& condition /*= true */)
{
	if(condition && enabled(intPassType) && colorPasses.enabled(intPassType))
	{
		int intPassIndex = passDefinitions->intPassIndexFromType(intPassType);
		colVector.at(intPassIndex) = colorPasses.colVector.at(intPassIndex);	
		return colorPasses.colVector.at(intPassIndex);
	}
	else return colorA_t(0.f);
}

colorA_t colorPasses_t::probe_add(const intPassTypes_t& intPassType, const colorA_t& renderedColor, const bool& condition /*= true */)
{
	if(condition && enabled(intPassType)) color(passDefinitions->intPassIndexFromType(intPassType)) += renderedColor;
	
	return renderedColor;
}

colorA_t colorPasses_t::probe_add(const intPassTypes_t& intPassType, const colorPasses_t& colorPasses, const bool& condition /*= true */)
{
	if(condition && enabled(intPassType) && colorPasses.enabled(intPassType))
	{
		int intPassIndex = passDefinitions->intPassIndexFromType(intPassType);
		colVector.at(intPassIndex) += colorPasses.colVector.at(intPassIndex);	
		return  colorPasses.colVector.at(intPassIndex);
	}
	else return colorA_t(0.f);
}

colorA_t colorPasses_t::probe_mult(const intPassTypes_t& intPassType, const colorA_t& renderedColor, const bool& condition /*= true */)
{
	if(condition && enabled(intPassType)) color(passDefinitions->intPassIndexFromType(intPassType)) *= renderedColor;
	
	return renderedColor;
}

colorA_t colorPasses_t::probe_mult(const intPassTypes_t& intPassType, const colorPasses_t& colorPasses, const bool& condition /*= true */)
{
	if(condition && enabled(intPassType) && colorPasses.enabled(intPassType))
	{
		int intPassIndex = passDefinitions->intPassIndexFromType(intPassType);
		colVector.at(intPassIndex) *= colorPasses.colVector.at(intPassIndex);	
		return colorPasses.colVector.at(intPassIndex);
	}
	else return colorA_t(0.f);
}

colorPasses_t & colorPasses_t::operator *= (float f)
{
	for(auto it = colVector.begin(); it != colVector.end(); ++it)
	{
		*it *= f;
	}
	return *this;
}

colorPasses_t & colorPasses_t::operator *= (const color_t &a)
{
	for(auto it = colVector.begin(); it != colVector.end(); ++it)
	{
		*it *= a;
	}
	return *this;
}

colorPasses_t & colorPasses_t::operator *= (const colorA_t &a)
{
	for(auto it = colVector.begin(); it != colVector.end(); ++it)
	{
		*it *= a;
	}
	return *this;
}

colorPasses_t & colorPasses_t::operator += (const colorPasses_t &a)
{
	for(auto it = colVector.begin(); it != colVector.end(); ++it)
	{
		*it += a.colVector.at(it - colVector.begin());
	}
	return *this;
}

float colorPasses_t::get_pass_mask_obj_index() const { return passDefinitions->pass_mask_obj_index; }
float colorPasses_t::get_pass_mask_mat_index() const { return passDefinitions->pass_mask_mat_index; }
bool colorPasses_t::get_pass_mask_invert() const { return passDefinitions->pass_mask_invert; }
bool colorPasses_t::get_pass_mask_only() const { return passDefinitions->pass_mask_only; }



int colorPasses_t::size() const { return colVector.size(); }


__END_YAFRAY
