
#ifndef Y_MICROFACET_H
#define Y_MICROFACET_H

#include <core_api/surface.h>
#include <core_api/vector3d.h>

__BEGIN_YAFRAY

#define NOTANGENT 0
#define TANGENT_U 1
#define TANGENT_V 2
#define RAW_VMAP 3

#define DIFFUSE_RATIO 1.21739130434782608696 // (28 / 23)
#define pdfDivisor(cos) (float)(8.f * cos)
#define ASDivisor(cos1, cosI, cosO) ( 8.f * cos1 * std::max(cosI, cosO) )

inline void sample_quadrant_aniso(vector3d_t &H, float s1, float s2, float e_u, float e_v)
{
	float phi = atan(fSqrt((e_u + 1.f)/(e_v + 1.f)) * fTan(M_PI_2 * s1));
	float cosPhi = fCos(phi);
	float sinPhi;
	float cosTheta, sinTheta;
	float cosPhi2 = cosPhi*cosPhi;
	float sinPhi2 = 1.f - cosPhi2;
	
	cosTheta = fPow(1.f - s2, 1.f / (e_u*cosPhi2 + e_v*sinPhi2 + 1.f));
	sinTheta = fSqrt(1.f - cosTheta*cosTheta);
	sinPhi = fSqrt(sinPhi2);
	
	H = vector3d_t(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

inline float AS_Aniso_D(vector3d_t h, float e_u, float e_v)
{
	if(h.z <= 0.f) return 0.f;
	float exponent = (e_u * h.x*h.x + e_v * h.y*h.y)/(1.f - h.z*h.z);
	return fSqrt( (e_u + 1.f)*(e_v + 1.f) ) * M_1_2PI * fPow(std::max(0.f, h.z), exponent);
}

inline float AS_Aniso_Pdf(vector3d_t h, float cos_w_H, float e_u, float e_v)
{
	return AS_Aniso_D(h, e_u, e_v) / pdfDivisor(cos_w_H);
}

inline void AS_Aniso_Sample(vector3d_t &H, float s1, float s2, float e_u, float e_v)
{
	if(s1 < 0.25f)
	{
		sample_quadrant_aniso(H, 4.f * s1, s2, e_u, e_v);
	}
	else if(s1 < 0.5f)
	{
		sample_quadrant_aniso(H, 1.f - 4.f * (0.5f - s1), s2, e_u, e_v);
		H.x = -H.x;
	}
	else if(s1 < 0.75f)
	{
		sample_quadrant_aniso(H, 4.f * (s1 - 0.5f), s2, e_u, e_v);
		H.x = -H.x;
		H.y = -H.y;
	}
	else
	{
		sample_quadrant_aniso(H, 1.f - 4.f * (1.f - s1), s2, e_u, e_v);
		H.y = -H.y;
	}
}

inline float Blinn_D(float cos_h, float e)
{
	return (e + 1.f) * M_1_2PI * fPow(std::max(0.f, cos_h), e);
}

inline float Blinn_Pdf(float costheta, float cos_w_H, float e)
{
	return Blinn_D(costheta, e) / pdfDivisor(cos_w_H);
}

inline void Blinn_Sample(vector3d_t &H, float s1, float s2, float exponent)
{
	// Compute sampled half-angle vector H for Blinn distribution
	float cosTheta = fPow(1.f - s1, 1.f / (exponent + 1.f));
	float sinTheta = fSqrt(1.f - cosTheta*cosTheta);
	float phi = s2 * M_2PI;
	H = vector3d_t(sinTheta*fCos(phi), sinTheta*fSin(phi), cosTheta);
}

// implementation of microfacet model with GGX facet distribution
// based on http://www.graphics.cornell.edu/~bjw/microfacetbsdf.pdf

inline void GGX_Sample(vector3d_t &H, float alpha2, float s1, float s2)
{
	// using the flollowing identity:
	// cosTheta == 1 / sqrt(1 + tanTheta2)
	float tanTheta2 = alpha2 * (s1 / (1.f - s1));
	float cosTheta = 1.f / fSqrt(1.f + tanTheta2);
	float sinTheta  = fSqrt(1 - (cosTheta*cosTheta));
	float phi = M_2PI * s2;
	
	H = vector3d_t(sinTheta*fCos(phi), sinTheta*fSin(phi), cosTheta);
}

inline float GGX_D(float alpha2, float cosTheta, float tanTheta2)
{
	float cosTheta2 = cosTheta * cosTheta;
	float cosTheta4 = cosTheta2 * cosTheta2;
	float aTan = alpha2 + tanTheta2;
	return alpha2 / (M_PI * cosTheta4 * aTan * aTan);
}

inline float GGX_G(float alpha2, float woN, float wiN)
{
	float woN2 = woN * woN;
	float wiN2 = wiN * wiN;
	// 2.f / (1.f + fSqrt(1.f + alpha2 * ( tanTheta^2 ));
	// By trigonometric identities: tanTheta^2  = sinTheta^2 / cosTheta^2 and sinTheta^2 = 1-cosTheta^2
	float G1wo = 2.f / (1.f + fSqrt(1.f + alpha2 * ( (1.f - woN2) / woN2 ) ));
	float G1wi = 2.f / (1.f + fSqrt(1.f + alpha2 * ( (1.f - wiN2) / wiN2 ) ));
	return G1wo * G1wi;
}

inline float GGX_Pdf(float D, float cosTheta, float Jacobian)
{
	return D * cosTheta * Jacobian;
}

inline float dielectricFresnel(float cosWiN, float ior)
{
    float c = std::fabs(cosWiN);
    float g = ior * ior - 1 + c * c;
    if (g > 0)
    {
        g = fSqrt(g);
        float A = (g - c) / (g + c);
        float B = (c * (g + c) - 1) / (c * (g - c) + 1);
        return 0.5f * A * A * (1 + B * B);
    }
    return 1.0f; // TIR
}

inline bool dielectricFresnelRefract(float rIor, float fIor, const vector3d_t &N, const vector3d_t &wo, vector3d_t &Refl, vector3d_t& Trans, bool &outside,float &Ft, float &Fr)
{
    float cos = N*wo, neta;
    vector3d_t Nn(0.f);

    // check which side of the surface we are on
    if (cos > 0.f)
    {
        neta = 1.f / rIor;
        Nn = N;
        outside = true;
    }
    else
    {
        cos  = -cos;
        neta = rIor;
        Nn = -N;
        outside = false;
    }
    
    // compute reflection
    Refl = (2 * cos) * Nn - wo;

    float arg = 1 - (neta * neta * (1 - (cos * cos)));
    
    if (arg < 0)
    {
        Trans = vector3d_t(0.f);
        Ft = 0.f;
        Fr = 1.f;
        return false; // total internal reflection
    }
    else
    {
		float dnp = fSqrt(arg);
		float nK = (neta * cos) - dnp;
		Trans = -(neta * wo) + (nK * Nn);
		float cosTheta2 = -Nn * Trans;
		float pPara = (cos - fIor * cosTheta2) / (cos + fIor * cosTheta2);
		float pPerp = (fIor * cos - cosTheta2) / (fIor * cos + cosTheta2);
		Fr = 0.5f * (pPara * pPara + pPerp * pPerp);
//		Fr = dielectricFresnel(cos, fIor);
		Ft = 1.f - Fr;
    }
    
    return true;
}

inline float SchlickFresnel(PFLOAT costheta, PFLOAT R)
{
	float c1 = (1.f - costheta);
	float c2 = c1 * c1;
	return R + ( (1.f - R) * c1 * c2 * c2 );
}

inline color_t diffuseReflect(float wiN, float woN, float mGlossy, float mDiffuse, const color_t &diff_base)
{
	float temp = 0.f;		
	float f_wi = (1.f - (0.5f * wiN));
	temp = f_wi * f_wi;
	f_wi = temp * temp * f_wi;
	float f_wo = (1.f - (0.5f * woN));
	temp = f_wo * f_wo;
	f_wo = temp * temp * f_wo;
	return DIFFUSE_RATIO * mDiffuse * (1.f - mGlossy) * (1.f - f_wi) * (1.f - f_wo) * diff_base;
}

inline color_t diffuseReflectFresnel(float wiN, float woN, float mGlossy, float mDiffuse, const color_t &diff_base, float Kt)
{
	return Kt * diffuseReflect(wiN, woN, mGlossy, mDiffuse, diff_base);
}

__END_YAFRAY

#endif // Y_MICROFACET_H
