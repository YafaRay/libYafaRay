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

#include "noise/musgrave/musgrave_hetero_terrain.h"
#include "math/math.h"
#include "noise/noise_generator.h"

namespace yafaray {

/*
 * Heterogeneous procedural terrain function: stats by altitude method.
 * Evaluated at "point"; returns value stored in "value".
 *
 * Parameters:
 *       ``H''  determines the fractal dimension of the roughest areas
 *       ``lacunarity''  is the gap between successive frequencies
 *       ``octaves''  is the number of frequencies in the fBm
 *       ``offset''  raises the terrain from `sea level'
 */
float HeteroTerrainMusgrave::operator()(const Point3f &pt) const
{
	float pw_hl = math::pow(lacunarity_, -h_);
	float pwr = pw_hl;	// starts with i=1 instead of 0
	Point3f tp(pt);

	// first unscaled octave of function; later octaves are scaled
	float value = offset_ + NoiseGenerator::getSignedNoise(n_gen_, tp);
	tp *= lacunarity_;
	float increment;
	for(int i = 1; i < (int)octaves_; i++)
	{
		increment = (NoiseGenerator::getSignedNoise(n_gen_, tp) + offset_) * pwr * value;
		value += increment;
		pwr *= pw_hl;
		tp *= lacunarity_;
	}

	float rmd = octaves_ - std::floor(octaves_);
	if(rmd != (float)0.0)
	{
		increment = (NoiseGenerator::getSignedNoise(n_gen_, tp) + offset_) * pwr * value;
		value += rmd * increment;
	}

	return value;
}

} //namespace yafaray
