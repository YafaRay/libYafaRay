/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software,  you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation,  either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY,  without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library,  if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "common/layer_definitions.h"

namespace yafaray {

// CRITICAL: Make sure the entries below are entered EXACTLY in the same order as in the Enum!!!!
// Each actual array index must match the integer value of the associated Type, otherwise a bad mismatch will happen between the expected layer definition and the actual layer index being used!
//Default initialization color in general is black/opaque, except for SHADOW and MASK passes where the default is black/transparent for easier masking

const std::array<LayerDef, LayerDef::Type::Size> LayerDef::definitions_array_ {{
	{Type::Combined, "combined", LayerDef::Flags::None, Image::Type::ColorAlpha}, //Combined should always be the first entry
	{Type::AaSamples, "debug-aa-samples", LayerDef::Flags::None, Image::Type::Color, {0.f, 1.f}, false},
	{Type::Ao, "ao", LayerDef::Flags::BasicLayers | LayerDef::Flags::AoLayers},
	{Type::AoClay, "ao-clay", LayerDef::Flags::BasicLayers | LayerDef::Flags::AoLayers, Image::Type::Gray},
	{Type::BarycentricUvw, "debug-barycentric-uvw", LayerDef::Flags::DebugLayers},
	{Type::DebugDpLengths, "debug-dp-lengths", LayerDef::Flags::DebugLayers},
	{Type::DebugDpdu, "debug-dpdu", LayerDef::Flags::DebugLayers},
	{Type::DebugDpdv, "debug-dpdv", LayerDef::Flags::DebugLayers},
	{Type::DebugDpdx, "debug-dpdx", LayerDef::Flags::DebugLayers},
	{Type::DebugDpdxy, "debug-dpdxy", LayerDef::Flags::DebugLayers},
	{Type::DebugDpdy, "debug-dpdy", LayerDef::Flags::DebugLayers},
	{Type::DebugDsdu, "debug-dsdu", LayerDef::Flags::DebugLayers},
	{Type::DebugDsdv, "debug-dsdv", LayerDef::Flags::DebugLayers},
	{Type::DebugDudxDvdx, "debug-dudx-dvdx", LayerDef::Flags::DebugLayers},
	{Type::DebugDudxyDvdxy, "debug-dudxy-dvdxy", LayerDef::Flags::DebugLayers},
	{Type::DebugDudyDvdy, "debug-dudy-dvdy", LayerDef::Flags::DebugLayers},
	{Type::DebugFacesEdges, "debug-faces-edges", LayerDef::Flags::DebugLayers | LayerDef::Flags::ToonEdgeLayers},
	{Type::DebugLightEstimationLightDirac, "debug-light-estimation-light-dirac", LayerDef::Flags::DebugLayers},
	{Type::DebugLightEstimationLightSampling, "debug-light-estimation-light-sampling", LayerDef::Flags::DebugLayers},
	{Type::DebugLightEstimationMatSampling, "debug-light-estimation-mat-sampling", LayerDef::Flags::DebugLayers},
	{Type::DebugNu, "debug-nu", LayerDef::Flags::DebugLayers},
	{Type::DebugNv, "debug-nv", LayerDef::Flags::DebugLayers},
	{Type::DebugObjectsEdges,"debug-objects-edges", LayerDef::Flags::DebugLayers | LayerDef::Flags::ToonEdgeLayers},
	{Type::DebugSamplingFactor, "debug-sampling-factor", LayerDef::Flags::DebugLayers, Image::Type::Color, {0.f, 1.f}, false},
	{Type::DebugWireframe, "debug-wireframe", LayerDef::Flags::DebugLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::DebugObjectTime, "debug-object-time", LayerDef::Flags::DebugLayers, Image::Type::Color, {0.f, 1.f}},
	{Type::Diffuse, "diffuse", LayerDef::Flags::BasicLayers | LayerDef::Flags::DiffuseLayers},
	{Type::DiffuseColor, "adv-diffuse-color", LayerDef::Flags::BasicLayers | LayerDef::Flags::DiffuseLayers},
	{Type::DiffuseIndirect, "adv-diffuse-indirect", LayerDef::Flags::BasicLayers | LayerDef::Flags::DiffuseLayers},
	{Type::DiffuseNoShadow, "diffuse-noshadow", LayerDef::Flags::BasicLayers | LayerDef::Flags::DiffuseLayers},
	{Type::Disabled, "disabled", LayerDef::Flags::None},
	{Type::Emit, "emit", LayerDef::Flags::BasicLayers},
	{Type::Env, "env", LayerDef::Flags::BasicLayers},
	{Type::Glossy, "adv-glossy", LayerDef::Flags::BasicLayers},
	{Type::GlossyColor, "adv-glossy-color", LayerDef::Flags::BasicLayers},
	{Type::GlossyIndirect, "adv-glossy-indirect", LayerDef::Flags::BasicLayers},
	{Type::Indirect, "adv-indirect", LayerDef::Flags::BasicLayers},
	{Type::IndirectAll, "indirect", LayerDef::Flags::BasicLayers},
	{Type::MatIndexAbs, "mat-index-abs", LayerDef::Flags::IndexLayers, Image::Type::Gray, {0.f, 1.f}, false},
	{Type::MatIndexAuto, "mat-index-auto", LayerDef::Flags::IndexLayers},
	{Type::MatIndexAutoAbs, "mat-index-auto-abs", LayerDef::Flags::IndexLayers, Image::Type::Color, {0.f, 1.f}, false},
	{Type::MatIndexMask, "mat-index-mask", LayerDef::Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::MatIndexMaskAll, "mat-index-mask-all", LayerDef::Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::MatIndexMaskShadow, "mat-index-mask-shadow", LayerDef::Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::MatIndexNorm, "mat-index-norm", LayerDef::Flags::IndexLayers, Image::Type::Gray},
	{Type::Mist, "mist", LayerDef::Flags::DepthLayers, Image::Type::GrayAlpha},
	{Type::NormalGeom, "debug-normal-geom", LayerDef::Flags::DebugLayers},
	{Type::NormalSmooth, "debug-normal-smooth", LayerDef::Flags::DebugLayers},
	{Type::ObjIndexAbs, "obj-index-abs", LayerDef::Flags::IndexLayers, Image::Type::Gray, {0.f, 1.f}, false},
	{Type::ObjIndexAuto, "obj-index-auto", LayerDef::Flags::IndexLayers},
	{Type::ObjIndexAutoAbs, "obj-index-auto-abs", LayerDef::Flags::IndexLayers, Image::Type::Color, {0.f, 1.f}, false},
	{Type::ObjIndexMask, "obj-index-mask", LayerDef::Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::ObjIndexMaskAll, "obj-index-mask-all", LayerDef::Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::ObjIndexMaskShadow, "obj-index-mask-shadow", LayerDef::Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::ObjIndexNorm, "obj-index-norm", LayerDef::Flags::IndexLayers, Image::Type::Gray},
	{Type::Radiance, "adv-radiance", LayerDef::Flags::BasicLayers},
	{Type::ReflectAll, "reflect", LayerDef::Flags::BasicLayers},
	{Type::ReflectPerfect, "adv-reflect", LayerDef::Flags::BasicLayers},
	{Type::RefractAll, "refract", LayerDef::Flags::BasicLayers},
	{Type::RefractPerfect, "adv-refract", LayerDef::Flags::BasicLayers},
	{Type::Shadow, "shadow", LayerDef::Flags::BasicLayers, Image::Type::GrayAlpha, {0.f, 0.f}},
	{Type::Subsurface, "adv-subsurface", LayerDef::Flags::BasicLayers},
	{Type::SubsurfaceColor, "adv-subsurface-color", LayerDef::Flags::BasicLayers},
	{Type::SubsurfaceIndirect, "adv-subsurface-indirect", LayerDef::Flags::BasicLayers},
	{Type::SurfaceIntegration, "adv-surface-integration", LayerDef::Flags::BasicLayers},
	{Type::Toon,"toon", LayerDef::Flags::BasicLayers | LayerDef::Flags::ToonEdgeLayers},
	{Type::Trans, "adv-trans", LayerDef::Flags::BasicLayers},
	{Type::TransColor, "adv-trans-color", LayerDef::Flags::BasicLayers},
	{Type::TransIndirect, "adv-trans-indirect", LayerDef::Flags::BasicLayers},
	{Type::Uv, "debug-uv", LayerDef::Flags::DebugLayers},
	{Type::VolumeIntegration, "adv-volume-integration", LayerDef::Flags::BasicLayers},
	{Type::VolumeTransmittance, "adv-volume-transmittance", LayerDef::Flags::BasicLayers, Image::Type::Gray},
	{Type::ZDepthAbs, "z-depth-abs", LayerDef::Flags::DepthLayers, Image::Type::GrayAlpha, {0.f, 1.f}, false},
	{Type::ZDepthNorm, "z-depth-norm", LayerDef::Flags::DepthLayers, Image::Type::GrayAlpha},
}};

const std::map<std::string, LayerDef::Type> LayerDef::layer_name_map_ = initLayerNameMap();

std::map<std::string, LayerDef::Type> LayerDef::initLayerNameMap()
{
	std::map<std::string, Type> layer_name_map;
	for(int i = Type::Combined; i < Type::Size; ++i)
	{
		const Type type = static_cast<Type>(i);
		layer_name_map[getName(type)] = type;
	}
	return layer_name_map;
}

LayerDef::Type LayerDef::getType(const std::string &name)
{
	if(name == "disabled" || name == "unknown" || name.empty()) return Disabled;
	const auto it = layer_name_map_.find(name);
	if(it == layer_name_map_.end()) return Disabled;
	else return it->second;
}


} //namespace yafaray