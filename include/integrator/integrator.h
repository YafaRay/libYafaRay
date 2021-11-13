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

#include "common/yafaray_common.h"
#include "common/logger.h"
#include <string>
#include <memory>

BEGIN_YAFARAY

/*!	Integrate the incoming light scattered by the surfaces
	hit by a given ray
*/

class ParamMap;
class Scene;
class ProgressBar;
class Rgba;
class Ray;
class ColorLayers;
class ImageFilm;
class RenderView;
class Camera;
class RandomGenerator;
struct RayDivision;
struct PixelSamplingData;

class Integrator
{
	public:
		static std::unique_ptr<Integrator> factory(Logger &logger, ParamMap &params, const Scene &scene);

		Integrator(Logger &logger) : logger_(logger) { }
		virtual ~Integrator() = default;
		//! this MUST be called before any other member function!
		virtual bool render(RenderControl &render_control, Timer &timer, const RenderView *render_view) { return false; }
		void setScene(const Scene *s) { scene_ = s; }
		/*! do whatever is required to render the image, if suitable for integrating whole image */
		void setProgressBar(std::shared_ptr<ProgressBar> pb) { intpb_ = std::move(pb); }
		/*! gets called before the scene rendering (i.e. before first call to integrate)
			\return false when preprocessing could not be done properly, true otherwise */
		virtual bool preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film) = 0;
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
		std::string render_info_;
		std::string aa_noise_info_;
		const Scene *scene_ = nullptr;
		std::shared_ptr<ProgressBar> intpb_;
		Logger &logger_;
};

class SurfaceIntegrator: public Integrator
{
	public:
		virtual Rgba integrate(int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator &random_generator, const PixelSamplingData &pixel_sampling_data, bool lights_geometry_material_emit) const = 0; 	//!< chromatic_enabled indicates wether the full spectrum is calculated (true) or only a single wavelength (false). wavelength is the (normalized) wavelength being used when chromatic is false. The range is defined going from 400nm (0.0) to 700nm (1.0), although the widest range humans can perceive is ofteb given 380-780nm.

	protected:
		SurfaceIntegrator(Logger &logger) : Integrator(logger) { }
		virtual Type getType() const override { return Surface; }

	protected:
		ImageFilm *image_film_ = nullptr;
};

class VolumeIntegrator: public Integrator
{
	public:
		virtual Rgba transmittance(RandomGenerator &random_generator, const Ray &ray) const = 0;
		virtual Rgba integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth = 0) const = 0;
		virtual bool preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film) override { return true; };
	protected:
		VolumeIntegrator(Logger &logger) : Integrator(logger) { }
		virtual Type getType() const override { return Volume; }
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_H
