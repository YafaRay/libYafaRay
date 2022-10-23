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

#ifndef LIBYAFARAY_VOLUME_REGION_H
#define LIBYAFARAY_VOLUME_REGION_H

#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include "geometry/bound.h"

namespace yafaray {

class Scene;
class Light;

class VolumeRegion
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "VolumeRegion"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<VolumeRegion>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		VolumeRegion(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		virtual ~VolumeRegion() = default;

		virtual Rgb sigmaA(const Point3f &p, const Vec3f &v) const = 0;
		virtual Rgb sigmaS(const Point3f &p, const Vec3f &v) const = 0;
		virtual Rgb emission(const Point3f &p, const Vec3f &v) const = 0;
		Rgb sigmaT(const Point3f &p, const Vec3f &v) const
		{
			return sigmaA(p, v) + sigmaS(p, v);
		}

		float attenuation(const Point3f &p, const Light *l) const;

		// w_l: dir *from* the light, w_s: direction, into which should be scattered
		virtual float p(const Vec3f &w_l, const Vec3f &w_s) const
		{
			const float k = 1.55f * params_.g_ - .55f * params_.g_ * params_.g_ * params_.g_;
			const float kcostheta = k * (w_l * w_s);
			return 1.f / (4.f * math::num_pi<>) * (1.f - k * k) / ((1.f - kcostheta) * (1.f - kcostheta));
		}

		virtual Rgb tau(const Ray &ray, float step, float offset) const = 0;

		Bound<float>::Cross crossBound(const Ray &ray) const
		{
			return b_box_.cross(ray, 10000.f);
		}

		Bound<float> getBb() const { return b_box_; }

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, ExpDensity, Grid, Noise, Sky, Uniform };
			inline static const EnumMap<ValueType_t> map_{{
					{"ExpDensityVolume", ExpDensity, ""},
					{"GridVolume", Grid, ""},
					{"NoiseVolume", Noise, ""},
					{"SkyVolume", Sky, ""},
					{"UniformVolume", Uniform, ""},
				}};
		};
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(float, sigma_s_, 0.1f, "sigma_s", "");
			PARAM_DECL(float, sigma_a_, 0.1f, "sigma_a", "");
			PARAM_DECL(float, l_e_, 0.f, "l_e", "");
			PARAM_DECL(float, g_, 0.f, "g", "");
			PARAM_DECL(float, min_x_, 0.f, "minX", "");
			PARAM_DECL(float, min_y_, 0.f, "minY", "");
			PARAM_DECL(float, min_z_, 0.f, "minZ", "");
			PARAM_DECL(float, max_x_, 0.f, "maxX", "");
			PARAM_DECL(float, max_y_, 0.f, "maxY", "");
			PARAM_DECL(float, max_z_, 0.f, "maxZ", "");
			PARAM_DECL(int, att_grid_scale_, 5, "attgridScale", "");
		} params_;
		const Rgb s_a_{params_.sigma_a_};
		const Rgb s_s_{params_.sigma_s_};
		const Rgb l_e_{params_.l_e_};
		const bool have_s_a_{s_a_.energy() > 1e-4f};
		const bool have_s_s_{s_s_.energy() > 1e-4f};
		const bool have_l_e_{l_e_.energy() > 1e-4f};
		const Bound<float> b_box_{
			Point3f{{params_.min_x_, params_.min_y_, params_.min_z_}},
			Point3f{{params_.max_x_, params_.max_y_, params_.max_z_}}
			};
		Logger &logger_;
		// TODO: add transform for BB

	public:
		const int att_grid_x_{8 * params_.att_grid_scale_}; // FIXME: un-hardcode
		const int att_grid_y_{8 * params_.att_grid_scale_}; // FIXME: un-hardcode
		const int att_grid_z_{8 * params_.att_grid_scale_}; // FIXME: un-hardcode
		std::map<const Light *, float *> attenuation_grid_map_; // FIXME: un-hardcode
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_REGION_H
