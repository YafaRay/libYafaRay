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

#ifndef YAFARAY_HASHGRID_H
#define YAFARAY_HASHGRID_H

#include "geometry/bound.h"
#include <list>
#include <vector>

namespace yafaray {

struct Photon;
struct FoundPhoton;
template<typename T> class Bound;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;


class HashGrid final
{
	public:
		HashGrid() = default;
		HashGrid(double cell_size, unsigned int grid_size, Bound<float> b_box);
		void setParm(double cell_size, unsigned int grid_size, Bound<float> b_box);
		void clear(); //remove all the photons in the grid;
		void updateGrid(); //build the hashgrid
		void pushPhoton(Photon &&p);
		unsigned int gather(const Point3f &p, FoundPhoton *found, unsigned int k, float sq_radius) const;

	private:
		unsigned int hash(const int ix, const int iy, const int iz) const
		{
			return static_cast<unsigned int>((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % grid_size_;
		}

	public:
		double cell_size_, inv_cell_size_;
		unsigned int grid_size_;
		Bound<float> bounding_box_;
		std::vector<Photon>photons_;
		std::unique_ptr<std::unique_ptr<std::list<const Photon *>>[]> hash_grid_;
};

} //namespace yafaray
#endif
