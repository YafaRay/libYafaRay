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

#include "ray.h"
#include <vector>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT camera_t
{
	public:
		camera_t() { }
		camera_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up, int _resx, int _resy, float aspect)
		:position(pos), resx(_resx), resy(_resy), aspect_ratio(aspect * (PFLOAT)resy / (PFLOAT)resx)
		{
			// Calculate and store camera axis
			camY = up - position;
			camZ = look - position;
			camX = camZ ^ camY;
			camY = camZ ^ camX;
			camX.normalize();
			camY.normalize();
			camZ.normalize();
		}
		virtual ~camera_t() {};
		virtual void setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz) const = 0; //<! set camera axis
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float u, float v, PFLOAT &wt) const = 0;
		virtual point3d_t screenproject(const point3d_t &p) const = 0;
		
		virtual int resX() const { return resx; } //<! get camera X resolution
		virtual int resY() const { return resy; } // <! get camera Y resolution
		virtual point3d_t getPosition() const { return position; } //<! get camera position
		virtual void setPosition(const point3d_t &pos) const { position = pos; } //<! set camera position
		virtual void getAxis(vector3d_t &vx, vector3d_t &vy, vector3d_t &vz) const { vx = camX; vy = camY; vz = camZ; } //<! get camera axis
		/*! indicate whether the lense need to be sampled (u, v parameters of shootRay), i.e.
			DOF-like effects. When false, no lense samples need to be computed */
		virtual bool sampleLense() const { return false; }
		virtual bool project(const ray_t &wo, PFLOAT lu, PFLOAT lv, PFLOAT &u, PFLOAT &v, float &pdf) const { return false; }
	protected:
		mutable point3d_t position; //<! camera position
		int resx, resy; //<! camera resolution
		mutable vector3d_t camX, camY, camZ; //<! camera coordinate system
		mutable vector3d_t vto, vup, vright;
		float aspect_ratio;  //<! aspect ratio of camera (not image in pixel units!)
};

__END_YAFRAY

#endif // Y_CAMERA_H
