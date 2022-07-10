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

#ifndef YAFARAY_VOLUME_H
#define YAFARAY_VOLUME_H

#include "geometry/bound.h"
#include "color/color.h"
#include <map>

namespace yafaray {

struct PSample;
class Light;
template<typename T> class Ray;
class ParamMap;
class Scene;
class Logger;
class MaterialData;

class VolumeHandler
{
	public:
		static VolumeHandler *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		explicit VolumeHandler(Logger &logger) : logger_(logger) { }
		virtual Rgb transmittance(const Ray<float> &ray) const = 0;
		virtual bool scatter(const Ray<float> &ray, Ray<float> &s_ray, PSample &s) const = 0;
		virtual ~VolumeHandler() = default;

	protected:
		Logger &logger_;
};

class VolumeRegion
{
	public:
		static VolumeRegion *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		explicit VolumeRegion(Logger &logger) : logger_(logger) { }
		VolumeRegion(Logger &logger, const Rgb &sa, const Rgb &ss, const Rgb &le, float gg, const Point3f &pmin, const Point3f &pmax, int attgrid_scale);
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
			float k = 1.55f * g_ - .55f * g_ * g_ * g_;
			float kcostheta = k * (w_l * w_s);
			return 1.f / (4.f * math::num_pi<>) * (1.f - k * k) / ((1.f - kcostheta) * (1.f - kcostheta));
		}

		virtual Rgb tau(const Ray<float> &ray, float step, float offset) const = 0;

		Bound<float>::Cross crossBound(const Ray<float> &ray) const
		{
			return b_box_.cross(ray, 10000.f);
		}

		Bound<float> getBb() const { return b_box_; }

		int att_grid_x_, att_grid_y_, att_grid_z_; // FIXME: un-hardcode
		std::map<const Light *, float *> attenuation_grid_map_; // FIXME: un-hardcode

	protected:
		Bound<float> b_box_;
		Rgb s_a_, s_s_, l_e_;
		bool have_s_a_, have_s_s_, have_l_e_;
		float g_;
		Logger &logger_;
		// TODO: add transform for BB
};

class DensityVolumeRegion : public VolumeRegion
{
	protected:
		explicit DensityVolumeRegion(Logger &logger) : VolumeRegion(logger) {}
		DensityVolumeRegion(Logger &logger, const Rgb &sa, const Rgb &ss, const Rgb &le, float gg, const Point3f &pmin, const Point3f &pmax, int attgrid_scale) :
			VolumeRegion(logger, sa, ss, le, gg, pmin, pmax, attgrid_scale) {}

		virtual float density(const Point3f &p) const = 0;

		Rgb tau(const Ray<float> &ray, float step_size, float offset) const override;

		Rgb sigmaA(const Point3f &p, const Vec3f &v) const override
		{
			if(!have_s_a_) return Rgb{0.f};
			if(b_box_.includes(p))
			{
				return s_a_ * density(p);
			}
			else
				return Rgb{0.f};

		}

		Rgb sigmaS(const Point3f &p, const Vec3f &v) const override
		{
			if(!have_s_s_) return Rgb{0.f};
			if(b_box_.includes(p))
			{
				return s_s_ * density(p);
			}
			else
				return Rgb{0.f};
		}

		Rgb emission(const Point3f &p, const Vec3f &v) const override
		{
			if(!have_l_e_) return Rgb{0.f};
			if(b_box_.includes(p))
			{
				return l_e_ * density(p);
			}
			else
				return Rgb{0.f};
		}

};



} //namespace yafaray

#endif // YAFARAY_VOLUME_H
