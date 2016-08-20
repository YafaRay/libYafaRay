#ifndef Y_SHINYDIFFMAT_H
#define Y_SHINYDIFFMAT_H

#include <yafray_config.h>
#include <yafraycore/nodematerial.h>
#include <core_api/shader.h>
#include <core_api/environment.h>
#include <core_api/color_ramp.h>

__BEGIN_YAFRAY

/*! A general purpose material for basic diffuse and specular reflecting
    surfaces with transparency and translucency support.
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
        shinyDiffuseMat_t(const color_t &diffuseColor, const color_t &mirrorColor, float diffuseStrength, float transparencyStrength=0.0, float translucencyStrength=0.0, float mirrorStrength=0.0, float emitStrength=0.0, float transmitFilterStrength=1.0, visibility_t eVisibility=NORMAL_VISIBLE);
        virtual ~shinyDiffuseMat_t();
        virtual void initBSDF(const renderState_t &state, surfacePoint_t &sp, BSDF_t &bsdfTypes)const;
        virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs, bool force_eval = false)const;
        virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s, float &W)const;
        virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
        virtual bool isTransparent() const { return mIsTransparent; }
        virtual color_t getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
        virtual color_t emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const; // { return emitCol; }
        virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const;
        virtual float getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
        virtual color_t getDiffuseColor(const renderState_t &state) const
        {
                SDDat_t *dat = (SDDat_t *)state.userdata;
                nodeStack_t stack(dat->nodeStack);
                
                if(mIsDiffuse) return (mDiffuseReflShader ? mDiffuseReflShader->getScalar(stack) : mDiffuseStrength) * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor);
                else return color_t(0.f);
        }
        virtual color_t getGlossyColor(const renderState_t &state) const
        {
                SDDat_t *dat = (SDDat_t *)state.userdata;
                nodeStack_t stack(dat->nodeStack);
                
                if(mIsMirror) return (mMirrorShader ? mMirrorShader->getScalar(stack) : mMirrorStrength) * (mMirrorColorShader ? mMirrorColorShader->getColor(stack) : mMirrorColor);
                else return color_t(0.f);
        }
        virtual color_t getTransColor(const renderState_t &state) const
        {
                SDDat_t *dat = (SDDat_t *)state.userdata;
                nodeStack_t stack(dat->nodeStack);
                
                if(mIsTransparent) return (mTransparencyShader ? mTransparencyShader->getScalar(stack) : mTransparencyStrength) * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor);
                else return color_t(0.f);
        }
        virtual color_t getMirrorColor(const renderState_t &state) const
        {
                SDDat_t *dat = (SDDat_t *)state.userdata;
                nodeStack_t stack(dat->nodeStack);
                
                if(mIsMirror) return (mMirrorShader ? mMirrorShader->getScalar(stack) : mMirrorStrength) * (mMirrorColorShader ? mMirrorColorShader->getColor(stack) : mMirrorColor);
                else return color_t(0.f);
        }
        virtual color_t getSubSurfaceColor(const renderState_t &state) const
        {
                SDDat_t *dat = (SDDat_t *)state.userdata;
                nodeStack_t stack(dat->nodeStack);
                
                if(mIsTranslucent) return (mTranslucencyShader ? mTranslucencyShader->getScalar(stack) : mTranslucencyStrength) * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor);
                else return color_t(0.f);
        }

        static material_t* factory(paraMap_t &params, std::list<paraMap_t> &eparams, renderEnvironment_t &render);

        struct SDDat_t
        {
            float component[4];
            void *nodeStack;
        };

    protected:
        void config();
        int getComponents(const bool *useNode, nodeStack_t &stack, float *component) const;
        void getFresnel(const vector3d_t &wo, const vector3d_t &N, float &Kr, float &currentIORSquared) const;

        void initOrenNayar(double sigma);
        float OrenNayar(const vector3d_t &wi, const vector3d_t &wo, const vector3d_t &N, bool useTextureSigma, double textureSigma) const;

        bool mIsTransparent = false;                  //!< Boolean value which is true if you have transparent component
        bool mIsTranslucent = false;                  //!< Boolean value which is true if you have translucent component
        bool mIsMirror = false;                       //!< Boolean value which is true if you have specular reflection component
        bool mIsDiffuse = false;                      //!< Boolean value which is true if you have diffuse component

        bool mHasFresnelEffect = false;               //!< Boolean value which is true if you have Fresnel specular effect
        float IOR = 1.f;                              //!< IOR
        float mIOR_Squared = 1.f;                     //!< Squared IOR

        bool viNodes[4], vdNodes[4];                  //!< describes if the nodes are viewdependant or not (if available)
        shaderNode_t *mDiffuseShader = nullptr;       //!< Shader node for diffuse color
        shaderNode_t *mBumpShader = nullptr;          //!< Shader node for bump
        shaderNode_t *mTransparencyShader = nullptr;  //!< Shader node for transparency strength (float)
        shaderNode_t *mTranslucencyShader = nullptr;  //!< Shader node for translucency strength (float)
        shaderNode_t *mMirrorShader = nullptr;        //!< Shader node for specular reflection strength (float)
        shaderNode_t *mMirrorColorShader = nullptr;   //!< Shader node for specular reflection color
        shaderNode_t *mSigmaOrenShader = nullptr;     //!< Shader node for sigma in Oren Nayar material
        shaderNode_t *mDiffuseReflShader = nullptr;   //!< Shader node for diffuse reflection strength (float)
        shaderNode_t *iorS = nullptr;                 //!< Shader node for IOR value (float)
        shaderNode_t *mWireFrameShader = nullptr;     //!< Shader node for wireframe shading (float)

        color_t mDiffuseColor;              //!< BSDF Diffuse component color
        color_t mEmitColor;                 //!< Emit color
        color_t mMirrorColor;               //!< BSDF Mirror component color
        float mMirrorStrength;              //!< BSDF Specular reflection component strength when not textured
        float mTransparencyStrength;        //!< BSDF Transparency component strength when not textured
        float mTranslucencyStrength;        //!< BSDF Translucency component strength when not textured
        float mDiffuseStrength;             //!< BSDF Diffuse component strength when not textured
        float mEmitStrength;                //!< Emit strength
        float mTransmitFilterStrength;      //!< determines how strong light passing through material gets tinted
        
        bool mUseOrenNayar = false;         //!< Use Oren Nayar reflectance (default Lambertian)
        float mOrenNayar_A = 0.f;           //!< Oren Nayar A coefficient
        float mOrenNayar_B = 0.f;           //!< Oren Nayar B coefficient

        int nBSDF = 0;

        BSDF_t cFlags[4];                   //!< list the BSDF components that are present
        int cIndex[4];                      //!< list the index of the BSDF components (0=specular reflection, 1=specular transparency, 2=translucency, 3=diffuse reflection)
};


__END_YAFRAY

#endif // Y_SHINYDIFFMAT_H
