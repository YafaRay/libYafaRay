#pragma once

#ifndef __Y_HASHGRID_H
#define __Y_HASHGRID_H

#include <yafray_constants.h>
#include <core_api/bound.h>
#include <list>
#include <vector>

__BEGIN_YAFRAY

class photon_t;
struct foundPhoton_t;
class bound_t;
class point3d_t;


class YAFRAYCORE_EXPORT hashGrid_t
{
public:
	hashGrid_t(){hashGrid = nullptr;}

	hashGrid_t(double _cellSize, unsigned int _gridSize, bound_t _bBox);

	void setParm(double _cellSize, unsigned int _gridSize, bound_t _bBox);

	void clear(); //remove all the photons in the grid;

	void updateGrid(); //build the hashgrid

	void pushPhoton(photon_t &p);

	unsigned int gather(const point3d_t &P, foundPhoton_t *found, unsigned int K, float radius);

private:
	unsigned int Hash(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % gridSize;
	}

public:
	double cellSize, invcellSize;
	unsigned int gridSize;
	bound_t bBox;
	std::vector<photon_t>photons;
	std::list<photon_t*> **hashGrid;
};


__END_YAFRAY
#endif
