#pragma once
/****************************************************************************
 *      integrator_volume.h: the interface definition for light volume integrators
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
 *      Copyright (C) 2010  Rodrigo Placencia (DarkTide)
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

#ifndef LIBYAFARAY_INTEGRATOR_VOLUME_H
#define LIBYAFARAY_INTEGRATOR_VOLUME_H

#include "integrator/integrator.h"
#include <map>
#include <memory>

namespace yafaray {

class RenderControl;
class Ray;
class RandomGenerator;
class Rgb;
class ParamMap;
class VolumeRegion;

class VolumeIntegrator: public Integrator
{
	public:
		static VolumeIntegrator *factory(Logger &logger, RenderControl &render_control, const Scene &scene, const std::string &name, const ParamMap &params);
		virtual Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const = 0;
		virtual Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const = 0;
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray) const;
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;

	protected:
		explicit VolumeIntegrator(Logger &logger) : Integrator(logger) { }
		Type getType() const override { return Volume; }
		const std::map<std::string, std::unique_ptr<VolumeRegion>> *volume_regions_ = nullptr;
};

} //namespace yafaray

#endif //LIBYAFARAY_INTEGRATOR_VOLUME_H
