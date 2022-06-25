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
	{Type::Combined, "combined", Flags::None, Image::Type::ColorAlpha}, //Combined should always be the first entry
	{Type::AaSamples, "debug-aa-samples", Flags::None, Image::Type::Color, {0.f, 1.f}, false},
	{Type::Ao, "ao", Flags::BasicLayers | Flags::AoLayers},
	{Type::AoClay, "ao-clay", Flags::BasicLayers | Flags::AoLayers, Image::Type::Gray},
	{Type::BarycentricUvw, "debug-barycentric-uvw", Flags::DebugLayers},
	{Type::DebugDpLengths, "debug-dp-lengths", Flags::DebugLayers},
	{Type::DebugDpdu, "debug-dpdu", Flags::DebugLayers},
	{Type::DebugDpdv, "debug-dpdv", Flags::DebugLayers},
	{Type::DebugDpdx, "debug-dpdx", Flags::DebugLayers},
	{Type::DebugDpdxy, "debug-dpdxy", Flags::DebugLayers},
	{Type::DebugDpdy, "debug-dpdy", Flags::DebugLayers},
	{Type::DebugDsdu, "debug-dsdu", Flags::DebugLayers},
	{Type::DebugDsdv, "debug-dsdv", Flags::DebugLayers},
	{Type::DebugDudxDvdx, "debug-dudx-dvdx", Flags::DebugLayers},
	{Type::DebugDudxyDvdxy, "debug-dudxy-dvdxy", Flags::DebugLayers},
	{Type::DebugDudyDvdy, "debug-dudy-dvdy", Flags::DebugLayers},
	{Type::DebugFacesEdges, "debug-faces-edges", Flags::DebugLayers | Flags::ToonEdgeLayers},
	{Type::DebugLightEstimationLightDirac, "debug-light-estimation-light-dirac", Flags::DebugLayers},
	{Type::DebugLightEstimationLightSampling, "debug-light-estimation-light-sampling", Flags::DebugLayers},
	{Type::DebugLightEstimationMatSampling, "debug-light-estimation-mat-sampling", Flags::DebugLayers},
	{Type::DebugNu, "debug-nu", Flags::DebugLayers},
	{Type::DebugNv, "debug-nv", Flags::DebugLayers},
	{Type::DebugObjectsEdges,"debug-objects-edges", Flags::DebugLayers | Flags::ToonEdgeLayers},
	{Type::DebugSamplingFactor, "debug-sampling-factor", Flags::DebugLayers, Image::Type::Color, {0.f, 1.f}, false},
	{Type::DebugWireframe, "debug-wireframe", Flags::DebugLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::DebugObjectTime, "debug-object-time", Flags::DebugLayers, Image::Type::Color, {0.f, 1.f}},
	{Type::Diffuse, "diffuse", Flags::BasicLayers | Flags::DiffuseLayers},
	{Type::DiffuseColor, "adv-diffuse-color", Flags::BasicLayers | Flags::DiffuseLayers},
	{Type::DiffuseIndirect, "adv-diffuse-indirect", Flags::BasicLayers | Flags::DiffuseLayers},
	{Type::DiffuseNoShadow, "diffuse-noshadow", Flags::BasicLayers | Flags::DiffuseLayers},
	{Type::Disabled, "disabled", Flags::None},
	{Type::Emit, "emit", Flags::BasicLayers},
	{Type::Env, "env", Flags::BasicLayers},
	{Type::Glossy, "adv-glossy", Flags::BasicLayers},
	{Type::GlossyColor, "adv-glossy-color", Flags::BasicLayers},
	{Type::GlossyIndirect, "adv-glossy-indirect", Flags::BasicLayers},
	{Type::Indirect, "adv-indirect", Flags::BasicLayers},
	{Type::IndirectAll, "indirect", Flags::BasicLayers},
	{Type::MatIndexAbs, "mat-index-abs", Flags::IndexLayers, Image::Type::Gray, {0.f, 1.f}, false},
	{Type::MatIndexAuto, "mat-index-auto", Flags::IndexLayers},
	{Type::MatIndexAutoAbs, "mat-index-auto-abs", Flags::IndexLayers, Image::Type::Color, {0.f, 1.f}, false},
	{Type::MatIndexMask, "mat-index-mask", Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::MatIndexMaskAll, "mat-index-mask-all", Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::MatIndexMaskShadow, "mat-index-mask-shadow", Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::MatIndexNorm, "mat-index-norm", Flags::IndexLayers, Image::Type::Gray},
	{Type::Mist, "mist", Flags::DepthLayers, Image::Type::GrayAlpha},
	{Type::NormalGeom, "debug-normal-geom", Flags::DebugLayers},
	{Type::NormalSmooth, "debug-normal-smooth", Flags::DebugLayers},
	{Type::ObjIndexAbs, "obj-index-abs", Flags::IndexLayers, Image::Type::Gray, {0.f, 1.f}, false},
	{Type::ObjIndexAuto, "obj-index-auto", Flags::IndexLayers},
	{Type::ObjIndexAutoAbs, "obj-index-auto-abs", Flags::IndexLayers, Image::Type::Color, {0.f, 1.f}, false},
	{Type::ObjIndexMask, "obj-index-mask", Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::ObjIndexMaskAll, "obj-index-mask-all", Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::ObjIndexMaskShadow, "obj-index-mask-shadow", Flags::IndexLayers, Image::Type::ColorAlpha, {0.f, 0.f}},
	{Type::ObjIndexNorm, "obj-index-norm", Flags::IndexLayers, Image::Type::Gray},
	{Type::Radiance, "adv-radiance", Flags::BasicLayers},
	{Type::ReflectAll, "reflect", Flags::BasicLayers},
	{Type::ReflectPerfect, "adv-reflect", Flags::BasicLayers},
	{Type::RefractAll, "refract", Flags::BasicLayers},
	{Type::RefractPerfect, "adv-refract", Flags::BasicLayers},
	{Type::Shadow, "shadow", Flags::BasicLayers, Image::Type::GrayAlpha, {0.f, 0.f}},
	{Type::Subsurface, "adv-subsurface", Flags::BasicLayers},
	{Type::SubsurfaceColor, "adv-subsurface-color", Flags::BasicLayers},
	{Type::SubsurfaceIndirect, "adv-subsurface-indirect", Flags::BasicLayers},
	{Type::SurfaceIntegration, "adv-surface-integration", Flags::BasicLayers},
	{Type::Toon,"toon", Flags::BasicLayers | Flags::ToonEdgeLayers},
	{Type::Trans, "adv-trans", Flags::BasicLayers},
	{Type::TransColor, "adv-trans-color", Flags::BasicLayers},
	{Type::TransIndirect, "adv-trans-indirect", Flags::BasicLayers},
	{Type::Uv, "debug-uv", Flags::DebugLayers},
	{Type::VolumeIntegration, "adv-volume-integration", Flags::BasicLayers},
	{Type::VolumeTransmittance, "adv-volume-transmittance", Flags::BasicLayers, Image::Type::Gray},
	{Type::ZDepthAbs, "z-depth-abs", Flags::DepthLayers, Image::Type::GrayAlpha, {0.f, 1.f}, false},
	{Type::ZDepthNorm, "z-depth-norm", Flags::DepthLayers, Image::Type::GrayAlpha},
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