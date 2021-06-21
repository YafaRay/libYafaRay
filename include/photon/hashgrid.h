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

#include "common/yafaray_common.h"
#include "geometry/bound.h"
#include <list>
#include <vector>

BEGIN_YAFARAY

class Photon;
struct FoundPhoton;
class Bound;
class Point3;


class HashGrid final
{
	public:
		HashGrid() { hash_grid_ = nullptr;}
		HashGrid(double cell_size, unsigned int grid_size, Bound b_box);
		void setParm(double cell_size, unsigned int grid_size, Bound b_box);
		void clear(); //remove all the photons in the grid;
		void updateGrid(); //build the hashgrid
		void pushPhoton(Photon &p);
		unsigned int gather(const Point3 &p, FoundPhoton *found, unsigned int k, float sq_radius);

	private:
		unsigned int hash(const int ix, const int iy, const int iz)
		{
			return static_cast<unsigned int>((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % grid_size_;
		}

	public:
		double cell_size_, inv_cell_size_;
		unsigned int grid_size_;
		Bound bounding_box_;
		std::vector<Photon>photons_;
		std::unique_ptr<std::unique_ptr<std::list<const Photon *>>[]> hash_grid_;
};


END_YAFARAY
#endif
