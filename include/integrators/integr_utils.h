/****************************************************************************
 * 			integr_utils.h: API for light integrator utilities
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

#ifndef Y_INTEGR_UTILS_H
#define Y_INTEGR_UTILS_H

#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/monitor.h>
#include <string>

__BEGIN_YAFRAY

class photonMap_t;

//from common.cc
//color_t estimateDirect(renderState_t &state, const surfacePoint_t &sp, const std::vector<light_t *> &lights, scene_t *scene, const vector3d_t &wo, bool trShad, int sDepth);
color_t estimateDirect_PH(renderState_t &state, const surfacePoint_t &sp, const std::vector<light_t *> &lights, scene_t *scene, const vector3d_t &wo, bool trShad, int sDepth);
color_t estimatePhotons(renderState_t &state, const surfacePoint_t &sp, const photonMap_t &map, const vector3d_t &wo, int nSearch, PFLOAT radius);

bool createCausticMap(const scene_t &scene, const std::vector<light_t *> &all_lights, photonMap_t &cMap, int depth, int count, progressBar_t *pb = 0, std::string intName = "None");

//! r_photon2: Square distance of photon path; ir_gather2: inverse of square gather radius
inline float kernel(float r_photon2, float ir_gather2)
{
	float s = (1.f - r_photon2 * ir_gather2);
	return 3.f * ir_gather2 * M_1_PI * s * s;
}

inline float ckernel(float r_photon2, float r_gather2, float ir_gather2)
{
	float r_p=fSqrt(r_photon2), ir_g=fISqrt(r_gather2);
	return 3.f * (1.f - r_p*ir_g) * ir_gather2 * M_1_PI;
}


/*! estimate direct lighting by sampling ONE light, i.e. only use this when you know that you'll
	call this function sufficiently often in your integration!
	precondition: userdata must be set and material must be initialized (initBSDF(...))
*/
inline color_t estimateOneDirect(renderState_t &state, const scene_t *scene, const surfacePoint_t &sp, vector3d_t wo, const std::vector<light_t *>  &lights,
								 bool trShad, int sDepth, int d1, int n)
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
	if(d1 > 50)  s1 = (*state.prng)() * nLights;
	else s1 = scrHalton(d1, n) * nLights;
	int lnum = (int)(s1);
	if(lnum > nLightsI-1) lnum = nLightsI-1;
	const light_t *light = lights[lnum];
	s1 = s1 - (float)lnum; // scrHalton(d1, n); // 
//	BSDF_t oneBSDFs;
//	oneMat->initBSDF(state, sp, oneBSDFs);
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
		lSample_t ls;
		// ...get sample val...	
		ls.s1 = s1;
		if(d1 > 49)  ls.s2 = (*state.prng)();
		else ls.s2 = scrHalton(d1+1, n);
		
		if( light->illumSample (sp, ls, lightRay) )
		{
			// ...shadowed...
			if(ls.pdf < 1e-6f)
			{
				lightPdf = 1.f;
			}
			else
			{
				lightPdf = 1.f/ls.pdf;
			}
			lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
			shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
			if(!shadowed)
			{
				if(trShad) ls.col *= scol;
				color_t surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
				col = surfCol * ls.col * std::fabs(sp.N*lightRay.dir) * lightPdf;
			}
		}
	}
	return col*nLights;
}

__END_YAFRAY

#endif // Y_INTEGR_UTILS_H
