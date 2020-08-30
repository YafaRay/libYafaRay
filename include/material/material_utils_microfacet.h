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

#ifndef YAFARAY_MATERIAL_UTILS_MICROFACET_H
#define YAFARAY_MATERIAL_UTILS_MICROFACET_H

#include "common/surface.h"
#include "common/vector.h"
#include "common/color_ramp.h"

BEGIN_YAFARAY

#define NOTANGENT 0
#define TANGENT_U 1
#define TANGENT_V 2
#define RAW_VMAP 3

#define DIFFUSE_RATIO 0.387507688 //1.21739130434782608696 // (28 / 23)
#define PDF_DIVISOR(cos) ( 8.f * M_PI * (cos * 0.99f + 0.04f) )
#define AS_DIVISOR(cos1, cosI, cosO) ( 8.f * M_PI * ((cos1 * std::max(cosI, cosO)) * 0.99f + 0.04f) )

inline void sampleQuadrantAniso__(Vec3 &h, float s_1, float s_2, float e_u, float e_v)
{
	float phi = atan(fSqrt__((e_u + 1.f) / (e_v + 1.f)) * tanf(M_PI_2 * s_1));
	float cos_phi = fCos__(phi);
	float sin_phi = fSin__(phi);
	float cos_theta, sin_theta;
	float cos_phi_2 = cos_phi * cos_phi;
	float sin_phi_2 = 1.f - cos_phi_2;

	cos_theta = fPow__(1.f - s_2, 1.f / (e_u * cos_phi_2 + e_v * sin_phi_2 + 1.f));
	sin_theta = fSqrt__(1.f - cos_theta * cos_theta);

	h = Vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
}

inline float asAnisoD__(Vec3 h, float e_u, float e_v)
{
	if(h.z_ <= 0.f) return 0.f;
	float exponent = (e_u * h.x_ * h.x_ + e_v * h.y_ * h.y_) / (1.00001f - h.z_ * h.z_);
	return fSqrt__((e_u + 1.f) * (e_v + 1.f)) * fPow__(std::max(0.f, h.z_), exponent);
}

inline float asAnisoPdf__(Vec3 h, float cos_w_h, float e_u, float e_v)
{
	return asAnisoD__(h, e_u, e_v) / PDF_DIVISOR(cos_w_h);
}

inline void asAnisoSample__(Vec3 &h, float s_1, float s_2, float e_u, float e_v)
{
	if(s_1 < 0.25f)
	{
		sampleQuadrantAniso__(h, 4.f * s_1, s_2, e_u, e_v);
	}
	else if(s_1 < 0.5f)
	{
		sampleQuadrantAniso__(h, 1.f - 4.f * (0.5f - s_1), s_2, e_u, e_v);
		h.x_ = -h.x_;
	}
	else if(s_1 < 0.75f)
	{
		sampleQuadrantAniso__(h, 4.f * (s_1 - 0.5f), s_2, e_u, e_v);
		h.x_ = -h.x_;
		h.y_ = -h.y_;
	}
	else
	{
		sampleQuadrantAniso__(h, 1.f - 4.f * (1.f - s_1), s_2, e_u, e_v);
		h.y_ = -h.y_;
	}
}

inline float blinnD__(float cos_h, float e)
{
	return (e + 1.f) * fPow__(cos_h, e);
}

inline float blinnPdf__(float costheta, float cos_w_h, float e)
{
	return blinnD__(costheta, e) / PDF_DIVISOR(cos_w_h);
}

inline void blinnSample__(Vec3 &h, float s_1, float s_2, float exponent)
{
	// Compute sampled half-angle vector H for Blinn distribution
	float cos_theta = fPow__(1.f - s_2, 1.f / (exponent + 1.f));
	float sin_theta = fSqrt__(1.f - cos_theta * cos_theta);
	float phi = s_1 * M_2PI;
	h = Vec3(sin_theta * fCos__(phi), sin_theta * fSin__(phi), cos_theta);
}

// implementation of microfacet model with GGX facet distribution
// based on http://www.graphics.cornell.edu/~bjw/microfacetbsdf.pdf

inline void ggxSample__(Vec3 &h, float alpha_2, float s_1, float s_2)
{
	// using the flollowing identity:
	// cosTheta == 1 / sqrt(1 + tanTheta2)
	float tan_theta_2 = alpha_2 * (s_1 / (1.00001f - s_1));
	float cos_theta = 1.f / fSqrt__(1.f + tan_theta_2);
	float sin_theta  = fSqrt__(1.00001f - (cos_theta * cos_theta));
	float phi = M_2PI * s_2;

	h = Vec3(sin_theta * fCos__(phi), sin_theta * fSin__(phi), cos_theta);
}

inline float ggxD__(float alpha_2, float cos_theta_2, float tan_theta_2)
{
	float cos_theta_4 = cos_theta_2 * cos_theta_2;
	float a_tan = alpha_2 + tan_theta_2;
	float div = (M_PI * cos_theta_4 * a_tan * a_tan);
	return alpha_2 / div;
}

inline float ggxG__(float alpha_2, float wo_n, float wi_n)
{
	// 2.f / (1.f + fSqrt(1.f + alpha2 * ( tanTheta^2 ));
	float wo_n_2 = wo_n * wo_n;
	float wi_n_2 = wi_n * wi_n;

	float sqr_term_1 = fSqrt__(1.f + alpha_2 * ((1.f - wo_n_2) / wo_n_2));
	float sqr_term_2 = fSqrt__(1.f + alpha_2 * ((1.f - wi_n_2) / wi_n_2));

	float g_1_wo = 2.f / (1.f + (sqr_term_1));
	float g_1_wi = 2.f / (1.f + (sqr_term_2));
	return g_1_wo * g_1_wi;
}

inline float ggxPdf__(float d, float cos_theta, float jacobian)
{
	return d * cos_theta * jacobian;
}

inline float microfacetFresnel__(float wo_h, float ior)
{
	float c = std::fabs(wo_h);
	float g = ior * ior - 1 + c * c;
	if(g > 0)
	{
		g = fSqrt__(g);
		float a = (g - c) / (g + c);
		float b = (c * (g + c) - 1) / (c * (g - c) + 1);
		return 0.5f * a * a * (1 + b * b);
	}
	return 1.0f; // TIR
}

inline bool refractMicrofacet__(float eta, const Vec3 &wo, Vec3 &wi, const Vec3 &h, float wo_h, float wo_n, float &kr, float &kt)
{
	wi = Vec3(0.f);
	float c = -wo * h;
	float sign = (c > 0.f) ? 1 : -1;
	float t_1 = 1 - (eta * eta * (1 - c * c));
	if(t_1 < 0.f) return false;
	wi = eta * wo + (eta * c - sign * fSqrt__(t_1)) * h;
	wi = -wi;

	kr = 0.f;
	kt = 0.f;
	kr = microfacetFresnel__(wo_h, 1.f / eta);
	if(kr == 1.f) return false;
	kt = 1 - kr;
	return true;
}

inline void reflectMicrofacet__(const Vec3 &wo, Vec3 &wi, const Vec3 &h, float wo_h)
{
	wi = wo + (2.f * (h * -wo) * h);
	wi = -wi;
}

inline float schlickFresnel__(float costheta, float r)
{
	float c_1 = (1.f - costheta);
	float c_2 = c_1 * c_1;
	return r + ((1.f - r) * c_1 * c_2 * c_2);
}

inline Rgb diffuseReflect__(float wi_n, float wo_n, float glossy, float diffuse, const Rgb &diff_base)
{
	float temp = 0.f;
	float f_wi = (1.f - (0.5f * wi_n));
	temp = f_wi * f_wi;
	f_wi = temp * temp * f_wi;
	float f_wo = (1.f - (0.5f * wo_n));
	temp = f_wo * f_wo;
	f_wo = temp * temp * f_wo;
	return DIFFUSE_RATIO * diffuse * (1.f - glossy) * (1.f - f_wi) * (1.f - f_wo) * diff_base;
}

inline Rgb diffuseReflectFresnel__(float wi_n, float wo_n, float glossy, float diffuse, const Rgb &diff_base, float kt)
{
	return kt * diffuseReflect__(wi_n, wo_n, glossy, diffuse, diff_base);
}

END_YAFARAY

#endif // YAFARAY_MATERIAL_UTILS_MICROFACET_H
