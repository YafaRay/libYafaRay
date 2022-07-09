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

#include "camera/camera.h"
#include <vector>

namespace yafaray {

class ParamMap;
class Scene;

class PerspectiveCamera : public Camera
{
	public:
		static const Camera * factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	protected:
		enum class BokehType : unsigned char {BkDisk1, BkDisk2, BkTri, BkSqr, BkPenta, BkHexa, BkRing};
		enum class BkhBiasType : unsigned char {BbNone, BbCenter, BbEdge};

		PerspectiveCamera(Logger &logger, const Point3f &pos, const Point3f &look, const Point3f &up,
						  int resx, int resy, float aspect = 1,
						  float df = 1, float ap = 0, float dofd = 0, BokehType bt = BokehType::BkDisk1, BkhBiasType bbt = BkhBiasType::BbNone, float bro = 0,
						  float near_clip_distance = 0.0f, float far_clip_distance = 1e6f);
		void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz) override;
		CameraRay shootRay(float px, float py, const Uv<float> &uv) const override;
		bool sampleLense() const override;
		Point3f screenproject(const Point3f &p) const override;
		bool project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const override;
		float biasDist(float r) const;
		Uv<float> sampleTsd(float r_1, float r_2) const;
		Uv<float> getLensUv(float r_1, float r_2) const;

		BokehType bkhtype_;
		BkhBiasType bkhbias_;
		Vec3f dof_up_, dof_rt_;
		float aperture_;
		float focal_distance_, dof_distance_;
		float fdist_;
		float a_pix_;
		std::vector<float> ls_;
};

} //namespace yafaray

#endif // YAFARAY_CAMERA_PERSPECTIVE_H
