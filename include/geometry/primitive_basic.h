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

#ifndef YAFARAY_PRIMITIVE_BASIC_H
#define YAFARAY_PRIMITIVE_BASIC_H

#include "geometry/primitive.h"
#include "vector.h"

BEGIN_YAFARAY

class Scene;
class ParamMap;
class ObjectGeometric;
class Bound;
class ExBound;
class Ray;
class SurfacePoint;

class Sphere final : public Primitive
{
	public:
		Sphere(Point3 centr, float rad, const Material *m): center_(centr), radius_(rad), material_(m) {}

	private:
		virtual Bound getBound() const override;
		virtual bool intersectsBound(ExBound &b) const override { return true; };
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const override;
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const override;
		virtual const Material *getMaterial() const override { return material_; }

		Point3 center_;
		float radius_;
		const Material *material_ = nullptr;
};

ObjectGeometric *sphereFactory__(ParamMap &params, Scene &scene);

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_BASIC_H
