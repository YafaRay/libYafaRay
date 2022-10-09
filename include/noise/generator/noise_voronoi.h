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

#ifndef LIBYAFARAY_NOISE_VORONOI_H
#define LIBYAFARAY_NOISE_VORONOI_H

#include "noise/noise_generator.h"

namespace yafaray {

//---------------------------------------
// Voronoi, a.k.a. Worley/cellular basis

typedef float (*DistMetricFunc_t)(float x, float y, float z, float e);


class VoronoiNoiseGenerator final : public NoiseGenerator
{
	public:
		struct DMetricType : public Enum<DMetricType>
		{
			enum : ValueType_t { DistReal, DistSquared, DistManhattan, DistChebychev, DistMinkovskyHalf, DistMinkovskyFour, DistMinkovsky };
			inline static const EnumMap<ValueType_t> map_{{
					{"real", DistReal, ""},
					{"squared", DistSquared, ""},
					{"manhattan", DistManhattan, ""},
					{"chebychev", DistChebychev, ""},
					{"minkovsky_half", DistMinkovskyHalf, ""},
					{"minkovsky_four", DistMinkovskyFour, ""},
					{"minkovsky", DistMinkovsky, ""},
				}};
		};
		explicit VoronoiNoiseGenerator(NoiseType vt, DMetricType dm, float mex);
		std::pair<std::array<float, 4>, std::array<Point3f, 4>> getFeatures(const Point3f &pt) const;
		static float getDistance(int x, const std::array<float, 4> &da) { return da[x & 3]; }
		static Point3f getPoint(int x, const std::array<Point3f, 4> &pa) { return pa[x & 3]; }

	private:
		float operator()(const Point3f &pt) const override;
		static float distRealF(float x, float y, float z, float e);
		static float distSquaredF(float x, float y, float z, float e);
		static float distManhattanF(float x, float y, float z, float e);
		static float distChebychevF(float x, float y, float z, float e);
		static float distMinkovskyHf(float x, float y, float z, float e);
		static float distMinkovsky4F(float x, float y, float z, float e);
		static float distMinkovskyF(float x, float y, float z, float e);

		NoiseType v_type_{NoiseType::VoronoiF1};
		DMetricType dm_type_;
		float mk_exp_;
		//	distanceMetric_t* distfunc; //test...replace functors
		DistMetricFunc_t distfunc_2_;
		//	mutable float da[4];			// distance array
		//	mutable point3d_t pa[4];	// feature point array
};

//Unused functions, just for reference:

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
	virtual float operator() (float x, float y, float z, float e) { return (std::abs(x) + std::abs(y) + std::abs(z)); }
};

struct dist_Chebychev : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e)
	{
		x = std::abs(x);
		y = std::abs(y);
		z = std::abs(z);
		float t = (x>y)?x:y;
		return ((z>t)?z:t);
	}
};

// minkovsky preset exponent 0.5
struct dist_MinkovskyH : public distanceMetric_t
{
	virtual float operator() (float x, float y, float z, float e)
	{
		float d = sqrt(std::abs(x)) + sqrt(std::abs(y)) + sqrt(std::abs(z));
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
		return pow(pow(std::abs(x), e) + pow(std::abs(y), e) + pow(std::abs(z), e), (float)1.0/e);
	}
};
*/

} //namespace yafaray

#endif  //LIBYAFARAY_NOISE_VORONOI_H
