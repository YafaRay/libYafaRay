/****************************************************************************
 * 			meshlight.h: a light source using a triangle mesh as shape
 *      This is part of the yafray package
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

#ifndef Y_MESHLIGHT_H
#define Y_MESHLIGHT_H

#include <core_api/light.h>

__BEGIN_YAFRAY

class triangleObject_t;
class triangle_t;
class pdf1D_t;
class paraMap_t;
class renderEnvironment_t;
class triKdTree_t;

class meshLight_t : public light_t
{
	public:
		meshLight_t(unsigned int msh, const color_t &col, int sampl, bool dbl_s=false);
		virtual ~meshLight_t();
		virtual void init(scene_t &scene);
		virtual color_t totalEnergy() const;
		virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
		virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
		virtual bool diracLight() const { return false; }
		virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
		virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi)const { return false; }
		virtual int nSamples() const { return samples; }
		virtual bool canIntersect() const{ return tree!=0 /* false */ ; }
		virtual bool intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const;
		virtual float illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const;
		virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wi, float &areaPdf, float &dirPdf, float &cos_wo) const;
		
		static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		void initIS();
		void sampleSurface(point3d_t &p, vector3d_t &n, float u, float v) const;
		unsigned int objID;
		bool doubleSided;
		color_t color;
		pdf1D_t *areaDist;
		const triangle_t **tris;
		int samples;
		int nTris; //!< gives the array size of uDist
		float area, invArea;
		triangleObject_t *mesh;
		triKdTree_t *tree;
		//debug stuff:
		int *stats;
};

__END_YAFRAY

#endif // Y_BACKGROUNDLIGHT_H
