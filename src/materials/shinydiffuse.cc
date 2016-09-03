
#include <materials/shinydiff.h>
#include <utilities/sample_utils.h>
#include <materials/microfacet.h>

__BEGIN_YAFRAY

shinyDiffuseMat_t::shinyDiffuseMat_t(const color_t &diffuseColor, const color_t &mirrorColor, float diffuseStrength, float transparencyStrength, float translucencyStrength, float mirrorStrength, float emitStrength, float transmitFilterStrength, visibility_t eVisibility):
            mDiffuseColor(diffuseColor), mMirrorColor(mirrorColor),
            mMirrorStrength(mirrorStrength), mTransparencyStrength(transparencyStrength), mTranslucencyStrength(translucencyStrength), mDiffuseStrength(diffuseStrength), mTransmitFilterStrength(transmitFilterStrength)
{
    mVisibility = eVisibility;
    mEmitColor = emitStrength * diffuseColor;
    mEmitStrength = emitStrength;
    bsdfFlags = BSDF_NONE;
    if(mEmitStrength > 0.f) bsdfFlags |= BSDF_EMIT;
    mVisibility = eVisibility;
}

shinyDiffuseMat_t::~shinyDiffuseMat_t()
{
    // Empty
}

/*! ATTENTION! You *MUST* call this function before using the material, no matter
    if you want to use shaderNodes or not!
*/
void shinyDiffuseMat_t::config()
{
    nBSDF=0;
    viNodes[0] = viNodes[1] = viNodes[2] = viNodes[3] = false;
    vdNodes[0] = vdNodes[1] = vdNodes[2] = vdNodes[3] = false;
    float acc = 1.f;
    if(mMirrorStrength > 0.00001f || mMirrorShader)
    {
        mIsMirror = true;
        if(mMirrorShader){ if(mMirrorShader->isViewDependant())vdNodes[0] = true; else viNodes[0] = true; }
        else if(!mHasFresnelEffect) acc = 1.f - mMirrorStrength;
        bsdfFlags |= BSDF_SPECULAR | BSDF_REFLECT;
        cFlags[nBSDF] = BSDF_SPECULAR | BSDF_REFLECT;
        cIndex[nBSDF] = 0;
        ++nBSDF;
    }
    if(mTransparencyStrength*acc > 0.00001f || mTransparencyShader)
    {
        mIsTransparent = true;
        if(mTransparencyShader){ if(mTransparencyShader->isViewDependant())vdNodes[1] = true; else viNodes[1] = true; }
        else acc *= 1.f - mTransparencyStrength;
        bsdfFlags |= BSDF_TRANSMIT | BSDF_FILTER;
        cFlags[nBSDF] = BSDF_TRANSMIT | BSDF_FILTER;
        cIndex[nBSDF] = 1;
        ++nBSDF;
    }
    if(mTranslucencyStrength*acc > 0.00001f || mTranslucencyShader)
    {
        mIsTranslucent = true;
        if(mTranslucencyShader){ if(mTranslucencyShader->isViewDependant())vdNodes[2] = true; else viNodes[2] = true; }
        else acc *= 1.f - mTransparencyStrength;
        bsdfFlags |= BSDF_DIFFUSE | BSDF_TRANSMIT;
        cFlags[nBSDF] = BSDF_DIFFUSE | BSDF_TRANSMIT;
        cIndex[nBSDF] = 2;
        ++nBSDF;
    }
    if(mDiffuseStrength*acc > 0.00001f)
    {
        mIsDiffuse = true;
        if(mDiffuseShader){ if(mDiffuseShader->isViewDependant())vdNodes[3] = true; else viNodes[3] = true; }
        bsdfFlags |= BSDF_DIFFUSE | BSDF_REFLECT;
        cFlags[nBSDF] = BSDF_DIFFUSE | BSDF_REFLECT;
        cIndex[nBSDF] = 3;
        ++nBSDF;
    }
    reqMem = reqNodeMem + sizeof(SDDat_t);
}

// component should be initialized with mMirrorStrength, mTransparencyStrength, mTranslucencyStrength, mDiffuseStrength
// since values for which useNode is false do not get touched so it can be applied
// twice, for view-independent (initBSDF) and view-dependent (sample/eval) nodes

int shinyDiffuseMat_t::getComponents(const bool *useNode, nodeStack_t &stack, float *component) const
{
    if(mIsMirror)
    {
        component[0] = useNode[0] ? mMirrorShader->getScalar(stack) : mMirrorStrength;
    }
    if(mIsTransparent)
    {
        component[1] = useNode[1] ? mTransparencyShader->getScalar(stack) : mTransparencyStrength;
    }
    if(mIsTranslucent)
    {
        component[2] = useNode[2] ? mTranslucencyShader->getScalar(stack) : mTranslucencyStrength;
    }
    if(mIsDiffuse)
    {
        component[3] = mDiffuseStrength;
    }
    return 0;
}

inline void shinyDiffuseMat_t::getFresnel(const vector3d_t &wo, const vector3d_t &n, float &Kr, float &currentIORSquared) const
{
    if(mHasFresnelEffect)
    {
        vector3d_t N;

        if((wo*n) < 0.f)
        {
            N=-n;
        }
        else
        {
            N=n;
        }

        float c = wo*N;
        float g = currentIORSquared + c*c - 1.f;
        if(g < 0.f) g = 0.f;
        else g = fSqrt(g);
        float aux = c * (g+c);

        Kr = ( ( 0.5f * (g-c) * (g-c) )/( (g+c)*(g+c) ) ) *
               ( 1.f + ((aux-1)*(aux-1))/((aux+1)*(aux+1)) );
    }
    else
    {
        Kr = 1.f;
    }
}

// calculate the absolute value of scattering components from the "normalized"
// fractions which are between 0 (no scattering) and 1 (scatter all remaining light)
// Kr is an optional reflection multiplier (e.g. from Fresnel)
static inline void accumulate(const float *component, float *accum, float Kr)
{
    accum[0] = component[0]*Kr;
    float acc = 1.f - accum[0];
    accum[1] = component[1] * acc;
    acc *= 1.f - component[1];
    accum[2] = component[2] * acc;
    acc *= 1.f - component[2];
    accum[3] = component[3] * acc;
}

void shinyDiffuseMat_t::initBSDF(const renderState_t &state, surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
    SDDat_t *dat = (SDDat_t *)state.userdata;
    memset(dat, 0, 8*sizeof(float));
    dat->nodeStack = (char*)state.userdata + sizeof(SDDat_t);
    //create our "stack" to save node results
    nodeStack_t stack(dat->nodeStack);

    //bump mapping (extremely experimental)
    if(mBumpShader)
    {
        evalBump(stack, state, sp, mBumpShader);
    }

    //eval viewindependent nodes
    auto end=allViewindep.end();
    for(auto iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
    bsdfTypes=bsdfFlags;

    getComponents(viNodes, stack, dat->component);
}

/** Initialize Oren Nayar reflectance.
 *  Initialize Oren Nayar A and B coefficient.
 *  @param  sigma Roughness of the surface
 */
void shinyDiffuseMat_t::initOrenNayar(double sigma)
{
    double sigma_squared = sigma * sigma;
    mOrenNayar_A = 1.0 - 0.5 * (sigma_squared / (sigma_squared + 0.33));
    mOrenNayar_B = 0.45 * sigma_squared / (sigma_squared + 0.09);
    mUseOrenNayar = true;
}

/** Calculate Oren Nayar reflectance.
 *  Calculate Oren Nayar reflectance for a given reflection.
 *  @param  wi Reflected ray direction
 *  @param  wo Incident ray direction
 *  @param  N  Surface normal
 *  @note   http://en.wikipedia.org/wiki/Oren-Nayar_reflectance_model
 */
float shinyDiffuseMat_t::OrenNayar(const vector3d_t &wi, const vector3d_t &wo, const vector3d_t &N, bool useTextureSigma, double textureSigma) const
{
    float cos_ti = std::max(-1.f,std::min(1.f,N*wi));
    float cos_to = std::max(-1.f,std::min(1.f,N*wo));
    float maxcos_f = 0.f;

    if(cos_ti < 0.9999f && cos_to < 0.9999f)
    {
        vector3d_t v1 = (wi - N*cos_ti).normalize();
        vector3d_t v2 = (wo - N*cos_to).normalize();
        maxcos_f = std::max(0.f, v1*v2);
    }

    float sin_alpha, tan_beta;

    if(cos_to >= cos_ti)
    {
        sin_alpha = fSqrt(1.f - cos_ti*cos_ti);
        tan_beta = fSqrt(1.f - cos_to*cos_to) / ((cos_to == 0.f)?1e-8f:cos_to); // white (black on windows) dots fix for oren-nayar, could happen with bad normals
    }
    else
    {
        sin_alpha = fSqrt(1.f - cos_to*cos_to);
        tan_beta = fSqrt(1.f - cos_ti*cos_ti) / ((cos_ti == 0.f)?1e-8f:cos_ti); // white (black on windows) dots fix for oren-nayar, could happen with bad normals
    }   

    if (useTextureSigma)
    {
        double sigma_squared = textureSigma * textureSigma;
        double mOrenNayar_TextureA = 1.0 - 0.5 * (sigma_squared / (sigma_squared + 0.33));
        double mOrenNayar_TextureB = 0.45 * sigma_squared / (sigma_squared + 0.09);
        return std::min(1.f, std::max(0.f, (float) (mOrenNayar_TextureA + mOrenNayar_TextureB * maxcos_f * sin_alpha * tan_beta)));
    }
    else
    {
        return std::min(1.f, std::max(0.f, (float) (mOrenNayar_A + mOrenNayar_B * maxcos_f * sin_alpha * tan_beta)));
    }
}


color_t shinyDiffuseMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs, bool force_eval)const
{
    float cos_Ng_wo = sp.Ng*wo;
    float cos_Ng_wl = sp.Ng*wl;
    // face forward:
    vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
    if(!(bsdfs & bsdfFlags & BSDF_DIFFUSE)) return color_t(0.f);

    SDDat_t *dat = (SDDat_t *)state.userdata;
    nodeStack_t stack(dat->nodeStack);

    float Kr;

    float cur_ior_squared;
    if(iorS)
    {
        cur_ior_squared = IOR + iorS->getScalar(stack);
        cur_ior_squared *= cur_ior_squared;            
    }
    else cur_ior_squared = mIOR_Squared;
    
    getFresnel(wo, N, Kr, cur_ior_squared);
    float mT = (1.f - Kr*dat->component[0])*(1.f - dat->component[1]);

    bool transmit = ( cos_Ng_wo * cos_Ng_wl ) < 0.f;

    if(transmit) // light comes from opposite side of surface
    {
        if(mIsTranslucent) return dat->component[2] * mT * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor);
    }

    if(N*wl < 0.0) return color_t(0.f);
    float mD = mT*(1.f - dat->component[2]) * dat->component[3];

    if(mUseOrenNayar)
    {
        double textureSigma=(mSigmaOrenShader ? mSigmaOrenShader->getScalar(stack) : 0.f);
        bool useTextureSigma=(mSigmaOrenShader ? true : false);
        if(mUseOrenNayar) mD *= OrenNayar(wo, wl, N, useTextureSigma, textureSigma);
    }

    if(mDiffuseReflShader) mD *= mDiffuseReflShader->getScalar(stack);
    
    color_t result = mD * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor);

    float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
    applyWireFrame(result, wireFrameAmount, sp);

    return result;
}

color_t shinyDiffuseMat_t::emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
    SDDat_t *dat = (SDDat_t *)state.userdata;
    nodeStack_t stack(dat->nodeStack);

    color_t result = (mDiffuseShader ? mDiffuseShader->getColor(stack) * mEmitStrength : mEmitColor);
        
    float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
    applyWireFrame(result, wireFrameAmount, sp);

    return result;
}

color_t shinyDiffuseMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s, float &W)const
{
    float accumC[4];
    float cos_Ng_wo = sp.Ng*wo, cos_Ng_wi, cos_N;
    vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);

    SDDat_t *dat = (SDDat_t *)state.userdata;
    nodeStack_t stack(dat->nodeStack);

    float Kr;

    float cur_ior_squared;
    if(iorS)
    {
        cur_ior_squared = IOR + iorS->getScalar(stack);
        cur_ior_squared *= cur_ior_squared;            
    }
    else cur_ior_squared = mIOR_Squared;

    getFresnel(wo, N, Kr, cur_ior_squared);
    accumulate(dat->component, accumC, Kr);

    float sum=0.f, val[4], width[4];
    BSDF_t choice[4];
    int nMatch=0, pick=-1;
    for(int i=0; i<nBSDF; ++i)
    {
        if((s.flags & cFlags[i]) == cFlags[i])
        {
            width[nMatch] = accumC[cIndex[i]];
            sum += width[nMatch];
            choice[nMatch] = cFlags[i];
            val[nMatch] = sum;
            ++nMatch;
        }
    }
    if(!nMatch || sum < 0.00001){ s.sampledFlags=BSDF_NONE; s.pdf=0.f; return color_t(1.f); }
    float inv_sum = 1.f/sum;
    for(int i=0; i<nMatch; ++i)
    {
        val[i] *= inv_sum;
        width[i] *= inv_sum;
        if((s.s1 <= val[i]) && (pick<0 ))   pick = i;
    }
    if(pick<0) pick=nMatch-1;
    float s1;
    if(pick>0) s1 = (s.s1 - val[pick-1]) / width[pick];
    else       s1 = s.s1 / width[pick];

    color_t scolor(0.f);
    switch(choice[pick])
    {
        case (BSDF_SPECULAR | BSDF_REFLECT): // specular reflect
            wi = reflect_dir(N, wo);
            s.pdf = width[pick];
            scolor = (mMirrorColorShader ? mMirrorColorShader->getColor(stack) : mMirrorColor) * (accumC[0]);
            if(s.reverse)
            {
                s.pdf_back = s.pdf;
                s.col_back = scolor/std::max(std::fabs(sp.N*wo), 1.0e-6f);
            }
            scolor *= 1.f/std::max(std::fabs(sp.N*wi), 1.0e-6f);
            break;
        case (BSDF_TRANSMIT | BSDF_FILTER): // "specular" transmit
            wi = -wo;
            scolor = accumC[1] * (mTransmitFilterStrength*(mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor) + color_t(1.f-mTransmitFilterStrength) );
            cos_N = std::fabs(wi*N);
            if(cos_N < 1e-6) s.pdf = 0.f;
            else s.pdf = width[pick];
            break;
        case (BSDF_DIFFUSE | BSDF_TRANSMIT): // translucency (diffuse transmitt)
            wi = SampleCosHemisphere(-N, sp.NU, sp.NV, s1, s.s2);
            cos_Ng_wi = sp.Ng*wi;
            if(cos_Ng_wo*cos_Ng_wi < 0) scolor = accumC[2] * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor);
            s.pdf = std::fabs(wi*N) * width[pick]; break;
        case (BSDF_DIFFUSE | BSDF_REFLECT): // diffuse reflect
        default:
            wi = SampleCosHemisphere(N, sp.NU, sp.NV, s1, s.s2);
            cos_Ng_wi = sp.Ng*wi;
            if(cos_Ng_wo*cos_Ng_wi > 0) scolor = accumC[3] * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor);

            if(mUseOrenNayar)
            {
                double textureSigma=(mSigmaOrenShader ? mSigmaOrenShader->getScalar(stack) : 0.f);
                bool useTextureSigma=(mSigmaOrenShader ? true : false);
        
                scolor *= OrenNayar(wo, wi, N, useTextureSigma, textureSigma);
            }
            s.pdf = std::fabs(wi*N) * width[pick]; break;
    }
    s.sampledFlags = choice[pick];
    W = (std::fabs(wi*sp.N))/(s.pdf*0.99f + 0.01f);
    
    float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
    applyWireFrame(scolor, wireFrameAmount, sp);
    
    return scolor;
}

float shinyDiffuseMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
    if(!(bsdfs & BSDF_DIFFUSE)) return 0.f;

    SDDat_t *dat = (SDDat_t *)state.userdata;
    nodeStack_t stack(dat->nodeStack);
    
    float pdf=0.f;
    float accumC[4];
    float cos_Ng_wo = sp.Ng*wo, cos_Ng_wi;
    vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
    float Kr;

    float cur_ior_squared;
    if(iorS)
    {
        cur_ior_squared = IOR + iorS->getScalar(stack);
        cur_ior_squared *= cur_ior_squared;            
    }
    else cur_ior_squared = mIOR_Squared;

    getFresnel(wo, N, Kr, cur_ior_squared);

    accumulate(dat->component, accumC, Kr);
    float sum=0.f, width;
    int nMatch=0;
    for(int i=0; i<nBSDF; ++i)
    {
        if((bsdfs & cFlags[i]))
        {
            width = accumC[cIndex[i]];
            sum += width;

            switch(cFlags[i])
            {
                case (BSDF_DIFFUSE | BSDF_TRANSMIT): // translucency (diffuse transmitt)
                    cos_Ng_wi = sp.Ng*wi;
                    if(cos_Ng_wo*cos_Ng_wi < 0) pdf += std::fabs(wi*N) * width;
                    break;

                case (BSDF_DIFFUSE | BSDF_REFLECT): // lambertian
                    cos_Ng_wi = sp.Ng*wi;
                    pdf += std::fabs(wi*N) * width;
                    break;
            }
            ++nMatch;
        }
    }
    if(!nMatch || sum < 0.00001) return 0.f;
    return pdf / sum;
}


/** Perfect specular reflection.
 *  Calculate perfect specular reflection and refraction from the material for
 *  a given surface point \a sp and a given incident ray direction \a wo
 *  @param  state Render state
 *  @param  sp Surface point
 *  @param  wo Incident ray direction
 *  @param  doReflect Boolean value which is true if you have a reflection, false otherwise
 *  @param  doRefract Boolean value which is true if you have a refraction, false otherwise
 *  @param  wi Array of two vectors to record reflected ray direction (wi[0]) and refracted ray direction (wi[1])
 *  @param  col Array of two colors to record reflected ray color (col[0]) and refracted ray color (col[1])
 */
void shinyDiffuseMat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, bool &doReflect, bool &doRefract, vector3d_t *const wi, color_t *const col)const
{
    SDDat_t *dat = (SDDat_t *)state.userdata;
    nodeStack_t stack(dat->nodeStack);

    const bool backface = wo * sp.Ng < 0.f;
    const vector3d_t N  = backface ? -sp.N  : sp.N;
    const vector3d_t Ng = backface ? -sp.Ng : sp.Ng;

    float Kr;

    float cur_ior_squared;
    if(iorS)
    {
        cur_ior_squared = IOR + iorS->getScalar(stack);
        cur_ior_squared *= cur_ior_squared;            
    }
    else cur_ior_squared = mIOR_Squared;

    getFresnel(wo, N, Kr, cur_ior_squared);

    if(mIsTransparent)
    {
        doRefract = true;
        wi[1] = -wo;
        color_t tcol = mTransmitFilterStrength * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor) + color_t(1.f-mTransmitFilterStrength);
        col[1] = (1.f - dat->component[0]*Kr) * dat->component[1] * tcol;
    }
    else
    {
        doRefract = false;
    }

    if(mIsMirror)
    {
        doReflect = true;
        //Y_WARNING << sp.N << " | " << N << yendl;
        wi[0] = wo;
        wi[0].reflect(N);
        float cos_wi_Ng = wi[0]*Ng;
        if(cos_wi_Ng < 0.01)
        {
            wi[0] += (0.01-cos_wi_Ng)*Ng;
            wi[0].normalize();
        }
        col[0] = (mMirrorColorShader ? mMirrorColorShader->getColor(stack) : mMirrorColor) * (dat->component[0]*Kr);
    }
    else
    {
        doReflect = false;
    }

    float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
    applyWireFrame(col, wireFrameAmount, sp);
}

color_t shinyDiffuseMat_t::getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
    if(!mIsTransparent) return color_t(0.f);
    
    nodeStack_t stack(state.userdata);
    auto end=allSorted.end();
    for(auto iter = allSorted.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
    float accum=1.f;
    float Kr;
    vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);

    float cur_ior_squared;
    if(iorS)
    {
        cur_ior_squared = IOR + iorS->getScalar(stack);
        cur_ior_squared *= cur_ior_squared;            
    }
    else cur_ior_squared = mIOR_Squared;

    getFresnel(wo, N, Kr, cur_ior_squared);

    if(mIsMirror)
    {
        accum = 1.f - Kr*(mMirrorShader ? mMirrorShader->getScalar(stack) : mMirrorStrength);
    }
    if(mIsTransparent) //uhm...should actually be true if this function gets called anyway...
    {
        accum *= mTransparencyShader ? mTransparencyShader->getScalar(stack) * accum : mTransparencyStrength * accum;
    }
    color_t tcol = mTransmitFilterStrength * (mDiffuseShader ? mDiffuseShader->getColor(stack) : mDiffuseColor) + color_t(1.f-mTransmitFilterStrength);
    
    color_t result = accum * tcol;

    float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
    applyWireFrame(result, wireFrameAmount, sp);

    return result;
}

float shinyDiffuseMat_t::getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
    SDDat_t *dat = (SDDat_t *)state.userdata;
    nodeStack_t stack(dat->nodeStack);
    
    if(mIsTransparent)
    {
        vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
        float Kr;

        float cur_ior_squared;
        if(iorS)
        {
            cur_ior_squared = IOR + iorS->getScalar(stack);
            cur_ior_squared *= cur_ior_squared;            
        }
        else cur_ior_squared = mIOR_Squared;

        getFresnel(wo, N, Kr, cur_ior_squared);
        float refl = (1.f - dat->component[0]*Kr) * dat->component[1];
        float result = 1.f - refl;

        float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
        applyWireFrame(result, wireFrameAmount, sp);

        return result;
    }
    return 1.f;
}

material_t* shinyDiffuseMat_t::factory(paraMap_t &params, std::list<paraMap_t> &paramsList, renderEnvironment_t &render)
{
    /// Material Parameters
    color_t diffuseColor=1.f;
    color_t mirrorColor=1.f;
    float diffuseStrength=1.f;
    float transparencyStrength=0.f;
    float translucencyStrength=0.f;
    float mirrorStrength=0.f;
    float emitStrength = 0.f;
    bool hasFresnelEffect=false;
    std::string sVisibility = "normal";
	visibility_t visibility = NORMAL_VISIBLE;
    bool receive_shadows = true;
    float IOR = 1.33f;
    double transmitFilterStrength=1.0;
    int mat_pass_index = 0;
	int additionaldepth = 0;
	float samplingfactor = 1.f;
    float WireFrameAmount = 0.f;           //!< Wireframe shading amount   
    float WireFrameThickness = 0.01f;      //!< Wireframe thickness
    float WireFrameExponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
    color_t WireFrameColor = color_t(1.f); //!< Wireframe shading color

    params.getParam("color",            diffuseColor);
    params.getParam("mirror_color",     mirrorColor);
    params.getParam("transparency",     transparencyStrength);
    params.getParam("translucency",     translucencyStrength);
    params.getParam("diffuse_reflect",  diffuseStrength);
    params.getParam("specular_reflect", mirrorStrength);
    params.getParam("emit",             emitStrength);
    params.getParam("IOR",              IOR);
    params.getParam("fresnel_effect",   hasFresnelEffect);
    params.getParam("transmit_filter",  transmitFilterStrength);
    
    params.getParam("receive_shadows",  receive_shadows);
    params.getParam("visibility",       sVisibility);
    params.getParam("mat_pass_index",   mat_pass_index);
	params.getParam("additionaldepth",   additionaldepth);
	params.getParam("samplingfactor",   samplingfactor);
	
    params.getParam("wireframe_amount",  WireFrameAmount);
    params.getParam("wireframe_thickness",  WireFrameThickness);
    params.getParam("wireframe_exponent",  WireFrameExponent);
    params.getParam("wireframe_color",  WireFrameColor);
    
	if(sVisibility == "normal") visibility = NORMAL_VISIBLE;
	else if(sVisibility == "no_shadows") visibility = VISIBLE_NO_SHADOWS;
	else if(sVisibility == "shadow_only") visibility = INVISIBLE_SHADOWS_ONLY;
	else if(sVisibility == "invisible") visibility = INVISIBLE;
	else visibility = NORMAL_VISIBLE;

    // !!remember to put diffuse multiplier in material itself!
    shinyDiffuseMat_t *mat = new shinyDiffuseMat_t(diffuseColor, mirrorColor, diffuseStrength, transparencyStrength, translucencyStrength, mirrorStrength, emitStrength, transmitFilterStrength, visibility);

    mat->setMaterialIndex(mat_pass_index);
    mat->mReceiveShadows = receive_shadows;
	mat->additionalDepth = additionaldepth;

    mat->mWireFrameAmount = WireFrameAmount;
    mat->mWireFrameThickness = WireFrameThickness;
    mat->mWireFrameExponent = WireFrameExponent;
    mat->mWireFrameColor = WireFrameColor;

	mat->setSamplingFactor(samplingfactor);

    if(hasFresnelEffect)
    {
        mat->IOR = IOR;
        mat->mIOR_Squared = IOR * IOR;
        mat->mHasFresnelEffect = true;
    }

    const std::string *name=0;
    if(params.getParam("diffuse_brdf", name))
    {
        if(*name == "oren_nayar")
        {
            double sigma=0.1;
            params.getParam("sigma", sigma);
            mat->initOrenNayar(sigma);
        }
    }

    /// Material Shader Nodes
    std::vector<shaderNode_t *> roots;
    std::map<std::string, shaderNode_t *> nodeList;

    // prepare shader nodes list
    nodeList["diffuse_shader"]      = nullptr;
    nodeList["mirror_color_shader"] = nullptr;
    nodeList["bump_shader"]         = nullptr;
    nodeList["mirror_shader"]       = nullptr;
    nodeList["transparency_shader"] = nullptr;
    nodeList["translucency_shader"] = nullptr;
    nodeList["sigma_oren_shader"]   = nullptr;
    nodeList["diffuse_refl_shader"] = nullptr;
    nodeList["IOR_shader"]          = nullptr;
    nodeList["wireframe_shader"]    = nullptr;

    // load shader nodes:
    if(mat->loadNodes(paramsList, render))
    {
        mat->parseNodes(params, roots, nodeList);
    }
    else Y_ERROR << "ShinyDiffuse: Loading shader nodes failed!" << yendl;

    mat->mDiffuseShader      = nodeList["diffuse_shader"];
    mat->mMirrorColorShader  = nodeList["mirror_color_shader"];
    mat->mBumpShader         = nodeList["bump_shader"];
    mat->mMirrorShader       = nodeList["mirror_shader"];
    mat->mTransparencyShader = nodeList["transparency_shader"];
    mat->mTranslucencyShader = nodeList["translucency_shader"];
    mat->mSigmaOrenShader    = nodeList["sigma_oren_shader"];
    mat->mDiffuseReflShader  = nodeList["diffuse_refl_shader"];
    mat->iorS                = nodeList["IOR_shader"];
    mat->mWireFrameShader    = nodeList["wireframe_shader"];
    
    // solve nodes order
    if(!roots.empty())
    {
        mat->solveNodesOrder(roots);

        std::vector<shaderNode_t *> colorNodes;

        if(mat->mDiffuseShader)      mat->getNodeList(mat->mDiffuseShader, colorNodes);
        if(mat->mMirrorColorShader)  mat->getNodeList(mat->mMirrorColorShader, colorNodes);
        if(mat->mMirrorShader)       mat->getNodeList(mat->mMirrorShader, colorNodes);
        if(mat->mTransparencyShader) mat->getNodeList(mat->mTransparencyShader, colorNodes);
        if(mat->mTranslucencyShader) mat->getNodeList(mat->mTranslucencyShader, colorNodes);
        if(mat->mSigmaOrenShader)    mat->getNodeList(mat->mSigmaOrenShader, colorNodes);
        if(mat->mDiffuseReflShader)  mat->getNodeList(mat->mDiffuseReflShader, colorNodes);
        if(mat->iorS)                mat->getNodeList(mat->iorS, colorNodes);
        if(mat->mWireFrameShader)    mat->getNodeList(mat->mWireFrameShader, colorNodes);
        
        mat->filterNodes(colorNodes, mat->allViewdep,   VIEW_DEP);
        mat->filterNodes(colorNodes, mat->allViewindep, VIEW_INDEP);

        if(mat->mBumpShader)         mat->getNodeList(mat->mBumpShader, mat->bumpNodes);
    }


    mat->config();

    //===!!!=== test <<< This test should go, is useless, DT
    /*if(params.getParam("name", name))
    {
        if(name->substr(0, 6) == "MAsss_")
        {
            paraMap_t map;
            map["type"] = std::string("sss");
            map["absorption_col"] = color_t(0.5f, 0.2f, 0.2f);
            map["absorption_dist"] = 0.5f;
            map["scatter_col"] = color_t(0.9f);
            mat->volI = render.createVolumeH(*name, map);
            mat->bsdfFlags |= BSDF_VOLUMETRIC;
        }
    }*/
    //===!!!=== end of test

    return mat;
}

extern "C"
{

    YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
    {
        render.registerFactory("shinydiffusemat", shinyDiffuseMat_t::factory);
    }

}

__END_YAFRAY
