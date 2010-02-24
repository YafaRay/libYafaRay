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
#include <sstream>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT pathIntegrator_t: public tiledIntegrator_t
{
	public:
		pathIntegrator_t(bool transpShad=false, int shadowDepth=4);
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
		enum { NONE, PATH, PHOTON, BOTH };
	protected:
		color_t estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, const std::vector<light_t *>  &lights, int d1, int n) const;
		background_t *background;
		bool trShad;
		bool traceCaustics; //!< use path tracing for caustics (determined by causticType)
		bool no_recursive;
		int sDepth, rDepth, bounces, nPaths;
		float invNPaths;
		int causticType, nPhotons, cDepth, nSearch;
		PFLOAT cRadius; //!< radius to search for caustic photons
		std::vector<light_t*> lights;
		photonMap_t causticMap;
		bool hasBGLight;
};

pathIntegrator_t::pathIntegrator_t(bool transpShad, int shadowDepth):
	trShad(transpShad), sDepth(shadowDepth), causticType(PATH)
{
	type = SURFACE;
	rDepth = 6;
	bounces = 5;
	nPaths = 64;
	invNPaths = 1.f/64.f;
	no_recursive = false;
	integratorName = "PathTracer";
	integratorShortName = "PT";
	hasBGLight = false;
}

bool pathIntegrator_t::preprocess()
{
	std::stringstream set;
	background = scene->getBackground();
	lights = scene->lights;
	
	if(trShad)
	{
		set << "ShadowDepth: [" << sDepth << "]";
	}
	if(!set.str().empty()) set << "+";
	set << "RayDepth: [" << rDepth << "]";

	if(background)
	{
		light_t *bgl = background->getLight();
		if(bgl)
		{
			lights.push_back(bgl);
			hasBGLight = true;
			set << "IBL";
		}
	}
	
	// create caustics photon map, if requested

	bool success = true;
	traceCaustics = false;
	
	if(causticType == PHOTON || causticType == BOTH)
	{
		progressBar_t *pb;
		if(intpb) pb = intpb;
		else pb = new ConsoleProgressBar_t(80);
		success = createCausticMap(*scene, lights, causticMap, cDepth, nPhotons, pb, integratorName);
		if(!intpb) delete pb;
	}

	if(causticType == BOTH || causticType == PATH) traceCaustics = true;
	
	if(causticType == PATH)
	{
		if(!set.str().empty()) set << "+";
		set << "Caustics: Path";
	}
	else if(causticType == PHOTON)
	{
		if(!set.str().empty()) set << "+";
		set << "Caustics: Photon(" << nPhotons << ")";
	}
	else if(causticType == BOTH)
	{
		if(!set.str().empty()) set << "+";
		set << "Caustics: Path+Photon(" << nPhotons << ")";
	}
	
	settings = set.str();
	
	return success;
}

inline color_t pathIntegrator_t::estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo,
												   const std::vector<light_t *>  &lights, int d1, int n)const
{
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

	s1 = scrHalton(d1, n) * nLights;
	int lnum = std::min((int)(s1), nLightsI-1);
	const light_t *light = lights[lnum];
	s1 = s1 - (float)lnum;

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
		// ...get sample val...
		lSample_t ls;
		ls.s1 = s1;
		ls.s2 = scrHalton(d1+1, n);

		bool canIntersect=light->canIntersect();
		
		if( light->illumSample (sp, ls, lightRay) )
		{
			// ...shadowed...
			lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
			shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
			if(!shadowed && ls.pdf > 1e-6f)
			{
				if(trShad) ls.col *= scol;

				color_t surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
				
				if( canIntersect )
				{
					float mPdf = oneMat->pdf(state, sp, wo, lightRay.dir, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
					float l2 = ls.pdf * ls.pdf;
					float m2 = mPdf * mPdf + 0.1f;
					float w = l2 / (l2 + m2);
					//test! limit lightPdf...
					if(ls.pdf < 1e-5f) ls.pdf = 1e-5f;
					col = surfCol * ls.col * std::fabs(sp.N*lightRay.dir) * w / ls.pdf;
				}
				else
				{
					//test! limit lightPdf...
					if(ls.pdf < 1e-5f) ls.pdf = 1e-5f;
					col = surfCol * ls.col * std::fabs(sp.N*lightRay.dir) / ls.pdf;
				}
			}
		}
		if(canIntersect) // sample from BSDF to complete MIS
		{
			ray_t bRay;
			bRay.tmin = MIN_RAYDIST;
			bRay.from = sp.P;

			sample_t s(ls.s1, ls.s2, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
			color_t surfCol = oneMat->sample(state, sp, wo, bRay.dir, s);

			if( s.pdf>1e-6f && light->intersect(bRay, bRay.tmax, lcol, lightPdf) )
			{
				shadowed = (trShad) ? scene->isShadowed(state, bRay, sDepth, scol) : scene->isShadowed(state, bRay);
				if(!shadowed)
				{
					if(trShad) lcol *= scol;
					float lPdf = 1.f/lightPdf;
					float l2 = lPdf * lPdf;
					float m2 = s.pdf * s.pdf + 0.1f;
					float w = m2 / (l2 + m2);
					float cos2 = std::fabs(sp.N*bRay.dir);
					col += surfCol * lcol * cos2 * w / s.pdf;
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
		const volumeHandler_t *vol;
		color_t vcol(0.f);

		// contribution of light emitting surfaces		
		if(bsdfs & BSDF_EMIT) col += material->emit(state, sp, wo);
		
		if(bsdfs & BSDF_DIFFUSE) col += estimateDirect_PH(state, sp, lights, scene, wo, trShad, sDepth);
		
		if((bsdfs & BSDF_DIFFUSE) && (causticType == PHOTON || causticType == BOTH)) col += estimatePhotons(state, sp, causticMap, wo, nSearch, cRadius);
				
		// path tracing:
		// the first path segment is "unrolled" from the loop because for the spot the camera hit
		// we do things slightly differently (e.g. may not sample specular, need not to init BSDF anymore,
		// have more efficient ways to compute samples...)
		
		bool was_chromatic = state.chromatic;
		BSDF_t path_flags = no_recursive ? BSDF_ALL : (BSDF_DIFFUSE);
		
		if(bsdfs & path_flags)
		{
			color_t pathCol(0.0), wl_col;
			path_flags |= (BSDF_DIFFUSE | BSDF_REFLECT | BSDF_TRANSMIT);
			int nSamples = std::max(1, nPaths/state.rayDivision);
			for(int i=0; i<nSamples; ++i)
			{
				void *first_udat = state.userdata;
				unsigned char userdata[USER_DATA_SIZE+7];
				void *n_udat = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
				unsigned int offs = nPaths * state.pixelSample + state.samplingOffs + i; // some redunancy here...
				color_t throughput( 1.0 );
				color_t lcol, scol;
				surfacePoint_t sp1=sp, sp2;
				surfacePoint_t *hit=&sp1, *hit2=&sp2;
				vector3d_t pwo = wo;
				ray_t pRay;

				state.chromatic = was_chromatic;
				if(was_chromatic) state.wavelength = RI_S(offs);
				//this mat already is initialized, just sample (diffuse...non-specular?)
				float s1 = RI_vdC(offs);
				float s2 = scrHalton(2, offs);
				if(state.rayDivision > 1)
				{
					s1 = addMod1(s1, state.dc1);
					s2 = addMod1(s2, state.dc2);
				}
				// do proper sampling now...
				sample_t s(s1, s2, path_flags);
				scol = material->sample(state, sp, pwo, pRay.dir, s);
				
				if(s.pdf <= 1e-6f) continue;
				
				scol *= (std::fabs(pRay.dir*sp.N)/s.pdf);
				throughput = scol;
				state.includeLights = false;

				pRay.tmin = MIN_RAYDIST;
				pRay.tmax = -1.0;
				pRay.from = sp.P;
				
				if(!scene->intersect(pRay, *hit)) continue; //hit background

				state.userdata = n_udat;
				const material_t *p_mat = hit->material;
				BSDF_t matBSDFs;
				p_mat->initBSDF(state, *hit, matBSDFs);
				pwo = -pRay.dir;
				lcol = estimateOneDirect(state, *hit, pwo, lights, 3, offs);
				if(matBSDFs & BSDF_EMIT) lcol += p_mat->emit(state, *hit, pwo);

				pathCol += lcol*throughput;
				
				bool caustic = false;
				
				for(int depth=1; depth<bounces; ++depth)
				{
					int d4 = 4*depth;
					s.s1 = scrHalton(d4+3, offs); //ourRandom();//
					s.s2 = scrHalton(d4+4, offs); //ourRandom();//

					if(state.rayDivision > 1)
					{
						s1 = addMod1(s1, state.dc1);
						s2 = addMod1(s2, state.dc2);
					}

					s.flags = BSDF_ALL;
					
					scol = p_mat->sample(state, *hit, pwo, pRay.dir, s);
					if(s.pdf <= 1.0e-6f) break;

					scol *= (std::fabs(pRay.dir*hit->N)/s.pdf);
					
					if(scol.isBlack()) break;
					
					throughput *= scol;
					caustic = traceCaustics && (s.sampledFlags & (BSDF_SPECULAR | BSDF_GLOSSY | BSDF_FILTER));
					state.includeLights = caustic;

					pRay.tmin = MIN_RAYDIST;
					pRay.tmax = -1.0;
					pRay.from = hit->P;

					if(!scene->intersect(pRay, *hit2)) //hit background
					{
						if((caustic && hasBGLight))
						{
							pathCol += throughput * (*background)(pRay, state);
						}
						break;
					}
					
					std::swap(hit, hit2);
					p_mat = hit->material;
					p_mat->initBSDF(state, *hit, matBSDFs);
					pwo = -pRay.dir;

					
					if(matBSDFs & BSDF_DIFFUSE) lcol = estimateOneDirect(state, *hit, pwo, lights, d4+3, offs);
					else lcol = color_t(0.f);

					if((matBSDFs & BSDF_VOLUMETRIC) && (vol=p_mat->getVolumeHandler(hit->N * pwo < 0)))
					{
						if(vol->transmittance(state, pRay, vcol)) throughput *= vcol;
					}
					
					if (matBSDFs & BSDF_EMIT && caustic) lcol += p_mat->emit(state, *hit, pwo);
					
					pathCol += lcol*throughput;
				}
				state.userdata = first_udat;
				
			}
			col += pathCol / nSamples;
		}
		//reset chromatic state:
		state.chromatic = was_chromatic;

		++state.raylevel;

		if(!no_recursive && state.raylevel <= rDepth)
		{
			// dispersive effects with recursive raytracing:
			if( (bsdfs & BSDF_DISPERSIVE) && state.chromatic)
			{
				state.includeLights = true; //debatable...
				int dsam = 8;
				int oldDivision = state.rayDivision;
				int oldOffset = state.rayOffset;
				float old_dc1 = state.dc1, old_dc2 = state.dc2;
				if(state.rayDivision > 1) dsam = std::max(1, dsam/oldDivision);
				state.rayDivision *= dsam;
				int branch = state.rayDivision*oldOffset;
				float d_1 = 1.f/(float)dsam;
				float ss1 = RI_S(state.pixelSample + state.samplingOffs);
				color_t dcol(0.f);
				vector3d_t wi;
				diffRay_t refRay;
				for(int ns=0; ns<dsam; ++ns)
				{
					state.wavelength = (ns + ss1)*d_1;
					state.dc1 = scrHalton(2*state.raylevel+1, branch + state.samplingOffs);
					state.dc2 = scrHalton(2*state.raylevel+2, branch + state.samplingOffs);
					if(oldDivision > 1)	state.wavelength = addMod1(state.wavelength, old_dc1);
					state.rayOffset = branch;
					++branch;
					sample_t s(0.5f, 0.5f, BSDF_REFLECT|BSDF_TRANSMIT|BSDF_DISPERSIVE);
					color_t mcol = material->sample(state, sp, wo, wi, s);
					if(s.pdf > 1e-6f && (s.sampledFlags & BSDF_DISPERSIVE))
					{
						mcol *= std::fabs(wi*sp.N)/s.pdf;
						state.chromatic = false;
						color_t wl_col;
						wl2rgb(state.wavelength, wl_col);
						refRay = diffRay_t(sp.P, wi, MIN_RAYDIST);
						dcol += (color_t)integrate(state, refRay) * mcol * wl_col;
						state.chromatic = true;
					}
					if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.N * refRay.dir < 0)))
					{
						if(vol->transmittance(state, refRay, vcol)) dcol *= vcol;
					}
				}
				col += dcol * d_1;
				state.rayDivision = oldDivision;
				state.rayOffset = oldOffset;
				state.dc1 = old_dc1; state.dc2 = old_dc2;
			}
			// glossy reflection with recursive raytracing:
			if( bsdfs & BSDF_GLOSSY )
			{
				state.includeLights = false;
				int gsam = 8;
				int oldDivision = state.rayDivision;
				int oldOffset = state.rayOffset;
				float old_dc1 = state.dc1, old_dc2 = state.dc2;
				if(state.rayDivision > 1) gsam = std::max(1, gsam/oldDivision);
				state.rayDivision *= gsam;
				int branch = state.rayDivision*oldOffset;
				unsigned int offs = gsam * state.pixelSample + state.samplingOffs;
				float d_1 = 1.f/(float)gsam;
				color_t gcol(0.f);
				vector3d_t wi;
				diffRay_t refRay;
				for(int ns=0; ns<gsam; ++ns)
				{
					state.dc1 = scrHalton(2*state.raylevel+1, branch + state.samplingOffs);
					state.dc2 = scrHalton(2*state.raylevel+2, branch + state.samplingOffs);
					state.rayOffset = branch;
					++offs;
					++branch;
					
					float s1 = RI_vdC(offs);
					float s2 = scrHalton(2, offs);
					
					if(oldDivision > 1) // create generalized halton sequence
					{
						s1 = addMod1(s1, old_dc1);
						s2 = addMod1(s2, old_dc2);
					}
					
					sample_t s(s1, s2, BSDF_REFLECT|BSDF_TRANSMIT|BSDF_GLOSSY);
					color_t mcol = material->sample(state, sp, wo, wi, s);
					if(s.pdf > 1.0e-6f && (s.sampledFlags & BSDF_GLOSSY))
					{
						mcol *= std::fabs(wi*sp.N)/s.pdf;
						refRay = diffRay_t(sp.P, wi, MIN_RAYDIST);
						gcol += (color_t)integrate(state, refRay) * mcol;
					}
					if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.N * refRay.dir < 0)))
					{
						if(vol->transmittance(state, refRay, vcol)) gcol *= vcol;
					}
				}

				col += gcol * d_1;
				//restore renderstate
				state.rayDivision = oldDivision;
				state.rayOffset = oldOffset;
				state.dc1 = old_dc1; state.dc2 = old_dc2;
			}
			//...perfect specular reflection/refraction with recursive raytracing...
			if(bsdfs & (BSDF_SPECULAR | BSDF_FILTER))
			{
				state.includeLights = true;
				bool reflect=false, refract=false;
				vector3d_t dir[2];
				color_t rcol[2];
				
				material->getSpecular(state, sp, wo, reflect, refract, &dir[0], &rcol[0]);
				
				if(reflect)
				{
					diffRay_t refRay(sp.P, dir[0], MIN_RAYDIST);
					color_t integ  = color_t(integrate(state, refRay) );
					if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.Ng * refRay.dir < 0)))
					{
						vol->transmittance(state, refRay, vcol);
						integ *= vcol;
					}
					col += integ * rcol[0];
				}
				if(refract)
				{
					diffRay_t refRay(sp.P, dir[1], MIN_RAYDIST);
					colorA_t integ = integrate(state, refRay);
					if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.Ng * refRay.dir < 0)))
					{
						vol->transmittance(state, refRay, vcol);
						integ *= vcol;
					}
					col += color_t(integ) * rcol[1];
					alpha = integ.A;
				}
			}
		}
		--state.raylevel;

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
	const std::string *cMethod=0;
	
	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transpShad);
	params.getParam("shadowDepth", shadowDepth);
	params.getParam("path_samples", path_samples);
	params.getParam("bounces", bounces);
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
	inte->invNPaths = 1.f / (float)path_samples;
	inte->bounces = bounces;
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
