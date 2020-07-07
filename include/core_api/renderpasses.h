#pragma once
/****************************************************************************
 *
 * 		renderpasses.h: Render Passes functionality
 *      This is part of the yafray package
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

#ifndef Y_RENDERPASSES_H
#define Y_RENDERPASSES_H

#include <yafray_constants.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>

__BEGIN_YAFRAY

class color_t;
class colorA_t;

enum extPassTypes_t : int
{
	PASS_EXT_DISABLED				=	-1,
	PASS_EXT_COMBINED				=	0,
	PASS_EXT_Z_DEPTH,				//From here, specific ext.passes for Blender Exporter
	PASS_EXT_VECTOR,
	PASS_EXT_NORMAL,
	PASS_EXT_UV,
	PASS_EXT_COLOR,
	PASS_EXT_EMIT,
	PASS_EXT_MIST,
	PASS_EXT_DIFFUSE,
	PASS_EXT_SPECULAR,
	PASS_EXT_AO,
	PASS_EXT_ENV,
	PASS_EXT_INDIRECT,
	PASS_EXT_SHADOW,
	PASS_EXT_REFLECT,
	PASS_EXT_REFRACT,
	PASS_EXT_OBJ_INDEX,
	PASS_EXT_MAT_INDEX,
	PASS_EXT_DIFFUSE_DIRECT,
	PASS_EXT_DIFFUSE_INDIRECT,
	PASS_EXT_DIFFUSE_COLOR,
	PASS_EXT_GLOSSY_DIRECT,
	PASS_EXT_GLOSSY_INDIRECT,
	PASS_EXT_GLOSSY_COLOR,
	PASS_EXT_TRANS_DIRECT,
	PASS_EXT_TRANS_INDIRECT,
	PASS_EXT_TRANS_COLOR,
	PASS_EXT_SUBSURFACE_DIRECT,
	PASS_EXT_SUBSURFACE_INDIRECT,
	PASS_EXT_SUBSURFACE_COLOR,
	PASS_EXT_1,						//From here, generic ext.passes for other exporters and plugins
	PASS_EXT_2,
	PASS_EXT_3,
	PASS_EXT_4,
	PASS_EXT_5,
	PASS_EXT_6,
	PASS_EXT_7,
	PASS_EXT_8,
	PASS_EXT_9,
	PASS_EXT_10,
	PASS_EXT_11,
	PASS_EXT_12,
	PASS_EXT_13,
	PASS_EXT_14,
	PASS_EXT_15,
	PASS_EXT_16,
	PASS_EXT_17,
	PASS_EXT_18,
	PASS_EXT_19,
	PASS_EXT_20,
	PASS_EXT_21,
	PASS_EXT_22,
	PASS_EXT_23,
	PASS_EXT_24,
	PASS_EXT_25,
	PASS_EXT_26,
	PASS_EXT_27,
	PASS_EXT_28,
	PASS_EXT_29,
	PASS_EXT_30,
	PASS_EXT_31,
	PASS_EXT_32,
	PASS_EXT_TOTAL_PASSES			//IMPORTANT: KEEP THIS ALWAYS IN THE LAST POSITION
};

enum externalPassTileTypes_t : int
{
	PASS_EXT_TILE_1_GRAYSCALE		=	 1,
	PASS_EXT_TILE_3_RGB				=	 3,
	PASS_EXT_TILE_4_RGBA			=	 4
};

enum intPassTypes_t : int
{
	PASS_INT_DISABLED				=	-1,
	PASS_INT_COMBINED				=	0,
	PASS_INT_Z_DEPTH_NORM,
	PASS_INT_Z_DEPTH_ABS,
	PASS_INT_NORMAL_SMOOTH,
	PASS_INT_NORMAL_GEOM,
	PASS_INT_UV,
	PASS_INT_RADIANCE,
	PASS_INT_EMIT,
	PASS_INT_DIFFUSE,
	PASS_INT_DIFFUSE_NO_SHADOW,
	PASS_INT_AO,
	PASS_INT_AO_CLAY,
	PASS_INT_ENV,
	PASS_INT_MIST,
	PASS_INT_INDIRECT,
	PASS_INT_INDIRECT_ALL,
	PASS_INT_SHADOW,
	PASS_INT_REFLECT_PERFECT,
	PASS_INT_REFRACT_PERFECT,
	PASS_INT_REFLECT_ALL,
	PASS_INT_REFRACT_ALL,
	PASS_INT_OBJ_INDEX_ABS,
	PASS_INT_OBJ_INDEX_NORM,
	PASS_INT_OBJ_INDEX_AUTO,
	PASS_INT_OBJ_INDEX_AUTO_ABS,
	PASS_INT_MAT_INDEX_ABS,
	PASS_INT_MAT_INDEX_NORM,
	PASS_INT_MAT_INDEX_AUTO,
	PASS_INT_MAT_INDEX_AUTO_ABS,
	PASS_INT_OBJ_INDEX_MASK,
	PASS_INT_OBJ_INDEX_MASK_SHADOW,
	PASS_INT_OBJ_INDEX_MASK_ALL,
	PASS_INT_MAT_INDEX_MASK,
	PASS_INT_MAT_INDEX_MASK_SHADOW,
	PASS_INT_MAT_INDEX_MASK_ALL,
	PASS_INT_DIFFUSE_INDIRECT,
	PASS_INT_DIFFUSE_COLOR,
	PASS_INT_GLOSSY,
	PASS_INT_GLOSSY_INDIRECT,
	PASS_INT_GLOSSY_COLOR,
	PASS_INT_TRANS,
	PASS_INT_TRANS_INDIRECT,
	PASS_INT_TRANS_COLOR,
	PASS_INT_SUBSURFACE,
	PASS_INT_SUBSURFACE_INDIRECT,
	PASS_INT_SUBSURFACE_COLOR,
	PASS_INT_SURFACE_INTEGRATION,
	PASS_INT_VOLUME_INTEGRATION,
	PASS_INT_VOLUME_TRANSMITTANCE,
	PASS_INT_DEBUG_NU,
	PASS_INT_DEBUG_NV,
	PASS_INT_DEBUG_DPDU,
	PASS_INT_DEBUG_DPDV,
	PASS_INT_DEBUG_DSDU,
	PASS_INT_DEBUG_DSDV,
	PASS_INT_AA_SAMPLES,
	PASS_INT_DEBUG_LIGHT_ESTIMATION_LIGHT_DIRAC,
	PASS_INT_DEBUG_LIGHT_ESTIMATION_LIGHT_SAMPLING,
	PASS_INT_DEBUG_LIGHT_ESTIMATION_MAT_SAMPLING,
	PASS_INT_DEBUG_WIREFRAME,
	PASS_INT_DEBUG_FACES_EDGES,
	PASS_INT_DEBUG_OBJECTS_EDGES,
	PASS_INT_TOON,
	PASS_INT_DEBUG_SAMPLING_FACTOR,
	PASS_INT_DEBUG_DP_LENGTHS,
	PASS_INT_DEBUG_DPDX,
	PASS_INT_DEBUG_DPDY,
	PASS_INT_DEBUG_DPDXY,
	PASS_INT_DEBUG_DUDX_DVDX,
	PASS_INT_DEBUG_DUDY_DVDY,
	PASS_INT_DEBUG_DUDXY_DVDXY,
	PASS_INT_TOTAL_PASSES			//IMPORTANT: KEEP THIS ALWAYS IN THE LAST POSITION
};



class YAFRAYCORE_EXPORT extPass_t  //Render pass to be exported, for example, to Blender, and mapping to the internal YafaRay render passes generated in different points of the rendering process
{
	public:
		extPass_t(extPassTypes_t extPassType, intPassTypes_t intPassType);
		extPassTypes_t extPassType;
		externalPassTileTypes_t tileType;
		intPassTypes_t intPassType;
};


class YAFRAYCORE_EXPORT auxPass_t  //Render pass to be used internally only, without exporting to images/Blender and mapping to the internal YafaRay render passes generated in different points of the rendering process
{
	public:
		auxPass_t(intPassTypes_t intPassType);
		intPassTypes_t intPassType;
};


class YAFRAYCORE_EXPORT renderPasses_t
{
	friend class colorPasses_t;

	public:
		renderPasses_t();
		int extPassesSize() const;
		int auxPassesSize() const;
		int intPassesSize() const;
		void generate_pass_maps();	//Generate text strings <-> pass type maps
		bool pass_enabled(intPassTypes_t intPassType) const { return indexIntPasses[intPassType] != PASS_INT_DISABLED; }
		void extPass_add(const std::string& sExternalPass, const std::string& sInternalPass);	//Adds a new External Pass associated to an internal pass. Strings are used as parameters and they must match the strings in the maps generated by generate_pass_maps()
		void auxPass_add(intPassTypes_t intPassType);	//Adds a new Auxiliary Pass associated to an internal pass. Strings are used as parameters and they must match the strings in the maps generated by generate_pass_maps()
		void intPass_add(intPassTypes_t intPassType);
		void auxPasses_generate();

        extPassTypes_t extPassTypeFromIndex(int extPassIndex) const;
        intPassTypes_t intPassTypeFromIndex(int intPassIndex) const;
		std::string extPassTypeStringFromIndex(int extPassIndex) const;
		std::string extPassTypeStringFromType(extPassTypes_t extPassType) const;
		std::string intPassTypeStringFromType(intPassTypes_t intPassType) const;
		extPassTypes_t extPassTypeFromString(std::string extPassTypeString) const;
		intPassTypes_t intPassTypeFromString(std::string intPassTypeString) const;
		int extPassIndexFromType(extPassTypes_t extPassType) const;
		int intPassIndexFromType(intPassTypes_t intPassType) const;
        intPassTypes_t intPassTypeFromExtPassIndex(int extPassIndex) const;
        intPassTypes_t intPassTypeFromAuxPassIndex(int auxPassIndex) const;
        externalPassTileTypes_t tileType(int extPassIndex) const;

		void set_pass_mask_obj_index(float new_obj_index);	//Object Index used for masking in/out in the Mask Render Passes
		void set_pass_mask_mat_index(float new_mat_index);	//Material Index used for masking in/out in the Mask Render Passes
		void set_pass_mask_invert(bool mask_invert);	//False=mask in, True=mask out
		void set_pass_mask_only(bool mask_only);
        
		std::map<extPassTypes_t, std::string> extPassMapIntString; //Map int-string for external passes
		std::map<std::string, extPassTypes_t> extPassMapStringInt; //Reverse map string-int for external passes
		std::map<intPassTypes_t, std::string> intPassMapIntString; //Map int-string for internal passes
		std::map<std::string, intPassTypes_t> intPassMapStringInt; //Reverse map string-int for internal passes
		std::vector<std::string> view_names;	//Render Views names
        
		//Options for Edge detection and Toon Render Pass
		std::vector<float> toonEdgeColor = std::vector<float> (3, 0.f);	//Color of the edges used in the Toon Render Pass
		int objectEdgeThickness = 2;		//Thickness of the edges used in the Object Edge and Toon Render Passes
		float objectEdgeThreshold = 0.3f;	//Threshold for the edge detection process used in the Object Edge and Toon Render Passes
		float objectEdgeSmoothness = 0.75f;	//Smoothness (blur) of the edges used in the Object Edge and Toon Render Passes
		float toonPreSmooth = 3.f;      //Toon effect: smoothness applied to the original image
		float toonQuantization = 0.1f;      //Toon effect: color Quantization applied to the original image
		float toonPostSmooth = 3.f;      //Toon effect: smoothness applied after Quantization		

		int facesEdgeThickness = 1;		//Thickness of the edges used in the Faces Edge Render Pass
		float facesEdgeThreshold = 0.01f;	//Threshold for the edge detection process used in the Faces Edge Render Pass
		float facesEdgeSmoothness = 0.5f;	//Smoothness (blur) of the edges used in the Faces Edge Render Pass

    protected:
		std::vector<extPass_t> extPasses;		//List of the external Render passes to be exported
		std::vector<auxPass_t> auxPasses;		//List of the intermediate auxiliary Render passes used for other operations
		std::vector<intPassTypes_t> intPasses;		//List of the internal passes to be generated by the YafaRay engine
		std::vector<int> indexExtPasses;	//List with all possible external passes and to which pass index are they mapped. -1 = pass disabled
		std::vector<int> indexIntPasses;	//List with all possible internal passes and to which pass index are they mapped. -1 = pass disabled

		float pass_mask_obj_index;	//Object Index used for masking in/out in the Mask Render Passes
		float pass_mask_mat_index;	//Material Index used for masking in/out in the Mask Render Passes
		bool pass_mask_invert;	//False=mask in, True=mask out
		bool pass_mask_only;	//False=rendered image is masked, True=only the mask is shown without rendered image		
};


class YAFRAYCORE_EXPORT colorPasses_t  //Internal YafaRay color passes generated in different points of the rendering process
{
	public:
		colorPasses_t(const renderPasses_t *renderPasses);
		int size() const;
		bool enabled(intPassTypes_t intPassType) const;
		intPassTypes_t intPassTypeFromIndex(int intPassIndex) const;
		colorA_t& color(int intPassIndex);
		colorA_t& color(intPassTypes_t intPassType);
		colorA_t& operator()(int intPassIndex);
		colorA_t& operator()(intPassTypes_t intPassType);
		void reset_colors();
		colorA_t init_color(intPassTypes_t intPassType);
		void multiply_colors(float factor);
		colorA_t probe_set(const intPassTypes_t& intPassType, const colorA_t& renderedColor, const bool& condition = true);
		colorA_t probe_set(const intPassTypes_t& intPassType, const colorPasses_t& colorPasses, const bool& condition = true);
		colorA_t probe_add(const intPassTypes_t& intPassType, const colorA_t& renderedColor, const bool& condition = true);
		colorA_t probe_add(const intPassTypes_t& intPassType, const colorPasses_t& colorPasses, const bool& condition = true);
		colorA_t probe_mult(const intPassTypes_t& intPassType, const colorA_t& renderedColor, const bool& condition = true);
		colorA_t probe_mult(const intPassTypes_t& intPassType, const colorPasses_t& colorPasses, const bool& condition = true);
		
		colorPasses_t & operator *= (float f);
		colorPasses_t & operator *= (const color_t &a);
		colorPasses_t & operator *= (const colorA_t &a);
		colorPasses_t & operator += (const colorPasses_t &a);

		float get_pass_mask_obj_index() const;	//Object Index used for masking in/out in the Mask Render Passes
		float get_pass_mask_mat_index() const;	//Material Index used for masking in/out in the Mask Render Passes
		bool get_pass_mask_invert() const;	//False=mask in, True=mask out
		bool get_pass_mask_only() const;
    
    protected:
		std::vector <colorA_t> colVector;
		const renderPasses_t *passDefinitions;
};

__END_YAFRAY

#endif // Y_RENDERPASSES_H
