
#ifndef Y_ANGULARCAMERA_H
#define Y_ANGULARCAMERA_H

#include <yafray_config.h>

#include <core_api/camera.h>

__BEGIN_YAFRAY

class paraMap_t;
class renderEnvironment_t;


class angularCam_t: public camera_t
{
	public:
		angularCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
                     int _resx, int _resy, float aspect, float angle, bool circ, bool orthographic,
                     float const near_clip_distance = 0.0f, float const far_clip_distance = 1e6f);
        virtual void setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz);
		virtual ray_t shootRay(float px, float py, float lu, float lv, float &wt) const;
		virtual point3d_t screenproject(const point3d_t &p) const;
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		float aspect,hor_phi, max_r;
		bool circular;
		bool orthographic; //Orthographic projection where the centre of the image is enlarged/more defined at the cost of much more distorted edges
};


__END_YAFRAY

#endif // Y_ANGULARCAMERA_H
