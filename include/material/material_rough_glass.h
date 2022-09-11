#pragma once
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

#ifndef YAFARAY_MATERIAL_ROUGH_GLASS_H
#define YAFARAY_MATERIAL_ROUGH_GLASS_H

#include "material/material_node.h"
#include "material/material_data.h"

namespace yafaray {

class RoughGlassMaterialData final : public MaterialData
{
	public:
		RoughGlassMaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : MaterialData(bsdf_flags, number_of_nodes) { }
		std::unique_ptr<MaterialData> clone() const override { return std::make_unique<RoughGlassMaterialData>(*this); }
};

class RoughGlassMaterial final : public NodeMaterial
{
	public:
		inline static std::string getClassName() { return "RoughGlassMaterial"; }
		static std::pair<Material *, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		[[nodiscard]] Type type() const override { return Type::RoughGlass; }
		struct ShaderNodeType : public Enum<ShaderNodeType>
		{
			enum : decltype(type()) { Bump, Wireframe, MirrorColor, FilterColor, Ior, Roughness, Size }; //Always leave the Size entry at the end!!
			inline static const EnumMap<decltype(type())> map_{{
					{"bump_shader", Bump, ""},
					{"wireframe_shader", Wireframe, "Shader node for wireframe shading (float)"},
					{"mirror_color_shader", MirrorColor, ""},
					{"filter_color_shader", FilterColor, ""},
					{"IOR_shader", Ior, ""},
					{"roughness_shader", Roughness, ""},
				}};
			bool isBump() const { return value() == Bump; }
		};
		const struct Params
		{
			PARAM_INIT_PARENT(Material);
			PARAM_DECL(float, ior_, 1.4f, "IOR", "Index of refraction");
			PARAM_DECL(Rgb, filter_color_, Rgb{1.f}, "filter_color", "");
			PARAM_DECL(float, transmit_filter_, 0.f, "transmit_filter", "");
			PARAM_DECL(Rgb, mirror_color_, Rgb{1.f}, "mirror_color", "");
			PARAM_DECL(float, alpha_, 0.5f, "alpha", "");
			PARAM_DECL(float, dispersion_power_, 0.f, "dispersion_power", "");
			PARAM_DECL(bool, fake_shadows_, false, "fake_shadows", "");
			PARAM_DECL(Rgb, absorption_color_, Rgb{1.f}, "absorption", "");
			PARAM_DECL(float, absorption_dist_, 1.f, "absorption_dist", "");
			PARAM_SHADERS_DECL;
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		RoughGlassMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const override;
		Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f *dir, Rgb &tcol, Sample &s, float *w, bool chromatic, float wavelength) const override;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs, bool force_eval) const override { return Rgb{0.f}; }
		float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const override { return 0.f; }
		bool isTransparent() const override { return params_.fake_shadows_; }
		Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const override;
		float getMatIor() const override { return params_.ior_; }
		Rgb getGlossyColor(const NodeTreeData &node_tree_data) const override;
		Rgb getTransColor(const NodeTreeData &node_tree_data) const override;
		Rgb getMirrorColor(const NodeTreeData &node_tree_data) const override;

		std::array<const ShaderNode *, static_cast<size_t>(ShaderNodeType::Size)> shaders_{initShaderArray<ShaderNodeType::Size>()};
		const Rgb filter_color_{params_.transmit_filter_ * params_.filter_color_ + Rgb(1.f - params_.transmit_filter_)};
		Rgb beer_sigma_a_;
		const float alpha_{std::max(1e-4f, std::min(params_.alpha_ * 0.5f, 1.f))};
		const float a_2_{alpha_ * alpha_};
		bool absorb_ = false;
		bool disperse_ = false;
		float cauchy_a_, cauchy_b_;
};

} //namespace yafaray

#endif // YAFARAY_MATERIAL_ROUGH_GLASS_H
