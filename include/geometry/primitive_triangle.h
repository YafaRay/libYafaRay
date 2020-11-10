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

#ifndef YAFARAY_PRIMITIVE_TRIANGLE_H
#define YAFARAY_PRIMITIVE_TRIANGLE_H

#include "geometry/primitive.h"
#include "geometry/vector.h"

/*! inherited triangle, so has virtual functions; connected to MeshObject;
	otherwise identical to Triangle
*/

BEGIN_YAFARAY

class MeshObject;

class VTriangle: public Primitive
{
	public:
		VTriangle() = default;
		VTriangle(int ia, int ib, int ic, MeshObject *m): pa_(ia), pb_(ib), pc_(ic), mesh_(m) { /*recNormal();*/ };
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const override;
		virtual Bound getBound() const override;
		virtual bool intersectsBound(ExBound &eb) const override;
		virtual bool clippingSupport() const override { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const override;
		virtual const Material *getMaterial() const override { return material_; }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const override;

		// following are methods which are not part of primitive interface:
		void setMaterial(const Material *m) { material_ = m; }
		void setNormals(int a, int b, int c) { na_ = a, nb_ = b, nc_ = c; }
		Vec3 getNormal() { return Vec3(normal_); }
		float surfaceArea() const;
		void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const;
		void recNormal();

	protected:
		int pa_ = -1, pb_ = -1, pc_ = -1; //!< indices in point array, referenced in mesh.
		int na_ = -1, nb_ = -1, nc_ = -1; //!< indices in normal array, if mesh is smoothed.
		Vec3 normal_; //!< the geometric normal
		const Material *material_ = nullptr;
		const MeshObject *mesh_ = nullptr;
};

END_YAFARAY

#endif //YAFARAY_PRIMITIVE_TRIANGLE_H
