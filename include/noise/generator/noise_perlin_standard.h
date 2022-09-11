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

#ifndef LIBYAFARAY_NOISE_PERLIN_STANDARD_H
#define LIBYAFARAY_NOISE_PERLIN_STANDARD_H

#include "noise/noise_generator.h"

namespace yafaray {

class StdPerlinNoiseGenerator final : public NoiseGenerator
{
	public:
		StdPerlinNoiseGenerator() = default;

	private:
		float operator()(const Point3f &pt) const override;
		static std::pair<std::array<int, 2>, std::array<float, 2>> setup(int i, const std::array<float, 3> &vec);
		static constexpr inline float stdpAt(float rx, float ry, float rz, const float *q);
		static constexpr inline float surve(float t);
		static const std::array<unsigned char, 512 + 2> stdp_p_;
		static const float stdp_g_[512 + 2][3];
};

} //namespace yafaray

#endif  //LIBYAFARAY_NOISE_PERLIN_STANDARD_H
