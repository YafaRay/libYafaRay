/****************************************************************************
 * 			common.cc: common methods for light integrators
 *      This is part of the yafray package
 *      Copyright (C) 2006  Mathias Wein
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

#include <yafray_config.h>
#include <core_api/material.h>
#include <core_api/integrator.h>
//#include <core_api/background.h>
#include <core_api/light.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <utilities/sample_utils.h>
#include <yafraycore/spectrum.h>

__BEGIN_YAFRAY

/*
// outdated...only for reference of sampling method from paper
// "Illumination in the Presence of Weak Singularities"
color_t estimateDirect(renderState_t &state, const surfacePoint_t &sp, const std::vector<light_t *> &lights, scene_t *scene, const vector3d_t &wo, bool trShad, int sDepth)
{
	color_t col;
	Halton hal3(3);
//	BSDF_t bsdfs;
	bool shadowed;
	unsigned int l_offs = 0;
	const material_t *material = sp.material;
//	material->initBSDF(state, sp, bsdfs);
//	vector3d_t wo = -ray.dir;
	ray_t lightRay;
	lightRay.from = sp.P;
	// contribution of light emitting surfaces
//	col += material->emit(state, wo, sp);
	for(std::vector<light_t *>::const_iterator l=lights.begin(); l!=lights.end(); ++l)
	{
		color_t lcol(0.0), scol;
		float lightPdf;
		// handle lights with delta distribution, e.g. point and directional lights
		if( (*l)->diracLight() )
		{
			if( (*l)->illuminate(sp, lcol, lightRay) )
			{
				// ...shadowed...
				lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.
				shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
				if(!shadowed)
				{
					if(trShad) lcol *= scol;
					color_t surfCol = material->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
					col += surfCol * lcol * std::std::fabs(sp.N*lightRay.dir);
				}
			}
		}
		
		else // area light and suchlike
		{
			int n = (*l)->nSamples();
			unsigned int offs = n * state.pixelSample + state.samplingOffs + l_offs;
			bool bias=(*l)->canIntersect();//false;
			l_offs += 4567; //just some number to have different sequences per light...and it's a prime even...
			color_t ccol(0.0);
			hal3.setStart(offs-1);
			lSample_t ls;
			for(int i=0; i<n; ++i)
			{
				// ...get sample val...
				ls.s1 = RI_vdC(offs+i);
				ls.s2 = hal3.getNext();
				
				if( (*l)->illumSample (sp, ls, lightRay) )
				{
					// ...shadowed...
					lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.
					shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
					if(!shadowed)
					{
						lightPdf = 1.f/ls.pdf;
						if(trShad) ls.col *= scol;
						color_t surfCol = material->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
						//lightPdf *= std::std::fabs(sp.N*lightRay.dir);
						if( lightPdf>1.f && (*l)->canIntersect() ) // bound samples and compensate by sampling from BSDF later
						{
							bias=true;
							lightPdf = 1.f;
						}
						ccol += surfCol * ls.col * std::std::fabs(sp.N*lightRay.dir) * lightPdf;
					}
				}
			}
			col += ccol * ( (CFLOAT)1.0 / (CFLOAT)n );
			if(bias) // sample from BSDF to compensate bias from bounding samples above
			{
				color_t ccol2(0.f);
				for(int i=0; i<n; ++i)
				{
					ray_t bRay;
					bRay.tmin = 0.0005; bRay.from = sp.P;
					float s1 = scrHalton(3, offs+i);
					float s2 = scrHalton(4, offs+i);
					sample_t s(s1, s2, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
					color_t surfCol = material->sample(state, sp, wo, bRay.dir, s);
					if( (*l)->intersect(bRay, bRay.tmax, lcol, lightPdf) )
					{
						shadowed = (trShad) ? scene->isShadowed(state, bRay, sDepth, scol) : scene->isShadowed(state, bRay);
						if(!shadowed)
						{
							if(trShad) lcol *= scol;
							CFLOAT cos2 = std::std::fabs(sp.N*bRay.dir);
							//lightPdf *= cos2;
							if(s.pdf>0.f) ccol2 += surfCol * lcol * cos2 * std::max(0.f, lightPdf-1.f)/(lightPdf*s.pdf);
						}
					}
				}
				col += ccol2 * ( (CFLOAT)1.0 / (CFLOAT)n );
			}
		}
	} //end light loop
	return col;
}
 */
//! estimate direct lighting with multiple importance sampling using the power heuristic with exponent=2
/*! sp.material must be initialized with "initBSDF()" before calling this function! */
color_t estimateDirect_PH(renderState_t &state, const surfacePoint_t &sp, const std::vector<light_t *> &lights, scene_t *scene, const vector3d_t &wo, bool trShad, int sDepth)
{
	color_t col;
	Halton hal3(3);
//	BSDF_t bsdfs;
	bool shadowed;
	unsigned int l_offs = 0;
	const material_t *material = sp.material;
	ray_t lightRay;
	lightRay.from = sp.P;
	for(std::vector<light_t *>::const_iterator l=lights.begin(); l!=lights.end(); ++l)
	{
		color_t lcol(0.0), scol;
		float lightPdf;
		// handle lights with delta distribution, e.g. point and directional lights
		if( (*l)->diracLight() )
		{
			if( (*l)->illuminate(sp, lcol, lightRay) )
			{
				// ...shadowed...
				lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
				shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
				if(!shadowed)
				{
					if(trShad) lcol *= scol;
					color_t surfCol = material->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
					color_t transmitCol = scene->volIntegrator->transmittance(state, lightRay); // FIXME: add also to the other lightsources!
					col += surfCol * lcol * std::fabs(sp.N*lightRay.dir) * transmitCol;
				}
				//else
				//	col = color_t(1, 0, 0); // make areas visible which are not lit due to being shadowed
			}
		}
		else // area light and suchlike
		{
			int n = (*l)->nSamples();
			if(state.rayDivision > 1) n = std::max(1, n/state.rayDivision);
			unsigned int offs = n * state.pixelSample + state.samplingOffs + l_offs;
			bool canIntersect=(*l)->canIntersect();//false;
			l_offs += 4567; //just some number to have different sequences per light...and it's a prime even...
			color_t ccol(0.0);
			hal3.setStart(offs-1);
			lSample_t ls;
			for(int i=0; i<n; ++i)
			{
				// ...get sample val...
				ls.s1 = RI_vdC(offs+i);
				ls.s2 = hal3.getNext();
				if(state.rayDivision > 1)
				{
					ls.s1 = addMod1(ls.s1, state.dc1);
					ls.s2 = addMod1(ls.s2, state.dc2);
				}
				
				if( (*l)->illumSample (sp, ls, lightRay) )
				{
					// ...shadowed...
					lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
					shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
					if(!shadowed && ls.pdf > 1e-6f)
					{
						if(trShad) ls.col *= scol;
						color_t transmitCol = scene->volIntegrator->transmittance(state, lightRay);
						ls.col *= transmitCol;
						color_t surfCol = material->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
						if( canIntersect)
						{
							float mPdf = material->pdf(state, sp, wo, lightRay.dir, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
							float l2 = ls.pdf * ls.pdf;
							float m2 = mPdf * mPdf + 0.01f;
							float w = l2 / (l2 + m2);
							ccol += surfCol * ls.col * std::fabs(sp.N*lightRay.dir) * w / ls.pdf;
						}
						else ccol += surfCol * ls.col * std::fabs(sp.N*lightRay.dir) / ls.pdf;
					}
				}
			}
			col += ccol * ( (CFLOAT)1.0 / (CFLOAT)n );
			if(canIntersect) // sample from BSDF to complete MIS
			{
				color_t ccol2(0.f);
				for(int i=0; i<n; ++i)
				{
					ray_t bRay;
					bRay.tmin = 0.0005; bRay.from = sp.P;
					float s1 = scrHalton(3, offs+i);
					float s2 = scrHalton(4, offs+i);
					if(state.rayDivision > 1)
					{
						s1 = addMod1(s1, state.dc1);
						s2 = addMod1(s2, state.dc2);
					}
					sample_t s(s1, s2, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
					color_t surfCol = material->sample(state, sp, wo, bRay.dir, s);
					if( s.pdf>1e-6f && (*l)->intersect(bRay, bRay.tmax, lcol, lightPdf) )
					{
						shadowed = (trShad) ? scene->isShadowed(state, bRay, sDepth, scol) : scene->isShadowed(state, bRay);
						if(!shadowed)
						{
							if(trShad) lcol *= scol;
							color_t transmitCol = scene->volIntegrator->transmittance(state, lightRay);
							lcol *= transmitCol;
							float lPdf = 1.f/lightPdf;
							float l2 = lPdf * lPdf;
							float m2 = s.pdf * s.pdf;
							float w = m2 / (l2 + m2);
							CFLOAT cos2 = std::fabs(sp.N*bRay.dir);
							if(s.pdf>1.0e-6f) ccol2 += surfCol * lcol * cos2 * w / s.pdf;
						}
					}
				}
				col += ccol2 * ( (CFLOAT)1.0 / (CFLOAT)n );
			}
		}
	} //end light loop
	return col;
}

inline float kernel(PFLOAT r_photon2, PFLOAT r_gather2)
{
	float s = (1.f - r_photon2 / r_gather2);
	return 3.f / (r_gather2 * M_PI) * s * s;
}

inline float ckernel(PFLOAT r_photon2, PFLOAT r_gather2)
{
	float r_p=fSqrt(r_photon2), r_g=fSqrt(r_gather2);
	return 3.f * (1.f - r_p/r_g) / (r_gather2 * M_PI);
}

color_t estimatePhotons(renderState_t &state, const surfacePoint_t &sp, const photonMap_t &map, const vector3d_t &wo, int nSearch, PFLOAT radius)
{
	static bool debug=true;
	if(!map.ready()) return color_t(0.f);
	foundPhoton_t *gathered = (foundPhoton_t *)alloca(nSearch * sizeof(foundPhoton_t));
	int nGathered=0;
	PFLOAT gRadiusSquare = radius;
	nGathered = map.gather(sp.P, gathered, nSearch, gRadiusSquare);
	color_t sum(0.0);
	if(nGathered > 0)
	{
		const material_t *material = sp.material;
		for(int i=0; i<nGathered; ++i)
		{
			const photon_t *photon = gathered[i].photon;
			color_t surfCol = material->eval(state, sp, wo, photon->direction(), BSDF_ALL);
			CFLOAT k = (CFLOAT) kernel(gathered[i].distSquare, gRadiusSquare);
			sum += surfCol * k * photon->color();
		}
		sum *= 1.f / ( float(map.nPaths()) );
	}
	if(debug && nGathered > 10)
	{
		std::cout << "\ncaustic color:" << sum << std::endl;
		debug=false;
	}
	return sum;
}

bool createCausticMap(const scene_t &scene, const std::vector<light_t *> &lights, photonMap_t &cMap, int depth, int count)
{
	cMap.clear();
	ray_t ray;
	int maxBounces = depth;
	unsigned int nPhotons=count;
	int numLights = lights.size();
	float lightNumPdf, lightPdf, s1, s2, s3, s4, s5, s6, s7, sL;
	float fNumLights = (float)numLights;
	float *energies = new float[numLights];
	for(int i=0;i<numLights;++i) energies[i] = lights[i]->totalEnergy().energy();
	pdf1D_t *lightPowerD = new pdf1D_t(energies, numLights);
	for(int i=0;i<numLights;++i) std::cout << "energy: "<< energies[i] <<" (dirac: "<<lights[i]->diracLight()<<")\n";
	delete[] energies;
	//shoot photons
	/* for(int i=0;i<numLights;++i)
	{
		color_t pcol = lights[i]->emitPhoton(.5, .5, .5, .5, ray, lightPdf);
		lightNumPdf = lightPowerD->func[i] * lightPowerD->invIntegral;
		pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of the pdf, hence *=...
		std::cout << "photon col:"<<pcol<<" lnpdf: "<<lightNumPdf<<"\n";
	} */
	bool done=false;
	unsigned int curr=0;
	surfacePoint_t sp1, sp2;
	surfacePoint_t *hit=&sp1, *hit2=&sp2;
	renderState_t state;
	unsigned char userdata[USER_DATA_SIZE+7];
	state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	while(!done)
	{
		state.chromatic = true;
		state.wavelength = RI_S(curr);
		s1 = RI_vdC(curr);
		s2 = scrHalton(2, curr);
		s3 = scrHalton(3, curr);
		s4 = scrHalton(4, curr);
		//sL = RI_S(curr);
		sL = float(curr) / float(nPhotons);
		int lightNum = lightPowerD->DSample(sL, &lightNumPdf);
		if(lightNum >= numLights){ std::cout << "lightPDF sample error! "<<sL<<"/"<<lightNum<<"\n"; delete lightPowerD; return false; }
		
		color_t pcol = lights[lightNum]->emitPhoton(s1, s2, s3, s4, ray, lightPdf);
		ray.tmin = 0.0005;
		ray.tmax = -1.0;
		pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= nPhotons);
			continue;
		}
		BSDF_t bsdfs = BSDF_NONE;
		int nBounces=0;
		bool causticPhoton = false;
		bool directPhoton = true;
		const material_t *material = 0;
		while( scene.intersect(ray, *hit2) )
		{
			if(isnan(pcol.R) || isnan(pcol.G) || isnan(pcol.B))
			{ std::cout << "NaN WARNING (photon color)" << std::endl; break; }
			color_t transm(1.f), vcol;
			// check for volumetric effects
			if(material)
			{
				if((bsdfs&BSDF_VOLUMETRIC) && material->volumeTransmittance(state, *hit, ray, vcol))
				{
					transm = vcol;
				}
			}
			std::swap(hit, hit2);
			vector3d_t wi = -ray.dir, wo;
			material = hit->material;
			material->initBSDF(state, *hit, bsdfs);
			if(bsdfs & (BSDF_DIFFUSE | BSDF_GLOSSY))
			{
				//deposit caustic photon on surface
				if(causticPhoton)
				{
					photon_t np(wi, hit->P, pcol);
					cMap.pushPhoton(np);
					cMap.setNumPaths(curr);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(nBounces == maxBounces) break;
			// scatter photon
			int d5 = 3*nBounces + 5;
			//int d6 = d5 + 1;
			if(d5+2 <= 50)
			{
				s5 = scrHalton(d5, curr);
				s6 = scrHalton(d5+1, curr);
				s7 = scrHalton(d5+2, curr);
			}
			else
			{
				s5 = ourRandom();
				s6 = ourRandom();
				s7 = ourRandom();
			}
			pSample_t sample(s5, s6, s7, BSDF_ALL_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE, pcol, transm);
			bool scattered = material->scatterPhoton(state, *hit, wi, wo, sample);
			if(!scattered) break; //photon was absorped.
			pcol = sample.color;
			// hm...dispersive is not really a scattering qualifier like specular/glossy/diffuse or the special case filter...
			causticPhoton = ((sample.sampledFlags & (BSDF_SPECULAR | BSDF_DISPERSIVE)) && directPhoton) ||
							((sample.sampledFlags & (BSDF_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE)) && causticPhoton);
			// light through transparent materials can be calculated by direct lighting, so still consider them direct!
			directPhoton = (sample.sampledFlags & BSDF_FILTER) && directPhoton;
			// caustic-only calculation can be stopped if:
			if(!(causticPhoton || directPhoton)) break;
			if(state.chromatic && sample.sampledFlags & BSDF_DISPERSIVE)
			{
				state.chromatic=false;
				color_t wl_col;
				wl2rgb(state.wavelength, wl_col);
				pcol *= wl_col;
			}
			ray.from = hit->P;
			ray.dir = wo;
			ray.tmin = 0.0005;
			ray.tmax = -1.0;
			++nBounces;
		}
		++curr;
		done = (curr >= nPhotons) ? true : false;
	}
	delete lightPowerD;
	if(cMap.nPhotons() > 0) cMap.updateTree();
	return true;
}

__END_YAFRAY
