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

#include "common/logging.h"
#include "geometry/ray.h"
#include <algorithm>
#include <string.h>

BEGIN_YAFARAY

//! r_photon2: Square distance of photon path; ir_gather2: inverse of square gather radius
inline float kernel__(float r_photon_2, float ir_gather_2)
{
	const float s = (1.f - r_photon_2 * ir_gather_2);
	return 3.f * ir_gather_2 * M_1_PI * s * s;
}

inline float ckernel__(float r_photon_2, float r_gather_2, float ir_gather_2)
{
	const float r_p = math::sqrt(r_photon_2), ir_g = 1.f / math::sqrt(r_gather_2);
	return 3.f * (1.f - r_p * ir_g) * ir_gather_2 * M_1_PI;
}

//! Sample a cosine-weighted hemisphere given the the coordinate system built by N, Ru, Rv.

Vec3 inline sampleCosHemisphere__(const Vec3 &n, const Vec3 &ru, const Vec3 &rv, float s_1, float s_2)
{
	if(s_1 >= 1.0f) return n; //Fix for some white/black dots when s1>1.0. Also, this returns a fast trivial value when s1=1.0.
	else
	{
		float z_1 = s_1;
		float z_2 = s_2 * math::mult_pi_by_2;
		return (ru * math::cos(z_2) + rv * math::sin(z_2)) * math::sqrt(1.0 - z_1) + n * math::sqrt(z_1);
	}
}

//! Uniform sample a sphere

Vec3 inline sampleSphere__(float s_1, float s_2)
{
	Vec3 dir;
	dir.z_ = 1.0f - 2.0f * s_1;
	float r = 1.0f - dir.z_ * dir.z_;
	if(r > 0.0f)
	{
		r = math::sqrt(r);
		float a = math::mult_pi_by_2 * s_2;
		dir.x_ = math::cos(a) * r;
		dir.y_ = math::sin(a) * r;
	}
	else
	{
		dir.x_ = 0.0f;
		dir.y_ = 0.0f;
	}
	return dir;
}

//! uniformly sample a cone. Using doubles because for small cone angles the cosine is very close to one...

Vec3 inline sampleCone__(const Vec3 &d, const Vec3 &u, const Vec3 &v, float max_cos_ang, float s_1, float s_2)
{
	const float cos_ang = 1.f - (1.f - max_cos_ang) * s_2;
	const float sin_ang = math::sqrt(1.f - cos_ang * cos_ang);
	const float t_1 = math::mult_pi_by_2 * s_1;
	return (u * math::cos(t_1) + v * math::sin(t_1)) * sin_ang + d * cos_ang;
}


void inline cumulateStep1DDf__(const float *f, int n_steps, float *integral, float *cdf)
{
	int i;
	double c = 0.0, delta = 1.0 / (double)n_steps;
	cdf[0] = 0.0;
	for(i = 1; i < n_steps + 1; ++i)
	{
		c += (double)f[i - 1] * delta;
		cdf[i] = float(c);
	}
	*integral = (float)c;// * delta;
	for(i = 1; i < n_steps + 1; ++i)
		cdf[i] /= *integral;
}

/*! class that holds a 1D probability distribution function (pdf) and is also able to
	take samples from it. In order to do this the cumulative distribution function (cdf)
	is also calculated on construction.
*/

class Pdf1D
{
	public:
		Pdf1D() {}
		Pdf1D(float *f, int n)
		{
			func_ = new float[n];
			cdf_ = new float[n + 1];
			count_ = n;
			memcpy(func_, f, n * sizeof(float));
			cumulateStep1DDf__(func_, n, &integral_, cdf_);
			inv_integral_ = 1.f / integral_;
			inv_count_ = 1.f / count_;
		}
		~Pdf1D()
		{
			delete[] func_, delete[] cdf_;
		}
		float sample(float u, float *pdf) const
		{
			// Find surrounding cdf segments
			const float *ptr = std::lower_bound(cdf_, cdf_ + count_ + 1, u);
			int index = (int)(ptr - cdf_ - 1);
			if(index < 0) //Hopefully this should no longer be necessary from now on, as a minimum value slightly over 0.f has been set to the scrHalton function to avoid ptr and cdf to coincide (which caused index = -1)
			{
				Y_ERROR << "Index out of bounds in pdf1D_t::Sample: index, u, ptr, cdf = " << index << ", " << u << ", " << ptr << ", " << cdf_ << YENDL;
				index = 0;
			}
			// Return offset along current cdf segment
			const float delta = (u - cdf_[index]) / (cdf_[index + 1] - cdf_[index]);
			if(pdf) *pdf = func_[index] * inv_integral_;
			return index + delta;
		}
		// take a discrete sample.
		// determines an index in the array from which the CDF was taked from, rather than a sample in [0;1]
		int dSample(float u, float *pdf) const
		{
			if(u == 0.f)
			{
				*pdf = func_[0] * inv_integral_;
				return 0;
			}
			const float *ptr = std::lower_bound(cdf_, cdf_ + count_ + 1, u);
			int index = (int)(ptr - cdf_ - 1);
			if(index < 0)
			{
				Y_ERROR << "Index out of bounds in pdf1D_t::Sample: index, u, ptr, cdf = " << index << ", " << u << ", " << ptr << ", " << cdf_ << YENDL;
				index = 0;
			}
			if(pdf) *pdf = func_[index] * inv_integral_;
			return index;
		}
		// Distribution1D Data
		float *func_, *cdf_;
		float integral_, inv_integral_, inv_count_;
		int count_;
};

// rotate the coord-system D, U, V with minimum rotation so that D gets
// mapped to D2, i.e. rotate around D^D2.
// V is assumed to be D^U, accordingly V2 is D2^U2; all input vectors must be normalized!

void inline minRot__(const Vec3 &d, const Vec3 &u,
					 const Vec3 &d_2, Vec3 &u_2, Vec3 &v_2)
{
	float cos_alpha = d * d_2;
	float sin_alpha = math::sqrt(1 - cos_alpha * cos_alpha);
	Vec3 v = d ^d_2;
	u_2 = cos_alpha * u + (1.f - cos_alpha) * (v * u) + sin_alpha * (v ^ u);
	v_2 = d_2 ^ u_2;
}

//! Just a "modulo 1" float addition, assumed that both values are in range [0;1]

inline float addMod1__(float a, float b)
{
	float s = a + b;
	return s > 1 ? s - 1.f : s;
}

END_YAFARAY

#endif // YAFARAY_SAMPLE_H
