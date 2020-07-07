
#ifndef Y_ROUGHGLASS_H
#define Y_ROUGHGLASS_H

#include <yafray_constants.h>
#include <yafraycore/nodematerial.h>
#include <core_api/scene.h>

__BEGIN_YAFRAY

class roughGlassMat_t: public nodeMaterial_t
{
	public:
		roughGlassMat_t(float IOR, color_t filtC, const color_t &srcol, bool fakeS, float alpha, float disp_pow, visibility_t eVisibility=NORMAL_VISIBLE);
        virtual void initBSDF(const renderState_t &state, surfacePoint_t &sp, unsigned int &bsdfTypes) const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s, float &W) const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t *const dir, color_t &tcol, sample_t &s, float *const W)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs, bool force_eval = false) const { return 0.f; }
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs) const { return 0.f; }
		virtual bool isTransparent() const { return fakeShadow; }
		virtual color_t getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const;
		virtual float getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const;
		virtual float getMatIOR() const;
		static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
		virtual color_t getGlossyColor(const renderState_t &state) const
		{
			nodeStack_t stack(state.userdata);
			return (mirColS ? mirColS->getColor(stack) : specRefCol);
		}
		virtual color_t getTransColor(const renderState_t &state) const
		{
			nodeStack_t stack(state.userdata);
			return (filterColS ? filterColS->getColor(stack) : filterCol);
		}
		virtual color_t getMirrorColor(const renderState_t &state) const
		{
			nodeStack_t stack(state.userdata);
			return (mirColS ? mirColS->getColor(stack) : specRefCol);
		}

	protected:
		shaderNode_t* bumpS = nullptr;
		shaderNode_t *mirColS = nullptr;
        shaderNode_t *roughnessS = nullptr;
        shaderNode_t *iorS = nullptr;
        shaderNode_t *filterColS = nullptr;
        shaderNode_t *mWireFrameShader = nullptr;     //!< Shader node for wireframe shading (float)
		color_t filterCol, specRefCol;
		color_t beer_sigma_a;
		float ior;
		float a2;
		float a;
		bool absorb = false, disperse = false, fakeShadow;
        float dispersion_power;
		float CauchyA, CauchyB;
};

__END_YAFRAY

#endif // Y_ROUGHGLASS_H
