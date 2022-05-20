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

#include "photon/photon.h"

#include <memory>
#include "common/file.h"

BEGIN_YAFARAY

void PhotonGather::operator()(const Photon *photon, float dist_2, float &max_dist_squared) const
{
	// Do usual photon heap management
	if(found_photons_ < n_lookup_)
	{
		// Add photon to unordered array of photons
		photons_[found_photons_++] = FoundPhoton(photon, dist_2);
		if(found_photons_ == n_lookup_)
		{
			std::make_heap(&photons_[0], &photons_[n_lookup_]);
			max_dist_squared = photons_[0].dist_square_;
		}
	}
	else
	{
		// Remove most distant photon from heap and add new photon
		std::pop_heap(&photons_[0], &photons_[n_lookup_]);
		photons_[n_lookup_ - 1] = FoundPhoton(photon, dist_2);
		std::push_heap(&photons_[0], &photons_[n_lookup_]);
		max_dist_squared = photons_[0].dist_square_;
	}
}

bool PhotonMap::load(const std::string &filename)
{
	clear();

	File file(filename);
	if(!file.open("rb"))
	{
		logger_.logWarning("PhotonMap file '", filename, "' not found, canceling load operation");
		return false;
	}

	std::string header;
	file.read(header);
	if(header != "YAF_PHOTONMAPv1")
	{
		logger_.logWarning("PhotonMap file '", filename, "' does not contain a valid YafaRay photon map");
		file.close();
		return false;
	}
	file.read(name_);
	file.read<int>(paths_);
	file.read<float>(search_radius_);
	file.read<int>(threads_pkd_tree_);
	unsigned int photons_size;
	file.read<unsigned int>(photons_size);
	photons_.resize(photons_size);
	for(auto &p : photons_)
	{
		file.read<float>(p.pos_.x());
		file.read<float>(p.pos_.y());
		file.read<float>(p.pos_.z());
		file.read<float>(p.c_.r_);
		file.read<float>(p.c_.g_);
		file.read<float>(p.c_.b_);
	}
	file.close();

	updateTree();
	return true;
}

bool PhotonMap::save(const std::string &filename) const
{
	File file(filename);
	file.open("wb");
	file.append(std::string("YAF_PHOTONMAPv1"));
	file.append(name_);
	file.append<int>(paths_);
	file.append<float>(search_radius_);
	file.append<int>(threads_pkd_tree_);
	file.append<unsigned int>((unsigned int) photons_.size());
	for(const auto &p : photons_)
	{
		file.append<float>(p.pos_.x());
		file.append<float>(p.pos_.y());
		file.append<float>(p.pos_.z());
		file.append<float>(p.c_.r_);
		file.append<float>(p.c_.g_);
		file.append<float>(p.c_.b_);
	}
	file.close();
	return true;
}

void PhotonMap::updateTree()
{
	if(photons_.size() > 0)
	{
		tree_ = std::make_unique<kdtree::PointKdTree<Photon>>(logger_, photons_, name_, threads_pkd_tree_);
		updated_ = true;
	}
	else tree_ = nullptr;
}

int PhotonMap::gather(const Point3 &p, FoundPhoton *found, unsigned int k, float &sq_radius) const
{
	PhotonGather proc(k, p);
	proc.photons_ = found;
	tree_->lookup(p, proc, sq_radius);
	return proc.found_photons_;
}

const Photon *PhotonMap::findNearest(const Point3 &p, const Vec3 &n, float dist) const
{
	NearestPhoton proc(p, n);
	//float dist=std::numeric_limits<float>::infinity(); //really bad idea...
	tree_->lookup(p, proc, dist);
	return proc.nearest_;
}

END_YAFARAY
