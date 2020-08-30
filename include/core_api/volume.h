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

#include <yafray_constants.h>
#include "bound.h"
#include "color.h"
#include <map>

BEGIN_YAFRAY

struct RenderState;
struct PSample;
class Light;
class Ray;

class VolumeHandler
{
	public:
		virtual bool transmittance(const RenderState &state, const Ray &ray, Rgb &col) const = 0;
		virtual bool scatter(const RenderState &state, const Ray &ray, Ray &s_ray, PSample &s) const = 0;
		virtual ~VolumeHandler() {}
};

class YAFRAYCORE_EXPORT VolumeRegion
{
	public:
		VolumeRegion() {}
		VolumeRegion(Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax, int attgrid_scale)
		{
			b_box_ = Bound(pmin, pmax);
			s_a_ = sa;
			s_s_ = ss;
			l_e_ = le;
			g_ = gg;
			have_s_a_ = (s_a_.energy() > 1e-4f);
			have_s_s_ = (s_s_.energy() > 1e-4f);
			have_l_e_ = (l_e_.energy() > 1e-4f);
			att_grid_x_ = 8 * attgrid_scale;
			att_grid_y_ = 8 * attgrid_scale;
			att_grid_z_ = 8 * attgrid_scale;
		}

		virtual ~VolumeRegion() {}

		virtual Rgb sigmaA(const Point3 &p, const Vec3 &v) = 0;
		virtual Rgb sigmaS(const Point3 &p, const Vec3 &v) = 0;
		virtual Rgb emission(const Point3 &p, const Vec3 &v) = 0;
		virtual Rgb sigmaT(const Point3 &p, const Vec3 &v)
		{
			return sigmaA(p, v) + sigmaS(p, v);
		}

		float attenuation(const Point3 p, Light *l);

		// w_l: dir *from* the light, w_s: direction, into which should be scattered
		virtual float p(const Vec3 &w_l, const Vec3 &w_s)
		{
			float k = 1.55f * g_ - .55f * g_ * g_ * g_;
			float kcostheta = k * (w_l * w_s);
			return 1.f / (4.f * M_PI) * (1.f - k * k) / ((1.f - kcostheta) * (1.f - kcostheta));
		}

		virtual Rgb tau(const Ray &ray, float step, float offset) = 0;

		bool intersect(const Ray &ray, float &t_0, float &t_1)
		{
			return b_box_.cross(ray, t_0, t_1, 10000.f);
		}

		Bound getBb() { return b_box_; }

		std::map<Light *, float *> attenuation_grid_map_;
		int att_grid_x_, att_grid_y_, att_grid_z_; // FIXME: un-hardcode

	protected:
		Bound b_box_;
		Rgb s_a_, s_s_, l_e_;
		bool have_s_a_, have_s_s_, have_l_e_;
		float g_;
		// TODO: add transform for BB
};

class YAFRAYCORE_EXPORT DensityVolume : public VolumeRegion
{
	public:
		DensityVolume() {}
		DensityVolume(Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax, int attgrid_scale) :
			VolumeRegion(sa, ss, le, gg, pmin, pmax, attgrid_scale) {}

		virtual ~DensityVolume() {}

		virtual float density(Point3 p) = 0;

		virtual Rgb tau(const Ray &ray, float step_size, float offset);

		Rgb sigmaA(const Point3 &p, const Vec3 &v)
		{
			if(!have_s_a_) return Rgb(0.f);
			if(b_box_.includes(p))
			{
				return s_a_ * density(p);
			}
			else
				return Rgb(0.f);

		}

		Rgb sigmaS(const Point3 &p, const Vec3 &v)
		{
			if(!have_s_s_) return Rgb(0.f);
			if(b_box_.includes(p))
			{
				return s_s_ * density(p);
			}
			else
				return Rgb(0.f);
		}

		Rgb emission(const Point3 &p, const Vec3 &v)
		{
			if(!have_l_e_) return Rgb(0.f);
			if(b_box_.includes(p))
			{
				return l_e_ * density(p);
			}
			else
				return Rgb(0.f);
		}

};



END_YAFRAY

#endif // YAFARAY_VOLUME_H
