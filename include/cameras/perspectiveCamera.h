#pragma once

#ifndef YAFARAY_PERSPECTIVECAMERA_H
#define YAFARAY_PERSPECTIVECAMERA_H

#include <yafray_constants.h>
#include <core_api/camera.h>
#include <vector>

BEGIN_YAFRAY

class ParamMap;
class RenderEnvironment;

class PerspectiveCamera: public Camera
{
	public:
		enum BokehType {BkDisk1, BkDisk2, BkTri = 3, BkSqr, BkPenta, BkHexa, BkRing};
		enum BkhBiasType {BbNone, BbCenter, BbEdge};
		PerspectiveCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
						  int resx, int resy, float aspect = 1,
						  float df = 1, float ap = 0, float dofd = 0, BokehType bt = BkDisk1, BkhBiasType bbt = BbNone, float bro = 0,
						  float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
		virtual ~PerspectiveCamera();
		virtual void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz);
		virtual Ray shootRay(float px, float py, float lu, float lv, float &wt) const;
		virtual bool sampleLense() const;
		virtual Point3 screenproject(const Point3 &p) const;

		virtual bool project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const;

		static Camera *factory(ParamMap &params, RenderEnvironment &render);
	protected:
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

END_YAFRAY

#endif // YAFARAY_PERSPECTIVECAMERA_H
