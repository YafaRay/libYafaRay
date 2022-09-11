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

#include "noise/generator/noise_cell.h"

namespace yafaray {

float CellNoiseGenerator::operator()(const Point3f &pt) const
{
	const int xi = static_cast<int>(std::floor(pt[Axis::X]));
	const int yi = static_cast<int>(std::floor(pt[Axis::Y]));
	const int zi = static_cast<int>(std::floor(pt[Axis::Z]));
	unsigned int n = xi + yi * 1301 + zi * 314159;
	n ^= (n << 13);
	return (static_cast<float>(n * (n * n * 15731 + 789221) + 1376312589) / 4294967296.0);
}

} //namespace yafaray
