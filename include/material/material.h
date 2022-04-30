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

#ifndef YAFARAY_MATERIAL_H
#define YAFARAY_MATERIAL_H

#include "common/yafaray_common.h"
#include "common/flags.h"
#include "common/visibility.h"
#include "shader/shader_node.h"
#include "geometry/vector.h"
#include <list>

BEGIN_YAFARAY

class ParamMap;
class Scene;
class SurfacePoint;
class VolumeHandler;
struct Sample;
struct PSample;
class Logger;

struct BsdfFlags : public Flags
{
	BsdfFlags() = default;
	BsdfFlags(const Flags &flags) : Flags(flags) { } // NOLINT(google-explicit-constructor)
	BsdfFlags(unsigned int flags) : Flags(flags) { } // NOLINT(google-explicit-constructor)
	enum Enum : unsigned int
	{
			None		= 0,
			Specular	= 1 << 0,
			Glossy		= 1 << 1,
			Diffuse		= 1 << 2,
			Dispersive	= 1 << 3,
			Reflect		= 1 << 4,
			Transmit	= 1 << 5,
			Filter		= 1 << 6,
			Emit		= 1 << 7,
			Volumetric	= 1 << 8,
			DiffuseReflect = Diffuse | Reflect,
			SpecularReflect = Specular | Reflect,
			SpecularTransmit = Transmit | Filter,
			Translucency = Diffuse | Transmit,// translucency (diffuse transmitt)
			AllSpecular = Specular | Reflect | Transmit,
			AllGlossy = Glossy | Reflect | Transmit,
			All = Specular | Glossy | Diffuse | Dispersive | Reflect | Transmit | Filter
	};
};

class MaterialData
{
	public:
		MaterialData(BsdfFlags bsdf_flags, size_t number_of_nodes) : bsdf_flags_(bsdf_flags), node_tree_data_(number_of_nodes) { }
		virtual ~MaterialData() = default;
		BsdfFlags bsdf_flags_;
		NodeTreeData node_tree_data_;
};

struct DirectionColor
{
	static std::unique_ptr<DirectionColor> blend(std::unique_ptr<DirectionColor> direction_color_1, std::unique_ptr<DirectionColor> direction_color_2, float blend_val);
	Vec3 dir_;
	Rgb col_;
};

struct Specular
{
	std::unique_ptr<DirectionColor> reflect_, refract_;
};

class Material
{
	public:
		static const Material *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params, const std::list<ParamMap> &nodes_params);
		explicit Material(Logger &logger);
		virtual ~Material();

		/*! Initialize the BSDF of a material. You must call this with the current surface point
			first before any other methods (except isTransparent/getTransparency)! The renderstate
			holds a pointer to preallocated userdata to save data that only depends on the current sp,
			like texture lookups etc.
			\param bsdf_types returns flags for all bsdf components the material has
		 */
		virtual const MaterialData * initBsdf(SurfacePoint &sp, const Camera *camera) const = 0;

		/*! evaluate the BSDF for the given components.
				@param types the types of BSDFs to be evaluated (e.g. diffuse only, or diffuse and glossy) */
		virtual Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &types, bool force_eval) const = 0;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &types) const { return eval(mat_data, sp, wo, wl, types, false); }

		/*! take a sample from the BSDF, given a 2-dimensional sample value and the BSDF types to be sampled from
			\param s s1, s2 and flags members give necessary information for creating the sample, pdf and sampledFlags need to be returned
			\param w returns the weight for importance sampling
		*/
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const = 0;// {return Rgb{0.f};}
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const {return Rgb{0.f};}
		/*! return the pdf for sampling the BSDF with wi and wo
		*/
		virtual float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const {return 0.f;}
		/*! indicate whether light can (partially) pass the material without getting refracted,
			e.g. a curtain or even very thin foils approximated as single non-refractive layer.
			used to trace transparent shadows. Note that in this case, initBSDF was NOT called before!
		*/
		virtual bool isTransparent() const { return false; }
		/*!	used for computing transparent shadows.	Default implementation returns black (i.e. solid shadow).
			This is only used for shadow calculations and may only be called when isTransparent returned true.	*/
		virtual Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const { return Rgb{0.f}; }
		/*! evaluate the specular components for given direction. Somewhat a specialization of sample(),
			because neither sample values nor pdf values are necessary for this.
			Typical use: recursive raytracing of integrators. */
		virtual Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, bool chromatic, float wavelength) const { return {}; }

		/*! get the overall reflectivity of the material (used to compute radiance map for example) */
		virtual Rgb getReflectivity(const MaterialData *mat_data, const SurfacePoint &sp, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const;

		/*!	allow light emitting materials, for realizing correctly visible area lights.
			default implementation returns black obviously.	*/
		virtual Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo) const { return Rgb{0.f}; }

		/*! get the volumetric handler for space at specified side of the surface
			\param inside true means space opposite of surface normal, which is considered "inside" */
		virtual const VolumeHandler *getVolumeHandler(bool inside) const { return inside ? vol_i_.get() : vol_o_.get(); }

		/*! special function, get the alpha-value of a material, used to calculate the alpha-channel */
		virtual float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wo, const Camera *camera) const { return 1.f; }

		/*! specialized function for photon mapping. Default function uses the sample function, which will do fine for
			most materials unless there's a less expensive way or smarter scattering approach */
		virtual bool scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const;

		virtual float getMatIor() const { return 1.5f; }
		virtual Rgb getDiffuseColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getGlossyColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getTransColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getMirrorColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getSubSurfaceColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		void setMaterialIndex(unsigned int new_mat_index)
		{
			material_index_ = new_mat_index;
			if(material_index_highest_ < material_index_) material_index_highest_ = material_index_;
		}
		static void resetMaterialIndex() { material_index_highest_ = 1.f; material_index_auto_ = 0; }
		unsigned int getAbsMaterialIndex() const { return material_index_; }
		float getNormMaterialIndex() const { return static_cast<float>(getAbsMaterialIndex()) / static_cast<float>(material_index_highest_); }
		Rgb getAbsMaterialIndexColor() const { return Rgb{static_cast<float>(getAbsMaterialIndex())}; }
		Rgb getNormMaterialIndexColor() const { return Rgb{getNormMaterialIndex()}; }
		Rgb getAutoMaterialIndexColor() const { return material_index_auto_color_; }
		static Rgb getAutoMaterialIndexNumber() { return Rgb{static_cast<float>(material_index_auto_)}; }
		Visibility getVisibility() const { return visibility_; }
		bool getReceiveShadows() const { return receive_shadows_; }

		bool isFlat() const { return flat_material_; }

		int getAdditionalDepth() const { return additional_depth_; }
		float getTransparentBiasFactor() const { return transparent_bias_factor_; }
		bool getTransparentBiasMultiplyRayDepth() const { return transparent_bias_multiply_ray_depth_; }

		template<typename T> void applyWireFrame(T &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const;
		void applyWireFrame(float &value, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgb &col, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgba &col, float wire_frame_amount, const SurfacePoint &sp) const;
		void setSamplingFactor(const float &new_sampling_factor)
		{
			sampling_factor_ = new_sampling_factor;
			if(highest_sampling_factor_ < sampling_factor_) highest_sampling_factor_ = sampling_factor_;
		}
		float getSamplingFactor() const { return sampling_factor_; }
		static Rgb sampleClay(const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w);
		static Rgb getShaderColor(const ShaderNode *shader_node, const NodeTreeData &node_tree_data, const Rgb &color_without_shader)
		{
			return shader_node ? shader_node->getColor(node_tree_data) : color_without_shader;
		}
		static Rgba getShaderColor(const ShaderNode *shader_node, const NodeTreeData &node_tree_data, const Rgba &color_without_shader)
		{
			return shader_node ? shader_node->getColor(node_tree_data) : color_without_shader;
		}
		static float getShaderScalar(const ShaderNode *shader_node, const NodeTreeData &node_tree_data, float value_without_shader)
		{
			return shader_node ? shader_node->getScalar(node_tree_data) : value_without_shader;
		}

	protected:
		/* small function to apply bump mapping to a surface point
			you need to determine the partial derivatives for NU and NV first, e.g. from a shader node */
		static void applyBump(SurfacePoint &sp, const DuDv &du_dv);

		BsdfFlags bsdf_flags_ = BsdfFlags::None;

		Visibility visibility_ = Visibility::NormalVisible; //!< sets material visibility (Normal:visible, visible without shadows, invisible (shadows only) or totally invisible.

		bool receive_shadows_ = true; //!< enables/disables material reception of shadows.
		std::unique_ptr<VolumeHandler> vol_i_; //!< volumetric handler for space inside material (opposed to surface normal)
		std::unique_ptr<VolumeHandler> vol_o_; //!< volumetric handler for space outside ofmaterial (where surface normal points to)
		unsigned int material_index_ = 0;	//!< Material Index for the material-index render pass
		Rgb material_index_auto_color_{0.f};	//!< Material Index color automatically generated for the material-index-auto (color) render pass
		int additional_depth_ = 0;	//!< Per-material additional ray-depth
		float transparent_bias_factor_ = 0.f;	//!< Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If >0.f this function is enabled and the result will no longer be realistic and may have artifacts.
		bool transparent_bias_multiply_ray_depth_ = false;	//!< Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If enabled the bias will be multiplied by the current ray depth so the first transparent surfaces are rendered better and subsequent surfaces might be skipped.

		float wireframe_amount_ = 0.f;           //!< Wireframe shading amount
		float wireframe_thickness_ = 0.01f;      //!< Wireframe thickness
		float wireframe_exponent_ = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
		Rgb wireframe_color_ = Rgb(1.f); //!< Wireframe shading color

		float sampling_factor_ = 1.f;	//!< Material sampling factor, to allow some materials to receive more samples than others

		bool flat_material_ = false;		//!< Flat Material is a special non-photorealistic material that does not multiply the surface color by the cosine of the angle with the light, as happens in real life. Also, if receive_shadows is disabled, this flat material does no longer self-shadow. For special applications only.

		static float highest_sampling_factor_;	//!< Class shared variable containing the highest material sampling factor. This is used to calculate the max. possible samples for the Sampling pass.
		static unsigned int material_index_auto_;	//!< Material Index automatically generated for the material-index-auto render pass
		static unsigned int material_index_highest_;	//!< Class shared variable containing the highest material index used for the Normalized Material Index pass.
		Logger &logger_;
};

template<typename T>
inline void Material::applyWireFrame(T &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const
{
	if(wireframe_thickness_ > 0.f)
	{
		const float wire_frame_amount = wireframe_shader ? wireframe_shader->getScalar(node_tree_data) * wireframe_amount_ : wireframe_amount_;
		if(wire_frame_amount > 0.f) applyWireFrame(value, wire_frame_amount, sp);
	}
}

template void Material::applyWireFrame<float>(float &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const;
template void Material::applyWireFrame<Rgb>(Rgb &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const;
template void Material::applyWireFrame<Rgba>(Rgba &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const;


struct Sample
{
	Sample(float s_1, float s_2, BsdfFlags sflags = BsdfFlags::All, bool revrs = false):
			s_1_(s_1), s_2_(s_2), pdf_(0.f), flags_(sflags), sampled_flags_(BsdfFlags::None), reverse_(revrs) {}
	float s_1_, s_2_;
	float pdf_;
	BsdfFlags flags_, sampled_flags_;
	bool reverse_; //!< if true, the sample method shall return the probability/color for swapped incoming/outgoing dir
	float pdf_back_;
	Rgb col_back_;
};

struct PSample final : public Sample // << whats with the public?? structs inherit publicly by default *DarkTide
{
	PSample(float s_1, float s_2, float s_3, BsdfFlags sflags, const Rgb &l_col, const Rgb &transm = Rgb(1.f)):
			Sample(s_1, s_2, sflags), s_3_(s_3), lcol_(l_col), alpha_(transm) {}
	float s_3_;
	const Rgb lcol_; //!< the photon color from last scattering
	const Rgb alpha_; //!< the filter color between last scattering and this hit (not pre-applied to lcol!)
	Rgb color_; //!< the new color after scattering, i.e. what will be lcol for next scatter.
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_H
