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

#include "noise/musgrave/musgrave_fbm.h"
#include "math/math.h"
#include "noise/noise_generator.h"

namespace yafaray {
/*
 * Procedural fBm evaluated at "point"; returns value stored in "value".
 *
 * Parameters:
 *    ``H''  is the fractal increment parameter
 *    ``lacunarity''  is the gap between successive frequencies
 *    ``octaves''  is the number of frequencies in the fBm
 */

float FBmMusgrave::operator()(const Point3f &pt) const
{
	float value = 0, pwr = 1, pw_hl = math::pow(lacunarity_, -h_);
	Point3f tp(pt);
	for(int i = 0; i < (int)octaves_; i++)
	{
		value += NoiseGenerator::getSignedNoise(n_gen_, tp) * pwr;
		pwr *= pw_hl;
		tp *= lacunarity_;
	}
	float rmd = octaves_ - std::floor(octaves_);
	if(rmd != 0.f) value += rmd * NoiseGenerator::getSignedNoise(n_gen_, tp) * pwr;
	return value;
}

} //namespace yafaray

