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

BEGIN_YAFARAY

inline constexpr float pdfDivisor_global(float cos) { return 8.f * M_PI * (cos * 0.99f + 0.04f); }
inline float asDivisor_global(float cos_1, float cos_i, float cos_o) { return 8.f * M_PI * ((cos_1 * std::max(cos_i, cos_o)) * 0.99f + 0.04f); }

inline void sampleQuadrantAniso_global(Vec3 &h, float s_1, float s_2, float e_u, float e_v)
{
	float phi = std::atan(math::sqrt((e_u + 1.f) / (e_v + 1.f)) * std::tan(M_PI_2 * s_1));
	float cos_phi = math::cos(phi);
	float sin_phi = math::sin(phi);
	float cos_theta, sin_theta;
	float cos_phi_2 = cos_phi * cos_phi;
	float sin_phi_2 = 1.f - cos_phi_2;

	cos_theta = math::pow(1.f - s_2, 1.f / (e_u * cos_phi_2 + e_v * sin_phi_2 + 1.f));
	sin_theta = math::sqrt(1.f - cos_theta * cos_theta);

	h = Vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
}

inline float asAnisoD_global(Vec3 h, float e_u, float e_v)
{
	if(h.z_ <= 0.f) return 0.f;
	float exponent = (e_u * h.x_ * h.x_ + e_v * h.y_ * h.y_) / (1.00001f - h.z_ * h.z_);
	return math::sqrt((e_u + 1.f) * (e_v + 1.f)) * math::pow(std::max(0.f, h.z_), exponent);
}

inline float asAnisoPdf_global(Vec3 h, float cos_w_h, float e_u, float e_v)
{
	return asAnisoD_global(h, e_u, e_v) / pdfDivisor_global(cos_w_h);
}

inline void asAnisoSample_global(Vec3 &h, float s_1, float s_2, float e_u, float e_v)
{
	if(s_1 < 0.25f)
	{
		sampleQuadrantAniso_global(h, 4.f * s_1, s_2, e_u, e_v);
	}
	else if(s_1 < 0.5f)
	{
		sampleQuadrantAniso_global(h, 1.f - 4.f * (0.5f - s_1), s_2, e_u, e_v);
		h.x_ = -h.x_;
	}
	else if(s_1 < 0.75f)
	{
		sampleQuadrantAniso_global(h, 4.f * (s_1 - 0.5f), s_2, e_u, e_v);
		h.x_ = -h.x_;
		h.y_ = -h.y_;
	}
	else
	{
		sampleQuadrantAniso_global(h, 1.f - 4.f * (1.f - s_1), s_2, e_u, e_v);
		h.y_ = -h.y_;
	}
}

inline float blinnD_global(float cos_h, float e)
{
	return (e + 1.f) * math::pow(cos_h, e);
}

inline float blinnPdf_global(float costheta, float cos_w_h, float e)
{
	return blinnD_global(costheta, e) / pdfDivisor_global(cos_w_h);
}

inline void blinnSample_global(Vec3 &h, float s_1, float s_2, float exponent)
{
	// Compute sampled half-angle vector H for Blinn distribution
	float cos_theta = math::pow(1.f - s_2, 1.f / (exponent + 1.f));
	float sin_theta = math::sqrt(1.f - cos_theta * cos_theta);
	float phi = s_1 * math::mult_pi_by_2;
	h = Vec3(sin_theta * math::cos(phi), sin_theta * math::sin(phi), cos_theta);
}

// implementation of microfacet model with GGX facet distribution
// based on http://www.graphics.cornell.edu/~bjw/microfacetbsdf.pdf

inline void ggxSample_global(Vec3 &h, float alpha_2, float s_1, float s_2)
{
	// using the flollowing identity:
	// cosTheta == 1 / sqrt(1 + tanTheta2)
	float tan_theta_2 = alpha_2 * (s_1 / (1.00001f - s_1));
	float cos_theta = 1.f / math::sqrt(1.f + tan_theta_2);
	float sin_theta  = math::sqrt(1.00001f - (cos_theta * cos_theta));
	float phi = math::mult_pi_by_2 * s_2;

	h = Vec3(sin_theta * math::cos(phi), sin_theta * math::sin(phi), cos_theta);
}

inline float ggxD_global(float alpha_2, float cos_theta_2, float tan_theta_2)
{
	float cos_theta_4 = cos_theta_2 * cos_theta_2;
	float a_tan = alpha_2 + tan_theta_2;
	float div = (M_PI * cos_theta_4 * a_tan * a_tan);
	return alpha_2 / div;
}

inline float ggxG_global(float alpha_2, float wo_n, float wi_n)
{
	// 2.f / (1.f + fSqrt(1.f + alpha2 * ( tanTheta^2 ));
	float wo_n_2 = wo_n * wo_n;
	float wi_n_2 = wi_n * wi_n;

	float sqr_term_1 = math::sqrt(1.f + alpha_2 * ((1.f - wo_n_2) / wo_n_2));
	float sqr_term_2 = math::sqrt(1.f + alpha_2 * ((1.f - wi_n_2) / wi_n_2));

	float g_1_wo = 2.f / (1.f + (sqr_term_1));
	float g_1_wi = 2.f / (1.f + (sqr_term_2));
	return g_1_wo * g_1_wi;
}

inline float ggxPdf_global(float d, float cos_theta, float jacobian)
{
	return d * cos_theta * jacobian;
}

inline float microfacetFresnel_global(float wo_h, float ior)
{
	float c = std::abs(wo_h);
	float g = ior * ior - 1 + c * c;
	if(g > 0)
	{
		g = math::sqrt(g);
		float a = (g - c) / (g + c);
		float b = (c * (g + c) - 1) / (c * (g - c) + 1);
		return 0.5f * a * a * (1 + b * b);
	}
	return 1.0f; // TIR
}

inline bool refractMicrofacet_global(float eta, const Vec3 &wo, Vec3 &wi, const Vec3 &h, float wo_h, float &kr, float &kt)
{
	wi = Vec3(0.f);
	const float c = -wo * h;
	const float sign = (c > 0.f) ? 1 : -1;
	const float t_1 = 1 - (eta * eta * (1 - c * c));
	if(t_1 < 0.f) return false;
	wi = eta * wo + (eta * c - sign * math::sqrt(t_1)) * h;
	wi = -wi;

	kt = 0.f;
	kr = microfacetFresnel_global(wo_h, 1.f / eta);
	if(kr == 1.f) return false;
	kt = 1 - kr;
	return true;
}

inline void reflectMicrofacet_global(const Vec3 &wo, Vec3 &wi, const Vec3 &h)
{
	wi = wo + (2.f * (h * -wo) * h);
	wi = -wi;
}

inline float schlickFresnel_global(float costheta, float r)
{
	float c_1 = (1.f - costheta);
	float c_2 = c_1 * c_1;
	return r + ((1.f - r) * c_1 * c_2 * c_2);
}

inline Rgb diffuseReflect_global(float wi_n, float wo_n, float glossy, float diffuse, const Rgb &diff_base)
{
	constexpr double diffuse_ratio = 0.387507688; //This comment was in the original macro define, but not sure what it means: 1.21739130434782608696 // (28 / 23)
	float f_wi = (1.f - (0.5f * wi_n));
	float temp = f_wi * f_wi;
	f_wi = temp * temp * f_wi;
	float f_wo = (1.f - (0.5f * wo_n));
	temp = f_wo * f_wo;
	f_wo = temp * temp * f_wo;
	return diffuse_ratio * diffuse * (1.f - glossy) * (1.f - f_wi) * (1.f - f_wo) * diff_base;
}

inline Rgb diffuseReflectFresnel_global(float wi_n, float wo_n, float glossy, float diffuse, const Rgb &diff_base, float kt)
{
	return kt * diffuseReflect_global(wi_n, wo_n, glossy, diffuse, diff_base);
}

END_YAFARAY

#endif // YAFARAY_MATERIAL_UTILS_MICROFACET_H
