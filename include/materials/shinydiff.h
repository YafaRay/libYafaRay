#ifndef Y_SHINYDIFFMAT_H
#define Y_SHINYDIFFMAT_H

#include <yafray_config.h>
#include <yafraycore/nodematerial.h>
#include <core_api/shader.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY

/*! A general purpose material for basic diffuse and specular reflecting
	reflecting surfaces with transparency and translucency support.
	Parameter definitions are as follows:
	Of the incoming Light, the specular reflected part is substracted.
		l' = l*(1.0 - specular_refl)
	Of the remaining light (l') the specular transmitted light is substracted.
		l" = l'*(1.0 - specular_transmit)
	Of the remaining light (l") the diffuse transmitted light (translucency)
	is substracted.
		l"' =  l"*(1.0 - translucency)
	The remaining (l"') light is either reflected diffuse or absorbed.
	
*/

class shinyDiffuseMat_t: public nodeMaterial_t
{
	public:
		shinyDiffuseMat_t(const color_t &col, const color_t &srcol, float diffuse, float transp=0.0, float transl=0.0, float sp_refl=0.0, float emit=0.0);
		virtual ~shinyDiffuseMat_t();
		virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
		virtual bool isTransparent() const { return isTranspar; }
		virtual color_t getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
		virtual color_t emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const; // { return emitCol; }
		virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
								bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const;
		virtual CFLOAT getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
		virtual bool scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, pSample_t &s) const;
		
		static material_t* factory(paraMap_t &params, std::list<paraMap_t> &eparams, renderEnvironment_t &render);
		struct SDDat_t
		{
			float component[4];
			void *nodeStack;
		};
	protected:
		//color_t getDiffuse(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl)const;
		void config(shaderNode_t *diff, shaderNode_t *refl, shaderNode_t *transp, shaderNode_t *transl, shaderNode_t *bump);
		int getComponents(const bool *useNode, nodeStack_t &stack, float *component) const;
		CFLOAT getFresnel(const vector3d_t &wo, const vector3d_t &N) const;
		CFLOAT OrenNayar(const vector3d_t &wi, const vector3d_t &wo, const vector3d_t &N) const;
		void initOrenNayar(double sigma);
		
		bool isTranspar;
		bool isTransluc;
		bool isReflective; //!< refers to mirror reflection
		bool isDiffuse;
		bool fresnelEffect;
		bool viNodes[4], vdNodes[4]; //!< describes if the nodes are viewdependant or not (if available)
		shaderNode_t *diffuseS; //!< shader node for diffuse color
		shaderNode_t *bumpS; //!< shader node for bump
		shaderNode_t *transpS; //!< shader node for transparency strength (float)
		shaderNode_t *translS; //!< shader node for translucency strength (float)
		shaderNode_t *specReflS; //!< shader node for specular reflection strength (float)
		shaderNode_t *mirColS; //!< shader node for specular reflection color

		color_t color, emitCol, specRefCol;
		CFLOAT mSpecRefl, mTransp, mTransl, mDiffuse; // BSDF components when not textured
		CFLOAT filter; //!< determines how strong light passing through material gets tinted
		CFLOAT A, B; //!< OrenNayar coefficients
		bool orenNayar;
		PFLOAT IOR;
		int nBSDF;
		BSDF_t cFlags[4]; //!< list the BSDF components that are present
		int cIndex[4]; //!< list the index of the BSDF components (0=spec. refl, 1=spec. transp, 2=transl. 3=diff refl.)
		float emitVal;
};


__END_YAFRAY

#endif // Y_SHINYDIFFMAT_H
