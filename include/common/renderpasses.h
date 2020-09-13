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
#include <set>

BEGIN_YAFARAY

class Rgba;

enum PassTypes : int
{
	PassDisabled = -1,
	PassCombined = 0,
	PassZDepthNorm,
	PassZDepthAbs,
	PassNormalSmooth,
	PassNormalGeom,
	PassUv,
	PassRadiance,
	PassEmit,
	PassDiffuse,
	PassDiffuseNoShadow,
	PassAo,
	PassAoClay,
	PassEnv,
	PassMist,
	PassIndirect,
	PassIndirectAll,
	PassShadow,
	PassReflectPerfect,
	PassRefractPerfect,
	PassReflectAll,
	PassRefractAll,
	PassObjIndexAbs,
	PassObjIndexNorm,
	PassObjIndexAuto,
	PassObjIndexAutoAbs,
	PassMatIndexAbs,
	PassMatIndexNorm,
	PassMatIndexAuto,
	PassMatIndexAutoAbs,
	PassObjIndexMask,
	PassObjIndexMaskShadow,
	PassObjIndexMaskAll,
	PassMatIndexMask,
	PassMatIndexMaskShadow,
	PassMatIndexMaskAll,
	PassDiffuseIndirect,
	PassDiffuseColor,
	PassGlossy,
	PassGlossyIndirect,
	PassGlossyColor,
	PassTrans,
	PassTransIndirect,
	PassTransColor,
	PassSubsurface,
	PassSubsurfaceIndirect,
	PassSubsurfaceColor,
	PassSurfaceIntegration,
	PassVolumeIntegration,
	PassVolumeTransmittance,
	PassDebugNu,
	PassDebugNv,
	PassDebugDpdu,
	PassDebugDpdv,
	PassDebugDsdu,
	PassDebugDsdv,
	PassAaSamples,
	PassDebugLightEstimationLightDirac,
	PassDebugLightEstimationLightSampling,
	PassDebugLightEstimationMatSampling,
	PassDebugWireframe,
	PassDebugFacesEdges,
	PassDebugObjectsEdges,
	PassToon,
	PassDebugSamplingFactor,
	PassDebugDpLengths,
	PassDebugDpdx,
	PassDebugDpdy,
	PassDebugDpdxy,
	PassDebugDudxDvdx,
	PassDebugDudyDvdy,
	PassDebugDudxyDvdxy,
};

class IntPassesSettings
{
	public:
		IntPassesSettings();
		bool enabled(const PassTypes &type) const;
		void enable(const PassTypes &type);

		const std::set<PassTypes> &listEnabled() const { return enabled_list_; }
		const std::map<PassTypes, std::string> &listAvailable() const { return map_type_name_; }

		std::string name(const PassTypes &type) const;
		PassTypes type(const std::string &name) const;
		Rgba defaultColor(const PassTypes &type) const;

	protected:
		std::set<PassTypes> enabled_list_; //!List with the enabled internal passes
		std::vector<bool> enabled_bool_; //!Enabled internal passes in bool vector format for performance search
		std::map<PassTypes, std::string> map_type_name_; //!Dictionary available internal passes type->name
		std::map<std::string, PassTypes> map_name_type_; //!Reverse dictionary name->type
};

inline bool IntPassesSettings::enabled(const PassTypes &type) const
{
	if(type == PassCombined) return true;
	else if(type == PassDisabled) return false;
	else if(type >= (int) enabled_bool_.size()) return false;
	else return enabled_bool_.at(type);
}


class IntPasses //Internal YafaRay color passes generated in different points of the rendering process
{
	public:
		IntPasses(const IntPassesSettings &settings);
		size_t size() const { return settings().listEnabled().size(); }
		bool enabled(const PassTypes &type) const { return settings().enabled(type); }
		const IntPassesSettings &settings() const { return settings_; }

		std::set<PassTypes>::const_iterator begin() const { return settings().listEnabled().begin(); }
		std::set<PassTypes>::const_iterator end() const { return settings().listEnabled().end(); }

		void setDefaults();
		Rgba &operator()(const PassTypes &type);
		const Rgba &operator()(const PassTypes &type) const;
		Rgba *find(const PassTypes &type);

	protected:
		std::vector <Rgba> passes_;
		const IntPassesSettings &settings_;
};


class ExtPassDefinition  //Render pass to be exported, for example, to Blender, and mapping to the internal YafaRay render passes generated in different points of the rendering process
{
	public:
		ExtPassDefinition(const std::string &name, PassTypes internal_type, int color_components = 4, bool save = true): name_(name), color_components_(color_components), internal_type_(internal_type), save_(save) { }
		std::string name() const { return name_; }
		int colorComponents() const { return color_components_; }
		PassTypes intPassType() const { return internal_type_; }
		bool toSave() const { return save_; }

	protected:
		std::string name_ = "default";
		int color_components_ = 4; //!< Valid values: 1=Grayscale, 3=RGB, 4=RGBA
		PassTypes internal_type_ = PassCombined;
		bool save_ = true; //!< Determine if this external pass should be saved or not (for example in auxiliary external passes that do not need to be saved to disk or exported to an external application)
};

class ExtPassesSettings
{
	public:
		size_t size() const { return passes_.size(); }
		void extPassAdd(const std::string &ext_pass_name, PassTypes int_pass_type, int color_components = 4, bool save = true);
		std::vector<ExtPassDefinition>::const_iterator begin() const { return passes_.begin(); }
		std::vector<ExtPassDefinition>::const_iterator end() const { return passes_.end(); }
		ExtPassDefinition &operator()(size_t index) { return passes_.at(index); }
		const ExtPassDefinition &operator()(size_t index) const { return passes_.at(index); }

	protected:
		std::vector<ExtPassDefinition> passes_; //List of the external Render passes to be exported
};

struct PassMaskParams
{
	float obj_index_ = 0.f; //!Object Index used for masking in/out in the Mask Render Passes
	float mat_index_ = 0.f; //!Material Index used for masking in/out in the Mask Render Passes
	bool invert_ = false; //!False=mask in, True=mask out
	bool only_ = false; //!False=rendered image is masked, True=only the mask is shown without rendered image
};

struct PassEdgeToonParams //Options for Edge detection and Toon Render Pass
{
	int thickness_ = 2; //!Thickness of the edges used in the Object Edge and Toon Render Passes
	float threshold_ = 0.3f; //!Threshold for the edge detection process used in the Object Edge and Toon Render Passes
	float smoothness_ = 0.75f; //!Smoothness (blur) of the edges used in the Object Edge and Toon Render Passes
	std::array<float, 3> toon_color_ = {0.f, 0.f, 0.f}; //!Color of the edges used in the Toon Render Pass.
	//Using array<float, 3> to avoid including color.h header dependency
	float toon_pre_smooth_ = 3.f; //!Toon effect: smoothness applied to the original image
	float toon_quantization_ = 0.1f; //!Toon effect: color Quantization applied to the original image
	float toon_post_smooth_ = 3.f; //!Toon effect: smoothness applied after Quantization
	int face_thickness_ = 1; //!Thickness of the edges used in the Faces Edge Render Pass
	float face_threshold_ = 0.01f; //!Threshold for the edge detection process used in the Faces Edge Render Pass
	float face_smoothness_ = 0.5f; //!Smoothness (blur) of the edges used in the Faces Edge Render Pass
};

class PassesSettings
{
	public:
		PassesSettings();
		void extPassAdd(const std::string &ext_pass_name, const std::string &int_pass_name, int color_components = 4);
		void auxPassAdd(const PassTypes &type);
		void auxPassesGenerate();
		const IntPassesSettings &intPassesSettings() const { return int_passes_settings_; }
		const ExtPassesSettings &extPassesSettings() const { return ext_passes_settings_; }
		const PassMaskParams &passMaskParams() const { return pass_mask_; }
		void setPassMaskParams(const PassMaskParams &mask_params) { pass_mask_ = mask_params; }
		const PassEdgeToonParams &passEdgeToonParams() const { return edge_toon_; }
		void setPassEdgeToonParams(const PassEdgeToonParams &edge_params) { edge_toon_ = edge_params; }

		std::vector<std::string> view_names_; //Render Views names

	protected:
		PassMaskParams pass_mask_;
		PassEdgeToonParams edge_toon_;
		ExtPassesSettings ext_passes_settings_;
		IntPassesSettings int_passes_settings_;
};

END_YAFARAY

#endif // YAFARAY_RENDERPASSES_H
