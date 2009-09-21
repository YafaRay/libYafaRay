
#ifndef Y_MICROFACET_H
#define Y_MICROFACET_H

#include <core_api/surface.h>
#include <core_api/vector3d.h>

__BEGIN_YAFRAY

#define NOTANGENT 0
#define TANGENT_U 1
#define TANGENT_V 2
#define RAW_VMAP 3

#define DIFFUSE_RATIO 1.2173913

inline void sample_quadrant(vector3d_t &H, float s1, float s2, PFLOAT e_u, PFLOAT e_v)
{
	PFLOAT phi = atan(fSqrt((e_u + 1.f)/(e_v + 1.f)) * fTan(M_PI * s1 * 0.5f));
	PFLOAT cos_phi = fCos(phi);
	PFLOAT sin_phi;
	PFLOAT cos_theta, sin_theta;
	PFLOAT c_2 = cos_phi*cos_phi;
	PFLOAT s_2 = 1.f - c_2;
	
	cos_theta = fPow(s2, 1.f / (e_u*c_2 + e_v*s_2 + 1.f));
	sin_theta = fSqrt(std::max(0.f, 1.f - cos_theta*cos_theta));
	sin_phi = fSqrt(std::max(0.f, 1.f - cos_phi*cos_phi));
	
	H.x = sin_theta * cos_phi;
	H.y = sin_theta * sin_phi;
	H.z = cos_theta;
}

inline float AS_Aniso_D(vector3d_t h, PFLOAT e_u, PFLOAT e_v)
{
	float exponent = (e_u * h.x*h.x + e_v * h.y*h.y)/(1.f - h.z*h.z);
	return fSqrt( (e_u + 1.f)*(e_v + 1.f) ) * fPow(h.z, exponent);
}

inline float AS_Aniso_Pdf(vector3d_t h, PFLOAT cos_w_H, PFLOAT e_u, PFLOAT e_v)
{
	return AS_Aniso_D(h, e_u, e_v) / (8.f * cos_w_H);
}

inline void AS_Aniso_Sample(vector3d_t &H, float s1, float s2, PFLOAT e_u, PFLOAT e_v)
{
	if(s1 < 0.25f)
	{
		sample_quadrant(H, 4.f * s1, s2, e_u, e_v);
	}
	else if(s1 < 0.5f)
	{
		sample_quadrant(H, 4.f * (0.5f - s1), s2, e_u, e_v);
		H.x = -H.x;
	}
	else if(s1 < 0.75f)
	{
		sample_quadrant(H, 4.f * (s1 - 0.5f), s2, e_u, e_v);
		H.x = -H.x;
		H.y = -H.y;
	}
	else
	{
		sample_quadrant(H, 4.f * (1.f - s1), s2, e_u, e_v);
		H.y = -H.y;
	}
}

static inline PFLOAT Blinn_D(PFLOAT cos_h, PFLOAT e)
{
	return (e + 1.f) * fPow(std::fabs(cos_h), e);
}

static inline float Blinn_Pdf(PFLOAT costheta, PFLOAT cos_w_H, PFLOAT exponent)
{
	return Blinn_D(costheta, exponent) / (8.f * cos_w_H);
}

static inline void Blinn_Sample(vector3d_t &H, float s1, float s2, PFLOAT exponent)
{
	// Compute sampled half-angle vector H for Blinn distribution
	PFLOAT costheta = fPow(s1, 1.f / (exponent+1.f));
	PFLOAT sintheta = fSqrt(std::max(0.f, 1.f - costheta*costheta));
	PFLOAT phi = s2 * M_2PI;
	H = vector3d_t(sintheta*fSin(phi), sintheta*fCos(phi), costheta);
}

static inline PFLOAT SchlickFresnel(PFLOAT costheta, PFLOAT R)
{
	return R + fPow(1.f - costheta, 5.f) * (1.0 - R);
}

__END_YAFRAY

#endif // Y_MICROFACET_H
