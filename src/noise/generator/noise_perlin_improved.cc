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

#include "noise/generator/noise_perlin_improved.h"
#include "math/interpolation.h"

namespace yafaray {

float NewPerlinNoiseGenerator::operator()(const Point3f &pt) const
{
	float x = pt[Axis::X];
	float y = pt[Axis::Y];
	float z = pt[Axis::Z];
	float u = std::floor(x);
	float v = std::floor(y);
	float w = std::floor(z);

	// FIND UNIT CUBE THAT CONTAINS POINT
	const int xi = static_cast<int>(u) & 255;
	const int yi = static_cast<int>(v) & 255;
	const int zi = static_cast<int>(w) & 255;
	x -= u;  // FIND RELATIVE X,Y,Z
	y -= v;  // OF POINT IN CUBE.
	z -= w;
	u = fade(x);  // COMPUTE FADE CURVES
	v = fade(y);  // FOR EACH OF X,Y,Z.
	w = fade(z);

	//Hash coordinates of the 8 cube corners and add blended results from 8 corners of cube
	const int a = hash_[xi] + yi;
	const int aa = hash_[a] + zi;
	const int ab = hash_[a + 1] + zi;
	const int b = hash_[xi + 1] + yi;
	const int ba = hash_[b] + zi;
	const int bb = hash_[b + 1] + zi;

	const float lerp_00 = math::lerp(grad(hash_[ab + 1], x, y - 1, z - 1), grad(hash_[bb + 1], x - 1, y - 1, z - 1), u);
	const float lerp_01 = math::lerp(grad(hash_[aa + 1], x, y, z - 1), grad(hash_[ba + 1], x - 1, y, z - 1), u);
	const float lerp_02 = math::lerp(lerp_01, lerp_00, v);
	const float lerp_03 = math::lerp(grad(hash_[ab], x, y - 1, z), grad(hash_[bb], x - 1, y - 1, z), u);
	const float lerp_04 = math::lerp(grad(hash_[aa], x, y, z), grad(hash_[ba], x - 1, y, z), u);
	const float lerp_05 = math::lerp(lerp_04, lerp_03, v);
	const float nv = math::lerp(lerp_05, lerp_02, w);

	return (0.5f + 0.5f * nv);
}

} //namespace yafaray
