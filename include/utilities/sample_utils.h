#pragma once

#ifndef Y_SAMPLEUTILS_H
#define Y_SAMPLEUTILS_H

#include <core_api/logging.h>
#include <core_api/ray.h>
#include <algorithm>
#include <string.h>

__BEGIN_YAFRAY

//! r_photon2: Square distance of photon path; ir_gather2: inverse of square gather radius
inline float kernel(float r_photon2, float ir_gather2)
{
	float s = (1.f - r_photon2 * ir_gather2);
	return 3.f * ir_gather2 * M_1_PI * s * s;
}

inline float ckernel(float r_photon2, float r_gather2, float ir_gather2)
{
	float r_p=fSqrt(r_photon2), ir_g=1.f/fSqrt(r_gather2);
	return 3.f * (1.f - r_p*ir_g) * ir_gather2 * M_1_PI;
}

//! Sample a cosine-weighted hemisphere given the the coordinate system built by N, Ru, Rv.

vector3d_t inline SampleCosHemisphere(const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv, float s1, float s2)
{
	if(s1>=1.0f) return N; //Fix for some white/black dots when s1>1.0. Also, this returns a fast trivial value when s1=1.0.
	else
	{
		float z1 = s1;
		float z2 = s2*M_2PI;
		return (Ru*fCos(z2) + Rv*fSin(z2))*fSqrt(1.0-z1) + N*fSqrt(z1);
	}
}

//! Uniform sample a sphere

vector3d_t inline SampleSphere(float s1, float s2)
{
	vector3d_t dir;
	dir.z = 1.0f - 2.0f*s1;
	float r = 1.0f - dir.z*dir.z;
	if(r>0.0f)
	{
		r = fSqrt(r);
		float a = M_2PI * s2;
		dir.x = fCos(a) * r;
		dir.y = fSin(a) * r;
	}
	else
	{
		dir.x = 0.0f;
		dir.y = 0.0f;
	}
	return dir;
}

//! uniformly sample a cone. Using doubles because for small cone angles the cosine is very close to one...

vector3d_t inline sampleCone(const vector3d_t &D, const vector3d_t &U, const vector3d_t &V, float maxCosAng, float s1, float s2)
{
	float cosAng = 1.f - (1.f - maxCosAng) * s2;
	float sinAng = fSqrt(1.f - cosAng * cosAng);
	float t1 = M_2PI * s1;
	return (U * fCos(t1) + V * fSin(t1)) * sinAng + D * cosAng;
}


void inline CumulateStep1dDF(const float *f, int nSteps, float *integral, float *cdf)
{
	int i;
	double c = 0.0, delta = 1.0/(double)nSteps;
	cdf[0] = 0.0;
	for (i = 1; i < nSteps+1; ++i)
	{
		c += (double)f[i-1] * delta;
		cdf[i] = float(c);
	}
	*integral = (float)c;// * delta;
	for (i = 1; i < nSteps+1; ++i)
		cdf[i] /= *integral;
}

/*! class that holds a 1D probability distribution function (pdf) and is also able to
	take samples from it. In order to do this the cumulative distribution function (cdf)
	is also calculated on construction.
*/

class pdf1D_t
{
public:
	pdf1D_t() {}
	pdf1D_t(float *f, int n)
	{
		func = new float[n];
		cdf = new float[n+1];
		count = n;
		memcpy(func, f, n*sizeof(float));
		CumulateStep1dDF(func, n, &integral, cdf);
		invIntegral = 1.f / integral;
		invCount = 1.f / count;
	}
	~pdf1D_t()
	{
		delete[] func, delete[] cdf;
	}
	float Sample(float u, float *pdf)const
	{
		// Find surrounding cdf segments
		float *ptr = std::lower_bound(cdf, cdf+count+1, u);
		int index = (int) (ptr-cdf-1);
		if(index<0) //Hopefully this should no longer be necessary from now on, as a minimum value slightly over 0.f has been set to the scrHalton function to avoid ptr and cdf to coincide (which caused index = -1)
		{
		    Y_ERROR << "Index out of bounds in pdf1D_t::Sample: index, u, ptr, cdf = " << index << ", " << u << ", " << ptr << ", " << cdf << yendl;
		    index=0;
		}
		// Return offset along current cdf segment
		float delta = (u - cdf[index]) / (cdf[index+1] - cdf[index]);
		if(pdf) *pdf = func[index] * invIntegral;
		return index + delta;
	}
	// take a discrete sample.
	// determines an index in the array from which the CDF was taked from, rather than a sample in [0;1]
	int DSample(float u, float *pdf)const
	{
		if(u == 0.f)
		{
			*pdf = func[0] * invIntegral;
			return 0;
		}
		float *ptr = std::lower_bound(cdf, cdf+count+1, u);
		int index = (int) (ptr-cdf-1);
		if(index<0)
		{
		    Y_ERROR << "Index out of bounds in pdf1D_t::Sample: index, u, ptr, cdf = " << index << ", " << u << ", " << ptr << ", " << cdf << yendl;
		    index=0;
		}
		if(pdf) *pdf = func[index] * invIntegral;
		return index;
	}
	// Distribution1D Data
	float *func, *cdf;
	float integral, invIntegral, invCount;
	int count;
};

// rotate the coord-system D, U, V with minimum rotation so that D gets
// mapped to D2, i.e. rotate around D^D2.
// V is assumed to be D^U, accordingly V2 is D2^U2; all input vectors must be normalized!

void inline minRot(const vector3d_t &D, const vector3d_t &U,
				   const vector3d_t &D2, vector3d_t &U2, vector3d_t &V2)
{
	float cosAlpha = D*D2;
	float sinAlpha = fSqrt(1 - cosAlpha*cosAlpha);
	vector3d_t v = D^D2;
	U2 = cosAlpha*U + (1.f-cosAlpha) * (v*U) + sinAlpha * (v^U);
	V2 = D2^U2;
}

//! Just a "modulo 1" float addition, assumed that both values are in range [0;1]

inline float addMod1(float a, float b)
{
	float s = a+b;
	return s>1 ? s-1.f : s;
}

__END_YAFRAY

#endif // Y_SAMPLEUTILS_H
