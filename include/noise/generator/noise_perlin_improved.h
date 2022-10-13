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

#ifndef LIBYAFARAY_NOISE_PERLIN_IMPROVED_H
#define LIBYAFARAY_NOISE_PERLIN_IMPROVED_H

#include "noise/noise_generator.h"

namespace yafaray {

//---------------------------------------------------------------------------
// Improved Perlin noise, based on Java reference code by Ken Perlin himself.
class NewPerlinNoiseGenerator final : public NoiseGenerator
{
	private:
		float operator()(const Point3f &pt) const override;
		static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
		static float grad(int hash, float x, float y, float z);
};

inline float NewPerlinNoiseGenerator::grad(int hash, float x, float y, float z)
{
	int h = hash & 15;                     // CONVERT LO 4 BITS OF HASH CODE
	float u = h < 8 ? x : y,            // INTO 12 GRADIENT DIRECTIONS.
	v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

} //namespace yafaray

#endif  //LIBYAFARAY_NOISE_PERLIN_IMPROVED_H
