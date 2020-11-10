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

#ifndef YAFARAY_TRIANGLE_INSTANCE_H
#define YAFARAY_TRIANGLE_INSTANCE_H

#include "geometry/triangle.h"

BEGIN_YAFARAY

class TriangleObjectInstance;

class TriangleInstance final : public Triangle
{
	public:
		TriangleInstance() = default;
		TriangleInstance(const Triangle *base, TriangleObjectInstance *m): triangle_(base), triangle_object_instance_(m) { updateIntersectCachedValues(); }
		virtual bool clippingSupport() const override { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual const Material *getMaterial() const override { return triangle_->getMaterial(); }
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const override;
		virtual Vec3 getNormal() const override;
		virtual void recNormal() override { /* Empty */ };
		virtual size_t getSelfIndex() const override { return triangle_->getSelfIndex(); }
		virtual void setSelfIndex(size_t index) override { } //FIXME? We should not change the self index of a triangle instance!
		virtual Point3 getVertex(size_t index) const override; //!< Get triangle vertex (index is the vertex number in the triangle: 0, 1 or 2
		virtual Point3 getOrcoVertex(size_t index) const override; //!< Get triangle original coordinates (orco) vertex in instance objects (index is the vertex number in the triangle: 0, 1 or 2
		virtual Point3 getVertexNormal(size_t index, const Vec3 &surface_normal) const override; //!< Get triangle vertex normal (index is the vertex number in the triangle: 0, 1 or 2
		virtual Uv getVertexUv(size_t index) const override; //!< Get triangle vertex Uv (index is the vertex number in the triangle: 0, 1 or 2
		virtual void calculateShadingSpace(SurfacePoint &sp) const override;
		virtual const TriangleObject *getMesh() const override;

	private:
		const Triangle *triangle_ = nullptr;
		const TriangleObjectInstance *triangle_object_instance_ = nullptr;
};

END_YAFARAY

#endif //YAFARAY_TRIANGLE_INSTANCE_H
