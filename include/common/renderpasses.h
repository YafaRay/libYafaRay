#pragma once
/****************************************************************************
 *
 *      renderpasses.h: Render Passes functionality
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
 *
 */

#ifndef YAFARAY_RENDERPASSES_H
#define YAFARAY_RENDERPASSES_H

#include "constants.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>

BEGIN_YAFARAY

class Rgb;
class Rgba;

enum ExtPassTypes : int
{
	PassExtDisabled				=	-1,
	PassExtCombined				=	0,
	PassExtZDepth,				//From here, specific ext.passes for Blender Exporter
	PassExtVector,
	PassExtNormal,
	PassExtUv,
	PassExtColor,
	PassExtEmit,
	PassExtMist,
	PassExtDiffuse,
	PassExtSpecular,
	PassExtAo,
	PassExtEnv,
	PassExtIndirect,
	PassExtShadow,
	PassExtReflect,
	PassExtRefract,
	PassExtObjIndex,
	PassExtMatIndex,
	PassExtDiffuseDirect,
	PassExtDiffuseIndirect,
	PassExtDiffuseColor,
	PassExtGlossyDirect,
	PassExtGlossyIndirect,
	PassExtGlossyColor,
	PassExtTransDirect,
	PassExtTransIndirect,
	PassExtTransColor,
	PassExtSubsurfaceDirect,
	PassExtSubsurfaceIndirect,
	PassExtSubsurfaceColor,
	PassExt1,						//From here, generic ext.passes for other exporters and plugins
	PassExt2,
	PassExt3,
	PassExt4,
	PassExt5,
	PassExt6,
	PassExt7,
	PassExt8,
	PassExt9,
	PassExt10,
	PassExt11,
	PassExt12,
	PassExt13,
	PassExt14,
	PassExt15,
	PassExt16,
	PassExt17,
	PassExt18,
	PassExt19,
	PassExt20,
	PassExt21,
	PassExt22,
	PassExt23,
	PassExt24,
	PassExt25,
	PassExt26,
	PassExt27,
	PassExt28,
	PassExt29,
	PassExt30,
	PassExt31,
	PassExt32,
	PassExtTotalPasses			//IMPORTANT: KEEP THIS ALWAYS IN THE LAST POSITION
};

enum ExternalPassTileTypes : int
{
	PassExtTile1Grayscale		=	 1,
	PassExtTile3Rgb				=	 3,
	PassExtTile4Rgba			=	 4
};

enum IntPassTypes : int
{
	PassIntDisabled				=	-1,
	PassIntCombined				=	0,
	PassIntZDepthNorm,
	PassIntZDepthAbs,
	PassIntNormalSmooth,
	PassIntNormalGeom,
	PassIntUv,
	PassIntRadiance,
	PassIntEmit,
	PassIntDiffuse,
	PassIntDiffuseNoShadow,
	PassIntAo,
	PassIntAoClay,
	PassIntEnv,
	PassIntMist,
	PassIntIndirect,
	PassIntIndirectAll,
	PassIntShadow,
	PassIntReflectPerfect,
	PassIntRefractPerfect,
	PassIntReflectAll,
	PassIntRefractAll,
	PassIntObjIndexAbs,
	PassIntObjIndexNorm,
	PassIntObjIndexAuto,
	PassIntObjIndexAutoAbs,
	PassIntMatIndexAbs,
	PassIntMatIndexNorm,
	PassIntMatIndexAuto,
	PassIntMatIndexAutoAbs,
	PassIntObjIndexMask,
	PassIntObjIndexMaskShadow,
	PassIntObjIndexMaskAll,
	PassIntMatIndexMask,
	PassIntMatIndexMaskShadow,
	PassIntMatIndexMaskAll,
	PassIntDiffuseIndirect,
	PassIntDiffuseColor,
	PassIntGlossy,
	PassIntGlossyIndirect,
	PassIntGlossyColor,
	PassIntTrans,
	PassIntTransIndirect,
	PassIntTransColor,
	PassIntSubsurface,
	PassIntSubsurfaceIndirect,
	PassIntSubsurfaceColor,
	PassIntSurfaceIntegration,
	PassIntVolumeIntegration,
	PassIntVolumeTransmittance,
	PassIntDebugNu,
	PassIntDebugNv,
	PassIntDebugDpdu,
	PassIntDebugDpdv,
	PassIntDebugDsdu,
	PassIntDebugDsdv,
	PassIntAaSamples,
	PassIntDebugLightEstimationLightDirac,
	PassIntDebugLightEstimationLightSampling,
	PassIntDebugLightEstimationMatSampling,
	PassIntDebugWireframe,
	PassIntDebugFacesEdges,
	PassIntDebugObjectsEdges,
	PassIntToon,
	PassIntDebugSamplingFactor,
	PassIntDebugDpLengths,
	PassIntDebugDpdx,
	PassIntDebugDpdy,
	PassIntDebugDpdxy,
	PassIntDebugDudxDvdx,
	PassIntDebugDudyDvdy,
	PassIntDebugDudxyDvdxy,
	PassIntTotalPasses			//IMPORTANT: KEEP THIS ALWAYS IN THE LAST POSITION
};



class ExtPass  //Render pass to be exported, for example, to Blender, and mapping to the internal YafaRay render passes generated in different points of the rendering process
{
	public:
		ExtPass(ExtPassTypes ext_pass_type, IntPassTypes int_pass_type);
		ExtPassTypes ext_pass_type_;
		ExternalPassTileTypes tile_type_;
		IntPassTypes int_pass_type_;
};


class AuxPass  //Render pass to be used internally only, without exporting to images/Blender and mapping to the internal YafaRay render passes generated in different points of the rendering process
{
	public:
		AuxPass(IntPassTypes int_pass_type);
		IntPassTypes int_pass_type_;
};


class RenderPasses
{
		friend class ColorPasses;

	public:
		RenderPasses();
		int extPassesSize() const;
		int auxPassesSize() const;
		int intPassesSize() const;
		void generatePassMaps();	//Generate text strings <-> pass type maps
		bool passEnabled(IntPassTypes int_pass_type) const { return index_int_passes_[int_pass_type] != PassIntDisabled; }
		void extPassAdd(const std::string &s_external_pass, const std::string &s_internal_pass);	//Adds a new External Pass associated to an internal pass. Strings are used as parameters and they must match the strings in the maps generated by generate_pass_maps()
		void auxPassAdd(IntPassTypes int_pass_type);	//Adds a new Auxiliary Pass associated to an internal pass. Strings are used as parameters and they must match the strings in the maps generated by generate_pass_maps()
		void intPassAdd(IntPassTypes int_pass_type);
		void auxPassesGenerate();

		ExtPassTypes extPassTypeFromIndex(int ext_pass_index) const;
		IntPassTypes intPassTypeFromIndex(int int_pass_index) const;
		std::string extPassTypeStringFromIndex(int ext_pass_index) const;
		std::string extPassTypeStringFromType(ExtPassTypes ext_pass_type) const;
		std::string intPassTypeStringFromType(IntPassTypes int_pass_type) const;
		ExtPassTypes extPassTypeFromString(std::string ext_pass_type_string) const;
		IntPassTypes intPassTypeFromString(std::string int_pass_type_string) const;
		int extPassIndexFromType(ExtPassTypes ext_pass_type) const;
		int intPassIndexFromType(IntPassTypes int_pass_type) const;
		IntPassTypes intPassTypeFromExtPassIndex(int ext_pass_index) const;
		IntPassTypes intPassTypeFromAuxPassIndex(int aux_pass_index) const;
		ExternalPassTileTypes tileType(int ext_pass_index) const;

		void setPassMaskObjIndex(float new_obj_index);	//Object Index used for masking in/out in the Mask Render Passes
		void setPassMaskMatIndex(float new_mat_index);	//Material Index used for masking in/out in the Mask Render Passes
		void setPassMaskInvert(bool mask_invert);	//False=mask in, True=mask out
		void setPassMaskOnly(bool mask_only);

		std::map<ExtPassTypes, std::string> ext_pass_map_int_string_; //Map int-string for external passes
		std::map<std::string, ExtPassTypes> ext_pass_map_string_int_; //Reverse map string-int for external passes
		std::map<IntPassTypes, std::string> int_pass_map_int_string_; //Map int-string for internal passes
		std::map<std::string, IntPassTypes> int_pass_map_string_int_; //Reverse map string-int for internal passes
		std::vector<std::string> view_names_;	//Render Views names

		//Options for Edge detection and Toon Render Pass
		std::vector<float> toon_edge_color_ = std::vector<float> (3, 0.f);	//Color of the edges used in the Toon Render Pass
		int object_edge_thickness_ = 2;		//Thickness of the edges used in the Object Edge and Toon Render Passes
		float object_edge_threshold_ = 0.3f;	//Threshold for the edge detection process used in the Object Edge and Toon Render Passes
		float object_edge_smoothness_ = 0.75f;	//Smoothness (blur) of the edges used in the Object Edge and Toon Render Passes
		float toon_pre_smooth_ = 3.f;      //Toon effect: smoothness applied to the original image
		float toon_quantization_ = 0.1f;      //Toon effect: color Quantization applied to the original image
		float toon_post_smooth_ = 3.f;      //Toon effect: smoothness applied after Quantization

		int faces_edge_thickness_ = 1;		//Thickness of the edges used in the Faces Edge Render Pass
		float faces_edge_threshold_ = 0.01f;	//Threshold for the edge detection process used in the Faces Edge Render Pass
		float faces_edge_smoothness_ = 0.5f;	//Smoothness (blur) of the edges used in the Faces Edge Render Pass

	protected:
		std::vector<ExtPass> ext_passes_;		//List of the external Render passes to be exported
		std::vector<AuxPass> aux_passes_;		//List of the intermediate auxiliary Render passes used for other operations
		std::vector<IntPassTypes> int_passes_;		//List of the internal passes to be generated by the YafaRay engine
		std::vector<int> index_ext_passes_;	//List with all possible external passes and to which pass index are they mapped. -1 = pass disabled
		std::vector<int> index_int_passes_;	//List with all possible internal passes and to which pass index are they mapped. -1 = pass disabled

		float pass_mask_obj_index_;	//Object Index used for masking in/out in the Mask Render Passes
		float pass_mask_mat_index_;	//Material Index used for masking in/out in the Mask Render Passes
		bool pass_mask_invert_;	//False=mask in, True=mask out
		bool pass_mask_only_;	//False=rendered image is masked, True=only the mask is shown without rendered image
};


class ColorPasses  //Internal YafaRay color passes generated in different points of the rendering process
{
	public:
		ColorPasses(const RenderPasses *render_passes);
		int size() const;
		bool enabled(IntPassTypes int_pass_type) const;
		IntPassTypes intPassTypeFromIndex(int int_pass_index) const;
		Rgba &color(int int_pass_index);
		Rgba &color(IntPassTypes int_pass_type);
		Rgba &operator()(int int_pass_index);
		Rgba &operator()(IntPassTypes int_pass_type);
		void resetColors();
		Rgba initColor(IntPassTypes int_pass_type);
		void multiplyColors(float factor);
		Rgba probeSet(const IntPassTypes &int_pass_type, const Rgba &rendered_color, const bool &condition = true);
		Rgba probeSet(const IntPassTypes &int_pass_type, const ColorPasses &color_passes, const bool &condition = true);
		Rgba probeAdd(const IntPassTypes &int_pass_type, const Rgba &rendered_color, const bool &condition = true);
		Rgba probeAdd(const IntPassTypes &int_pass_type, const ColorPasses &color_passes, const bool &condition = true);
		Rgba probeMult(const IntPassTypes &int_pass_type, const Rgba &rendered_color, const bool &condition = true);
		Rgba probeMult(const IntPassTypes &int_pass_type, const ColorPasses &color_passes, const bool &condition = true);

		ColorPasses &operator *= (float f);
		ColorPasses &operator *= (const Rgb &a);
		ColorPasses &operator *= (const Rgba &a);
		ColorPasses &operator += (const ColorPasses &a);

		float getPassMaskObjIndex() const;	//Object Index used for masking in/out in the Mask Render Passes
		float getPassMaskMatIndex() const;	//Material Index used for masking in/out in the Mask Render Passes
		bool getPassMaskInvert() const;	//False=mask in, True=mask out
		bool getPassMaskOnly() const;

	protected:
		std::vector <Rgba> col_vector_;
		const RenderPasses *pass_definitions_;
};

END_YAFARAY

#endif // YAFARAY_RENDERPASSES_H
