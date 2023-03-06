#pragma once
/****************************************************************************
 *      material_glass.h: a dielectric material with dispersion, two trivial mats
 *
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef LIBYAFARAY_MATERIAL_GLASS_H
#define LIBYAFARAY_MATERIAL_GLASS_H

#include "material/material_node.h"
#include "material/material_data.h"

namespace yafaray {

class GlassMaterialData final : public MaterialData
{
	public:
		GlassMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<GlassMaterialData>(*this); }
};

class GlassMaterial final : public NodeMaterial
{
		using ThisClassType_t = GlassMaterial; using ParentClassType_t = NodeMaterial; using MaterialData_t = GlassMaterialData;

	public:
		inline static std::string getClassName() { return "GlassMaterial"; }
		static std::pair<std::unique_ptr<Material>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		GlassMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials);

	private:
		[[nodiscard]] Type type() const override { return Type::Glass; }
		struct ShaderNodeType : public Enum<ShaderNodeType, size_t>
		{
			enum : ValueType_t { Bump, Wireframe, MirrorColor, FilterColor, Ior, Size }; //Always leave the Size entry at the end!!
			inline static const EnumMap<ValueType_t> map_{{
					{"bump_shader", Bump, ""},
					{"wireframe_shader", Wireframe, "Shader node for wireframe shading (float)"},
					{"mirror_color_shader", MirrorColor, ""},
					{"filter_color_shader", FilterColor, ""},
					{"IOR_shader", Ior, ""},
				}};
			bool isBump() const { return value() == Bump; }
		};
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float, ior_, 1.4f, "IOR", "Index of refraction");
			PARAM_DECL(Rgb, filter_color_, Rgb{1.f}, "filter_color", "");
			PARAM_DECL(float, transmit_filter_, 0.f, "transmit_filter", "");
			PARAM_DECL(Rgb, mirror_color_, Rgb{1.f}, "mirror_color", "");
			PARAM_DECL(float, dispersion_power_, 0.f, "dispersion_power", "");
			PARAM_DECL(bool, fake_shadows_, false, "fake_shadows", "");
			PARAM_DECL(Rgb, absorption_color_, Rgb{1.f}, "absorption", "");
			PARAM_DECL(float, absorption_dist_, 1.f, "absorption_dist", "");
			inline static const auto shader_node_names_meta_{ParamMeta::enumToParamMetaArray<ShaderNodeType>()};
		} params_;
		std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags bsdfs, bool force_eval) const override {return Rgb(0.0);}
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const override {return 0.f;}
		bool isTransparent() const override { return params_.fake_shadows_; }
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const override;
		float getMatIor() const override { return params_.ior_; }
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		Rgb getTransColor(const NodeTreeData &node_tree_data) const override;
		Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;

		std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> shaders_{initShaderArray<ShaderNodeType::Size>()};
		const Rgb filter_color_{params_.transmit_filter_ * params_.filter_color_ + Rgb(1.f - params_.transmit_filter_)};
		Rgb beer_sigma_a_;
		bool absorb_ = false;
		bool disperse_ = false;
		BsdfFlags transmit_flags_{BsdfFlags::None};
		float cauchy_a_, cauchy_b_;
};

} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_GLASS_H