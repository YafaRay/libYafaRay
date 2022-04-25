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

BEGIN_YAFARAY

namespace sample
{
//! r_photon2: Square distance of photon path; ir_gather2: inverse of square gather radius
inline float kernel(float r_photon_2, float ir_gather_2)
{
	const float s = (1.f - r_photon_2 * ir_gather_2);
	return 3.f * ir_gather_2 * math::div_1_by_pi<> * s * s;
}

inline float cKernel(float r_photon_2, float r_gather_2, float ir_gather_2)
{
	const float r_p = math::sqrt(r_photon_2), ir_g = 1.f / math::sqrt(r_gather_2);
	return 3.f * (1.f - r_p * ir_g) * ir_gather_2 * math::div_1_by_pi<>;
}

//! Sample a cosine-weighted hemisphere given the the coordinate system built by N, Ru, Rv.

Vec3 inline cosHemisphere(const Vec3 &n, const Vec3 &ru, const Vec3 &rv, float s_1, float s_2)
{
	if(s_1 >= 1.0f) return n; //Fix for some white/black dots when s1>1.0. Also, this returns a fast trivial value when s1=1.0.
	else
	{
		const float z_1 = s_1;
		const float z_2 = s_2 * math::mult_pi_by_2<>;
		return (ru * math::cos(z_2) + rv * math::sin(z_2)) * math::sqrt(1.f - z_1) + n * math::sqrt(z_1);
	}
}

//! Uniform sample a sphere

Vec3 inline sphere(float s_1, float s_2)
{
	Vec3 dir;
	dir.z() = 1.0f - 2.0f * s_1;
	float r = 1.0f - dir.z() * dir.z();
	if(r > 0.0f)
	{
		r = math::sqrt(r);
		const float a = math::mult_pi_by_2<> * s_2;
		dir.x() = math::cos(a) * r;
		dir.y() = math::sin(a) * r;
	}
	else
	{
		dir.x() = 0.0f;
		dir.y() = 0.0f;
	}
	return dir;
}

//! uniformly sample a cone. Using doubles because for small cone angles the cosine is very close to one...

Vec3 inline cone(const Vec3 &d, const Vec3 &u, const Vec3 &v, float max_cos_ang, float s_1, float s_2)
{
	const float cos_ang = 1.f - (1.f - max_cos_ang) * s_2;
	const float sin_ang = math::sqrt(1.f - cos_ang * cos_ang);
	const float t_1 = math::mult_pi_by_2<> * s_1;
	return (u * math::cos(t_1) + v * math::sin(t_1)) * sin_ang + d * cos_ang;
}

// rotate the coord-system D, U, V with minimum rotation so that D gets
// mapped to D2, i.e. rotate around D^D2.
// V is assumed to be D^U, accordingly V2 is D2^U2; all input vectors must be normalized!

void inline minRot(const Vec3 &d, const Vec3 &u,
					 const Vec3 &d_2, Vec3 &u_2, Vec3 &v_2)
{
	const float cos_alpha = d * d_2;
	const float sin_alpha = math::sqrt(1 - cos_alpha * cos_alpha);
	const Vec3 v{d ^d_2};
	u_2 = cos_alpha * u + Vec3{(1.f - cos_alpha) * (v * u)} + sin_alpha * (v ^ u); //FIXME DAVID: strange inconsistency detected when made Vec3 explicit, what is the middle part of the equation now surrounded by Vec3{}? Does it make sense? To be investigated... does the Vec3 portion make sense?
	v_2 = d_2 ^ u_2;
}


inline float riVdC(unsigned int bits, unsigned int r = 0)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
	return std::max(0.f, std::min(1.f, static_cast<float>(static_cast<double>(bits ^ r) * math::sample_mult_ratio<double>)));
}

inline float riS(unsigned int i, unsigned int r = 0)
{
	for(unsigned int v = 1 << 31; i; i >>= 1, v ^= v >> 1)
		if(i & 1) r ^= v;
	return std::max(0.f, std::min(1.f, (static_cast<float>(static_cast<double>(r) * math::sample_mult_ratio<double>))));
}

inline float riLp(unsigned int i, unsigned int r = 0)
{
	for(unsigned int v = 1 << 31; i; i >>= 1, v |= v >> 1)
		if(i & 1) r ^= v;
	return std::max(0.f, std::min(1.f, (static_cast<float>(static_cast<double>(r) * math::sample_mult_ratio<double>))));
}

/* The fnv - Fowler/Noll/Vo- hash code
   unrolled for the special case of hashing 32bit unsigned integers
   very easy but fast
   more details on http://www.isthe.com/chongo/tech/comp/fnv/
*/

inline unsigned int fnv32ABuf(unsigned int value)
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

} // namespace sample

END_YAFARAY

#endif // YAFARAY_SAMPLE_H
