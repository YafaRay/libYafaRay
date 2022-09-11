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

#include "noise/generator/noise_voronoi.h"

namespace yafaray {

// distance metric function:
float VoronoiNoiseGenerator::distRealF(float x, float y, float z, float e)
{
	return math::sqrt(x * x + y * y + z * z);
}

float VoronoiNoiseGenerator::distSquaredF(float x, float y, float z, float e)
{
	return x * x + y * y + z * z;
}

float VoronoiNoiseGenerator::distManhattanF(float x, float y, float z, float e)
{
	return std::abs(x) + std::abs(y) + std::abs(z);
}

float VoronoiNoiseGenerator::distChebychevF(float x, float y, float z, float e)
{
	x = std::abs(x);
	y = std::abs(y);
	z = std::abs(z);
	const float t = (x > y) ? x : y;
	return ((z > t) ? z : t);
}
// minkovsky preset exponent 0.5
float VoronoiNoiseGenerator::distMinkovskyHf(float x, float y, float z, float e)
{
	const float d = math::sqrt(std::abs(x)) + math::sqrt(std::abs(y)) + math::sqrt(std::abs(z));
	return (d * d);
}
// minkovsky preset exponent 4
float VoronoiNoiseGenerator::distMinkovsky4F(float x, float y, float z, float e)
{
	x *= x;
	y *= y;
	z *= z;
	return math::sqrt(math::sqrt(x * x + y * y + z * z));
}
// Minkovsky, general case, slow
float VoronoiNoiseGenerator::distMinkovskyF(float x, float y, float z, float e)
{
	return math::pow(math::pow(std::abs(x), e) + math::pow(std::abs(y), e) + math::pow(std::abs(z), e), (float) 1.0 / e);
}

// Voronoi/Worley/Celullar basis

VoronoiNoiseGenerator::VoronoiNoiseGenerator(NoiseType vt, DMetricType dm, float mex) : v_type_{vt}, dm_type_{dm}, mk_exp_{mex}
{
	switch(dm.value())
	{
		case DMetricType::DistSquared: distfunc_2_ = distSquaredF; break;
		case DMetricType::DistManhattan: distfunc_2_ = distManhattanF; break;
		case DMetricType::DistChebychev: distfunc_2_ = distChebychevF; break;
		case DMetricType::DistMinkovskyHalf: distfunc_2_ = distMinkovskyHf; break;
		case DMetricType::DistMinkovskyFour: distfunc_2_ = distMinkovsky4F; break;
		case DMetricType::DistMinkovsky: distfunc_2_ = distMinkovskyF; break;
		case DMetricType::DistReal:
		default: distfunc_2_ = distRealF; break;
	}
}

std::pair<std::array<float, 4>, std::array<Point3f, 4>> VoronoiNoiseGenerator::getFeatures(const Point3f &pt) const
{
	const int xi = static_cast<int>(std::floor(pt[Axis::X]));
	const int yi = static_cast<int>(std::floor(pt[Axis::Y]));
	const int zi = static_cast<int>(std::floor(pt[Axis::Z]));
	std::array<float, 4> da{1e10f, 1e10f, 1e10f, 1e10f};
	std::array<Point3f, 4> pa;
	for(int xx = xi - 1; xx <= xi + 1; xx++)
	{
		for(int yy = yi - 1; yy <= yi + 1; yy++)
		{
			for(int zz = zi - 1; zz <= zi + 1; zz++)
			{
				const Point3f p{hashPnt({{xx, yy, zz}})};
				const float xd = pt[Axis::X] - (p[0] + xx);
				const float yd = pt[Axis::Y] - (p[1] + yy);
				const float zd = pt[Axis::Z] - (p[2] + zz);
				//d = (*distfunc)(xd, yd, zd, mk_exp);
				const float d = distfunc_2_(xd, yd, zd, mk_exp_);
				if(d < da[0])
				{
					da[3] = da[2];  da[2] = da[1];  da[1] = da[0];  da[0] = d;
					pa[3] = pa[2];  pa[2] = pa[1];  pa[1] = pa[0];  pa[0] = {{p[0] + xx, p[1] + yy, p[2] + zz}};
				}
				else if(d < da[1])
				{
					da[3] = da[2];  da[2] = da[1];  da[1] = d;
					pa[3] = pa[2];  pa[2] = pa[1];  pa[1] = {{p[0] + xx, p[1] + yy, p[2] + zz}};
				}
				else if(d < da[2])
				{
					da[3] = da[2];  da[2] = d;
					pa[3] = pa[2];  pa[2] = {{p[0] + xx, p[1] + yy, p[2] + zz}};
				}
				else if(d < da[3])
				{
					da[3] = d;
					pa[3] = {{p[0] + xx, p[1] + yy, p[2] + zz}};
				}
			}
		}
	}
	return {std::move(da), std::move(pa)};
}

float VoronoiNoiseGenerator::operator()(const Point3f &pt) const
{
	const auto [da, pa] = getFeatures(pt);
	switch(v_type_.value())
	{
		case NoiseType::VoronoiF2: return da[1];
		case NoiseType::VoronoiF3: return da[2];
		case NoiseType::VoronoiF4: return da[3];
		case NoiseType::VoronoiF2F1: return da[1] - da[0];
		case NoiseType::VoronoiCrackle:
		{
			const float t = 10.f * (da[1] - da[0]);
			return (t > 1.f) ? 1.f : t;
		}
		case NoiseType::VoronoiF1:
		default: return da[0];
	}
}

} //namespace yafaray
