/****************************************************************************
 * 			pathtracer.cc: a rather simple QMC path integrator
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
 
//#include <mcqmc.h>
#include <yafray_config.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/volume.h>
#include <yafraycore/tiledintegrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/photon.h>
#include <yafraycore/spectrum.h>
#include <integrators/integr_utils.h>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT pathIntegrator_t: public tiledIntegrator_t
{
	public:
		pathIntegrator_t(/*scene_t &s,*/ bool transpShad=false, int shadowDepth=4);
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
		enum { NONE, PATH, PHOTON, BOTH };
	protected:
//	color_t estimateDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, light_t *light, int d1, int n) const;
		color_t estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, const std::vector<light_t *>  &lights, int d1, int n)const;
		background_t *background;
		bool trShad;
		bool use_bg; //!< configuration; include background for GI
		bool ibl; //!< configuration; use background light, if available
		bool include_bg; //!< determined on precrocess;
		bool traceCaustics; //!< use path tracing for caustics (determined by causticType)
		bool no_recursive;
		int sDepth, rDepth, bounces, nPaths;
		int causticType, nPhotons, cDepth, nSearch;
		PFLOAT cRadius; //!< radius to search for caustic photons
		std::vector<light_t*> lights;
		photonMap_t causticMap;
};

pathIntegrator_t::pathIntegrator_t(/*scene_t &s,*/ bool transpShad, int shadowDepth):
	trShad(transpShad), sDepth(shadowDepth), causticType(PATH)
{
	std::cout << "created pathIntegrator!\n";
	type = SURFACE;
//	scene = &s;
	rDepth = 6;
	bounces = 5;
	nPaths = 64;
	use_bg = true;
	no_recursive = false;
}

bool pathIntegrator_t::preprocess()
{
	background = scene->getBackground();
	lights = scene->lights;
	if(background)
	{
		light_t *bgl = background->getLight();
		if(bgl)
		{
			lights.push_back(bgl);
			ibl = true;
		}
		else ibl = false;
		include_bg = use_bg && !ibl;
	}
	else
	{
		include_bg = false;
		ibl = false;
	}
	
	// create caustics photon map, if requested
	std::vector<light_t*> cLights;
	std::vector<light_t*>::const_iterator li;
	bool success;
	if(causticType == PHOTON || causticType == BOTH)
	{
		success = createCausticMap(*scene, lights, causticMap, cDepth, nPhotons);
	}
/*
	else if(causticType == BOTH)
	{
		for(li=scene->lights.begin(); li!=scene->lights.end(); ++li)
		{
			if((*li)->diracLight()) cLights.push_back(*li);
		}
		if(cLights.size()>0) success = createCausticMap(*scene, cLights, causticMap, cDepth, nPhotons);
	}
*/

	if(causticType == BOTH || causticType == PATH) traceCaustics = true;
	else traceCaustics = false;
	return true;
}

inline color_t pathIntegrator_t::estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, const std::vector<light_t *>  &lights, int d1, int n)const
{
	//static bool debug=true;
	color_t lcol(0.0), scol, col(0.0);
	ray_t lightRay;
	float lightPdf;
	bool shadowed;
	const material_t *oneMat = sp.material;
	lightRay.from = sp.P;
	int nLightsI = lights.size();
	if(nLightsI == 0) return color_t(0.f);
	float nLights = float(nLightsI);
	float s1;
	if(d1 > 50)  s1 = (*state.prng)();
	else s1 = scrHalton(d1, n) * nLights;
	int lnum = (int)(s1);
	if(lnum > nLightsI-1) lnum = nLightsI-1;
	const light_t *light = lights[lnum];
	s1 = s1 - (float)lnum; // scrHalton(d1, n); // 
	//BSDF_t oneBSDFs = oneMat->getFlags();
	//oneMat->initBSDF(state, sp, oneBSDFs);
	// handle lights with delta distribution, e.g. point and directional lights
	if( light->diracLight() )
	{
		if( light->illuminate(sp, lcol, lightRay) )
		{
			// ...shadowed...
			lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
			shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
			if(!shadowed)
			{
				if(trShad) lcol *= scol;
				color_t surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
				col = surfCol * lcol * std::fabs(sp.N*lightRay.dir);
			}
		}
	}
	else // area light and suchlike
	{
		//color_t ccol(0.0);
		// ...get sample val...
		lSample_t ls;
		ls.s1 = s1;
		if(d1 > 49)  ls.s2 = (*state.prng)();
		else ls.s2 = scrHalton(d1+1, n);
		bool canIntersect=light->canIntersect();
		
		if( light->illumSample (sp, ls, lightRay) )
		{
			// ...shadowed...
			lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
			shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
			if(!shadowed && ls.pdf > 1e-6f)
			{
				if(trShad) ls.col *= scol;
				//color_t surfCol = bsdf->eval(sp, wo, lightRay.dir, userdata);
				color_t surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
				
				if( canIntersect )
				{
					float mPdf = oneMat->pdf(state, sp, wo, lightRay.dir, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
					float l2 = ls.pdf * ls.pdf;
					float m2 = mPdf * mPdf + 0.1f;
					float w = l2 / (l2 + m2);
					//test! limit lightPdf...
					if(ls.pdf < 0.1f) ls.pdf = 0.1f;
					col = surfCol * ls.col * std::fabs(sp.N*lightRay.dir) * w / ls.pdf;
				}
				else
				{
					//test! limit lightPdf...
					if(ls.pdf < 0.0001f) ls.pdf = 0.0001f;
					col = surfCol * ls.col * std::fabs(sp.N*lightRay.dir) / ls.pdf;
				}
			}
		}
		if(canIntersect) // sample from BSDF to complete MIS
		{
			//color_t ccol2(0.f);

			ray_t bRay;
			bRay.tmin = 0.0005; bRay.from = sp.P;
			sample_t s(ls.s1, ls.s2, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
			color_t surfCol = oneMat->sample(state, sp, wo, bRay.dir, s);
			if( s.pdf>1e-6f &&  light->intersect(bRay, bRay.tmax, lcol, lightPdf) )
			{
				shadowed = (trShad) ? scene->isShadowed(state, bRay, sDepth, scol) : scene->isShadowed(state, bRay);
				if(!shadowed)
				{
					if(trShad) lcol *= scol;
					float lPdf = 1.f/lightPdf;
					float l2 = lPdf * lPdf;
					float m2 = s.pdf * s.pdf + 0.1f;
					float w = m2 / (l2 + m2);
					CFLOAT cos2 = std::fabs(sp.N*bRay.dir);
					if(s.pdf>1.0e-6f) col += surfCol * lcol * cos2 * w / s.pdf;
				}
			}
		}
	}
	return col*nLights;
}

colorA_t pathIntegrator_t::integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const
{
	static int calls=0;
	++calls;
	color_t col(0.0);
	CFLOAT alpha=0.0;
	surfacePoint_t sp;
	void *o_udat = state.userdata;
	//shoot ray into scene
	if(scene->intersect(ray, sp))
	{
		// if camera ray initialize sampling offset:
		if(state.raylevel == 0)
		{
			state.includeLights = true;
			//...
		}
		unsigned char userdata[USER_DATA_SIZE+7];
		userdata[0] = 0;
		state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
		BSDF_t bsdfs;
		const material_t *material = sp.material;
		material->initBSDF(state, sp, bsdfs);
		vector3d_t wo = -ray.dir;
		// contribution of light emitting surfaces
		col += material->emit(state, sp, wo);
		
		if(bsdfs & (BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE))
		{
			col += estimateDirect_PH(state, sp, lights, scene, wo, trShad, sDepth);
		}
		if(bsdfs & (BSDF_DIFFUSE | BSDF_GLOSSY))
		{
			col += estimatePhotons(state, sp, causticMap, wo, nSearch, cRadius);
		}
		// path tracing:
		// the first path segment is "unrolled" from the loop because for the spot the camera hit
		// we do things slightly differently (e.g. may not sample specular, need not to init BSDF anymore,
		// have more efficient ways to compute samples...)
		bool was_chromatic = state.chromatic;
		BSDF_t path_flags = no_recursive ? BSDF_ALL : (BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE);
		if(bsdfs & path_flags)
		{
			color_t pathCol(0.0), wl_col;
			path_flags |= (BSDF_REFLECT | BSDF_TRANSMIT);
			for(int i=0; i<nPaths; ++i)
			{
				void *first_udat = state.userdata;
				unsigned char userdata[USER_DATA_SIZE+7];
				void *n_udat = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
				unsigned int offs = nPaths * state.pixelSample + state.samplingOffs + i; // some redunancy here...
				//bool vol_scatter = false;
				color_t throughput( 1.0 );
				color_t lcol, scol, vcol;
				surfacePoint_t sp1=sp, sp2;
				surfacePoint_t *hit=&sp1, *hit2=&sp2;
				vector3d_t pwo = wo;
				ray_t pRay;
				state.chromatic = was_chromatic;
				if(was_chromatic) state.wavelength = RI_S(offs);
				//this mat already is initialized, just sample (diffuse...non-specular?)
				float s1 = RI_vdC(offs);
				float s2 = scrHalton(2, offs);
				// do proper sampling now...
				sample_t s(s1, s2, path_flags);
				scol = material->sample(state, sp, pwo, pRay.dir, s);
				if(s.pdf > 1.0e-6f) scol *= (std::fabs(pRay.dir*sp.N)/s.pdf);
				else continue;
				throughput = scol;
				state.includeLights = false;
				//state.chromatic = state.chromatic && !(s.sampledFlags & BSDF_DISPERSIVE);
				if(state.chromatic && (s.sampledFlags & BSDF_DISPERSIVE))
				{
					state.chromatic = false;
					wl2rgb(state.wavelength, wl_col);
					throughput *= wl_col;
				}
				pRay.tmin = 0.0005;
				pRay.tmax = -1.0;
				pRay.from = sp.P;
				if(!scene->intersect(pRay, *hit)) //hit background
				{
					if(include_bg) pathCol += throughput * (*background)(pRay, state, true);
					continue;
				}
				//if((bsdfs&BSDF_VOLUMETRIC) && material->volumeTransmittance(state, sp, pRay, vcol))
				const volumeHandler_t *vol;
				if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.Ng * pRay.dir < 0)) != 0)
				{
					vol->transmittance(state, pRay, vcol);
					throughput *= vcol;
					/*ray_t sRay;
					pSample_t s((*state.prng)(),(*state.prng)(),(*state.prng)(),BSDF_ALL,color_t(0.f));
					if(vol->scatter(state, pRay, sRay, s))
					{
						vol_scatter = true;
					}*/
				}
				state.userdata = n_udat;
				const material_t *p_mat = hit->material;
				BSDF_t matBSDFs;
				p_mat->initBSDF(state, *hit, matBSDFs);
				pwo = -pRay.dir;
				lcol = estimateOneDirect(state, *hit, pwo, lights, 3, offs);
				lcol += p_mat->emit(state, *hit, pwo);
				/* if((matBSDFs&BSDF_VOLUMETRIC) && p_mat->volumeTransmittance(state, hit, pRay, vcol))
				{
					throughput *= vcol;
				} */
				pathCol += lcol*throughput;
				
				for(int depth=1; depth<bounces; ++depth)
				{
					int d4 = 4*depth;
					if(d4 > 49)
					{
						s.s1 = (*state.prng)();
						s.s2 = (*state.prng)();
					}
					else
					{
						s.s1 = scrHalton(4*depth+1, offs); //ourRandom();//
						s.s2 = scrHalton(4*depth+2, offs); //ourRandom();//
					}
					s.flags = BSDF_ALL;
					
					scol = p_mat->sample(state, *hit, pwo, pRay.dir, s);
					if(s.pdf > 1.0e-6f) scol *= (std::fabs(pRay.dir*hit->N)/s.pdf);
					else break;
					if(scol.isBlack()) break;
					throughput *= scol;
					state.includeLights = traceCaustics && (s.sampledFlags & BSDF_SPECULAR);
					//state.chromatic = state.chromatic && !(s.sampledFlags & BSDF_DISPERSIVE);
					if(state.chromatic && (s.sampledFlags & BSDF_DISPERSIVE))
					{
						state.chromatic = false;
						wl2rgb(state.wavelength, wl_col);
						throughput *= wl_col;
					}
					pRay.tmin = 0.0005;
					pRay.tmax = -1.0;
					pRay.from = hit->P;

					if(!scene->intersect(pRay, *hit2)) //hit background
					{
						if(include_bg || (state.includeLights && ibl)) pathCol += throughput * (*background)(pRay, state, true);
						break;
					}
					//if((matBSDFs&BSDF_VOLUMETRIC) && p_mat->volumeTransmittance(state, *hit, pRay, vcol))
					if((matBSDFs&BSDF_VOLUMETRIC) && (vol=p_mat->getVolumeHandler(hit->Ng * pRay.dir < 0)) != 0)
					{
						vol->transmittance(state, pRay, vcol);
						throughput *= vcol;
					}
					std::swap(hit, hit2);
					p_mat = hit->material;
					p_mat->initBSDF(state, *hit, matBSDFs);
					pwo = -pRay.dir;
					/*for(std::vector<light_t *>::iterator l=scene->lights.begin(); l!=scene->lights.end(); ++l)
					{
						lcol += estimateDirect(state, hit, pwo, *l, 4*depth+3, offs);
					}*/
					if(matBSDFs & (BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE)) lcol = estimateOneDirect(state, *hit, pwo, lights, 4*depth+3, offs);
					else lcol = color_t(0.f);
					lcol += p_mat->emit(state, *hit, pwo);
					/* if((matBSDFs&BSDF_VOLUMETRIC) && p_mat->volumeTransmittance(state, hit, pRay, vcol))
					{
						throughput *= vcol;
					} */
					pathCol += lcol*throughput;
				}
				state.userdata = first_udat;
				
			}
			col += pathCol * ( (CFLOAT)1.0 / (CFLOAT)nPaths );
		}
		//reset chromatic state:
		state.chromatic = was_chromatic;
		//...reflection/refraction with recursive raytracing...
		++state.raylevel;
		if(no_recursive == false && state.raylevel <= rDepth)
		{
			state.includeLights = true;
			bool reflect=false, refract=false;
			vector3d_t dir[2];
			color_t rcol[2], vcol;
			material->getSpecular(state, sp, wo, reflect, refract, &dir[0], &rcol[0]);
			if(reflect)
			{
				diffRay_t refRay(sp.P, dir[0], 0.0005);
				color_t integ  = color_t(integrate(state, refRay) );
				if((bsdfs&BSDF_VOLUMETRIC) && material->volumeTransmittance(state, sp, refRay, vcol))
				{	integ *= vcol;	}
				col += color_t(integ) * rcol[0];
			}
			if(refract)
			{
				diffRay_t refRay(sp.P, dir[1], 0.0005);
				colorA_t integ = integrate(state, refRay);
				if((bsdfs&BSDF_VOLUMETRIC) && material->volumeTransmittance(state, sp, refRay, vcol))
				{	integ *= vcol;	}
				col += color_t(integ) * rcol[1];
				alpha = integ.A;
			}
		}
		--state.raylevel;
		// account for volumetric effects:
		/* color_t vcol;
		if(material->volumeTransmittance(state, sp, ray, vcol))
		{
			col *= vcol;
		} */
		
		CFLOAT m_alpha = material->getAlpha(state, sp, wo);
		alpha = m_alpha + (1.f-m_alpha)*alpha;
	}
	else //nothing hit, return background
	{
		if(background)
		{
			col += (*background)(ray, state, false);
		}
	}
	//dbg = false;
	state.userdata = o_udat;
	return colorA_t(col, alpha);
}

integrator_t* pathIntegrator_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	bool transpShad=false, noRec=false;
	int shadowDepth = 5;
	int path_samples = 32;
	int bounces = 3;
	int raydepth = 5;
	bool use_bg = true;
	const std::string *cMethod=0;
	
	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transpShad);
	params.getParam("shadowDepth", shadowDepth);
	params.getParam("path_samples", path_samples);
	params.getParam("bounces", bounces);
	params.getParam("use_background", use_bg);
	params.getParam("no_recursive", noRec);
	
	pathIntegrator_t* inte = new pathIntegrator_t(transpShad, shadowDepth);
	if(params.getParam("caustic_type", cMethod))
	{
		bool usePhotons=false;
		if(*cMethod == "photon"){ inte->causticType = PHOTON; usePhotons=true; }
		else if(*cMethod == "both"){ inte->causticType = BOTH; usePhotons=true; }
		else if(*cMethod == "none") inte->causticType = NONE;
		if(usePhotons)
		{
			double cRad = 0.25;
			int cDepth=10, search=100, photons=500000;
			params.getParam("photons", photons);
			params.getParam("caustic_mix", search);
			params.getParam("caustic_depth", cDepth);
			params.getParam("caustic_radius", cRad);
			inte->nPhotons = photons;
			inte->nSearch = search;
			inte->cDepth = cDepth;
			inte->cRadius = cRad;
		}
	}
	inte->rDepth = raydepth;
	inte->nPaths = path_samples;
	inte->bounces = bounces;
	inte->use_bg = use_bg;
	inte->no_recursive = noRec;
	return inte;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("pathtracing",pathIntegrator_t::factory);
	}

}

__END_YAFRAY
