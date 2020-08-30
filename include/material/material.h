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

#include "constants.h"
#include "common/color.h"
#include <list>

BEGIN_YAFARAY

class ParamMap;
class RenderEnvironment;
class Vec3;

#define FACE_FORWARD(Ng,N,I) ((((Ng)*(I))<0) ? (-N) : (N))
/*
class BSDF_t
{
	public:
	BSDF_t(){};
	virtual ~BSDF_t(){};
	BSDF_t(const surfacePoint_t &sp, void* userdata);
	virtual Rgb sample(const vector3d_t &wo, vector3d_t &wi, float s1, float s2, void* userdata) = 0;
	virtual Rgb eval(const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, void* userdata) = 0;
};
*/

class SurfacePoint;
struct RenderState;
class VolumeHandler;

enum BsdfFlags
{
	BsdfNone		= 0x0000,
	BsdfSpecular	= 0x0001,
	BsdfGlossy		= 0x0002,
	BsdfDiffuse	= 0x0004,
	BsdfDispersive	= 0x0008,
	BsdfReflect	= 0x0010,
	BsdfTransmit	= 0x0020,
	BsdfFilter		= 0x0040,
	BsdfEmit		= 0x0080,
	BsdfVolumetric = 0x0100,
	BsdfAllSpecular = BsdfSpecular | BsdfReflect | BsdfTransmit,
	BsdfAllGlossy = BsdfGlossy | BsdfReflect | BsdfTransmit,
	BsdfAll = BsdfSpecular | BsdfGlossy | BsdfDiffuse | BsdfDispersive | BsdfReflect | BsdfTransmit | BsdfFilter
};

typedef unsigned int Bsdf_t;

struct Sample
{
	Sample(float s_1, float s_2, Bsdf_t sflags = BsdfAll, bool revrs = false):
			s_1_(s_1), s_2_(s_2), pdf_(0.f), flags_(sflags), sampled_flags_(BsdfNone), reverse_(revrs) {}
	float s_1_, s_2_;
	float pdf_;
	Bsdf_t flags_, sampled_flags_;
	bool reverse_; //!< if true, the sample method shall return the probability/color for swapped incoming/outgoing dir
	float pdf_back_;
	Rgb col_back_;
};

struct PSample final : public Sample // << whats with the public?? structs inherit publicly by default *DarkTide
{
	PSample(float s_1, float s_2, float s_3, Bsdf_t sflags, const Rgb &l_col, const Rgb &transm = Rgb(1.f)):
			Sample(s_1, s_2, sflags), s_3_(s_3), lcol_(l_col), alpha_(transm) {}
	float s_3_;
	const Rgb lcol_; //!< the photon color from last scattering
	const Rgb alpha_; //!< the filter color between last scattering and this hit (not pre-applied to lcol!)
	Rgb color_; //!< the new color after scattering, i.e. what will be lcol for next scatter.
};

enum Visibility
{
	NormalVisible			= 0,
	VisibleNoShadows		= 1,
	InvisibleShadowsOnly	= 2,
	Invisible				= 3
};


class Material
{
	public:
		static Material *factory(ParamMap &params, std::list<ParamMap> &eparams, RenderEnvironment &render);

		Material();
		virtual ~Material() { resetMaterialIndex(); }

		/*! Initialize the BSDF of a material. You must call this with the current surface point
			first before any other methods (except isTransparent/getTransparency)! The renderstate
			holds a pointer to preallocated userdata to save data that only depends on the current sp,
			like texture lookups etc.
			\param bsdf_types returns flags for all bsdf components the material has
		 */
		virtual void initBsdf(const RenderState &state, SurfacePoint &sp, Bsdf_t &bsdf_types) const = 0;

		/*! evaluate the BSDF for the given components.
				@param types the types of BSDFs to be evaluated (e.g. diffuse only, or diffuse and glossy) */
		virtual Rgb eval(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, Bsdf_t types, bool force_eval = false) const = 0;

		/*! take a sample from the BSDF, given a 2-dimensional sample value and the BSDF types to be sampled from
			\param s s1, s2 and flags members give necessary information for creating the sample, pdf and sampledFlags need to be returned
			\param w returns the weight for importance sampling
		*/
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const = 0;// {return Rgb(0.f);}
		virtual Rgb sample(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w) const {return Rgb(0.f);}

		virtual Rgb sampleClay(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const;
		/*! return the pdf for sampling the BSDF with wi and wo
		*/
		virtual float pdf(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, Bsdf_t bsdfs) const {return 0.f;}


		/*! indicate whether light can (partially) pass the material without getting refracted,
			e.g. a curtain or even very thin foils approximated as single non-refractive layer.
			used to trace transparent shadows. Note that in this case, initBSDF was NOT called before!
		*/
		virtual bool isTransparent() const { return false; }

		/*!	used for computing transparent shadows.	Default implementation returns black (i.e. solid shadow).
			This is only used for shadow calculations and may only be called when isTransparent returned true.	*/
		virtual Rgb getTransparency(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const { return Rgb(0.0); }
		/*! evaluate the specular components for given direction. Somewhat a specialization of sample(),
			because neither sample values nor pdf values are necessary for this.
			Typical use: recursive raytracing of integrators.
			\param dir dir[0] returns reflected direction, dir[1] refracted direction
			\param col col[0] returns reflected spectrum, dir[1] refracted spectrum */
		virtual void getSpecular(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo,
								 bool &reflect, bool &refract, Vec3 *const dir, Rgb *const col) const
		{ reflect = false; refract = false; }

		/*! get the overall reflectivity of the material (used to compute radiance map for example) */
		virtual Rgb getReflectivity(const RenderState &state, const SurfacePoint &sp, Bsdf_t flags) const;

		/*!	allow light emitting materials, for realizing correctly visible area lights.
			default implementation returns black obviously.	*/
		virtual Rgb emit(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const { return Rgb(0.0); }

		/*! get the volumetric handler for space at specified side of the surface
			\param inside true means space opposite of surface normal, which is considered "inside" */
		virtual const VolumeHandler *getVolumeHandler(bool inside) const { return inside ? vol_i_ : vol_o_; }

		/*! special function, get the alpha-value of a material, used to calculate the alpha-channel */
		virtual float getAlpha(const RenderState &state, const SurfacePoint &sp, const Vec3 &wo) const { return 1.f; }

		/*! specialized function for photon mapping. Default function uses the sample function, which will do fine for
			most materials unless there's a less expensive way or smarter scattering approach */
		virtual bool scatterPhoton(const RenderState &state, const SurfacePoint &sp, const Vec3 &wi, Vec3 &wo, PSample &s) const;

		Bsdf_t getFlags() const { return bsdf_flags_; }
		/*! Materials may have to do surface point specific (pre-)calculation that need extra storage.
			returns the required amount of "userdata" memory for all the functions that require a render state */
		size_t getReqMem() const { return req_mem_; }

		/*! Get materials IOR (for refracted photons) */

		virtual float getMatIor() const { return 1.5f; }
		virtual Rgb getDiffuseColor(const RenderState &state) const { return Rgb(0.f); }
		virtual Rgb getGlossyColor(const RenderState &state) const { return Rgb(0.f); }
		virtual Rgb getTransColor(const RenderState &state) const { return Rgb(0.f); }
		virtual Rgb getMirrorColor(const RenderState &state) const { return Rgb(0.f); }
		virtual Rgb getSubSurfaceColor(const RenderState &state) const { return Rgb(0.f); }
		void setMaterialIndex(const float &new_mat_index)
		{
			material_index_ = new_mat_index;
			if(material_index_highest_ < material_index_) material_index_highest_ = material_index_;
		}
		void resetMaterialIndex() { material_index_highest_ = 1.f; material_index_auto_ = 0; }
		void setMaterialIndex(const int &new_mat_index) { setMaterialIndex((float) new_mat_index); }
		float getAbsMaterialIndex() const { return material_index_; }
		float getNormMaterialIndex() const { return (material_index_ / material_index_highest_); }
		Rgb getAbsMaterialIndexColor() const
		{
			return Rgb(material_index_);
		}
		Rgb getNormMaterialIndexColor() const
		{
			float normalized_material_index = getNormMaterialIndex();
			return Rgb(normalized_material_index);
		}
		Rgb getAutoMaterialIndexColor() const
		{
			return material_index_auto_color_;
		}
		Rgb getAutoMaterialIndexNumber() const
		{
			return material_index_auto_number_;
		}

		Visibility getVisibility() const { return visibility_; }
		bool getReceiveShadows() const { return receive_shadows_; }

		bool isFlat() const { return flat_material_; }

		int getAdditionalDepth() const { return additional_depth_; }
		float getTransparentBiasFactor() const { return transparent_bias_factor_; }
		bool getTransparentBiasMultiplyRayDepth() const { return transparent_bias_multiply_ray_depth_; }

		void applyWireFrame(float &value, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgb &col, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgb *const col, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgba &col, float wire_frame_amount, const SurfacePoint &sp) const;
		void applyWireFrame(Rgba *const col, float wire_frame_amount, const SurfacePoint &sp) const;

		void setSamplingFactor(const float &new_sampling_factor)
		{
			sampling_factor_ = new_sampling_factor;
			if(highest_sampling_factor_ < sampling_factor_) highest_sampling_factor_ = sampling_factor_;
		}
		float getSamplingFactor() const { return sampling_factor_; }

	protected:
		/* small function to apply bump mapping to a surface point
			you need to determine the partial derivatives for NU and NV first, e.g. from a shader node */
		void applyBump(SurfacePoint &sp, float df_dnu, float df_dnv) const;

		Bsdf_t bsdf_flags_;

		Visibility visibility_; //!< sets material visibility (Normal:visible, visible without shadows, invisible (shadows only) or totally invisible.

		bool receive_shadows_; //!< enables/disables material reception of shadows.

		size_t req_mem_; //!< the amount of "temporary" memory required to compute/store surface point specific data
		VolumeHandler *vol_i_ = nullptr; //!< volumetric handler for space inside material (opposed to surface normal)
		VolumeHandler *vol_o_ = nullptr; //!< volumetric handler for space outside ofmaterial (where surface normal points to)
		float material_index_;	//!< Material Index for the material-index render pass
		static unsigned int material_index_auto_;	//!< Material Index automatically generated for the material-index-auto render pass
		Rgb material_index_auto_color_;	//!< Material Index color automatically generated for the material-index-auto (color) render pass
		float material_index_auto_number_ = 0.f;	//!< Material Index number automatically generated for the material-index-auto-abs (numeric) render pass
		static float material_index_highest_;	//!< Class shared variable containing the highest material index used for the Normalized Material Index pass.
		int additional_depth_;	//!< Per-material additional ray-depth
		float transparent_bias_factor_ = 0.f;	//!< Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If >0.f this function is enabled and the result will no longer be realistic and may have artifacts.
		bool transparent_bias_multiply_ray_depth_ = false;	//!< Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If enabled the bias will be multiplied by the current ray depth so the first transparent surfaces are rendered better and subsequent surfaces might be skipped.

		float wireframe_amount_ = 0.f;           //!< Wireframe shading amount
		float wireframe_thickness_ = 0.01f;      //!< Wireframe thickness
		float wireframe_exponent_ = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
		Rgb wireframe_color_ = Rgb(1.f); //!< Wireframe shading color

		float sampling_factor_ = 1.f;	//!< Material sampling factor, to allow some materials to receive more samples than others

		bool flat_material_ = false;		//!< Flat Material is a special non-photorealistic material that does not multiply the surface color by the cosine of the angle with the light, as happens in real life. Also, if receive_shadows is disabled, this flat material does no longer self-shadow. For special applications only.

		static float highest_sampling_factor_;	//!< Class shared variable containing the highest material sampling factor. This is used to calculate the max. possible samples for the Sampling pass.
};

END_YAFARAY

#endif // YAFARAY_MATERIAL_H
