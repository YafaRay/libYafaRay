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

#ifndef YAFARAY_NOISE_GENERATOR_H
#define YAFARAY_NOISE_GENERATOR_H

#include "common/vector.h"
#include "common/color.h"

#include "constants.h"

BEGIN_YAFARAY

class NoiseGenerator
{
	public:
		NoiseGenerator() = default;
		virtual ~NoiseGenerator() = default;
		virtual float operator()(const Point3 &pt) const = 0;
		// offset only added by blendernoise
		virtual Point3 offset(const Point3 &pt) const { return pt; }
};

//---------------------------------------------------------------------------
// Improved Perlin noise, based on Java reference code by Ken Perlin himself.
class NewPerlinNoiseGenerator final : public NoiseGenerator
{
	public:
		NewPerlinNoiseGenerator() = default;

	private:
		virtual float operator()(const Point3 &pt) const override;
		float fade(float t) const { return t * t * t * (t * (t * 6 - 15) + 10); }
		float grad(int hash, float x, float y, float z) const
		{
			int h = hash & 15;                     // CONVERT LO 4 BITS OF HASH CODE
			float u = h < 8 ? x : y,              // INTO 12 GRADIENT DIRECTIONS.
			      v = h < 4 ? y : h == 12 || h == 14 ? x : z;
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
		}
};

//---------------------------------------------------------------------------
// Standard Perlin noise.
class StdPerlinNoiseGenerator final : public NoiseGenerator
{
	public:
		StdPerlinNoiseGenerator() = default;

	private:
		virtual float operator()(const Point3 &pt) const override;
};

// Blender noise, similar to Perlin's
class BlenderNoiseGenerator final : public NoiseGenerator
{
	public:
		BlenderNoiseGenerator() = default;
	private:
		virtual float operator()(const Point3 &pt) const override;
		// offset texture point coordinates by one
		virtual Point3 offset(const Point3 &pt) const override { return pt + Point3(1.0, 1.0, 1.0); }
};

//---------------------------------------
// Voronoi, a.k.a. Worley/cellular basis

typedef float (*DistMetricFunc_t)(float x, float y, float z, float e);
// distance metrics as functors
/*struct distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e)=0;
};

struct dist_Real : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e) { return sqrt(x*x + y*y + z*z); }
};

struct dist_Squared : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e) { return (x*x + y*y + z*z); }
};

struct dist_Manhattan : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e) { return (std::fabs(x) + std::fabs(y) + std::fabs(z)); }
};

struct dist_Chebychev : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e)
	{
		x = std::fabs(x);
		y = std::fabs(y);
		z = std::fabs(z);
		float t = (x>y)?x:y;
		return ((z>t)?z:t);
	}
};

// minkovsky preset exponent 0.5
struct dist_MinkovskyH : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e)
	{
		float d = sqrt(std::fabs(x)) + sqrt(std::fabs(y)) + sqrt(std::fabs(z));
		return (d*d);
	}
};

// minkovsky preset exponent 4
struct dist_Minkovsky4 : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e)
	{
		x *= x;
		y *= y;
		z *= z;
		return sqrt(sqrt(x*x + y*y + z*z));
	}
};

// Minkovsky, general case, slow
struct dist_Minkovsky : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e)
	{
		return pow(pow(std::fabs(x), e) + pow(std::fabs(y), e) + pow(std::fabs(z), e), (float)1.0/e);
	}
};
*/
class VoronoiNoiseGenerator final : public NoiseGenerator
{
	public:
		enum VoronoiType {Vf1, Vf2, Vf3, Vf4, Vf2F1, VCrackle};
		enum DMetricType {DistReal, DistSquared, DistManhattan, DistChebychev,
		                  DistMinkovskyHalf, DistMinkovskyFour, DistMinkovsky
		                 };
		VoronoiNoiseGenerator(VoronoiType vt = Vf1, DMetricType dm = DistReal, float mex = 2.5);
		//virtual ~VoronoiNoiseGenerator() override { if (distfunc) { delete distfunc;  distfunc=nullptr; }
		void setDistM(DMetricType dm);
		void setMinkovskyExponent(float me) { mk_exp_ = me; }
		void getFeatures(const Point3 &pt, float da[4], Point3 pa[4]) const;
		float getDistance(int x, float da[4]) const { return da[x & 3]; }
		Point3 getPoint(int x, Point3 pa[4]) const { return pa[x & 3]; }

	private:
		virtual float operator()(const Point3 &pt) const override;

		VoronoiType v_type_;
		DMetricType dm_type_;
		float mk_exp_, w_1_, w_2_, w_3_, w_4_;
		//	distanceMetric_t* distfunc; //test...replace functors
		DistMetricFunc_t distfunc_2_;
		//	mutable float da[4];			// distance array
		//	mutable point3d_t pa[4];	// feature point array
};

// cell noise
class CellNoiseGenerator final : public NoiseGenerator
{
	public:
		CellNoiseGenerator() = default;

	private:
		virtual float operator()(const Point3 &pt) const override;
};

//------------------
// Musgrave types

class Musgrave
{
	public:
		Musgrave() = default;
		virtual ~Musgrave() = default;
		virtual float operator()(const Point3 &pt) const = 0;
};

class FBmMusgrave final : public Musgrave
{
	public:
		FBmMusgrave(float h, float lacu, float octs, const NoiseGenerator *n_gen)
			: h_(h), lacunarity_(lacu), octaves_(octs), n_gen_(n_gen) {}

	private:
		virtual float operator()(const Point3 &pt) const override;

		float h_, lacunarity_, octaves_;
		const NoiseGenerator *n_gen_;
};

class MFractalMusgrave final : public Musgrave
{
	public:
		MFractalMusgrave(float h, float lacu, float octs, const NoiseGenerator *n_gen)
			: h_(h), lacunarity_(lacu), octaves_(octs), n_gen_(n_gen) {}

	private:
		virtual float operator()(const Point3 &pt) const override;

		float h_, lacunarity_, octaves_;
		const NoiseGenerator *n_gen_;
};

class HeteroTerrainMusgrave final : public Musgrave
{
	public:
		HeteroTerrainMusgrave(float h, float lacu, float octs, float offs, const NoiseGenerator *n_gen)
			: h_(h), lacunarity_(lacu), octaves_(octs), offset_(offs), n_gen_(n_gen) {}

	private:
		virtual float operator()(const Point3 &pt) const;

		float h_, lacunarity_, octaves_, offset_;
		const NoiseGenerator *n_gen_;
};

class HybridMFractalMusgrave final : public Musgrave
{
	public:
		HybridMFractalMusgrave(float h, float lacu, float octs, float offs, float gain, const NoiseGenerator *n_gen)
			: h_(h), lacunarity_(lacu), octaves_(octs), offset_(offs), gain_(gain), n_gen_(n_gen) {}

	private:
		virtual float operator()(const Point3 &pt) const;

		float h_, lacunarity_, octaves_, offset_, gain_;
		const NoiseGenerator *n_gen_;
};

class RidgedMFractalMusgrave final : public Musgrave
{
	public:
		RidgedMFractalMusgrave(float h, float lacu, float octs, float offs, float gain, const NoiseGenerator *n_gen)
			: h_(h), lacunarity_(lacu), octaves_(octs), offset_(offs), gain_(gain), n_gen_(n_gen) {}

	private:
		virtual float operator()(const Point3 &pt) const;

		float h_, lacunarity_, octaves_, offset_, gain_;
		const NoiseGenerator *n_gen_;
};


// basic turbulence, half amplitude, double frequency defaults
// returns value in range (0,1)
float turbulence__(const NoiseGenerator *ngen, const Point3 &pt, int oct, float size, bool hard);
// noise cell color (used with voronoi)
Rgba cellNoiseColor__(const Point3 &pt);

static inline float getSignedNoise__(const NoiseGenerator *n_gen, const Point3 &pt)
{
	return (float)2.0 * (*n_gen)(pt) - (float)1.0;
}


END_YAFARAY
//---------------------------------------------------------------------------
#endif  //YAFARAY_NOISE_GENERATOR_H
