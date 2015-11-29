/****************************************************************************
 *		directlight.cc: an integrator for direct lighting only
 *		This is part of the yafaray package
 *		Copyright (C) 2006  Mathias Wein (Lynx)
 *		Copyright (C) 2009  Rodrigo Placencia (DarkTide)
 *
 *		This library is free software; you can redistribute it and/or
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
#include <core_api/mcintegrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <sstream>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT directLighting_t: public mcIntegrator_t
{
	public:
		directLighting_t(bool transpShad=false, int shadowDepth=4, int rayDepth=6);
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray, colorPasses_t &colorPasses) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
};

directLighting_t::directLighting_t(bool transpShad, int shadowDepth, int rayDepth)
{
	type = SURFACE;
	causRadius = 0.25;
	causDepth = 10;
	nCausPhotons = 100000;
	nCausSearch = 100;
	trShad = transpShad;
	usePhotonCaustics = false;
	sDepth = shadowDepth;
	rDepth = rayDepth;
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
	lights = scene->lights;

	if(usePhotonCaustics)
	{
		success = createCausticMap();
		if(!set.str().empty()) set << "+";
		set << "Caustics:" << nCausPhotons << " photons. ";
	}

	if(useAmbientOcclusion)
	{
		if(!set.str().empty()) set << "+";
		set << "AO";
	}

	settings = set.str();

	return success;
}

colorA_t directLighting_t::integrate(renderState_t &state, diffRay_t &ray, colorPasses_t &colorPasses) const
{
	color_t col(0.0);
	float alpha;
	surfacePoint_t sp;
	void *o_udat = state.userdata;
	bool oldIncludeLights = state.includeLights;

	if(transpBackground) alpha=0.0;
	else alpha=1.0;

	// Shoot ray into scene

	if(scene->intersect(ray, sp)) // If it hits
	{
		unsigned char userdata[USER_DATA_SIZE];
		const material_t *material = sp.material;
		BSDF_t bsdfs;

		state.userdata = (void *) userdata;
		vector3d_t wo = -ray.dir;
		if(state.raylevel == 0) state.includeLights = true;

		material->initBSDF(state, sp, bsdfs);

		if((bsdfs & BSDF_EMIT) && isLightGroupEnabledByFilter(material->getLightGroup())) 
		{
			col += colorPasses.probe_set(PASS_INT_EMIT, material->emit(state, sp, wo), state.raylevel == 0);
		}

		if(bsdfs & BSDF_DIFFUSE)
		{
			col += estimateAllDirectLight(state, sp, wo, colorPasses);
			
			if(usePhotonCaustics)
			{
				if(AA_clamp_indirect>0)
				{
					color_t tmpCol = estimateCausticPhotons(state, sp, wo);
					tmpCol.clampProportionalRGB(AA_clamp_indirect);
					col += colorPasses.probe_add(PASS_INT_INDIRECT, tmpCol, state.raylevel == 0);
				}
				else col += colorPasses.probe_add(PASS_INT_INDIRECT, estimateCausticPhotons(state, sp, wo), state.raylevel == 0);
			}

			if(useAmbientOcclusion) col += sampleAmbientOcclusion(state, sp, wo);
		}

		recursiveRaytrace(state, ray, bsdfs, sp, wo, col, alpha, colorPasses);

		if(colorPasses.size() > 1 && state.raylevel == 0)
		{
			generateCommonRenderPasses(colorPasses, state, sp);
			
			if(colorPasses.enabled(PASS_INT_AO))
			{
				colorPasses(PASS_INT_AO) = sampleAmbientOcclusion(state, sp, wo);
			}

			if(colorPasses.enabled(PASS_INT_AO_CLAY))
			{
				colorPasses(PASS_INT_AO_CLAY) = sampleAmbientOcclusionPass(state, sp, wo);
			}
		}
		
		if(transpRefractedBackground)
		{
			float m_alpha = material->getAlpha(state, sp, wo);
			alpha = m_alpha + (1.f - m_alpha) * alpha;
		}
		else alpha = 1.0;
	}
	else // Nothing hit, return background if any
	{
		if(background && !transpRefractedBackground)
		{
			col += colorPasses.probe_set(PASS_INT_ENV, (*background)(ray, state, false), state.raylevel == 0);
		}
	}

	state.userdata = o_udat;
	state.includeLights = oldIncludeLights;

	color_t colVolTransmittance = scene->volIntegrator->transmittance(state, ray);
	color_t colVolIntegration = scene->volIntegrator->integrate(state, ray, colorPasses);

	if(transpBackground) alpha = std::max(alpha, 1.f-colVolTransmittance.R);
	
	colorPasses.probe_set(PASS_INT_VOLUME_TRANSMITTANCE, colVolTransmittance);
	colorPasses.probe_set(PASS_INT_VOLUME_INTEGRATION, colVolIntegration);

	col = (col * colVolTransmittance) + colVolIntegration;
	
	return colorA_t(col, alpha);
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
	bool bg_transp = false;
	bool bg_transp_refract = false;

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
	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);

	directLighting_t *inte = new directLighting_t(transpShad, shadowDepth, raydepth);
	// caustic settings
	inte->usePhotonCaustics = caustics;
	inte->nCausPhotons = photons;
	inte->nCausSearch = search;
	inte->causDepth = cDepth;
	inte->causRadius = cRad;
	// AO settings
	inte->useAmbientOcclusion = do_AO;
	inte->aoSamples = AO_samples;
	inte->aoDist = AO_dist;
	inte->aoCol = AO_col;
	// Background settings
	inte->transpBackground = bg_transp;
	inte->transpRefractedBackground = bg_transp_refract;
	
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
