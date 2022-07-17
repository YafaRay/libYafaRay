#pragma once
/****************************************************************************
 *      integrator.h: the interface definition for light integrators
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

#ifndef YAFARAY_INTEGRATOR_H
#define YAFARAY_INTEGRATOR_H

#include <string>
#include <memory>

namespace yafaray {

class Logger;
class Scene;
class ImageFilm;
class RenderView;
class Accelerator;
class FastRandom;

class Integrator
{
	public:
		explicit Integrator(Logger &logger) : logger_(logger) { }
		virtual ~Integrator() = default;
		//! this MUST be called before any other member function!
		virtual bool render(FastRandom &fast_random, unsigned int object_index_highest, unsigned int material_index_highest) { return false; }
		/*! do whatever is required to render the image, if suitable for integrating whole image */
		/*! gets called before the scene rendering (i.e. before first call to integrate)
			\return false when preprocessing could not be done properly, true otherwise */
		virtual bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene);
		/*! allow the integrator to do some cleanup when an image is done
		(possibly also important for multiframe rendering in the future)	*/
		virtual void cleanup() { render_info_.clear(); aa_noise_info_.clear(); }
		virtual std::string getShortName() const = 0;
		virtual std::string getName() const = 0;
		enum Type { Surface, Volume };
		virtual Type getType() const = 0;
		std::string getRenderInfo() const { return render_info_; }
		std::string getAaNoiseInfo() const { return aa_noise_info_; }

	protected:
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool time_forced_ = false;
		float time_forced_value_ = 0.f;
		std::string render_info_;
		std::string aa_noise_info_;
		const Accelerator *accelerator_;
		Logger &logger_;
};

} //namespace yafaray

#endif // YAFARAY_INTEGRATOR_H
