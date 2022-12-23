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

#ifndef YAFARAY_SAMPLE_H
#define YAFARAY_SAMPLE_H

#include "math/math.h"
#include "geometry/vector.h"

namespace yafaray::sample
{
//! r_photon2: Square distance of photon path; ir_gather2: inverse of square gather radius
[[nodiscard]] inline float kernel(float r_photon_2, float ir_gather_2)
{
	const float s = (1.f - r_photon_2 * ir_gather_2);
	return 3.f * ir_gather_2 * math::div_1_by_pi<> * s * s;
}

[[nodiscard]] inline float cKernel(float r_photon_2, float r_gather_2, float ir_gather_2)
{
	const float r_p = math::sqrt(r_photon_2), ir_g = 1.f / math::sqrt(r_gather_2);
	return 3.f * (1.f - r_p * ir_g) * ir_gather_2 * math::div_1_by_pi<>;
}

//! Sample a cosine-weighted hemisphere given the the coordinate system built by N, Ru, Rv.

[[nodiscard]] Vec3f inline cosHemisphere(const Vec3f &n, const Uv<Vec3f> &r, float s_1, float s_2)
{
	if(s_1 >= 1.0f) return n; //Fix for some white/black dots when s1>1.0. Also, this returns a fast trivial value when s1=1.0.
	else
	{
		const float z_1 = s_1;
		const float z_2 = s_2 * math::mult_pi_by_2<>;
		return (r.u_ * math::cos(z_2) + r.v_ * math::sin(z_2)) * math::sqrt(1.f - z_1) + n * math::sqrt(z_1);
	}
}

//! Uniform sample a sphere

[[nodiscard]] Vec3f inline sphere(float s_1, float s_2)
{
	Vec3f dir;
	dir[Axis::Z] = 1.0f - 2.0f * s_1;
	float r = 1.0f - dir[Axis::Z] * dir[Axis::Z];
	if(r > 0.0f)
	{
		r = math::sqrt(r);
		const float a = math::mult_pi_by_2<> * s_2;
		dir[Axis::X] = math::cos(a) * r;
		dir[Axis::Y] = math::sin(a) * r;
	}
	else
	{
		dir[Axis::X] = 0.0f;
		dir[Axis::Y] = 0.0f;
	}
	return dir;
}

//! uniformly sample a cone. Using doubles because for small cone angles the cosine is very close to one...

[[nodiscard]] inline Vec3f cone(const Vec3f &d, const Uv<Vec3f> &uv, float max_cos_ang, float s_1, float s_2)
{
	const float cos_ang = 1.f - (1.f - max_cos_ang) * s_2;
	const float sin_ang = math::sqrt(1.f - cos_ang * cos_ang);
	const float t_1 = math::mult_pi_by_2<> * s_1;
	return (uv.u_ * math::cos(t_1) + uv.v_ * math::sin(t_1)) * sin_ang + d * cos_ang;
}

// rotate the coord-system D, U, V with minimum rotation so that D gets
// mapped to D2, i.e. rotate around D^D2.
// V is assumed to be D^U, accordingly V2 is D2^U2; all input vectors must be normalized!

[[nodiscard]] Uv<Vec3f> inline minRot(const Vec3f &d, const Vec3f &u, const Vec3f &d_2)
{
	const float cos_alpha = d * d_2;
	const float sin_alpha = math::sqrt(1 - cos_alpha * cos_alpha);
	const Vec3f v{d ^ d_2};
	Vec3f u_2{cos_alpha * u + Vec3f{(1.f - cos_alpha) * (v * u)} + sin_alpha * (v ^ u)}; //FIXME DAVID: strange inconsistency detected when made Vec3 explicit, what is the middle part of the equation now surrounded by Vec3{}? Does it make sense? To be investigated... does the Vec3 portion make sense?
	Vec3f v_2{d_2 ^ u_2};
	return {std::move(u_2), std::move(v_2)};
}

[[nodiscard]] inline float riVdC(unsigned int bits, unsigned int r = 0)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
	return std::max(0.f, std::min(1.f, static_cast<float>(static_cast<double>(bits ^ r) * math::sample_mult_ratio<double>)));
}

[[nodiscard]] inline float riS(unsigned int i, unsigned int r = 0)
{
	for(unsigned int v = 1U << 31; i; i >>= 1, v ^= v >> 1)
		if(i & 1U) r ^= v;
	return std::max(0.f, std::min(1.f, (static_cast<float>(static_cast<double>(r) * math::sample_mult_ratio<double>))));
}

[[nodiscard]] inline float riLp(unsigned int i, unsigned int r = 0)
{
	for(unsigned int v = 1U << 31; i; i >>= 1, v |= v >> 1)
		if(i & 1U) r ^= v;
	return std::max(0.f, std::min(1.f, (static_cast<float>(static_cast<double>(r) * math::sample_mult_ratio<double>))));
}

/* The fnv - Fowler/Noll/Vo- hash code
   unrolled for the special case of hashing 32bit unsigned integers
   very easy but fast
   more details on http://www.isthe.com/chongo/tech/comp/fnv/
*/

[[nodiscard]] inline unsigned int fnv32ABuf(unsigned int value)
{
	constexpr unsigned int fnv_1_32_init = 0x811c9dc5;
	constexpr unsigned int fnv_32_prime = 0x01000193;
	unsigned int hash = fnv_1_32_init;
	union Fnv32
	{
		unsigned int in_;
		unsigned char outs_[4];
	} val;
	val.in_ = value;

	for(unsigned char out : val.outs_)
	{
		hash ^= out;
		hash *= fnv_32_prime;
	}
	return hash;
}

} // namespace yafaray::sample

#endif // YAFARAY_SAMPLE_H
