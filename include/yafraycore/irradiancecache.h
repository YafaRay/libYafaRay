#ifndef Y_IRRAD_CACHE_H
#define Y_IRRAD_CACHE_H

#include <yafraycore/ccthreads.h>
#include <core_api/vector3d.h>
#include <core_api/color.h>

__BEGIN_YAFRAY

class scene_t;
class dynPKdTree_t;
class irradLookup_t;
class surfacePoint_t;
template <class NodeData> class octree_t;

struct irradSample_t
{
	//irradSample_t(const point3d_t &p, const vector3d_t &n, const color_t &c, PFLOAT a, PFLOAT m, PFLOAT a):
	//	P(p), N(n), color(c), harmD(a), minD(m), area(a)	{}
	
	point3d_t P; //!< position of sample
	vector3d_t N; //!< normal (without bump) at sample point
	vector3d_t w_r; //!< dominant incoming red light direction
	vector3d_t w_g; //!< dominant incoming green light direction
	vector3d_t w_b; //!< dominant incoming blue light direction
	color_t col; //!< actual irradiance estimation
	PFLOAT Rmin; //!< minimum radius
	PFLOAT Apix; //!< projected pixel area (not actually required for extrapolation...only for octree insert)
};

class YAFRAYCORE_EXPORT irradianceCache_t
{
	public:
		irradianceCache_t(): tree(0){};
		void init(const scene_t &scene, PFLOAT Kappa);
		/*! return an extrapolated irradiance sample (only col and  w_* are relevant here)
			\return true when enough information in cache to extrapolate, false otherwise */
		bool gatherSamples(const surfacePoint_t sp, PFLOAT A_pix, irradSample_t &irr, bool debug=false) const;
		bool enoughSamples(const surfacePoint_t sp, PFLOAT A_pix) const;
		void insert(const irradSample_t &s);
		float weight(const irradSample_t &s, const surfacePoint_t &sp, PFLOAT A_proj) const;
	private:
		float K; //!< overall quality setting
		yafthreads::mutex_t tree_mutex;
		octree_t<irradSample_t> *tree;
		//dynPKdTree_t<lightSample_t> tree;
};

struct irradLookup_t
{
	irradLookup_t(const irradianceCache_t &cache, const surfacePoint_t &spt, PFLOAT pix_area, bool dbg):
		c(cache), sp(spt), sum_w_r(0), sum_w_g(0), sum_w_b(0),
		A(pix_area), sum_E(0.f), sum_wi(0), found(0), debug(dbg)
	{
		if(dbg) std::cout << "\nsp.P:" << sp.P << " A:" << pix_area << std::endl;
	}
	bool operator()(const point3d_t &p, const irradSample_t &s);
	bool getIrradiance(irradSample_t &result);
	
	const irradianceCache_t &c;
	const surfacePoint_t &sp;
	vector3d_t sum_w_r;
	vector3d_t sum_w_g;
	vector3d_t sum_w_b;
	PFLOAT A;
	color_t sum_E;
	CFLOAT sum_wi;
	int found;
	bool debug;
};

struct availabilityLookup_t
{
	availabilityLookup_t(const irradianceCache_t &cache, const surfacePoint_t &spt, PFLOAT pix_area):
		c(cache), sp(spt), A(pix_area), sum_wi(0), found(0), enough(false) {}
	bool operator()(const point3d_t &p, const irradSample_t &s);
	bool getIrradiance(color_t &result);
	
	const irradianceCache_t &c;
	const surfacePoint_t &sp;
	PFLOAT A;
	CFLOAT sum_wi;
	int found;
	bool enough;
};

__END_YAFRAY

#endif // Y_IRRAD_CACHE_H
