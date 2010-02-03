/****************************************************************************
 *
 * 			surface.h: Surface sampling representation and api
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estevez
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
#include "vector3d.h"
//#include <yafray/core/extern/vector2d.h>
#include <core_api/light.h>
#include "color.h"

#include<yafray_config.h>

__BEGIN_YAFRAY
class material_t;
//class light_t;
class object3d_t;
class diffRay_t;

/*
struct YAFRAYCORE_EXPORT surfacePoint_t
{
		surfacePoint_t(const point3d_t &p, const point3d_t &orc,
				const vector3d_t &n,const vector3d_t &g,
				GFLOAT u, GFLOAT v, color_t vcol,
				PFLOAT d,const shader_t *sha=NULL,
				bool uv=false, bool hvcol=false,bool horco=false)
		{	P=p;  N=n;  Ng=g;
			U=u;  V=v;  vertex_col=vcol;
			Z=d;  object=-1; shader=sha;  hasUV=uv;  hasVertexCol=hvcol;
			hasOrco=horco;
			orcoP=orc;
			createCS(N,NU,NV);
			dudNU=dudNV=dvdNU=dvdNV=0;
			origin=NULL;
		}

		surfacePoint_t() { hasOrco=false;hasUV=false;  hasVertexCol=false;  shader=NULL; origin=NULL; }

		vector3d_t N; /// the normal.
		vector3d_t Ng; /// the geometric normal.
		point3d_t P; /// the position.
		point3d_t orcoP;
		GFLOAT U; /// the u texture coord.
		GFLOAT V; /// the v texture coord.
		bool hasUV; /// whatever it has valid UV or not
		bool hasVertexCol; /// if the point has a valid vertex color
		bool hasOrco;
		color_t vertex_col;
		PFLOAT Z;
		int object; /// the object owner of the point.
		const shader_t *shader; /// the surface shader
		vector3d_t  NU;
		vector3d_t  NV;
		GFLOAT dudNU;
		GFLOAT dudNV;
		GFLOAT dvdNU;
		GFLOAT dvdNV;
		void setUvGradient(GFLOAT uu, GFLOAT uv, GFLOAT vu, GFLOAT vv)
		{ dudNU=uu;  dudNV=uv;  dvdNU=vu;  dvdNV=vv; }

		point2d_t screenpos; // only used with 'win' texture coord. mode
		void *origin;
};

*/

/*! This holds a sampled surface point's data
	When a ray intersects an object, a surfacePoint_t is computed.
	It contains data about normal, position, assigned material and other
	things.
 */
struct YAFRAYCORE_EXPORT surfacePoint_t
{
	//int object; //!< the object owner of the point.
	const material_t *material; //!< the surface material
	const light_t *light; //!< light source if surface point is on a light
	const object3d_t *object; //!< object the prim belongs to
//	point2d_t screenpos; // only used with 'win' texture coord. mode
	void *origin;
	
	// Geometry related
	mutable vector3d_t N; //!< the shading normal.
	vector3d_t Ng; //!< the geometric normal.
	vector3d_t orcoNg; //!< the untransformed geometric normal.
	point3d_t P; //!< the (world) position.
	point3d_t orcoP;
//	color_t vertex_col;
	bool hasUV; 
	bool hasOrco;
	bool available;
	int primNum;

	GFLOAT U; //!< the u texture coord.
	GFLOAT V; //!< the v texture coord.
	mutable vector3d_t  NU; //!< second vector building orthogonal shading space with N
	mutable vector3d_t  NV; //!< third vector building orthogonal shading space with N
	vector3d_t dPdU; //!< u-axis in world space
	vector3d_t dPdV; //!< v-axis in world space
	vector3d_t dSdU; //!< u-axis in shading space (NU, NV, N)
	vector3d_t dSdV; //!< v-axis in shading space (NU, NV, N)
	GFLOAT sU; //!< raw surface parametric coordinate; required to evaluate vmaps
	GFLOAT sV; //!< raw surface parametric coordinate; required to evaluate vmaps
	//GFLOAT dudNU;
	//GFLOAT dudNV;
	//GFLOAT dvdNU;
	//GFLOAT dvdNV;
};

/*! computes and stores the additional data for surface intersections for
	differential rays */
class YAFRAYCORE_EXPORT spDifferentials_t
{
	public:
		spDifferentials_t(const surfacePoint_t &spoint, const diffRay_t &ray);
		//! compute differentials for a scattered ray
		void reflectedRay(const diffRay_t &in, diffRay_t &out) const;
		//! compute differentials for a refracted ray
		void refractedRay(const diffRay_t &in, diffRay_t &out, PFLOAT IOR) const;
		PFLOAT projectedPixelArea();
		vector3d_t dPdx;
		vector3d_t dPdy;
		const surfacePoint_t &sp;
};

__END_YAFRAY

#endif
