#pragma once

#ifndef YAFARAY_HASHGRID_H
#define YAFARAY_HASHGRID_H

#include <yafray_constants.h>
#include <core_api/bound.h>
#include <list>
#include <vector>

BEGIN_YAFRAY

class Photon;
struct FoundPhoton;
class Bound;
class Point3;


class YAFRAYCORE_EXPORT HashGrid final
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
			return (unsigned int)((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % grid_size_;
		}

	public:
		double cell_size_, inv_cell_size_;
		unsigned int grid_size_;
		Bound bounding_box_;
		std::vector<Photon>photons_;
		std::list<Photon *> **hash_grid_;
};


END_YAFRAY
#endif
