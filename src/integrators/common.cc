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
#include <core_api/light.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <utilities/sample_utils.h>
#include <yafraycore/spectrum.h>
#include <integrators/integr_utils.h>

__BEGIN_YAFRAY

//! estimate direct lighting with multiple importance sampling using the power heuristic with exponent=2
/*! sp.material must be initialized with "initBSDF()" before calling this function! */
color_t estimateDirect_PH(renderState_t &state, const surfacePoint_t &sp, const std::vector<light_t *> &lights, scene_t *scene, const vector3d_t &wo, bool trShad, int sDepth)
{
	color_t col;
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
			Halton hal3(3);
			int n = (*l)->nSamples();
			if(state.rayDivision > 1) n = std::max(1, n/state.rayDivision);
			float invNS = 1.f / (float)n;
			unsigned int offs = n * state.pixelSample + state.samplingOffs + l_offs;
			bool canIntersect=(*l)->canIntersect();//false;
			l_offs += 4567; //just some number to have different sequences per light...and it's a prime even...
			color_t ccol(0.0);
			lSample_t ls;
			hal3.setStart(offs-1);
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
							if(mPdf > 1e-6f)
							{
								float l2 = ls.pdf * ls.pdf;
								float m2 = mPdf * mPdf;
								float w = l2 / (l2 + m2);
								ccol += surfCol * ls.col * std::fabs(sp.N*lightRay.dir) * w / ls.pdf;
							}
							else
							{
								ccol += surfCol * ls.col * std::fabs(sp.N*lightRay.dir) / ls.pdf;
							}
						}
						else ccol += surfCol * ls.col * std::fabs(sp.N*lightRay.dir) / ls.pdf;
					}
				}
			}
			col += ccol * invNS;
			if(canIntersect) // sample from BSDF to complete MIS
			{
				color_t ccol2(0.f);
				for(int i=0; i<n; ++i)
				{
					ray_t bRay;
					bRay.tmin = MIN_RAYDIST; bRay.from = sp.P;
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
						if(!shadowed && lightPdf > 1e-6f)
						{
							if(trShad) lcol *= scol;
							color_t transmitCol = scene->volIntegrator->transmittance(state, lightRay);
							lcol *= transmitCol;
							float lPdf = 1.f/lightPdf;
							float l2 = lPdf * lPdf;
							float m2 = s.pdf * s.pdf;
							float w = m2 / (l2 + m2);
							CFLOAT cos2 = std::fabs(sp.N*bRay.dir);
							ccol2 += surfCol * lcol * cos2 * w / s.pdf;
						}
					}
				}
				col += ccol2 * invNS;
			}
		}
	} //end light loop
	return col;
}

color_t estimatePhotons(renderState_t &state, const surfacePoint_t &sp, const photonMap_t &map, const vector3d_t &wo, int nSearch, PFLOAT radius)
{
	if(!map.ready()) return color_t(0.f);
	
	foundPhoton_t *gathered = (foundPhoton_t *)alloca(nSearch * sizeof(foundPhoton_t));
	int nGathered = 0;
	
	float gRadiusSquare = radius * radius;
	
	nGathered = map.gather(sp.P, gathered, nSearch, gRadiusSquare);
	
	gRadiusSquare = 1.f / gRadiusSquare;
	
	color_t sum(0.f);
	
	if(nGathered > 0)
	{
		const material_t *material = sp.material;
		color_t surfCol(0.f);
		float k = 0.f;
		const photon_t *photon;

		for(int i=0; i<nGathered; ++i)
		{
			photon = gathered[i].photon;
			surfCol = material->eval(state, sp, wo, photon->direction(), BSDF_ALL);
			k = kernel(gathered[i].distSquare, gRadiusSquare);
			sum += surfCol * k * photon->color();
		}
		sum *= 1.f / ( float(map.nPaths()) );
	}
	return sum;
}

bool createCausticMap(const scene_t &scene, const std::vector<light_t *> &all_lights, photonMap_t &cMap, int depth, int count, progressBar_t *pb, std::string intName)
{
	cMap.clear();
	ray_t ray;
	int maxBounces = depth;
	unsigned int nPhotons=count;
	std::vector<light_t *> lights;
	
	for(unsigned int i=0;i<all_lights.size();++i)
	{
		if(all_lights[i]->shootsCausticP())
		{
			lights.push_back(all_lights[i]);
		}
		
	}

	int numLights = lights.size();

	if(numLights > 0)
	{
		float lightNumPdf, lightPdf, s1, s2, s3, s4, s5, s6, s7, sL;
		float fNumLights = (float)numLights;
		float *energies = new float[numLights];
		for(int i=0;i<numLights;++i) energies[i] = lights[i]->totalEnergy().energy();
		pdf1D_t *lightPowerD = new pdf1D_t(energies, numLights);
		
		Y_INFO << intName << ": Light(s) photon color testing for caustics map:" << yendl;
		color_t pcol(0.f);
		for(int i=0;i<numLights;++i)
		{
			pcol = lights[i]->emitPhoton(.5, .5, .5, .5, ray, lightPdf);
			lightNumPdf = lightPowerD->func[i] * lightPowerD->invIntegral;
			pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			Y_INFO << intName << ": Light [" << i+1 << "] Photon col:" << pcol << " | lnpdf: " << lightNumPdf << yendl;
		}
		
		delete[] energies;

		int pbStep;
		Y_INFO << intName << ": Building caustics photon map..." << yendl;
		pb->init(128);
		pbStep = std::max(1U, nPhotons / 128);
		pb->setTag("Building caustics photon map...");

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
			
			sL = float(curr) / float(nPhotons);
			
			int lightNum = lightPowerD->DSample(sL, &lightNumPdf);
			
			if(lightNum >= numLights)
			{
				Y_ERROR << intName << ": lightPDF sample error! " << sL << "/" << lightNum << yendl;
				delete lightPowerD;
				return false;
			}
			
			color_t pcol = lights[lightNum]->emitPhoton(s1, s2, s3, s4, ray, lightPdf);
			ray.tmin = MIN_RAYDIST;
			ray.tmax = -1.0;
			pcol *= fNumLights * lightPdf / lightNumPdf; //remember that lightPdf is the inverse of th pdf, hence *=...
			if(pcol.isBlack())
			{
				++curr;
				done = (curr >= nPhotons);
				continue;
			}
			BSDF_t bsdfs = BSDF_NONE;
			int nBounces = 0;
			bool causticPhoton = false;
			bool directPhoton = true;
			const material_t *material = 0;
			const volumeHandler_t *vol = 0;
			
			while( scene.intersect(ray, *hit2) )
			{
				if(isnan(pcol.R) || isnan(pcol.G) || isnan(pcol.B))
				{
					Y_WARNING << intName << ": NaN (photon color)" << yendl;
					break;
				}
				color_t transm(1.f), vcol;
				// check for volumetric effects
				if(material)
				{
					if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(hit->Ng * ray.dir < 0)))
					{
						vol->transmittance(state, ray, vcol);
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

				s5 = scrHalton(d5, curr);
				s6 = scrHalton(d5+1, curr);
				s7 = scrHalton(d5+2, curr);

				pSample_t sample(s5, s6, s7, BSDF_ALL_SPECULAR | BSDF_GLOSSY | BSDF_FILTER | BSDF_DISPERSIVE, pcol, transm);
				bool scattered = material->scatterPhoton(state, *hit, wi, wo, sample);
				if(!scattered) break; //photon was absorped.
				pcol = sample.color;
				// hm...dispersive is not really a scattering qualifier like specular/glossy/diffuse or the special case filter...
				causticPhoton = ((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_DISPERSIVE)) && directPhoton) ||
								((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE)) && causticPhoton);
				// light through transparent materials can be calculated by direct lighting, so still consider them direct!
				directPhoton = (sample.sampledFlags & BSDF_FILTER) && directPhoton;
				// caustic-only calculation can be stopped if:
				if(!(causticPhoton || directPhoton)) break;

				if(state.chromatic && (sample.sampledFlags & BSDF_DISPERSIVE))
				{
					state.chromatic=false;
					color_t wl_col;
					wl2rgb(state.wavelength, wl_col);
					pcol *= wl_col;
				}
				ray.from = hit->P;
				ray.dir = wo;
				ray.tmin = MIN_RAYDIST;
				ray.tmax = -1.0;
				++nBounces;
			}
			++curr;
			if(curr % pbStep == 0) pb->update();
			done = (curr >= nPhotons);
		}
		pb->done();
		pb->setTag("Caustic photon map built.");
		Y_INFO << intName << ": Done." << yendl;
		Y_INFO << intName << ": Shot " << curr << " caustic photons from " << numLights <<" light(s)." << yendl;
		Y_INFO << intName << ": Stored caustic photons: " << cMap.nPhotons() << yendl;
		
		delete lightPowerD;
		
		if(cMap.nPhotons() > 0)
		{
			pb->setTag("Building caustic photons kd-tree...");
			cMap.updateTree();
			Y_INFO << intName << ": Done." << yendl;
		}
	}
	else
	{
		Y_INFO << intName << ": No caustic source lights found, skiping caustic map building..." << yendl;
	}
	return true;
}

__END_YAFRAY
