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

#include "noise/musgrave/musgrave_ridged_mfractal.h"
#include "math/math.h"
#include "noise/noise_generator.h"

namespace yafaray {

/* Ridged multifractal terrain model.
 *
 * Some good parameter values to start with:
 *
 *      H:           1.0
 *      offset:      1.0
 *      gain:        2.0
 */
float RidgedMFractalMusgrave::operator()(const Point3f &pt) const
{
	float pw_hl = math::pow(lacunarity_, -h_);
	float pwr = pw_hl;	// starts with i=1 instead of 0
	Point3f tp(pt);

	float signal = offset_ - std::abs(NoiseGenerator::getSignedNoise(n_gen_, tp));
	signal *= signal;
	float result = signal;
	float weight = 1.0;

	for(int i = 1; i < (int)octaves_; i++)
	{
		tp *= lacunarity_;
		weight = signal * gain_;
		if(weight > (float)1.0) weight = (float)1.0; else if(weight < (float)0.0) weight = (float)0.0;
		signal = offset_ - std::abs(NoiseGenerator::getSignedNoise(n_gen_, tp));
		signal *= signal;
		signal *= weight;
		result += signal * pwr;
		pwr *= pw_hl;
	}
	return result;
}

} //namespace yafaray
