#pragma once

#ifndef YAFARAY_ARCHITECTCAMERA_H
#define YAFARAY_ARCHITECTCAMERA_H

#include <yafray_constants.h>
#include <cameras/perspectiveCamera.h>

BEGIN_YAFRAY

class ParamMap;
class RenderEnvironment;

class ArchitectCamera final : public PerspectiveCamera
{
	public:
		static Camera *factory(ParamMap &params, RenderEnvironment &render);

	private:
		ArchitectCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
						int resx, int resy, float aspect = 1,
						float df = 1, float ap = 0, float dofd = 0, BokehType bt = BkDisk1, BkhBiasType bbt = BbNone, float bro = 0,
						float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
		virtual void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz) override;
		virtual Point3 screenproject(const Point3 &p) const override;
};

END_YAFRAY

#endif // YAFARAY_ARCHITECTCAMERA_H
