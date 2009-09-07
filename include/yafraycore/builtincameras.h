
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
			PFLOAT df=1, PFLOAT ap=0, PFLOAT dofd=0, bokehType bt=BK_DISK1, bkhBiasType bbt=BB_NONE,
			PFLOAT bro=0);
		virtual ~perspectiveCam_t();
		virtual int resX() const { return resx; }
		virtual int resY() const { return resy; }
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float lu, float lv, PFLOAT &wt) const;
		virtual bool project(const ray_t &wo, PFLOAT lu, PFLOAT lv, PFLOAT &u, PFLOAT &v, float &pdf) const;
		virtual bool sampleLense() const;
		PFLOAT getFocal() const { return focal_distance; }
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		void biasDist(PFLOAT &r) const;
		void sampleTSD(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const;
		void getLensUV(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const;
		
		int resx, resy;
		point3d_t eye; //!< camera position 
		PFLOAT focal_distance, dof_distance;
		PFLOAT aspect_ratio; //<! aspect ratio of camera (not image in pixel units!)
		vector3d_t vto, vup, vright, dof_up, dof_rt;
		vector3d_t camX, camY, camZ; // camera coordinate system
		PFLOAT fdist, aperture;
		PFLOAT A_pix;
		bokehType bkhtype;
		bkhBiasType bkhbias;
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
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
};

class YAFRAYCORE_EXPORT orthoCam_t: public camera_t
{
	public:
		orthoCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
				   int _resx, int _resy, PFLOAT aspect, PFLOAT scale);
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float lu, float lv, PFLOAT &wt) const;
		virtual bool sampleLense() const;
		virtual int resX() const { return resx; }
		virtual int resY() const { return resy; }
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		int resx, resy;
		point3d_t position;
		vector3d_t vto, vup, vright;
};

class YAFRAYCORE_EXPORT angularCam_t: public camera_t
{
	public:
		angularCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
				   int _resx, int _resy, PFLOAT aspect, PFLOAT angle, bool circ);
		virtual ray_t shootRay(PFLOAT px, PFLOAT py, float lu, float lv, PFLOAT &wt) const;
		virtual bool sampleLense() const;
		virtual int resX() const { return resx; }
		virtual int resY() const { return resy; }
		
		static camera_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		int resx, resy;
		point3d_t position;
		vector3d_t vto, vup, vright;
		PFLOAT aspect, hor_phi, max_r;
		bool circular;
};


__END_YAFRAY

#endif // Y_BUILTINCAMERAS_H
