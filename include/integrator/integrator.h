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

#include "constants.h"
#include <string>

BEGIN_YAFARAY

/*!	Integrate the incoming light scattered by the surfaces
	hit by a given ray
*/

class ParamMap;
class RenderEnvironment;
class Scene;
class ProgressBar;
class ImageFilm;
struct RenderArea;
class Rgba;
struct RenderState;
class Ray;
class DiffRay;
class ColorPasses;


class Integrator
{
	public:
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);

		Integrator() = default;
		virtual ~Integrator() = default;
		//! this MUST be called before any other member function!
		virtual bool render(int num_view, ImageFilm *image_film) { return false; }
		void setScene(Scene *s) { scene_ = s; }
		/*! do whatever is required to render the image, if suitable for integrating whole image */
		void setProgressBar(ProgressBar *pb) { intpb_ = pb; }
		std::string getShortName() const { return integrator_short_name_; }
		std::string getName() const { return integrator_name_; }
		enum Type { Surface, Volume };
		Type integratorType() { return type_; }

	protected:
		Type type_;
		Scene *scene_ = nullptr;
		ProgressBar *intpb_ = nullptr;
		std::string integrator_name_;
		std::string integrator_short_name_;
};

class SurfaceIntegrator: public Integrator
{
	public:
		/*! gets called before the scene rendering (i.e. before first call to integrate)
			\return false when preprocessing could not be done properly, true otherwise */
		virtual bool preprocess() { return true; };
		/*! allow the integrator to do some cleanup when an image is done
		(possibly also important for multiframe rendering in the future)	*/
		virtual void cleanup() {}
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &col_passes, int additional_depth = 0) const = 0;
	protected:
		SurfaceIntegrator() {} //don't use...
};

class VolumeIntegrator: public Integrator
{
	public:
		VolumeIntegrator() {}
		virtual Rgba transmittance(RenderState &state, Ray &ray) const = 0;
		virtual Rgba integrate(RenderState &state, Ray &ray, ColorPasses &col_passes, int additional_depth = 0) const = 0;
		virtual bool preprocess() { return true; }

	protected:
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_H
