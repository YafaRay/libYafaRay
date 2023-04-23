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

#ifndef YAFARAY_PHOTON_H
#define YAFARAY_PHOTON_H



#include "pkdtree.h"
#include "color/color.h"

namespace yafaray {

struct Photon
{
	Vec3f dir_;
	Point3f pos_;
	Rgb col_;
	float time_{0.f};
};

struct RadData
{
	RadData(const Point3f &p, const Vec3f &n, float time): pos_{p}, normal_(n), time_{time} { }
	Point3f pos_;
	Vec3f normal_;
	Rgb refl_;
	Rgb transm_;
	float time_{0.f};
	mutable bool use_ = true;
};

struct FoundPhoton
{
	bool operator<(const FoundPhoton &photon) const { return dist_square_ < photon.dist_square_; }
	const Photon *photon_;
	float dist_square_;
};

class PhotonMap final
{
	public:
		explicit PhotonMap(Logger &logger) : logger_(logger) { }
		PhotonMap(Logger &logger, std::string mapname, int threads): name_(std::move(mapname)), threads_pkd_tree_(threads), logger_(logger) { }
		void setNumPaths(int n) { paths_ = n; }
		void setName(const std::string &mapname) { name_ = mapname; }
		void setNumThreadsPkDtree(int threads) { threads_pkd_tree_ = threads; }
		int nPaths() const { return paths_; }
		int nPhotons() const { return photons_.size(); }
		void pushPhoton(Photon &p) { photons_.emplace_back(p); updated_ = false; }
		void swapVector(std::vector<Photon> &vec) { photons_.swap(vec); updated_ = false; }
		void appendVector(std::vector<Photon> &vec, unsigned int curr) { photons_.insert(std::end(photons_), std::begin(vec), std::end(vec)); updated_ = false; paths_ += curr;}
		void reserveMemory(size_t num_photons) { photons_.reserve(num_photons); }
		void updateTree(const RenderMonitor &render_monitor, const RenderControl &render_control);
		void clear() { photons_.clear(); tree_ = nullptr; updated_ = false; }
		bool ready() const { return updated_; }
		//	void gather(const point3d_t &P, std::vector< foundPhoton_t > &found, unsigned int K, float &sqRadius) const;
		int gather(const Point3f &p, FoundPhoton *found, unsigned int k, float &sq_radius) const;
		const Photon *findNearest(const Point3f &p, const Vec3f &n, float dist) const;
		void lock() { mutx_.lock(); }
		void unlock() { mutx_.unlock(); }

	protected:
		std::vector<Photon> photons_;
		int paths_ = 0; //!< amount of photon paths that have been traced for generating the map
		bool updated_ = false;
		float search_radius_ = 1.f;
		std::unique_ptr<kdtree::PointKdTree<Photon>> tree_;
		std::string name_;
		int threads_pkd_tree_ = 1;
		std::mutex mutx_;
		Logger &logger_;
};

// photon "processes" for lookup

struct PhotonGather
{
	PhotonGather(uint32_t mp, const Point3f &p) : p_{p}, n_lookup_{mp} { }
	void operator()(const Photon *photon, float dist_2, float &max_dist_squared) const;
	const Point3f &p_;
	FoundPhoton *found_photon_ = nullptr;
	uint32_t n_lookup_;
	mutable uint32_t found_photons_ = 0;
};

struct NearestPhoton
{
	void operator()(const Photon *photon, float dist_squared, float &max_dist_squared)
	{
		if(photon->dir_ * n_ > 0.f) { photon_ = photon; max_dist_squared = dist_squared; }
	}
	const Vec3f &n_;
	const Photon *photon_ = nullptr;
};

/*! "eliminates" photons within lookup radius (sets use=false) */
struct EliminatePhoton
{
	explicit EliminatePhoton(const Vec3f &norm): n_{norm} { }
	explicit EliminatePhoton(Vec3f &&norm): n_{std::move(norm)} { }
	void operator()(const RadData *rpoint, float dist_2, float &max_dist_squared) const
	{
		if(rpoint->normal_ * n_ > 0.f) { rpoint->use_ = false; }
	}
	const Vec3f &n_;
};

} //namespace yafaray

#endif // YAFARAY_PHOTON_H
