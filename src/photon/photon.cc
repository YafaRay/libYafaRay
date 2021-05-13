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
#include "common/file.h"

BEGIN_YAFARAY

PhotonGather::PhotonGather(uint32_t mp, const Point3 &p): p_(p)
{
	photons_ = 0;
	n_lookup_ = mp;
	found_photons_ = 0;
}

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
		Y_WARNING << "PhotonMap file '" << filename << "' not found, canceling load operation";
		return false;
	}

	std::string header;
	file.read(header);
	if(header != "YAF_PHOTONMAPv1")
	{
		Y_WARNING << "PhotonMap file '" << filename << "' does not contain a valid YafaRay photon map";
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
		file.read<float>(p.pos_.x_);
		file.read<float>(p.pos_.y_);
		file.read<float>(p.pos_.z_);
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
		file.append<float>(p.pos_.x_);
		file.append<float>(p.pos_.y_);
		file.append<float>(p.pos_.z_);
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
		tree_ = std::unique_ptr<kdtree::PointKdTree<Photon>>(new kdtree::PointKdTree<Photon>(photons_, name_, threads_pkd_tree_));
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

#ifdef SMALL_PHOTONS
DirConverter dirconverter_global;

DirConverter::DirConverter()
{
	for(int i = 0; i < 255; ++i)
	{
		float angle = (float)i * c_inv_255_ratio_;
		costheta_[i] = math::cos(angle);
		sintheta_[i] = math::sin(angle);
	}
	for(int i = 0; i < 256; ++i)
	{
		float angle = (float)i * c_inv_256_ratio_;
		cosphi_[i] = math::cos(angle);
		sinphi_[i] = math::sin(angle);
	}
}

std::pair<unsigned char, unsigned char> DirConverter::convert(const Vec3 &dir) {
	int t = (int)(math::acos(dir.z_) * c_255_ratio_);
	int p = (int)(atan2(dir.y_, dir.x_) * c_256_ratio_);
	if(t > 254) t = 254;
	else if(t < 0) t = 0;
	if(p > 255) p = 255;
	else if(p < 0) p += 256;
	return std::pair<unsigned char, unsigned char>(t, p);
}
#endif // SMALL_PHOTONS

END_YAFARAY
