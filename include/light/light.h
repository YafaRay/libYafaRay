#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_LIGHT_H
#define YAFARAY_LIGHT_H

#include "common/yafaray_common.h"
#include "common/flags.h"
#include "color/color.h"
#include <sstream>
#include <common/logger.h>
#include <memory>

namespace yafaray {

class ParamMap;
class Scene;
class SurfacePoint;
class Background;
class Ray;
class Scene;
class Vec3;
class Point3;
struct LSample;

class Light
{
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		struct Flags : public yafaray::Flags
		{
			Flags() = default;
			Flags(unsigned int flags) : yafaray::Flags(flags) { } // NOLINT(google-explicit-constructor)
			enum Enum : unsigned int { None = 0, DiracDir = 1, Singular = 1 << 1 };
		};
		explicit Light(Logger &logger) : logger_(logger) { }
		Light(Logger &logger, const Flags &flags): flags_(flags), logger_(logger) { }
		virtual ~Light() = default;
		//! allow for preprocessing when scene loading has finished
		virtual void init(Scene &scene) {}
		//! total energy emmitted during whole frame
		[[nodiscard]] virtual Rgb totalEnergy() const = 0;
		//! emit a photon
		[[nodiscard]] virtual std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const = 0;
		//! create a sample of light emission, similar to emitPhoton, just more suited for bidirectional methods
		/*! fill in s.dirPdf, s.areaPdf, s.col and s.flags, and s.sp if not nullptr */
		[[nodiscard]] virtual std::pair<Vec3, Rgb> emitSample(LSample &s, float time) const = 0;
		//! indicate whether the light has a dirac delta distribution or not
		[[nodiscard]] virtual bool diracLight() const = 0;
		//! illuminate a given surface point, generating sample s, fill in s.sp if not nullptr; Set ray to test visibility by integrator
		/*! fill in s.pdf, s.col and s.flags */
		[[nodiscard]] virtual std::pair<bool, Ray> illumSample(const Point3 &surface_p, LSample &s, float time) const = 0;
		//! illuminate a given surfance point; Set ray to test visibility by integrator. Only for dirac lights.
		/*!	return false only if no light is emitted towards sp, e.g. outside cone angle of spot light	*/
		[[nodiscard]] virtual std::tuple<bool, Ray, Rgb> illuminate(const Point3 &surface_p, float time) const = 0;
		//! indicate whether the light can intersect with a ray (by the sphereIntersect function)
		[[nodiscard]] virtual bool canIntersect() const { return false; }
		//! sphereIntersect the light source with a ray, giving back distance, energy and 1/PDF
		[[nodiscard]] virtual std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const { return {}; }
		//! get the pdf for sampling the incoming direction wi at surface point sp (illumSample!)
		/*! this method requires an intersection point with the light (sp_light). Otherwise, use sphereIntersect() */
		[[nodiscard]] virtual float illumPdf(const Point3 &surface_p, const Point3 &light_p, const Vec3 &light_ng) const { return 0.f; }
		//! get the pdf values for sampling point sp on the light and outgoing direction wo when emitting energy (emitSample, NOT illumSample)
		/*! sp should've been generated from illumSample or emitSample, and may only be complete enough to call light functions! */
		[[nodiscard]] virtual std::array<float, 3> emitPdf(const Vec3 &surface_n, const Vec3 &wo) const { return {0.f, 0.f, 0.f}; }
		//! (preferred) number of samples for direct lighting
		[[nodiscard]] virtual int nSamples() const { return 8; }
		//! This method must be called right after the factory is called on a background light or the light will fail
		virtual void setBackground(const Background *bg) { background_ = bg; }
		//! Enable/disable entire light source
		[[nodiscard]] bool lightEnabled() const { return light_enabled_;}
		[[nodiscard]] bool castShadows() const { return cast_shadows_; }
		//! checks if the light can shoot caustic photons (photonmap integrator)
		[[nodiscard]] bool shootsCausticP() const { return shoot_caustic_; }
		//! checks if the light can shoot diffuse photons (photonmap integrator)
		[[nodiscard]] bool shootsDiffuseP() const { return shoot_diffuse_; }
		//! checks if the light is a photon-only light (only shoots photons, not illuminating)
		[[nodiscard]] bool photonOnly() const { return photon_only_; }
		//! sets clampIntersect value to reduce noise at the expense of realism and inexact overall lighting
		void setClampIntersect(float clamp) { clamp_intersect_ = clamp; }
		[[nodiscard]] Flags getFlags() const { return flags_; }
		[[nodiscard]] std::string getName() const { return name_; }
		void setName(const std::string &name) { name_ = name; }

	protected:
		std::string name_;
		Flags flags_;
		const Background* background_ = nullptr;
		bool light_enabled_; //!< enable/disable light
		bool cast_shadows_; //!< enable/disable if the light should cast direct shadows
		bool shoot_caustic_; //!<enable/disable if the light can shoot caustic photons (photonmap integrator)
		bool shoot_diffuse_; //!<enable/disable if the light can shoot diffuse photons (photonmap integrator)
		bool photon_only_; //!<enable/disable if the light is a photon-only light (only shoots photons, not illuminating)
		float clamp_intersect_ = 0.f;	//!<trick to reduce light sampling noise at the expense of realism and inexact overall light. 0.f disables clamping
		Logger &logger_;
};

struct LSample
{
	float s_1_, s_2_; //<! 2d sample value for choosing a surface point on the light.
	float s_3_, s_4_; //<! 2d sample value for choosing an outgoing direction on the light (emitSample)
	float pdf_; //<! "standard" directional pdf from illuminated surface point for MC integration of direct lighting (illumSample)
	float dir_pdf_; //<! probability density for generating this sample direction (emitSample)
	float area_pdf_; //<! probability density for generating this sample point on light surface (emitSample)
	Rgb col_; //<! color of the generated sample
	Light::Flags flags_; //<! flags of the sampled light source
	SurfacePoint *sp_ = nullptr; //!< surface point on the light source, may only be complete enough to call other light methods with it!
};

} //namespace yafaray

#endif // YAFARAY_LIGHT_H
