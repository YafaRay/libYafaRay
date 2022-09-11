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

#include "noise/musgrave/musgrave_hybrid_mfractal.h"
#include "math/math.h"
#include "noise/noise_generator.h"

namespace yafaray {

/* Hybrid additive/multiplicative multifractal terrain model.
 *
 * Some good parameter values to start with:
 *
 *      H:           0.25
 *      offset:      0.7
 */
float HybridMFractalMusgrave::operator()(const Point3f &pt) const
{
	float pw_hl = math::pow(lacunarity_, -h_);
	float pwr = pw_hl;	// starts with i=1 instead of 0
	Point3f tp(pt);

	float result = NoiseGenerator::getSignedNoise(n_gen_, tp) + offset_;
	float weight = gain_ * result;
	tp *= lacunarity_;

	for(int i = 1; (weight > (float)0.001) && (i < (int)octaves_); i++)
	{
		if(weight > (float)1.0)  weight = (float)1.0;
		float signal = (NoiseGenerator::getSignedNoise(n_gen_, tp) + offset_) * pwr;
		pwr *= pw_hl;
		result += weight * signal;
		weight *= gain_ * signal;
		tp *= lacunarity_;
	}

	float rmd = octaves_ - std::floor(octaves_);
	if(rmd != (float)0.0) result += rmd * ((NoiseGenerator::getSignedNoise(n_gen_, tp) + offset_) * pwr);

	return result;

}

} //namespace yafaray
