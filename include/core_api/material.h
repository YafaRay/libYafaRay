#pragma once

#ifndef Y_MATERIAL_H
#define Y_MATERIAL_H

#include <yafray_constants.h>
#include "color.h"
#include "vector3d.h"
#include "surface.h"
#include "utilities/sample_utils.h"
#include "object3d.h"

__BEGIN_YAFRAY

#define FACE_FORWARD(Ng,N,I) ((((Ng)*(I))<0) ? (-N) : (N))
/*
class YAFRAYCORE_EXPORT BSDF_t
{
	public:
	BSDF_t(){};
	virtual ~BSDF_t(){};
	BSDF_t(const surfacePoint_t &sp, void* userdata);
	virtual color_t sample(const vector3d_t &wo, vector3d_t &wi, float s1, float s2, void* userdata) = 0;
	virtual color_t eval(const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, void* userdata) = 0;
};
*/

class surfacePoint_t;
struct renderState_t;
class volumeHandler_t;

enum bsdfFlags_t
{
	BSDF_NONE		= 0x0000,
	BSDF_SPECULAR	= 0x0001,
	BSDF_GLOSSY		= 0x0002,
	BSDF_DIFFUSE	= 0x0004,
	BSDF_DISPERSIVE	= 0x0008,
	BSDF_REFLECT	= 0x0010,
	BSDF_TRANSMIT	= 0x0020,
	BSDF_FILTER		= 0x0040,
	BSDF_EMIT		= 0x0080,
	BSDF_VOLUMETRIC = 0x0100,
	BSDF_ALL_SPECULAR = BSDF_SPECULAR | BSDF_REFLECT | BSDF_TRANSMIT,
	BSDF_ALL_GLOSSY = BSDF_GLOSSY | BSDF_REFLECT | BSDF_TRANSMIT,
	BSDF_ALL = BSDF_SPECULAR | BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT | BSDF_FILTER
};

typedef unsigned int BSDF_t;

struct sample_t
{
	sample_t(float s_1, float s_2, BSDF_t sflags=BSDF_ALL, bool revrs=false):
			s1(s_1), s2(s_2), pdf(0.f), flags(sflags), sampledFlags(BSDF_NONE), reverse(revrs){}
	float s1, s2;
	float pdf;
	BSDF_t flags, sampledFlags;
	bool reverse; //!< if true, the sample method shall return the probability/color for swapped incoming/outgoing dir
	float pdf_back;
	color_t col_back;
};

struct pSample_t: public sample_t // << whats with the public?? structs inherit publicly by default *DarkTide
{
	pSample_t(float s_1, float s_2, float s_3, BSDF_t sflags, const color_t &l_col, const color_t& transm=color_t(1.f)):
		sample_t(s_1, s_2, sflags), s3(s_3), lcol(l_col), alpha(transm){}
	float s3;
	const color_t lcol; //!< the photon color from last scattering
	const color_t alpha; //!< the filter color between last scattering and this hit (not pre-applied to lcol!)
	color_t color; //!< the new color after scattering, i.e. what will be lcol for next scatter.
};

enum visibility_t
{
	NORMAL_VISIBLE			= 0,
	VISIBLE_NO_SHADOWS		= 1,
	INVISIBLE_SHADOWS_ONLY	= 2,
	INVISIBLE				= 3
};


class YAFRAYCORE_EXPORT material_t
{
	public:
		material_t(): bsdfFlags(BSDF_NONE), mVisibility(NORMAL_VISIBLE), mReceiveShadows(true), reqMem(0), volI(nullptr), volO(nullptr),additionalDepth(0)
		{
			materialIndexAuto++;
			srand(materialIndexAuto);
			float R,G,B;
			do
			{
				R = (float) (rand() % 8) / 8.f;
				G = (float) (rand() % 8) / 8.f;
				B = (float) (rand() % 8) / 8.f;
			}
			while (R+G+B < 0.5f);
			materialIndexAutoColor = color_t(R,G,B);
            materialIndexAutoNumber = materialIndexAuto;
		}
		virtual ~material_t() { resetMaterialIndex(); }

		/*! Initialize the BSDF of a material. You must call this with the current surface point
			first before any other methods (except isTransparent/getTransparency)! The renderstate
			holds a pointer to preallocated userdata to save data that only depends on the current sp,
			like texture lookups etc.
			\param bsdfTypes returns flags for all bsdf components the material has
		 */
        virtual void initBSDF(const renderState_t &state, surfacePoint_t &sp, BSDF_t &bsdfTypes)const = 0;

		/*! evaluate the BSDF for the given components.
				@param types the types of BSDFs to be evaluated (e.g. diffuse only, or diffuse and glossy) */
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t types, bool force_eval = false)const = 0;

		/*! take a sample from the BSDF, given a 2-dimensional sample value and the BSDF types to be sampled from
			\param s s1, s2 and flags members give necessary information for creating the sample, pdf and sampledFlags need to be returned
			\param W returns the weight for importance sampling
		*/
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s, float &W)const = 0;// {return color_t(0.f);}
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t *const dir, color_t &tcol, sample_t &s, float *const W)const {return color_t(0.f);}

		virtual color_t sampleClay(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s, float &W)const 
		{
			vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
			wi = SampleCosHemisphere(N, sp.NU, sp.NV, s.s1, s.s2);
			s.pdf = std::fabs(wi*N);
			W = (std::fabs(wi*sp.N))/(s.pdf*0.99f + 0.01f);
			return color_t(1.0f);	//Clay color White 100%
		}
		/*! return the pdf for sampling the BSDF with wi and wo
		*/
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const {return 0.f;}


		/*! indicate whether light can (partially) pass the material without getting refracted,
			e.g. a curtain or even very thin foils approximated as single non-refractive layer.
			used to trace transparent shadows. Note that in this case, initBSDF was NOT called before!
		*/
		virtual bool isTransparent() const { return false; }

		/*!	used for computing transparent shadows.	Default implementation returns black (i.e. solid shadow).
			This is only used for shadow calculations and may only be called when isTransparent returned true.	*/
		virtual color_t getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const { return color_t(0.0); }
		/*! evaluate the specular components for given direction. Somewhat a specialization of sample(),
			because neither sample values nor pdf values are necessary for this.
			Typical use: recursive raytracing of integrators.
			\param dir dir[0] returns reflected direction, dir[1] refracted direction
			\param col col[0] returns reflected spectrum, dir[1] refracted spectrum */
		virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
								 bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const
		{ reflect=false; refract=false; }

		/*! get the overall reflectivity of the material (used to compute radiance map for example) */
		virtual color_t getReflectivity(const renderState_t &state, const surfacePoint_t &sp, BSDF_t flags)const;

		/*!	allow light emitting materials, for realizing correctly visible area lights.
			default implementation returns black obviously.	*/
		virtual color_t emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const { return color_t(0.0); }

		/*! get the volumetric handler for space at specified side of the surface
			\param inside true means space opposite of surface normal, which is considered "inside" */
		virtual const volumeHandler_t* getVolumeHandler(bool inside)const { return inside ? volI : volO; }

		/*! special function, get the alpha-value of a material, used to calculate the alpha-channel */
		virtual float getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const { return 1.f; }

		/*! specialized function for photon mapping. Default function uses the sample function, which will do fine for
			most materials unless there's a less expensive way or smarter scattering approach */
		virtual bool scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const;

		BSDF_t getFlags() const { return bsdfFlags; }
		/*! Materials may have to do surface point specific (pre-)calculation that need extra storage.
			returns the required amount of "userdata" memory for all the functions that require a render state */
		size_t getReqMem() const { return reqMem; }

		/*! Get materials IOR (for refracted photons) */

		virtual float getMatIOR() const { return 1.5f; }
	        virtual color_t getDiffuseColor(const renderState_t &state) const { return color_t(0.f); }
	        virtual color_t getGlossyColor(const renderState_t &state) const { return color_t(0.f); }
	        virtual color_t getTransColor(const renderState_t &state) const { return color_t(0.f); }
	        virtual color_t getMirrorColor(const renderState_t &state) const { return color_t(0.f); }
	        virtual color_t getSubSurfaceColor(const renderState_t &state) const { return color_t(0.f); }
	        void setMaterialIndex(const float &newMatIndex)
	        {
			materialIndex = newMatIndex;
			if(highestMaterialIndex < materialIndex) highestMaterialIndex = materialIndex;
		}
	        void resetMaterialIndex() { highestMaterialIndex = 1.f; materialIndexAuto = 0; }
	        void setMaterialIndex(const int &newMatIndex) { setMaterialIndex((float) newMatIndex); }
	        float getAbsMaterialIndex() const { return materialIndex; }
	        float getNormMaterialIndex() const { return (materialIndex / highestMaterialIndex); }
	        color_t getAbsMaterialIndexColor() const
		{
			return color_t(materialIndex);
		}
		color_t getNormMaterialIndexColor() const
		{
			float normalizedMaterialIndex = getNormMaterialIndex();
			return color_t(normalizedMaterialIndex);
		}
		color_t getAutoMaterialIndexColor() const
		{
			return materialIndexAutoColor;
		}
		color_t getAutoMaterialIndexNumber() const
		{
			return materialIndexAutoNumber;
		}

		visibility_t getVisibility() const { return mVisibility; }
		bool getReceiveShadows() const { return mReceiveShadows; }

		bool isFlat() const { return mFlatMaterial; }

		int getAdditionalDepth() const { return additionalDepth; }
		float getTransparentBiasFactor() const { return transparentBiasFactor; }
		bool getTransparentBiasMultiplyRayDepth() const { return transparentBiasMultiplyRayDepth; }
		
		void applyWireFrame(float & value, float wireFrameAmount, const surfacePoint_t &sp) const;
		void applyWireFrame(color_t & col, float wireFrameAmount, const surfacePoint_t &sp) const;
		void applyWireFrame(color_t *const col, float wireFrameAmount, const surfacePoint_t &sp) const;
		void applyWireFrame(colorA_t & col, float wireFrameAmount, const surfacePoint_t &sp) const;
		void applyWireFrame(colorA_t *const col, float wireFrameAmount, const surfacePoint_t &sp) const;

		void setSamplingFactor(const float &newSamplingFactor)
		{
			mSamplingFactor = newSamplingFactor;
			if(mHighestSamplingFactor < mSamplingFactor) mHighestSamplingFactor = mSamplingFactor;
		}
		float getSamplingFactor() const { return mSamplingFactor; }
		
	protected:
		/* small function to apply bump mapping to a surface point
			you need to determine the partial derivatives for NU and NV first, e.g. from a shader node */
        void applyBump(surfacePoint_t &sp, float dfdNU, float dfdNV) const;

		BSDF_t bsdfFlags;
		
		visibility_t mVisibility; //!< sets material visibility (Normal:visible, visible without shadows, invisible (shadows only) or totally invisible.

		bool mReceiveShadows; //!< enables/disables material reception of shadows.
		
		size_t reqMem; //!< the amount of "temporary" memory required to compute/store surface point specific data
		volumeHandler_t* volI; //!< volumetric handler for space inside material (opposed to surface normal)
		volumeHandler_t* volO; //!< volumetric handler for space outside ofmaterial (where surface normal points to)
	        float materialIndex;	//!< Material Index for the material-index render pass
		static unsigned int materialIndexAuto;	//!< Material Index automatically generated for the material-index-auto render pass
		color_t materialIndexAutoColor;	//!< Material Index color automatically generated for the material-index-auto (color) render pass
        float materialIndexAutoNumber = 0.f;	//!< Material Index number automatically generated for the material-index-auto-abs (numeric) render pass
		static float highestMaterialIndex;	//!< Class shared variable containing the highest material index used for the Normalized Material Index pass.
		int additionalDepth;	//!< Per-material additional ray-depth
		float transparentBiasFactor = 0.f;	//!< Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If >0.f this function is enabled and the result will no longer be realistic and may have artifacts.
		bool transparentBiasMultiplyRayDepth = false;	//!< Per-material additional ray-bias setting for transparency (trick to avoid black areas due to insufficient depth when many transparent surfaces stacked). If enabled the bias will be multiplied by the current ray depth so the first transparent surfaces are rendered better and subsequent surfaces might be skipped.

		float mWireFrameAmount = 0.f;           //!< Wireframe shading amount   
        float mWireFrameThickness = 0.01f;      //!< Wireframe thickness
        float mWireFrameExponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
        color_t mWireFrameColor = color_t(1.f); //!< Wireframe shading color
        
        float mSamplingFactor = 1.f;	//!< Material sampling factor, to allow some materials to receive more samples than others

        bool mFlatMaterial = false;		//!< Flat Material is a special non-photorealistic material that does not multiply the surface color by the cosine of the angle with the light, as happens in real life. Also, if receive_shadows is disabled, this flat material does no longer self-shadow. For special applications only.

		static float mHighestSamplingFactor;	//!< Class shared variable containing the highest material sampling factor. This is used to calculate the max. possible samples for the Sampling pass.        
};


inline void material_t::applyWireFrame(float & value, float wireFrameAmount, const surfacePoint_t &sp) const
{
	if(wireFrameAmount > 0.f && mWireFrameThickness > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= mWireFrameThickness)
		{
			if(mWireFrameExponent > 0.f)
			{
				wireFrameAmount *= std::pow((mWireFrameThickness - dist) / mWireFrameThickness, mWireFrameExponent);
			}
			value = value * (1.f - wireFrameAmount);
		}
	}
}

inline void material_t::applyWireFrame(color_t & col, float wireFrameAmount, const surfacePoint_t &sp) const
{
	if(wireFrameAmount > 0.f && mWireFrameThickness > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= mWireFrameThickness)
		{
			color_t wireFrameCol = mWireFrameColor * wireFrameAmount;
			if(mWireFrameExponent > 0.f)
			{
				wireFrameAmount *= std::pow((mWireFrameThickness - dist) / mWireFrameThickness, mWireFrameExponent);
			}
			col.blend(wireFrameCol, wireFrameAmount);
		}
	}
}

inline void material_t::applyWireFrame(color_t *const col, float wireFrameAmount, const surfacePoint_t &sp) const
{
	if(wireFrameAmount > 0.f && mWireFrameThickness > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= mWireFrameThickness)
		{
			color_t wireFrameCol = mWireFrameColor * wireFrameAmount;
			if(mWireFrameExponent > 0.f)
			{
				wireFrameAmount *= std::pow((mWireFrameThickness - dist) / mWireFrameThickness, mWireFrameExponent);
			}
            col[0].blend(wireFrameCol, wireFrameAmount);
            col[1].blend(wireFrameCol, wireFrameAmount);
		}
	}
}

inline void material_t::applyWireFrame(colorA_t & col, float wireFrameAmount, const surfacePoint_t &sp) const
{
	if(wireFrameAmount > 0.f && mWireFrameThickness > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= mWireFrameThickness)
		{
			color_t wireFrameCol = mWireFrameColor * wireFrameAmount;
			if(mWireFrameExponent > 0.f)
			{
				wireFrameAmount *= std::pow((mWireFrameThickness - dist) / mWireFrameThickness, mWireFrameExponent);
			}
			col.blend(wireFrameCol, wireFrameAmount);
			col.A = wireFrameAmount;
		}
	}
}

inline void material_t::applyWireFrame(colorA_t *const col, float wireFrameAmount, const surfacePoint_t &sp) const
{
	if(wireFrameAmount > 0.f && mWireFrameThickness > 0.f)
	{
		float dist = sp.getDistToNearestEdge();
		if(dist <= mWireFrameThickness)
		{
			color_t wireFrameCol = mWireFrameColor * wireFrameAmount;
			if(mWireFrameExponent > 0.f)
			{
				wireFrameAmount *= std::pow((mWireFrameThickness - dist) / mWireFrameThickness, mWireFrameExponent);
			}
            col[0].blend(wireFrameCol, wireFrameAmount);
			col[0].A = wireFrameAmount;
            col[1].blend(wireFrameCol, wireFrameAmount);
			col[1].A = wireFrameAmount;
		}
	}
}

__END_YAFRAY

#endif // Y_MATERIAL_H
