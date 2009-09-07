
#include <core_api/scene.h>
#include <core_api/surface.h>
#include <yafraycore/irradiancecache.h>
#include <yafraycore/octree.h>

__BEGIN_YAFRAY

float irradianceCache_t::weight(const irradSample_t &s, const surfacePoint_t &sp, PFLOAT A_proj) const
{
	const PFLOAT R_proj = sqrt(A_proj);
	const float R_minus = 1.5f * R_proj;
	const float R_plus = 10.f * R_proj;
	const vector3d_t v = s.P - sp.P;
	float Eps_pi = v.length() / ( std::max( std::min(0.5f*s.Rmin, R_plus), R_minus) );
	float Eps_ni = sqrt ( 1.00001 - (s.N*sp.N) ) * 8.11314; // 1.0 / sqrt(1-cos(10°)) == 8.11314
	return 1.f - K * std::max(Eps_pi, Eps_ni);
}

void irradianceCache_t::init(const scene_t &scene, PFLOAT Kappa)
{
	K = std::max(PFLOAT(0.1), Kappa);
	if(tree) delete tree;
	tree = new octree_t<irradSample_t>(scene.getSceneBound(), 20);
}

bool irradianceCache_t::gatherSamples(const surfacePoint_t sp, PFLOAT A_pix, irradSample_t &irr, bool debug) const
{
	irradLookup_t lk(*this, sp, A_pix, debug);
	tree->lookup(sp.P, lk);
	bool success = lk.getIrradiance(irr);
	return success;
}

bool irradianceCache_t::enoughSamples(const surfacePoint_t sp, PFLOAT A_pix) const
{
	availabilityLookup_t lk(*this, sp, A_pix);
	tree->lookup(sp.P, lk);
	return lk.enough;
}

void irradianceCache_t::insert(const irradSample_t &s)
{
	const PFLOAT R_proj = sqrt(s.Apix);
	PFLOAT R_plus = 15.f * R_proj; //larger than actual R_plus, in weight()
	vector3d_t diag(R_plus, R_plus, R_plus);
	tree->add(s, bound_t(s.P - diag, s.P + diag) );
}

bool irradLookup_t::operator()(const point3d_t &p, const irradSample_t &s)
{
	float wi = c.weight(s, sp, A);
	if(wi > 0.0001)
	{
		if(debug) std::cout << "wi: " << wi << ", s.P:" << s.P << "\tE:" << s.col.energy() << " d:" << (sp.P-s.P).length() << std::endl;
		++found;
		sum_E += wi * s.col;
		sum_wi += wi;
		sum_w_r += wi * s.w_r;
		sum_w_g += wi * s.w_g;
		sum_w_b += wi * s.w_b;
	}
	return true;
}

bool irradLookup_t::getIrradiance(irradSample_t &result)
{
	if(found > 0)
	{
		result.col = sum_E * (1.f/sum_wi);
		result.w_r = sum_w_r.normalize();
		result.w_g = sum_w_g.normalize();
		result.w_b = sum_w_b.normalize();
		return true;
	}
	return false;
}

bool availabilityLookup_t::operator()(const point3d_t &p, const irradSample_t &s)
{
	float wi = c.weight(s, sp, A);
	if(wi > 0.0001)
	{
		sum_wi += wi;
		++found;
		if(sum_wi > 1.f)
		{
			enough=true;
			return false;
		}
	}
	return true;
}

__END_YAFRAY
