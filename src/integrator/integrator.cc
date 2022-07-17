/****************************************************************************
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

#include "integrator/integrator.h"
#include "scene/scene.h"

namespace yafaray {

bool Integrator::preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene)
{
	accelerator_ = scene.getAccelerator();
	if(!accelerator_) return false;
	shadow_bias_ = scene.getShadowBias();
	ray_min_dist_ = scene.getRayMinDist();
	return true;
}

} //namespace yafaray
