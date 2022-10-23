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

#ifndef LIBYAFARAY_MATERIAL_H
#define LIBYAFARAY_MATERIAL_H

#include "common/visibility.h"
#include "shader/shader_node.h"
#include "geometry/vector.h"
#include "geometry/direction_color.h"
#include "material/bsdf.h"
#include "material/specular_data.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include <list>

namespace yafaray {

class ParamMap;
class Scene;
class SurfacePoint;
class VolumeHandler;
struct Sample;
struct PSample;
class MaterialData;
template <typename T> class SceneItems;
class Logger;

class Material
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "Material"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<Material>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		Material(Logger &logger, ParamError &param_error, const ParamMap &param_map, const SceneItems <Material> &materials);
		virtual ~Material();

		/*! Initialize the BSDF of a material. You must call this with the current surface point
			first before any other methods (except isTransparent/getTransparency)! The renderstate
			holds a pointer to preallocated userdata to save data that only depends on the current sp,
			like texture lookups etc.
			\param bsdf_types returns flags for all bsdf components the material has
		 */
		virtual std::unique_ptr<const MaterialData> initBsdf(SurfacePoint &sp, const Camera *camera) const = 0;

		/*! evaluate the BSDF for the given components.
				@param types the types of BSDFs to be evaluated (e.g. diffuse only, or diffuse and glossy) */
		virtual Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags types, bool force_eval) const = 0;
		Rgb eval(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wl, BsdfFlags types) const { return eval(mat_data, sp, wo, wl, types, false); }

		/*! take a sample from the BSDF, given a 2-dimensional sample value and the BSDF types to be sampled from
			\param s s1, s2 and flags members give necessary information for creating the sample, pdf and sampledFlags need to be returned
			\param w returns the weight for importance sampling
		*/
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const = 0;// {return Rgb{0.f};}
		virtual Rgb sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const {return Rgb{0.f};}
		/*! return the pdf for sampling the BSDF with wi and wo
		*/
		virtual float pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const {return 0.f;}
		/*! indicate whether light can (partially) pass the material without getting refracted,
			e.g. a curtain or even very thin foils approximated as single non-refractive layer.
			used to trace transparent shadows. Note that in this case, initBSDF was NOT called before!
		*/
		virtual bool isTransparent() const { return false; }
		/*!	used for computing transparent shadows.	Default implementation returns black (i.e. solid shadow).
			This is only used for shadow calculations and may only be called when isTransparent returned true.	*/
		virtual Rgb getTransparency(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const { return Rgb{0.f}; }
		/*! evaluate the specular components for given direction. Somewhat a specialization of sample(),
			because neither sample values nor pdf values are necessary for this.
			Typical use: recursive raytracing of integrators. */
		virtual Specular getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const { return {}; }

		/*! get the overall reflectivity of the material (used to compute radiance map for example) */
		virtual Rgb getReflectivity(FastRandom &fast_random, const MaterialData *mat_data, const SurfacePoint &sp, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const;

		/*!	allow light emitting materials, for realizing correctly visible area lights.
			default implementation returns black obviously.	*/
		virtual Rgb emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const { return Rgb{0.f}; }

		/*! get the volumetric handler for space at specified side of the surface
			\param inside true means space opposite of surface normal, which is considered "inside" */
		virtual const VolumeHandler *getVolumeHandler(bool inside) const { return inside ? vol_i_.get() : vol_o_.get(); }

		/*! special function, get the alpha-value of a material, used to calculate the alpha-channel */
		virtual float getAlpha(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Camera *camera) const { return 1.f; }

		/*! specialized function for photon mapping. Default function uses the sample function, which will do fine for
			most materials unless there's a less expensive way or smarter scattering approach */
		virtual bool scatterPhoton(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wi, Vec3f &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const;

		virtual float getMatIor() const { return 1.5f; }
		virtual Rgb getDiffuseColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getGlossyColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getTransColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getMirrorColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		virtual Rgb getSubSurfaceColor(const NodeTreeData &node_tree_data) const { return Rgb{0.f}; }
		unsigned int getPassIndex() const { return params_.mat_pass_index_; }
		size_t getId() const { return id_; }
		void setId(size_t id);
		std::string getName() const;
		Rgb getPassIndexAutoColor() const { return index_auto_color_; }
		Visibility getVisibility() const { return params_.visibility_; }
		bool getReceiveShadows() const { return params_.receive_shadows_; }

		bool isFlat() const { return params_.flat_material_; }

		int getAdditionalDepth() const { return additional_depth_; }
		float getTransparentBiasFactor() const { return params_.transparent_bias_factor_; }
		bool getTransparentBiasMultiplyRayDepth() const { return params_.transparent_bias_multiply_raydepth_; }

		template<typename T> void applyWireFrame(T &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const;
		void applyWireFrame(float &value, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgb &col, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgba &col, float wire_frame_amount, const SurfacePoint &sp) const;
		float getSamplingFactor() const { return params_.sampling_factor_; }
		static Rgb sampleClay(const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w);
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
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Blend, CoatedGlossy, Glass, Mirror, Null, Glossy, RoughGlass, ShinyDiffuse, Light, Mask };
			inline static const EnumMap<ValueType_t> map_{{
					{"blend_mat", Blend, ""},
					{"coated_glossy", CoatedGlossy, ""},
					{"glass", Glass, ""},
					{"mirror", Mirror, ""},
					{"null", Null, ""},
					{"glossy", Glossy, ""},
					{"rough_glass", RoughGlass, ""},
					{"shinydiffusemat", ShinyDiffuse, ""},
					{"light_mat", Light, ""},
					{"mask_mat", Mask, ""},
				}};
		};
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(bool, receive_shadows_, true, "receive_shadows", "If true, objects with this material receive shadows from other objects");
			PARAM_DECL(bool, flat_material_, false, "flat_material", "Flat Material is a special non-photorealistic material that does not multiply the surface color by the cosine of the angle with the light, as happens in real life. Also, if receive_shadows is disabled, this flat material does no longer self-shadow. For special applications only, for example to fill cross sections in architectural drawings.");
			PARAM_ENUM_DECL(Visibility, visibility_, Visibility::Normal, "visibility", "");
			PARAM_DECL(int, mat_pass_index_, 0, "mat_pass_index", "Material index used in the material index render layer");
			PARAM_DECL(int, additional_depth_, 0, "additionaldepth", "Per material additional ray depth, useful for certain materials like glass, so it is not needed to increase the entire scene ray depth for a slight performance improvement");
			PARAM_DECL(float, transparent_bias_factor_, 0.f, "transparentbias_factor", "Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If >0.f this function is enabled and the result will no longer be realistic and may have artifacts");
			PARAM_DECL(bool, transparent_bias_multiply_raydepth_, false, "transparentbias_multiply_raydepth", "Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If enabled the bias will be multiplied by the current ray depth so the first transparent surfaces are rendered better and subsequent surfaces might be skipped.");
			PARAM_DECL(float, sampling_factor_, 1.f, "samplingfactor", "Per material sampling factor, to allow some materials to receive more samples than others");
			PARAM_DECL(float, wireframe_amount_, 0.f, "wireframe_amount", "Wireframe shading amount");
			PARAM_DECL(float, wireframe_thickness_, 0.01f, "wireframe_thickness", "Wireframe thickness");
			PARAM_DECL(float, wireframe_exponent_, 0.f, "wireframe_exponent", "Wireframe exponent (0.0 = solid, 1.0 = linearly gradual, etc)");
			PARAM_DECL(Rgb, wireframe_color_, Rgb{1.f}, "wireframe_color", "Wireframe shading color");
		} params_;
		/* small function to apply bump mapping to a surface point
			you need to determine the partial derivatives for NU and NV first, e.g. from a shader node */
		static void applyBump(SurfacePoint &sp, const DuDv &du_dv);
		template <size_t N> static std::array<const ShaderNode *, N> initShaderArray();

		size_t id_{0}; //!< Material Id within the scene
		Rgb index_auto_color_{0.f}; //!< Material Index color automatically generated for the material-index-auto (color) render pass
		BsdfFlags bsdf_flags_{BsdfFlags::None};
		std::unique_ptr<VolumeHandler> vol_i_; //!< volumetric handler for space inside material (opposed to surface normal)
		std::unique_ptr<VolumeHandler> vol_o_; //!< volumetric handler for space outside ofmaterial (where surface normal points to)
		int additional_depth_{params_.additional_depth_};	//!< Per-material additional ray-depth
		const SceneItems<Material> &materials_;
		Logger &logger_;
};

template <size_t N>
std::array<const ShaderNode *, N> Material::initShaderArray()
{
	std::array<const ShaderNode *, N> result;
	for(auto node : result) node = nullptr;
	return result;
}

template<typename T>
inline void Material::applyWireFrame(T &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const
{
	if(params_.wireframe_thickness_ > 0.f)
	{
		const float wire_frame_amount = wireframe_shader ? wireframe_shader->getScalar(node_tree_data) * params_.wireframe_amount_ : params_.wireframe_amount_;
		if(wire_frame_amount > 0.f) applyWireFrame(value, wire_frame_amount, sp);
	}
}

template void Material::applyWireFrame<float>(float &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const;
template void Material::applyWireFrame<Rgb>(Rgb &value, const ShaderNode *wireframe_shader, const NodeTreeData &node_tree_data, const SurfacePoint &sp) const;


} //namespace yafaray

#endif // LIBYAFARAY_MATERIAL_H
