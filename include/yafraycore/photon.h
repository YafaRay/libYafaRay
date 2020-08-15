#pragma once

#ifndef Y_PHOTONMAP_H
#define Y_PHOTONMAP_H

#include <yafray_constants.h>

#include "pkdtree.h"
#include <core_api/color.h>

__BEGIN_YAFRAY
#define c255Ratio 81.16902097686662123083
#define c256Ratio 40.74366543152520595687


#define cInv255Ratio 0.01231997119054820878
#define cInv256Ratio 0.02454369260617025968


class dirConverter_t
{
	public:
		dirConverter_t();

		vector3d_t convert(unsigned char theta, unsigned char phi)
		{
			return vector3d_t(sintheta[theta] * cosphi[phi],
			                  sintheta[theta] * sinphi[phi],
			                  costheta[theta]);
		}
		std::pair<unsigned char, unsigned char> convert(const vector3d_t &dir)
		{
			int t = (int)(fAcos(dir.z) * c255Ratio);
			int p = (int)(atan2(dir.y, dir.x) * c256Ratio);
			if(t > 254) t = 254;
			else if(t < 0) t = 0;
			if(p > 255) p = 255;
			else if(p < 0) p += 256;
			return std::pair<unsigned char, unsigned char>(t, p);
		}

	protected:
		float cosphi[256];
		float sinphi[256];
		float costheta[255];
		float sintheta[255];
};

extern YAFRAYCORE_EXPORT dirConverter_t dirconverter;

class photon_t
{
	public:
		photon_t() {/*theta=255;*/};
		photon_t(const vector3d_t &d, const point3d_t &p, const color_t &col)
		{
#ifdef _SMALL_PHOTONS
			direction(d);
#else
			dir = d;
#endif
			pos = p;
			c = col;
		};
		//		photon_t(const runningPhoton_t &p)
		//		{
		//			pos=p.pos;
		//			c=p.c;
		//			vector3d_t dir=p.lastpos-p.pos;
		//			dir.normalize();
		//			direction(dir);
		//		};
		const point3d_t &position()const {return pos;};
		const color_t color()const {return c;};
		void color(const color_t &col) {c = col;};
		vector3d_t direction()const
		{
#ifdef _SMALL_PHOTONS
			if(theta == 255) return vector3d_t(0, 0, 0);
			else return dirconverter.convert(theta, phi);
#else //_SMALL_PHOTONS
			return (vector3d_t)dir;
#endif //_SMALL_PHOTONS
		};
		void direction(const vector3d_t &d)
		{
#ifdef _SMALL_PHOTONS
			if(dir.null()) theta = 255;
			else
			{
				std::pair<unsigned char, unsigned char> cd = dirconverter.convert(d);
				theta = cd.first;
				phi = cd.second;
			}
#else //_SMALL_PHOTONS
			dir = d;
#endif //_SMALL_PHOTONS
		}

		point3d_t pos;

#ifdef _SMALL_PHOTONS
		rgbe_t c;
		unsigned char theta, phi;

#else //_SMALL_PHOTONS
		color_t c;
		normal_t dir;
#endif //_SMALL_PHOTONS
};

struct radData_t
{
	radData_t(point3d_t &p, vector3d_t n): pos(p), normal(n), use(true) {}
	point3d_t pos;
	vector3d_t normal;
	color_t refl;
	color_t transm;
	mutable bool use;
};

struct foundPhoton_t
{
	foundPhoton_t() {};
	foundPhoton_t(const photon_t *p, float d): photon(p), distSquare(d) {}
	bool operator<(const foundPhoton_t &p2) const { return distSquare < p2.distSquare; }
	const photon_t *photon;
	float distSquare;
	//temp!!
	float dis;
};

class YAFRAYCORE_EXPORT photonMap_t
{
	public:
		photonMap_t(): paths(0), updated(false), searchRadius(1.), tree(nullptr) { }
		photonMap_t(const std::string &mapname, int threads): paths(0), updated(false), searchRadius(1.), tree(nullptr), name(mapname), threadsPKDtree(threads) { }
		~photonMap_t() { if(tree) delete tree; }
		void setNumPaths(int n) { paths = n; }
		void setName(const std::string &mapname) { name = mapname; }
		void setNumThreadsPKDtree(int threads) { threadsPKDtree = threads; }
		int nPaths() const { return paths; }
		int nPhotons() const { return photons.size(); }
		void pushPhoton(photon_t &p) { photons.push_back(p); updated = false; }
		void swapVector(std::vector<photon_t> &vec) { photons.swap(vec); updated = false; }
		void appendVector(std::vector<photon_t> &vec, unsigned int curr) { photons.insert(std::end(photons), std::begin(vec), std::end(vec)); updated = false; paths += curr;}
		void reserveMemory(size_t numPhotons) { photons.reserve(numPhotons); }
		void updateTree();
		void clear() { photons.clear(); delete tree; tree = nullptr; updated = false; }
		bool ready() const { return updated; }
		//	void gather(const point3d_t &P, std::vector< foundPhoton_t > &found, unsigned int K, float &sqRadius) const;
		int gather(const point3d_t &P, foundPhoton_t *found, unsigned int K, float &sqRadius) const;
		const photon_t *findNearest(const point3d_t &P, const vector3d_t &n, float dist) const;
		bool load(const std::string &filename);
		bool save(const std::string &filename) const;
		std::mutex mutx;

	protected:
		std::vector<photon_t> photons;
		int paths; //!< amount of photon paths that have been traced for generating the map
		bool updated;
		float searchRadius;
		kdtree::pointKdTree<photon_t> *tree;
		std::string name;
		int threadsPKDtree = 1;
};

// photon "processes" for lookup

struct photonGather_t
{
	photonGather_t(uint32_t mp, const point3d_t &p);
	void operator()(const photon_t *photon, float dist2, float &maxDistSquared) const;
	const point3d_t &p;
	foundPhoton_t *photons;
	uint32_t nLookup;
	mutable uint32_t foundPhotons;
};

struct nearestPhoton_t
{
	nearestPhoton_t(const point3d_t &pos, const vector3d_t &norm): p(pos), n(norm), nearest(nullptr) {}
	void operator()(const photon_t *photon, float dist2, float &maxDistSquared) const
	{
		if(photon->direction() * n > 0.f) { nearest = photon; maxDistSquared = dist2; }
	}
	const point3d_t p; //wth do i need this for actually??
	const vector3d_t n;
	mutable const photon_t *nearest;
};

/*! "eliminates" photons within lookup radius (sets use=false) */
struct eliminatePhoton_t
{
	eliminatePhoton_t(const vector3d_t &norm): n(norm) {}
	void operator()(const radData_t *rpoint, float dist2, float &maxDistSquared) const
	{
		if(rpoint->normal * n > 0.f) { rpoint->use = false; }
	}
	const vector3d_t n;
};

__END_YAFRAY

#endif // Y_PHOTONMAP_H
