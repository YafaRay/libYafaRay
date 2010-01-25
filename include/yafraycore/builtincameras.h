
#ifndef Y_BUILTINCAMERAS_H
#define Y_BUILTINCAMERAS_H

#include <yafray_config.h>

#include <vector>
#include <core_api/camera.h>

__BEGIN_YAFRAY

class paraMap_t;
class renderEnvironment_t;

class YAFRAYCORE_EXPORT perspectiveCam_t: public camera_t
{
	public:
		enum bokehType {BK_DISK1, BK_DISK2, BK_TRI=3, BK_SQR, BK_PENTA, BK_HEXA, BK_RING};
		enum bkhBiasType {BB_NONE, BB_CENTER, BB_EDGE};
		perspectiveCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
			int _resx, int _resy, PFLOAT aspect=1,
			PFLOAT df=1, PFLOAT ap=0, PFLOAT dofd=0, bokehType bt=BK_DISK1, bkhBiasType bbt=BB_NONE, PFLOAT bro=0);
		virtual ~perspectiveCam_t();
		virtual void setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz);
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float lu, float lv, PFLOAT &wt) const;
		virtual bool sampleLense() const;
		virtual point3d_t screenproject(const point3d_t &p) const;
		
		virtual bool project(const ray_t &wo, PFLOAT lu, PFLOAT lv, PFLOAT &u, PFLOAT &v, float &pdf) const;
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		void biasDist(PFLOAT &r) const;
		void sampleTSD(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const;
		void getLensUV(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const;
		
		bokehType bkhtype;
		bkhBiasType bkhbias;
		mutable vector3d_t dof_up, dof_rt;
		PFLOAT aperture;
		PFLOAT focal_distance, dof_distance;
		PFLOAT fdist;
		PFLOAT A_pix;
		std::vector<PFLOAT> LS;
};

class YAFRAYCORE_EXPORT architectCam_t: public perspectiveCam_t
{
	public:
		architectCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
			int _resx, int _resy, PFLOAT aspect=1,
			PFLOAT df=1, PFLOAT ap=0, PFLOAT dofd=0, bokehType bt=BK_DISK1, bkhBiasType bbt=BB_NONE,
			PFLOAT bro=0);
		virtual ~architectCam_t();
		virtual void setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz);
		virtual point3d_t screenproject(const point3d_t &p) const;

		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
};

class YAFRAYCORE_EXPORT orthoCam_t: public camera_t
{
	public:
		orthoCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
				   int _resx, int _resy, PFLOAT aspect, PFLOAT scale);
		virtual void setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz);
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float lu, float lv, PFLOAT &wt) const;
		virtual point3d_t screenproject(const point3d_t &p) const;
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		PFLOAT scale;
		mutable point3d_t pos;
};

class YAFRAYCORE_EXPORT angularCam_t: public camera_t
{
	public:
		angularCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
				   int _resx, int _resy, PFLOAT aspect, PFLOAT angle, bool circ);
		virtual void setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz);
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float lu, float lv, PFLOAT &wt) const;
		virtual point3d_t screenproject(const point3d_t &p) const;
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		PFLOAT aspect,hor_phi, max_r;
		bool circular;
};


__END_YAFRAY

#endif // Y_BUILTINCAMERAS_H
