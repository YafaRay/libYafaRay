#pragma once
/****************************************************************************
 *      integrator_surface.h: the interface definition for light surface integrators
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

#ifndef LIBYAFARAY_INTEGRATOR_SURFACE_H
#define LIBYAFARAY_INTEGRATOR_SURFACE_H

#include "integrator/integrator.h"
#include "geometry/bound.h"
#include "color/color.h"
#include "common/aa_noise_params.h"
#include "common/mask_edge_toon_params.h"
#include <vector>

namespace yafaray {

class RenderControl;
class RandomGenerator;
class Ray;
class ParamMap;
class ColorLayers;
class VolumeIntegrator;
class Camera;
class Background;
class Timer;
class Layers;
struct PixelSamplingData;
struct RayDivision;

class SurfaceIntegrator: public Integrator
{
	public:
		static SurfaceIntegrator *factory(Logger &logger, RenderControl &render_control, const Scene &scene, const std::string &name, const ParamMap &params);
		virtual std::pair<Rgb, float> integrate(Ray &ray, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest) const = 0; 	//!< chromatic_enabled indicates wether the full spectrum is calculated (true) or only a single wavelength (false). wavelength is the (normalized) wavelength being used when chromatic is false. The range is defined going from 400nm (0.0) to 700nm (1.0), although the widest range humans can perceive is ofteb given 380-780nm.
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;

	protected:
		SurfaceIntegrator(RenderControl &render_control, Logger &logger) : Integrator(logger), render_control_(render_control) { }
		Type getType() const override { return Surface; }

	protected:
		RenderControl &render_control_;
		int num_threads_ = 1;
		int num_threads_photons_ = 1;
		bool shadow_bias_auto_ = true;  //enable automatic shadow bias calculation
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation
		AaNoiseParams aa_noise_params_;
		EdgeToonParams edge_toon_params_;
		MaskParams mask_params_;
		Bound<float> scene_bound_;
		const RenderView *render_view_ = nullptr;
		const VolumeIntegrator *vol_integrator_ = nullptr;
		const Camera *camera_ = nullptr;
		const Background *background_ = nullptr;
		Timer *timer_ = nullptr;
		ImageFilm *image_film_ = nullptr;
		const Layers *layers_ = nullptr;
};

struct ColorLayerAccum
{
	explicit ColorLayerAccum(Rgba *color) : color_(color) { }
	Rgba *color_;
	Rgba accum_{0.f};
};

} //namespace yafaray

#endif //LIBYAFARAY_INTEGRATOR_SURFACE_H
