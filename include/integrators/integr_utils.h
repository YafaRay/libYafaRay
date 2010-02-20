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

__END_YAFRAY

#endif // Y_INTEGR_UTILS_H
