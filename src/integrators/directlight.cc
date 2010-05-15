/****************************************************************************
 * 			directlight.cc: an integrator for direct lighting only
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
#include <core_api/environment.h>
#include <core_api/material.h>
#include <yafraycore/tiledintegrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <integrators/integr_utils.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/spectrum.h>
#include <sstream>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT directLighting_t: public tiledIntegrator_t
{
	public:
		directLighting_t(bool transpShad=false, int shadowDepth=4, int rayDepth=6);
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		color_t sampleAO(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const;
		background_t *background;
		bool trShad, caustics, do_AO;
		int sDepth, rDepth, cDepth;
		int nPhotons, nSearch, AO_samples;
		PFLOAT cRadius, AO_dist;
		color_t AO_col;
		std::vector<light_t*> lights;
		photonMap_t causticMap;
};

directLighting_t::directLighting_t(bool transpShad, int shadowDepth, int rayDepth):
	trShad(transpShad), caustics(false), sDepth(shadowDepth), rDepth(rayDepth)
{
	type = SURFACE;
	cRadius = 0.25;
	cDepth = 10;
	nPhotons = 100000;
	nSearch = 100;
	intpb = 0;
	integratorName = "DirectLight";
	integratorShortName = "DL";
}

bool directLighting_t::preprocess()
{
	bool success = true;
	std::stringstream set;
	settings = "";
	
	if(trShad)
	{
		set << "ShadowDepth: [" << sDepth << "]";
	}
	if(!set.str().empty()) set << "+";
	set << "RayDepth: [" << rDepth << "]";
	
	background = scene->getBackground();
	lights.clear();
	for(unsigned int i=0;i<scene->lights.size();++i)
	{
		lights.push_back(scene->lights[i]);
	}
	if(background)
	{
		light_t *bgl = background->getLight();
		if(bgl)
		{
			lights.push_back(bgl);
			if(!set.str().empty()) set << "+";
			set << "IBL";
		}
	}
	if(caustics)
	{
		progressBar_t *pb;
		if(intpb) pb = intpb;
		else pb = new ConsoleProgressBar_t(80);
		success = createCausticMap(*scene, lights, causticMap, cDepth, nPhotons, pb, integratorName);
		if(!intpb) delete pb;
		if(!set.str().empty()) set << "+";
		set << "Caustics:" << nPhotons << " photons. ";
	}
	
	if(do_AO)
	{
		if(!set.str().empty()) set << "+";
		set << "AO";
	}
	
	settings = set.str();

	return success;
}

colorA_t directLighting_t::integrate(renderState_t &state, diffRay_t &ray) const
{
	color_t col(0.0);
	CFLOAT alpha=0.0;
	surfacePoint_t sp;
	void *o_udat = state.userdata;
	bool oldIncludeLights = state.includeLights;

	//shoot ray into scene
	if(scene->intersect(ray, sp))
	{
		// if camera ray:
		if(state.raylevel == 0)
		{
			state.includeLights = true;
		}
		
		//Halton hal3(3);
		unsigned char userdata[USER_DATA_SIZE];//+7];
		//userdata[0] = 0;
		state.userdata = (void *) userdata;//(void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
		BSDF_t bsdfs;

		const material_t *material = sp.material;
		material->initBSDF(state, sp, bsdfs);
		vector3d_t wo = -ray.dir;
		
		if(bsdfs & BSDF_EMIT) col += material->emit(state, sp, wo);		
		if(bsdfs & BSDF_DIFFUSE) col += estimateDirect_PH(state, sp, lights, scene, wo, trShad, sDepth);
		if(bsdfs & BSDF_DIFFUSE) col += estimatePhotons(state, sp, causticMap, wo, nSearch, cRadius);
		if((bsdfs & BSDF_DIFFUSE) && do_AO) col += sampleAO(state, sp, wo);
		
		recursiveRaytrace(state, ray, (int)rDepth, bsdfs, sp, wo, col, alpha);
		
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
	state.userdata = o_udat;
	state.includeLights = oldIncludeLights;
	return colorA_t(col, alpha);
}

color_t directLighting_t::sampleAO(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const
{
	color_t col(0.f);
	bool shadowed;
	const material_t *material = sp.material;
	Halton hal3(3);
	ray_t lightRay;
	lightRay.from = sp.P;
	
	int n = AO_samples;
	if(state.rayDivision > 1) n = std::max(1, n/state.rayDivision);
	unsigned int offs = n * state.pixelSample + state.samplingOffs;
	hal3.setStart(offs-1);
	color_t surfCol(0.f), scol(0.f);
	for(int i=0; i<n; ++i)
	{
		float s1 = RI_vdC(offs+i);
		float s2 = hal3.getNext();
		if(state.rayDivision > 1)
		{
			s1 = addMod1(s1, state.dc1);
			s2 = addMod1(s2, state.dc2);
		}
		lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is still bad...
		lightRay.tmax = AO_dist;
		
		sample_t s(s1, s2, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_REFLECT );
		surfCol = material->sample(state, sp, wo, lightRay.dir, s);
		if(s.pdf > 1e-6f)
		{
			shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
			if(!shadowed)
			{
				CFLOAT cos = std::fabs(sp.N*lightRay.dir);
				if(trShad) col += AO_col * scol * surfCol * cos / s.pdf;
				else col += AO_col * surfCol * cos / s.pdf;
			}
		}
	}
	return col / (float)n;
}

integrator_t* directLighting_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	bool transpShad=false;
	bool caustics=false;
	bool do_AO=false;
	int shadowDepth=5;
	int raydepth=5, cDepth=10;
	int search=100, photons=500000;
	int AO_samples = 32;
	double cRad = 0.25;
	double AO_dist = 1.0;
	color_t AO_col(1.f);
	
	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transpShad);
	params.getParam("shadowDepth", shadowDepth);
	params.getParam("caustics", caustics);
	params.getParam("photons", photons);
	params.getParam("caustic_mix", search);
	params.getParam("caustic_depth", cDepth);
	params.getParam("caustic_radius", cRad);
	params.getParam("do_AO", do_AO);
	params.getParam("AO_samples", AO_samples);
	params.getParam("AO_distance", AO_dist);
	params.getParam("AO_color", AO_col);
	
	directLighting_t *inte = new directLighting_t(transpShad, shadowDepth, raydepth);
	// caustic settings
	inte->caustics = caustics;
	inte->nPhotons = photons;
	inte->nSearch = search;
	inte->cDepth = cDepth;
	inte->cRadius = cRad;
	// AO settings
	inte->do_AO = do_AO;
	inte->AO_samples = AO_samples;
	inte->AO_dist = (PFLOAT)AO_dist;
	inte->AO_col = AO_col;
	return inte;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("directlighting",directLighting_t::factory);
	}

}

__END_YAFRAY
