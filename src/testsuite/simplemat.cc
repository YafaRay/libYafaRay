#include <cstdlib>
#include <testsuite/simplemat.h>
#include <core_api/scene.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

simplemat_t::simplemat_t(const color_t &col, float transp, float emit, texture_t *tex): texture(tex)
{
	transparent = (transp > 0.0) ? true : false;
	trCol=col*transp;
	emitCol=col*emit;
//		bsdf.col = (1.0-transp)*col;
	color = (1.0-transp)*col;
	// photon scattering precalculations:
	CFLOAT lum = std::max(col.getR(), std::max(col.getG(), col.getB()));//col.energy();
	pScatter = std::min(1.f, lum);
	photonCol = col * (1.f/pScatter);
	bsdfFlags = BSDF_DIFFUSE;
	if(transparent)
	{
		bsdfFlags |= BSDF_SPECULAR;
		pDiffuse = 1.f-transp;
	}
}

void simplemat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							  bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const
{
	if(transparent)
	{
		refract=true;
		dir[1] = -wo;
		col[1] = trCol;
	}
}

color_t simplemat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, unsigned int bsdfs)const
{
//	if(sp.N*wl < 0.0) return color_t(0.f);
	if(texture) return texture->getColor( point3d_t(sp.U, sp.V, 0.f/*1.f - sp.U - sp.V*/) );
	return color;
}

color_t simplemat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	wi = SampleCosHemisphere(sp.N, sp.NU, sp.NV, s.s1, s.s2);
	return color;
}

bool simplemat_t::scatterPhoton(const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, float s1, float s2,
								BSDF_t bsdfs, BSDF_t &sampledBSDF, color_t &col) const
{
	if(s1 > pScatter) return false;
	col = photonCol;
	if(transparent)
	{
		if(s2 > pDiffuse) // photon goes through material
		{
			sampledBSDF = BSDF_NONE;
			wo = -wi;
			return true;
		}
		s2 /= pDiffuse;
	}
	// scatter photon
	s1 /= pScatter;
	sampledBSDF = BSDF_DIFFUSE;
	if(s1>1.0 || s2>1.0 || s1<0.0 || s2<0){ std::cout << "wasted!\n"; exit(1); }
	wo = SampleCosHemisphere(sp.N, sp.NU, sp.NV, s1, s2);
	if(wo*sp.N < 0.0) std::cout << "wtf!?\n";
	return true;
}

__END_YAFRAY
