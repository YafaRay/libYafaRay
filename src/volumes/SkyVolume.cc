#include <yafray_config.h>

#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/surface.h>
#include <core_api/texture.h>
#include <core_api/environment.h>
//#include <textures/noise.h>
//#include <textures/basictex.h>
#include <utilities/mcqmc.h>


#include <cmath>

__BEGIN_YAFRAY

class renderState_t;
class pSample_t;

class SkyVolume : public VolumeRegion {
	public:
	
		SkyVolume(color_t sa, color_t ss, color_t le, point3d_t pmin, point3d_t pmax) {
			bBox = bound_t(pmin, pmax);
			s_a = color_t(0.f);
			s_ray = sa;
			s_ray.B /= 3.f;
			s_mie = ss;
			s_s = color_t(0.f);
			l_e = le;
			g = 0.f;
			std::cout << "SkyVolume vol: " << s_ray << " " << s_mie << " " << l_e << std::endl;
		}
		
		virtual float p(const vector3d_t &w_l, const vector3d_t &w_s);

		float phaseRayleigh(const vector3d_t &w_l, const vector3d_t &w_s);
		float phaseMie(const vector3d_t &w_l, const vector3d_t &w_s);

		virtual color_t sigma_a(const point3d_t &p, const vector3d_t &v);
		virtual color_t sigma_s(const point3d_t &p, const vector3d_t &v);
		virtual color_t emission(const point3d_t &p, const vector3d_t &v);
		virtual color_t tau(const ray_t &ray, float step, float offset);
		
		static VolumeRegion* factory(paraMap_t &params, renderEnvironment_t &render);
	
	protected:
		color_t s_ray;
		color_t s_mie;
		

};

color_t SkyVolume::sigma_a(const point3d_t &p, const vector3d_t &v) {
	return color_t(0.f);
}

color_t SkyVolume::sigma_s(const point3d_t &p, const vector3d_t &v) {
	//if (bBox.includes(p)) {
		return s_ray + s_mie;
	//}
	//else
	//	return color_t(0.f);
}

color_t SkyVolume::tau(const ray_t &ray, float step, float offset) {
	float t0 = -1, t1 = -1;
	
	// ray doesn't hit the BB
	if (!intersect(ray, t0, t1)) {
		//std::cout << "no hit of BB " << t0 << " " << t1 << std::endl;
		return color_t(0.f);
	}
	
	//std::cout << " ray.from: " << ray.from << " ray.dir: " << ray.dir << " ray.tmax: " << ray.tmax << " t0: " << t0 << " t1: " << t1 << std::endl;
	
	if (ray.tmax < t0 && ! (ray.tmax < 0)) return color_t(0.f);
	
	if (ray.tmax < t1 && ! (ray.tmax < 0)) t1 = ray.tmax;
	
	// t0 < 0 means, ray.from is in the volume
	if (t0 < 0.f) t0 = 0.f;
	
	// distance travelled in the volume
	float dist = t1 - t0;
	
	//std::cout << "dist " << dist << " t0: " << t0 << " t1: " << t1 << std::endl;
	
	return (s_ray + s_mie) * dist;
}

color_t SkyVolume::emission(const point3d_t &p, const vector3d_t &v) {
	if (bBox.includes(p)) {
		return l_e;
	}
	else
		return color_t(0.f);
}

float SkyVolume::p(const vector3d_t &w_l, const vector3d_t &w_s) {
	return phaseRayleigh(w_l, w_s) + phaseMie(w_l, w_s);
}

float SkyVolume::phaseRayleigh(const vector3d_t &w_l, const vector3d_t &w_s) {
	float costheta = (w_l * w_s);
	return 3.f / (16.f * M_PI) * (1.f + costheta * costheta) * s_ray.energy();
}

float SkyVolume::phaseMie(const vector3d_t &w_l, const vector3d_t &w_s) {
	float k = 1.55f * g - .55f * g * g * g;
	float kcostheta = k * (w_l * w_s);
	return 1.f / (4.f * M_PI) * (1.f - k * k) / ((1.f - kcostheta) * (1.f - kcostheta)) * s_mie.energy();
}
	

VolumeRegion* SkyVolume::factory(paraMap_t &params,renderEnvironment_t &render) {
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	params.getParam("sigma_s", ss);
	params.getParam("sigma_a", sa);
	params.getParam("l_e", le);
	params.getParam("g", g);
	params.getParam("minX", min[0]);
	params.getParam("minY", min[1]);
	params.getParam("minZ", min[2]);
	params.getParam("maxX", max[0]);
	params.getParam("maxY", max[1]);
	params.getParam("maxZ", max[2]);
	
	SkyVolume *vol = new SkyVolume(color_t(sa), color_t(ss), color_t(le),
						point3d_t(min[0], min[1], min[2]), point3d_t(max[0], max[1], max[2]));
	return vol;
}

extern "C"
{	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("SkyVolume", SkyVolume::factory);
	}
}

__END_YAFRAY
