/****************************************************************************
 *
 * 			camera.h: Camera implementation api
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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

#ifndef Y_CAMERA_H
#define Y_CAMERA_H

#include <yafray_config.h>

//#include "vector3d.h"
//#include "matrix4.h"
//#include "mcqmc.h"
#include "ray.h"
#include <vector>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT camera_t
{
	public:
		virtual ~camera_t() {};
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float u, float v, PFLOAT &wt) const = 0;
		virtual int resX() const = 0;
		virtual int resY() const = 0;
		virtual bool project(const ray_t &wo, PFLOAT lu, PFLOAT lv, PFLOAT &u, PFLOAT &v, float &pdf) const
			{ return false; }
		/*! indicate whether the lense need to be sampled (u, v parameters of shootRay), i.e.
			DOF-like effects. When false, no lense samples need to be computed */
		virtual bool sampleLense() const = 0;
};


__END_YAFRAY

#endif // Y_CAMERA_H
