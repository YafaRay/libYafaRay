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

#ifndef LIBYAFARAY_LIGHT_H
#define LIBYAFARAY_LIGHT_H

#include "color/color.h"
#include "common/logger.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include <sstream>
#include <memory>

namespace yafaray {

class ParamMap;
class Scene;
class SurfacePoint;
class Background;
class Ray;
class Scene;
template <typename T> class Items;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;
struct LSample;

class Light
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "Light"; }
		[[nodiscard]] virtual Type type() const = 0;
		struct Flags : Enum<Flags>
		{
			using Enum::Enum;
			enum : ValueType_t { None = 0, DiracDir = 1, Singular = 1 << 1, All = DiracDir | Singular };
			inline static const EnumMap<ValueType_t> map_{{
					{"None", None, ""},
					{"DiracDir", DiracDir, ""},
					{"Singular", Singular, ""},
					{"All", All, ""},
				}};
		};
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		Light(Logger &logger, ParamResult &param_result, const ParamMap &param_map, Flags flags, const Items<Light> &lights) : params_{param_result, param_map}, lights_{lights}, flags_{flags}, logger_{logger} { }
		virtual ~Light() = default;
		void setId(size_t id) { id_ = id; }
		[[nodiscard]] size_t getId() const { return id_; }
		//! allow for preprocessing when scene loading has finished. Returns object_id in lights linked to objects
		virtual size_t init(const Scene &scene) { return math::invalid<size_t>; }
		//! total energy emmitted during whole frame
		[[nodiscard]] virtual Rgb totalEnergy() const = 0;
		//! emit a photon
		[[nodiscard]] virtual std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const = 0;
		//! create a sample of light emission, similar to emitPhoton, just more suited for bidirectional methods
		/*! fill in s.dirPdf, s.areaPdf, s.col and s.flags, and s.sp if not nullptr */
		[[nodiscard]] virtual std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const = 0;
		//! indicate whether the light has a dirac delta distribution or not
		[[nodiscard]] virtual bool diracLight() const = 0;
		//! illuminate a given surface point, generating sample s, fill in s.sp if not nullptr; Set ray to test visibility by integrator
		/*! fill in s.pdf, s.col and s.flags */
		[[nodiscard]] virtual std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const = 0;
		//! illuminate a given surfance point; Set ray to test visibility by integrator. Only for dirac lights.
		/*!	return false only if no light is emitted towards sp, e.g. outside cone angle of spot light	*/
		[[nodiscard]] virtual std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const = 0;
		//! indicate whether the light can intersect with a ray (by the sphereIntersect function)
		[[nodiscard]] virtual bool canIntersect() const { return false; }
		//! sphereIntersect the light source with a ray, giving back distance, energy and 1/PDF
		[[nodiscard]] virtual std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const { return {}; }
		//! get the pdf for sampling the incoming direction wi at surface point sp (illumSample!)
		/*! this method requires an intersection point with the light (sp_light). Otherwise, use sphereIntersect() */
		[[nodiscard]] virtual float illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const { return 0.f; }
		//! get the pdf values for sampling point sp on the light and outgoing direction wo when emitting energy (emitSample, NOT illumSample)
		/*! sp should've been generated from illumSample or emitSample, and may only be complete enough to call light functions! */
		[[nodiscard]] virtual std::array<float, 3> emitPdf(const Vec3f &surface_n, const Vec3f &wo) const { return {0.f, 0.f, 0.f}; }
		//! (preferred) number of samples for direct lighting
		[[nodiscard]] virtual int nSamples() const { return 8; }
		//! Check whether the light is enabled
		[[nodiscard]] bool lightEnabled() const { return params_.light_enabled_;}
		//! Check whether the light casts shadows
		[[nodiscard]] bool castShadows() const { return params_.cast_shadows_; }
		//! checks if the light can shoot caustic photons (photonmap integrator)
		[[nodiscard]] bool shootsCausticP() const { return params_.shoot_caustic_; }
		//! checks if the light can shoot diffuse photons (photonmap integrator)
		[[nodiscard]] bool shootsDiffuseP() const { return params_.shoot_diffuse_; }
		//! checks if the light is a photon-only light (only shoots photons, not illuminating)
		[[nodiscard]] bool photonOnly() const { return params_.photon_only_; }
		[[nodiscard]] Flags getFlags() const { return flags_; }
		[[nodiscard]] [[nodiscard]] std::string getName() const;

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Area, BackgroundPortal, Object, Background, Directional, Ies, Point, Sphere, Spot, Sun };
			inline static const EnumMap<ValueType_t> map_{{
					{"arealight", Area, ""},
					{"bgPortalLight", BackgroundPortal, ""},
					{"objectlight", Object, ""},
					{"bglight", Background, ""},
					{"directional", Directional, ""},
					{"ieslight", Ies, ""},
					{"pointlight", Point, ""},
					{"spherelight", Sphere, ""},
					{"spotlight", Spot, ""},
					{"sunlight", Sun, ""},
				}};
		};
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(bool, light_enabled_, true, "light_enabled", "Enable/disable light");
			PARAM_DECL(bool, cast_shadows_, true, "cast_shadows", "Enable/disable if the light should cast direct shadows");
			PARAM_DECL(bool, shoot_caustic_, true, "with_caustic", "Enable/disable if the light can shoot caustic photons (only for integrators using caustic photons)");
			PARAM_DECL(bool, shoot_diffuse_, true, "with_diffuse", "Enable/disable if the light can shoot diffuse photons (only for integrators using diffuse photons)");
			PARAM_DECL(bool, photon_only_, false, "photon_only", "Enable/disable if the light is a photon-only light (only shoots photons, not illuminating)");
		} params_;
		size_t id_{0};
		const Items<Light> &lights_;
		Flags flags_{Flags::None};
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
	Light::Flags flags_{Light::Flags::None}; //<! flags of the sampled light source
	SurfacePoint *sp_ = nullptr; //!< surface point on the light source, may only be complete enough to call other light methods with it!
};

} //namespace yafaray

#endif // LIBYAFARAY_LIGHT_H
