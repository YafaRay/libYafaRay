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

#ifndef YAFARAY_CAMERA_PERSPECTIVE_H
#define YAFARAY_CAMERA_PERSPECTIVE_H

#include "constants.h"
#include "camera/camera.h"
#include <vector>

BEGIN_YAFARAY

class ParamMap;
class Scene;

class PerspectiveCamera : public Camera
{
	public:
		static Camera *factory(ParamMap &params, Scene &scene);

	protected:
		enum BokehType {BkDisk1, BkDisk2, BkTri = 3, BkSqr, BkPenta, BkHexa, BkRing};
		enum BkhBiasType {BbNone, BbCenter, BbEdge};

		PerspectiveCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
						  int resx, int resy, float aspect = 1,
						  float df = 1, float ap = 0, float dofd = 0, BokehType bt = BkDisk1, BkhBiasType bbt = BbNone, float bro = 0,
						  float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
		virtual void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz) override;
		virtual Ray shootRay(float px, float py, float lu, float lv, float &wt) const override;
		virtual bool sampleLense() const override;
		virtual Point3 screenproject(const Point3 &p) const override;
		virtual bool project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const override;
		void biasDist(float &r) const;
		void sampleTsd(float r_1, float r_2, float &u, float &v) const;
		void getLensUv(float r_1, float r_2, float &u, float &v) const;

		BokehType bkhtype_;
		BkhBiasType bkhbias_;
		Vec3 dof_up_, dof_rt_;
		float aperture_;
		float focal_distance_, dof_distance_;
		float fdist_;
		float a_pix_;
		std::vector<float> ls_;
};

END_YAFARAY

#endif // YAFARAY_CAMERA_PERSPECTIVE_H
