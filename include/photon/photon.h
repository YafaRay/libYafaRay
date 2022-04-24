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

class Photon
{
	public:
		Photon() {/*theta=255;*/};
		Photon(const Vec3 &d, const Point3 &p, const Rgb &col)
		{
#ifdef SMALL_PHOTONS //FIXME: SMALL_PHOTONS not working at the moment because Rgbe members do not include r_, g_ and b_ as needed in the rest of the code
			direction(d);
#else
			dir_ = d;
#endif
			pos_ = p;
			c_ = col;
		};
		//		photon_t(const runningPhoton_t &p)
		//		{
		//			pos=p.pos;
		//			c=p.c;
		//			vector3d_t dir=p.lastpos-p.pos;
		//			dir.normalize();
		//			direction(dir);
		//		};
		const Point3 &position() const {return pos_;};
		const Rgb color() const {return c_;};
		void color(const Rgb &col) { c_ = col;};
		Vec3 direction() const
		{
#ifdef SMALL_PHOTONS //FIXME: SMALL_PHOTONS not working at the moment because Rgbe members do not include r_, g_ and b_ as needed in the rest of the code
			if(theta_ == 255) return {0, 0, 0};
			else return dirconverter_global.convert(theta_, phi_);
#else //SMALL_PHOTONS
			return dir_;
#endif //SMALL_PHOTONS
		};
		void direction(const Vec3 &d)
		{
#ifdef SMALL_PHOTONS //FIXME: SMALL_PHOTONS not working at the moment because Rgbe members do not include r_, g_ and b_ as needed in the rest of the code
			if(d.null()) theta_ = 255;
			else std::tie(theta_, phi_) = dirconverter_global.convert(d);
#else //SMALL_PHOTONS
			dir_ = d;
#endif //SMALL_PHOTONS
		}

		Point3 pos_;

#ifdef SMALL_PHOTONS //FIXME: SMALL_PHOTONS not working at the moment because Rgbe members do not include r_, g_ and b_ as needed in the rest of the code
		Rgbe c_;
		unsigned char theta_, phi_;

#else //SMALL_PHOTONS
		Rgb c_;
		Vec3 dir_;
#endif //SMALL_PHOTONS
};

struct RadData
{
	RadData(const Point3 &p, Vec3 n): pos_(p), normal_(n), use_(true) {}
	Point3 pos_;
	Vec3 normal_;
	Rgb refl_;
	Rgb transm_;
	mutable bool use_;
};

struct FoundPhoton
{
	FoundPhoton() {};
	FoundPhoton(const Photon *p, float d): photon_(p), dist_square_(d) {}
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
		void pushPhoton(Photon &p) { photons_.push_back(p); updated_ = false; }
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
	PhotonGather(uint32_t mp, const Point3 &p);
	void operator()(const Photon *photon, float dist_2, float &max_dist_squared) const;
	const Point3 &p_;
	FoundPhoton *photons_;
	uint32_t n_lookup_;
	mutable uint32_t found_photons_;
};

struct NearestPhoton
{
	NearestPhoton(const Point3 &pos, const Vec3 &norm): p_(pos), n_(norm) {}
	void operator()(const Photon *photon, float dist_2, float &max_dist_squared)
	{
		if(photon->direction() * n_ > 0.f) { nearest_ = photon; max_dist_squared = dist_2; }
	}
	const Point3 p_; //wth do i need this for actually??
	const Vec3 n_;
	const Photon *nearest_ = nullptr;
};

/*! "eliminates" photons within lookup radius (sets use=false) */
struct EliminatePhoton
{
	explicit EliminatePhoton(const Vec3 &norm): n_(norm) {}
	void operator()(const RadData *rpoint, float dist_2, float &max_dist_squared) const
	{
		if(rpoint->normal_ * n_ > 0.f) { rpoint->use_ = false; }
	}
	const Vec3 n_;
};

#ifdef SMALL_PHOTONS
class DirConverter
{
	public:
		DirConverter();

		Vec3 convert(unsigned char theta, unsigned char phi)
		{
			return {sintheta_[theta] * cosphi_[phi],
					sintheta_[theta] * sinphi_[phi],
					costheta_[theta]};
		}
		std::pair<unsigned char, unsigned char> convert(const Vec3 &dir);

	protected:
		static constexpr double c_255_ratio_ = 81.16902097686662123083;
		static constexpr double c_256_ratio_ = 40.74366543152520595687;
		static constexpr double c_inv_255_ratio_ = 0.01231997119054820878;
		static constexpr double c_inv_256_ratio_ = 0.02454369260617025968;
		float cosphi_[256];
		float sinphi_[256];
		float costheta_[255];
		float sintheta_[255];
};

extern DirConverter dirconverter_global;
#endif // SMALL_PHOTONS

END_YAFARAY

#endif // YAFARAY_PHOTON_H
