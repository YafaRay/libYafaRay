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
#include "common/aa_noise_params.h"
#include "common/mask_edge_toon_params.h"
#include "geometry/bound.h"
#include "color/color.h"
#include <string>
#include <memory>
#include <map>

BEGIN_YAFARAY

/*!	Integrate the incoming light scattered by the surfaces
	hit by a given ray
*/

class ParamMap;
class Scene;
class ProgressBar;
class Ray;
class ColorLayers;
class ImageFilm;
class RenderView;
class Camera;
class RandomGenerator;
struct RayDivision;
struct PixelSamplingData;
class VolumeRegion;
class Accelerator;
class Layers;
class VolumeIntegrator;
class Background;

struct ColorLayerAccum
{
	explicit ColorLayerAccum(Rgba *color) : color_(color) { }
	Rgba *color_;
	Rgba accum_{0.f};
};

class Integrator
{
	public:
		static Integrator *factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &params);

		explicit Integrator(Logger &logger) : logger_(logger) { }
		virtual ~Integrator() = default;
		//! this MUST be called before any other member function!
		virtual bool render() { return false; }
		/*! do whatever is required to render the image, if suitable for integrating whole image */
		void setProgressBar(std::shared_ptr<ProgressBar> pb) { intpb_ = std::move(pb); }
		/*! gets called before the scene rendering (i.e. before first call to integrate)
			\return false when preprocessing could not be done properly, true otherwise */
		virtual bool preprocess(ImageFilm *image_film, const RenderView *render_view, const Scene &scene);
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
		std::string render_info_;
		std::string aa_noise_info_;
		const Accelerator *accelerator_;
		std::shared_ptr<ProgressBar> intpb_;
		Logger &logger_;
};

class SurfaceIntegrator: public Integrator
{
	public:
		virtual std::pair<Rgb, float> integrate(Ray &ray, RandomGenerator &random_generator, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data) const = 0; 	//!< chromatic_enabled indicates wether the full spectrum is calculated (true) or only a single wavelength (false). wavelength is the (normalized) wavelength being used when chromatic is false. The range is defined going from 400nm (0.0) to 700nm (1.0), although the widest range humans can perceive is ofteb given 380-780nm.
		bool preprocess(ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;

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
		Bound scene_bound_;
		const RenderView *render_view_ = nullptr;
		const VolumeIntegrator *vol_integrator_ = nullptr;
		const Camera *camera_ = nullptr;
		const Background *background_ = nullptr;
		Timer *timer_ = nullptr;
		ImageFilm *image_film_ = nullptr;
		const Layers *layers_ = nullptr;
};

class VolumeIntegrator: public Integrator
{
	public:
		virtual Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const = 0;
		virtual Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const = 0;
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray) const { return integrate(random_generator, ray, 0); };
		bool preprocess(ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;

	protected:
		explicit VolumeIntegrator(Logger &logger) : Integrator(logger) { }
		Type getType() const override { return Volume; }
		const std::map<std::string, std::unique_ptr<VolumeRegion>> *volume_regions_ = nullptr;
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_H
