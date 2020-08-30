#pragma once
/****************************************************************************
 *      meshlight.h: a light source using a triangle mesh as shape
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef YAFARAY_LIGHT_MESHLIGHT_H
#define YAFARAY_LIGHT_MESHLIGHT_H

#include "light/light.h"

BEGIN_YAFARAY

class TriangleObject;
class Triangle;
class Pdf1D;
class ParamMap;
class RenderEnvironment;
class TriKdTree;

class MeshLight final : public Light
{
	public:
		static Light *factory(ParamMap &params, RenderEnvironment &render);

	private:
		MeshLight(unsigned int msh, const Rgb &col, int sampl, bool dbl_s = false, bool light_enabled = true, bool cast_shadows = true);
		virtual ~MeshLight() override;
		virtual void init(Scene &scene) override;
		virtual Rgb totalEnergy() const override;
		virtual Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		virtual Rgb emitSample(Vec3 &wo, LSample &s) const override;
		virtual bool diracLight() const override { return false; }
		virtual bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const override;
		virtual bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const override { return false; }
		virtual int nSamples() const override { return samples_; }
		virtual bool canIntersect() const override { return tree_ != 0 /* false */ ; }
		virtual bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		virtual float illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const override;
		virtual void emitPdf(const SurfacePoint &sp, const Vec3 &wi, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		void initIs();
		void sampleSurface(Point3 &p, Vec3 &n, float s_1, float s_2) const;

		unsigned int obj_id_;
		bool double_sided_;
		Rgb color_;
		Pdf1D *area_dist_;
		const Triangle **tris_;
		int samples_;
		int n_tris_; //!< gives the array size of uDist
		float area_, inv_area_;
		TriangleObject *mesh_;
		TriKdTree *tree_;
		//debug stuff:
		int *stats_;
};

END_YAFARAY

#endif // YAFARAY_LIGHT_MESHLIGHT_H
