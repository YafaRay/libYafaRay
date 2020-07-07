
#ifndef Y_EQUIRECTANGULARCAMERA_H
#define Y_EQUIRECTANGULARCAMERA_H

#include <yafray_constants.h>
#include <core_api/camera.h>

__BEGIN_YAFRAY

class paraMap_t;
class renderEnvironment_t;


class equirectangularCam_t: public camera_t
{
	public:
		equirectangularCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
                     int _resx, int _resy, float aspect,
                     float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
        virtual void setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz);
		virtual ray_t shootRay(float px, float py, float lu, float lv, float &wt) const;
		virtual point3d_t screenproject(const point3d_t &p) const;
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		float aspect;
};


__END_YAFRAY

#endif // Y_EQUIRECTANGULARCAMERA_H
