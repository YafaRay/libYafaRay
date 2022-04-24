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

#include "photon/hashgrid.h"

#include <memory>
#include "photon/photon.h"

BEGIN_YAFARAY

HashGrid::HashGrid(double cell_size, unsigned int grid_size, Bound b_box)
	: cell_size_(cell_size), grid_size_(grid_size), bounding_box_(b_box)
{
	inv_cell_size_ = 1. / cell_size;
}

void HashGrid::setParm(double cell_size, unsigned int grid_size, Bound b_box)
{
	cell_size_ = cell_size;
	inv_cell_size_ = 1. / cell_size;
	grid_size_ = grid_size;
	bounding_box_ = b_box;
}

void HashGrid::clear()
{
	photons_.clear();
}

void HashGrid::pushPhoton(Photon &p)
{
	photons_.push_back(p);
}

void HashGrid::updateGrid()
{
	if(!hash_grid_)
	{
		hash_grid_ = std::unique_ptr<std::unique_ptr<std::list<const Photon *>>[]>(new std::unique_ptr<std::list<const Photon *>>[grid_size_]);
		for(unsigned int i = 0; i < grid_size_; ++i)
		{
			hash_grid_[i] = nullptr;
		}
	}
	else
	{
		for(unsigned int i = 0; i < grid_size_; ++i)
		{
			if(hash_grid_[i])
			{
				hash_grid_[i]->clear(); // fix me! too many time consumed here
			}
		}
	}

	//travel the vector to build the Grid
	for(const auto &photon : photons_)
	{
		const Point3 hashindex{(photon.pos_ - bounding_box_.a_) * inv_cell_size_};
		const int ix = abs(int(hashindex.x()));
		const int iy = abs(int(hashindex.y()));
		const int iz = abs(int(hashindex.z()));
		const unsigned int index = hash(ix, iy, iz);
		if(!hash_grid_[index]) hash_grid_[index] = std::make_unique<std::list<const Photon *>>();
		hash_grid_[index]->push_front(&photon);
	}
	unsigned int notused = 0;
	for(unsigned int i = 0; i < grid_size_; ++i)
	{
		if(!hash_grid_[i] || hash_grid_[i]->size() == 0) notused++;
	}
	//if(logger_.isVerbose()) logger_.logVerbose("HashGrid: there are ", notused, " enties not used!", std::endl;
}

unsigned int HashGrid::gather(const Point3 &p, FoundPhoton *found, unsigned int k, float sq_radius)
{
	unsigned int count = 0;
	float radius = math::sqrt(sq_radius);

	const Point3 rad(radius, radius, radius);
	const Point3 b_min{((p - rad) - bounding_box_.a_) * inv_cell_size_};
	const Point3 b_max{((p + rad) - bounding_box_.a_) * inv_cell_size_};

	for(int iz = abs(int(b_min.z())); iz <= abs(int(b_max.z())); iz++)
	{
		for(int iy = abs(int(b_min.y())); iy <= abs(int(b_max.y())); iy++)
		{
			for(int ix = abs(int(b_min.x())); ix <= abs(int(b_max.x())); ix++)
			{
				const int hv = hash(ix, iy, iz);
				if(hash_grid_[hv] == nullptr) continue;
				for(const auto &photon : *hash_grid_[hv])
				{
					if((photon->pos_ - p).lengthSqr() < sq_radius)
					{
						found[count++] = FoundPhoton(photon, sq_radius);
					}
				}
			}
		}
	}
	return count;
}

END_YAFARAY