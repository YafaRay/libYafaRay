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

#include <utility>

#include "common/yafaray_common.h"

#include "pkdtree.h"
#include "color/color.h"

BEGIN_YAFARAY

class Photon final
{
	public:
		Photon() = default;
		Photon(const Vec3 &d, const Point3 &p, const Rgb &col) : pos_{p}, c_{col}, dir_{d} { }
		Photon(Vec3 &&d, Point3 &&p, Rgb &&col) : pos_{std::move(p)}, c_{std::move(col)}, dir_{std::move(d)} { }
		const Point3 &position() const { return pos_; }
		Rgb color() const { return c_; }
		void color(const Rgb &col) { c_ = col;}
		void color(Rgb &&col) { c_ = std::move(col); }
		Vec3 direction() const { return dir_; }
		void direction(const Vec3 &d) { dir_ = d; }
		void direction(Vec3 &&d) { dir_ = std::move(d); }

		Point3 pos_;
		Rgb c_;
		Vec3 dir_;
};

struct RadData
{
	RadData(const Point3 &p, const Vec3 &n): pos_{p}, normal_(n) { }
	RadData(Point3 &&p, Vec3 &&n): pos_(std::move(p)), normal_(std::move(n)) { }
	Point3 pos_;
	Vec3 normal_;
	Rgb refl_;
	Rgb transm_;
	mutable bool use_ = true;
};

struct FoundPhoton
{
	FoundPhoton() = default;
	FoundPhoton(const Photon *p, float d): photon_{p}, dist_square_{d} { }
	bool operator<(const FoundPhoton &p_2) const { return dist_square_ < p_2.dist_square_; }
	const Photon *photon_;
	float dist_square_;
	//temp!!
	float dis_;
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
		void updateTree();
		void clear() { photons_.clear(); tree_ = nullptr; updated_ = false; }
		bool ready() const { return updated_; }
		//	void gather(const point3d_t &P, std::vector< foundPhoton_t > &found, unsigned int K, float &sqRadius) const;
		int gather(const Point3 &p, FoundPhoton *found, unsigned int k, float &sq_radius) const;
		const Photon *findNearest(const Point3 &p, const Vec3 &n, float dist) const;
		bool load(const std::string &filename);
		bool save(const std::string &filename) const;
		std::mutex mutx_;

	protected:
		std::vector<Photon> photons_;
		int paths_ = 0; //!< amount of photon paths that have been traced for generating the map
		bool updated_ = false;
		float search_radius_ = 1.f;
		std::unique_ptr<kdtree::PointKdTree<Photon>> tree_;
		std::string name_;
		int threads_pkd_tree_ = 1;
		Logger &logger_;
};

// photon "processes" for lookup

struct PhotonGather
{
	PhotonGather(uint32_t mp, const Point3 &p) : p_{p}, n_lookup_{mp} { }
	PhotonGather(uint32_t mp, Point3 &&p) : p_{std::move(p)}, n_lookup_{mp} { }
	void operator()(const Photon *photon, float dist_2, float &max_dist_squared) const;
	const Point3 &p_;
	FoundPhoton *photons_ = nullptr;
	uint32_t n_lookup_;
	mutable uint32_t found_photons_ = 0;
};

struct NearestPhoton
{
	NearestPhoton(const Point3 &pos, const Vec3 &norm): p_{pos}, n_{norm} { }
	NearestPhoton(Point3 &&pos, Vec3 &&norm): p_{std::move(pos)}, n_{std::move(norm)} { }
	void operator()(const Photon *photon, float dist_2, float &max_dist_squared)
	{
		if(photon->direction() * n_ > 0.f) { nearest_ = photon; max_dist_squared = dist_2; }
	}
	const Point3 &p_; //wth do i need this for actually??
	const Vec3 &n_;
	const Photon *nearest_ = nullptr;
};

/*! "eliminates" photons within lookup radius (sets use=false) */
struct EliminatePhoton
{
	explicit EliminatePhoton(const Vec3 &norm): n_{norm} { }
	explicit EliminatePhoton(Vec3 &&norm): n_{std::move(norm)} { }
	void operator()(const RadData *rpoint, float dist_2, float &max_dist_squared) const
	{
		if(rpoint->normal_ * n_ > 0.f) { rpoint->use_ = false; }
	}
	const Vec3 &n_;
};

END_YAFARAY

#endif // YAFARAY_PHOTON_H
