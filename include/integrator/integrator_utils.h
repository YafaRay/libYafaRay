#pragma once
/****************************************************************************
 *      integr_utils.h: API for light integrator utilities
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_INTEGRATOR_UTILS_H
#define YAFARAY_INTEGRATOR_UTILS_H

#include "constants.h"
#include <string>

BEGIN_YAFARAY

class PhotonMap;
class Rgb;
class RenderState;
class Light;
class Scene;
class Vec3;
class ProgressBar;
class SurfacePoint;

//from common.cc
Rgb estimateDirectPh__(RenderState &state, const SurfacePoint &sp, const std::vector<Light *> &lights, Scene *scene, const Vec3 &wo, bool tr_shad, int s_depth);
Rgb estimatePhotons__(RenderState &state, const SurfacePoint &sp, const PhotonMap &map, const Vec3 &wo, int n_search, float radius);

bool createCausticMap__(const Scene &scene, const std::vector<Light *> &all_lights, PhotonMap &c_map, int depth, int count, ProgressBar *pb = nullptr, std::string int_name = "None");

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_UTILS_H
