#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/volume.h>
#include <core_api/surface.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY

class renderState_t;
class pSample_t;

class UniformVolume : public VolumeRegion {
	public:
	
		UniformVolume(color_t sa, color_t ss, color_t le, float gg, point3d_t pmin, point3d_t pmax, int attgridScale) :
			VolumeRegion(sa, ss, le, gg, pmin, pmax, attgridScale) {
			std::cout << "uniform vol: " << s_a << " " << s_s << " " << l_e << " " << pmin << " " << pmax << " " << attgridScale << std::endl;
		}
	
		virtual color_t sigma_a(const point3d_t &p, const vector3d_t &v);
		virtual color_t sigma_s(const point3d_t &p, const vector3d_t &v);
		virtual color_t emission(const point3d_t &p, const vector3d_t &v);
		virtual color_t tau(const ray_t &ray, float step, float offset);
				
		static VolumeRegion* factory(paraMap_t &params, renderEnvironment_t &render);
};

color_t UniformVolume::sigma_a(const point3d_t &p, const vector3d_t &v) {
	if (!haveS_a) return color_t(0.f);
	if (bBox.includes(p)) {
		return s_a;
	}
	else
		return color_t(0.f);

}

color_t UniformVolume::sigma_s(const point3d_t &p, const vector3d_t &v) {
	if (!haveS_s) return color_t(0.f);
	if (bBox.includes(p)) {
		return s_s;
	}
	else
		return color_t(0.f);
}

color_t UniformVolume::tau(const ray_t &ray, float step, float offset) {
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
	
	return dist * (s_s + s_a);
}

color_t UniformVolume::emission(const point3d_t &p, const vector3d_t &v) {
	if (!haveL_e) return color_t(0.f);
	if (bBox.includes(p)) {
		return l_e;
	}
	else
		return color_t(0.f);
}

VolumeRegion* UniformVolume::factory(paraMap_t &params,renderEnvironment_t &render)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	int attSc = 5;
	
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
	params.getParam("attgridScale", attSc);

	UniformVolume *vol = new UniformVolume(color_t(sa), color_t(ss), color_t(le), g,
								point3d_t(min[0], min[1], min[2]), point3d_t(max[0], max[1], max[2]), attSc);
	return vol;
}

extern "C"
{	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("UniformVolume", UniformVolume::factory);
	}
}

__END_YAFRAY
