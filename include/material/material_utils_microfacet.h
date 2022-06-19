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

namespace yafaray::microfacet
{

inline constexpr float pdfDivisor(float cos)
{
	return 8.f * math::num_pi<> * (cos * 0.99f + 0.04f);
}

inline float asDivisor(float cos_1, float cos_i, float cos_o)
{
	return 8.f * math::num_pi<> * ((cos_1 * std::max(cos_i, cos_o)) * 0.99f + 0.04f);
}

inline Vec3 sampleQuadrantAniso(float s_1, float s_2, float e_u, float e_v)
{
	const float phi = std::atan(math::sqrt((e_u + 1.f) / (e_v + 1.f)) * std::tan(math::div_pi_by_2<> * s_1));
	const float cos_phi = math::cos(phi);
	const float sin_phi = math::sin(phi);
	const float cos_phi_2 = cos_phi * cos_phi;
	const float sin_phi_2 = 1.f - cos_phi_2;
	const float cos_theta = math::pow(1.f - s_2, 1.f / (e_u * cos_phi_2 + e_v * sin_phi_2 + 1.f));
	const float sin_theta = math::sqrt(1.f - cos_theta * cos_theta);
	return { sin_theta * cos_phi, sin_theta * sin_phi, cos_theta };
}

inline float asAnisoD(Vec3 h, float e_u, float e_v)
{
	if(h.z() <= 0.f) return 0.f;
	float exponent = (e_u * h.x() * h.x() + e_v * h.y() * h.y()) / (1.00001f - h.z() * h.z());
	return math::sqrt((e_u + 1.f) * (e_v + 1.f)) * math::pow(std::max(0.f, h.z()), exponent);
}

inline float asAnisoPdf(Vec3 h, float cos_w_h, float e_u, float e_v)
{
	return asAnisoD(h, e_u, e_v) / pdfDivisor(cos_w_h);
}

inline Vec3 asAnisoSample(float s_1, float s_2, float e_u, float e_v)
{
	if(s_1 < 0.25f)
	{
		return sampleQuadrantAniso(4.f * s_1, s_2, e_u, e_v);
	}
	else if(s_1 < 0.5f)
	{
		Vec3 vec{sampleQuadrantAniso(1.f - 4.f * (0.5f - s_1), s_2, e_u, e_v)};
		vec.x() = -vec.x();
		return vec;
	}
	else if(s_1 < 0.75f)
	{
		Vec3 vec{sampleQuadrantAniso(4.f * (s_1 - 0.5f), s_2, e_u, e_v)};
		vec.x() = -vec.x();
		vec.y() = -vec.y();
		return vec;
	}
	else
	{
		Vec3 vec{sampleQuadrantAniso(1.f - 4.f * (1.f - s_1), s_2, e_u, e_v)};
		vec.y() = -vec.y();
		return vec;
	}
}

inline float blinnD(float cos_h, float e)
{
	return (e + 1.f) * math::pow(cos_h, e);
}

inline float blinnPdf(float costheta, float cos_w_h, float e)
{
	return blinnD(costheta, e) / pdfDivisor(cos_w_h);
}

inline Vec3 blinnSample(float s_1, float s_2, float exponent)
{
	// Compute sampled half-angle vector H for Blinn distribution
	const float cos_theta = math::pow(1.f - s_2, 1.f / (exponent + 1.f));
	const float sin_theta = math::sqrt(1.f - cos_theta * cos_theta);
	const float phi = s_1 * math::mult_pi_by_2<>;
	return { sin_theta * math::cos(phi), sin_theta * math::sin(phi), cos_theta };
}

// implementation of microfacet model with GGX facet distribution
// based on http://www.graphics.cornell.edu/~bjw/microfacetbsdf.pdf

inline Vec3 ggxSample(float alpha_2, float s_1, float s_2)
{
	// using the flollowing identity:
	// cosTheta == 1 / sqrt(1 + tanTheta2)
	const float tan_theta_2 = alpha_2 * (s_1 / (1.00001f - s_1));
	const float cos_theta = 1.f / math::sqrt(1.f + tan_theta_2);
	const float sin_theta = math::sqrt(1.00001f - (cos_theta * cos_theta));
	const float phi = math::mult_pi_by_2<> * s_2;
	return { sin_theta * math::cos(phi), sin_theta * math::sin(phi), cos_theta };
}

inline float ggxD(float alpha_2, float cos_theta_2, float tan_theta_2)
{
	const float cos_theta_4 = cos_theta_2 * cos_theta_2;
	const float a_tan = alpha_2 + tan_theta_2;
	const float div = math::num_pi<> * cos_theta_4 * a_tan * a_tan;
	return alpha_2 / div;
}

inline float ggxG(float alpha_2, float wo_n, float wi_n)
{
	// 2.f / (1.f + fSqrt(1.f + alpha2 * ( tanTheta^2 ));
	const float wo_n_2 = wo_n * wo_n;
	const float wi_n_2 = wi_n * wi_n;
	const float sqr_term_1 = math::sqrt(1.f + alpha_2 * ((1.f - wo_n_2) / wo_n_2));
	const float sqr_term_2 = math::sqrt(1.f + alpha_2 * ((1.f - wi_n_2) / wi_n_2));
	const float g_1_wo = 2.f / (1.f + (sqr_term_1));
	const float g_1_wi = 2.f / (1.f + (sqr_term_2));
	return g_1_wo * g_1_wi;
}

inline float ggxPdf(float d, float cos_theta, float jacobian)
{
	return d * cos_theta * jacobian;
}

inline float fresnel(float wo_h, float ior)
{
	const float c = std::abs(wo_h);
	float g = ior * ior - 1 + c * c;
	if(g > 0.f)
	{
		g = math::sqrt(g);
		const float a = (g - c) / (g + c);
		const float b = (c * (g + c) - 1) / (c * (g - c) + 1);
		return 0.5f * a * a * (1 + b * b);
	}
	else return 1.f; // TIR
}

inline bool refract(float eta, const Vec3 &wo, Vec3 &wi, const Vec3 &h, float wo_h, float &kr, float &kt)
{
	wi = Vec3{0.f};
	const float c = -wo * h;
	const float sign = (c > 0.f) ? 1 : -1;
	const float t_1 = 1 - (eta * eta * (1 - c * c));
	if(t_1 < 0.f) return false;
	wi = eta * wo + (eta * c - sign * math::sqrt(t_1)) * h;
	wi = -wi;
	kt = 0.f;
	kr = fresnel(wo_h, 1.f / eta);
	if(kr == 1.f) return false;
	kt = 1 - kr;
	return true;
}

inline void reflect(const Vec3 &wo, Vec3 &wi, const Vec3 &h)
{
	wi = wo + (2.f * (h * -wo) * h);
	wi = -wi;
}

inline float schlickFresnel(float costheta, float r)
{
	const float c_1 = (1.f - costheta);
	const float c_2 = c_1 * c_1;
	return r + ((1.f - r) * c_1 * c_2 * c_2);
}

inline Rgb diffuseReflect(float wi_n, float wo_n, float glossy, float diffuse, const Rgb &diff_base)
{
	constexpr double diffuse_ratio = 0.387507688; //This comment was in the original macro define, but not sure what it means: 1.21739130434782608696 // (28 / 23)
	float f_wi = 1.f - (0.5f * wi_n);
	float temp = f_wi * f_wi;
	f_wi = temp * temp * f_wi;
	float f_wo = (1.f - (0.5f * wo_n));
	temp = f_wo * f_wo;
	f_wo = temp * temp * f_wo;
	return diffuse_ratio * diffuse * (1.f - glossy) * (1.f - f_wi) * (1.f - f_wo) * diff_base;
}

inline Rgb diffuseReflectFresnel(float wi_n, float wo_n, float glossy, float diffuse, const Rgb &diff_base, float kt)
{
	return kt * diffuseReflect(wi_n, wo_n, glossy, diffuse, diff_base);
}

} // namespace yafaray::microfacet

#endif // YAFARAY_MATERIAL_UTILS_MICROFACET_H
