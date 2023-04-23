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

namespace yafaray {

void PhotonGather::operator()(const Photon *photon, float dist_2, float &max_dist_squared) const
{
	// Do usual photon heap management
	if(found_photons_ < n_lookup_)
	{
		// Add photon to unordered array of photons
		found_photon_[found_photons_++] = {photon, dist_2};
		if(found_photons_ == n_lookup_)
		{
			std::make_heap(&found_photon_[0], &found_photon_[n_lookup_]);
			max_dist_squared = found_photon_[0].dist_square_;
		}
	}
	else
	{
		// Remove most distant photon from heap and add new photon
		std::pop_heap(&found_photon_[0], &found_photon_[n_lookup_]);
		found_photon_[n_lookup_ - 1] = {photon, dist_2};
		std::push_heap(&found_photon_[0], &found_photon_[n_lookup_]);
		max_dist_squared = found_photon_[0].dist_square_;
	}
}

void PhotonMap::updateTree(const RenderControl &render_control)
{
	if(photons_.size() > 0)
	{
		tree_ = std::make_unique<kdtree::PointKdTree<Photon>>(logger_, render_control, photons_, name_, threads_pkd_tree_);
		updated_ = true;
	}
	else tree_ = nullptr;
}

int PhotonMap::gather(const Point3f &p, FoundPhoton *found, unsigned int k, float &sq_radius) const
{
	PhotonGather proc{k, p};
	proc.found_photon_ = found;
	tree_->lookup(p, proc, sq_radius);
	return proc.found_photons_;
}

const Photon *PhotonMap::findNearest(const Point3f &p, const Vec3f &n, float dist) const
{
	NearestPhoton proc{n};
	//float dist=std::numeric_limits<float>::max(); //really bad idea...
	tree_->lookup(p, proc, dist);
	return proc.photon_;
}

} //namespace yafaray
