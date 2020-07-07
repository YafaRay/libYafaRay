#pragma once
/****************************************************************************
 *
 * 			surface.h: Surface sampling representation and api
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estévez
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
 *
 */
#ifndef Y_SURFACE_H
#define Y_SURFACE_H

#include <yafray_constants.h>
#include "vector3d.h"
#include "color.h"


__BEGIN_YAFRAY
class material_t;
class light_t;
class object3d_t;
class diffRay_t;
class vector3d_t;

class intersectData_t
{
	public:
		intersectData_t()
		{
			// Empty
		}
		float b0 = 0.f;
		float b1 = 0.f;
		float b2 = 0.f;
		float t = 0.f;
		const vector3d_t * edge1 = nullptr;
		const vector3d_t * edge2 = nullptr;
};

/*! This holds a sampled surface point's data
	When a ray intersects an object, a surfacePoint_t is computed.
	It contains data about normal, position, assigned material and other
	things.
 */
class YAFRAYCORE_EXPORT surfacePoint_t
{
	public:
		float getDistToNearestEdge() const;
	
		//int object; //!< the object owner of the point.
		const material_t *material; //!< the surface material
		const light_t *light; //!< light source if surface point is on a light
		const object3d_t *object; //!< object the prim belongs to
	//	point2d_t screenpos; // only used with 'win' texture coord. mode
		void *origin;
		intersectData_t data;

		// Geometry related
		vector3d_t N; //!< the shading normal.
		vector3d_t Ng; //!< the geometric normal.
		vector3d_t orcoNg; //!< the untransformed geometric normal.
		point3d_t P; //!< the (world) position.
		point3d_t orcoP;
	//	color_t vertex_col;
		bool hasUV;
		bool hasOrco;
		bool available;
		int primNum;

		float U; //!< the u texture coord.
		float V; //!< the v texture coord.
		vector3d_t  NU; //!< second vector building orthogonal shading space with N
		vector3d_t  NV; //!< third vector building orthogonal shading space with N
		vector3d_t dPdU; //!< u-axis in world space (normalized)
		vector3d_t dPdV; //!< v-axis in world space (normalized)
		vector3d_t dSdU; //!< u-axis in shading space (NU, NV, N)
		vector3d_t dSdV; //!< v-axis in shading space (NU, NV, N)
		vector3d_t dPdU_abs; //!< u-axis in world space (before normalization)
		vector3d_t dPdV_abs; //!< v-axis in world space (before normalization)
		//float dudNU;
		//float dudNV;
		//float dvdNU;
		//float dvdNV;
		
		// Differential ray for mipmaps calculations
		const diffRay_t * ray = nullptr;
};

inline float surfacePoint_t::getDistToNearestEdge() const
{
	if(data.edge1 && data.edge2)
	{
		float edge1len = data.edge1->length();
		float edge2len = data.edge2->length();
		float edge12len = (*(data.edge1) + *(data.edge2)).length() * 0.5f;
		
		float edge1dist = data.b1 * edge1len;
		float edge2dist = data.b2 * edge2len;
		float edge12dist = data.b0 * edge12len;
						
		float edgeMinDist = std::min(edge12dist, std::min(edge1dist, edge2dist));
		
		return edgeMinDist;
	}
	else return std::numeric_limits<float>::infinity();
}


YAFRAYCORE_EXPORT surfacePoint_t blend_surface_points(surfacePoint_t const& sp_0, surfacePoint_t const& sp_1, float const alpha);

/*! computes and stores the additional data for surface intersections for
	differential rays */
class YAFRAYCORE_EXPORT spDifferentials_t
{
	public:
		spDifferentials_t(const surfacePoint_t &spoint, const diffRay_t &ray);
		//! compute differentials for a scattered ray
		void reflectedRay(const diffRay_t &in, diffRay_t &out) const;
		//! compute differentials for a refracted ray
		void refractedRay(const diffRay_t &in, diffRay_t &out, float IOR) const;
		float projectedPixelArea();
		void getUVdifferentials(float &dUdx, float &dVdx, float &dUdy, float &dVdy) const;
		vector3d_t dPdx;
		vector3d_t dPdy;
		const surfacePoint_t &sp;
	protected:
		void dU_dV_from_dP_dPdU_dPdV(float &dU, float &dV, const point3d_t &dP, const vector3d_t &dPdU, const vector3d_t &dPdV) const;
};

__END_YAFRAY

#endif
