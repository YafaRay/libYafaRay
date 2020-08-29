#include <core_api/logging.h>
#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/volume.h>
#include <core_api/surface.h>
#include <core_api/environment.h>
#include <core_api/params.h>

BEGIN_YAFRAY

struct RenderState;
struct PSample;

class UniformVolume : public VolumeRegion
{
	public:

		UniformVolume(Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax, int attgrid_scale) :
			VolumeRegion(sa, ss, le, gg, pmin, pmax, attgrid_scale)
		{
			Y_VERBOSE << "UniformVolume: Vol.[" << s_a_ << ", " << s_s_ << ", " << l_e_ << ", " << pmin << ", " << pmax << ", " << attgrid_scale << "]" << YENDL;
		}

		virtual Rgb sigmaA(const Point3 &p, const Vec3 &v);
		virtual Rgb sigmaS(const Point3 &p, const Vec3 &v);
		virtual Rgb emission(const Point3 &p, const Vec3 &v);
		virtual Rgb tau(const Ray &ray, float step, float offset);

		static VolumeRegion *factory(ParamMap &params, RenderEnvironment &render);
};

Rgb UniformVolume::sigmaA(const Point3 &p, const Vec3 &v)
{
	if(!have_s_a_) return Rgb(0.f);
	if(b_box_.includes(p))
	{
		return s_a_;
	}
	else
		return Rgb(0.f);

}

Rgb UniformVolume::sigmaS(const Point3 &p, const Vec3 &v)
{
	if(!have_s_s_) return Rgb(0.f);
	if(b_box_.includes(p))
	{
		return s_s_;
	}
	else
		return Rgb(0.f);
}

Rgb UniformVolume::tau(const Ray &ray, float step, float offset)
{
	float t_0 = -1, t_1 = -1;

	// ray doesn't hit the BB
	if(!intersect(ray, t_0, t_1))
	{
		return Rgb(0.f);
	}

	if(ray.tmax_ < t_0 && !(ray.tmax_ < 0)) return Rgb(0.f);

	if(ray.tmax_ < t_1 && !(ray.tmax_ < 0)) t_1 = ray.tmax_;

	// t0 < 0 means, ray.from is in the volume
	if(t_0 < 0.f) t_0 = 0.f;

	// distance travelled in the volume
	float dist = t_1 - t_0;

	return dist * (s_s_ + s_a_);
}

Rgb UniformVolume::emission(const Point3 &p, const Vec3 &v)
{
	if(!have_l_e_) return Rgb(0.f);
	if(b_box_.includes(p))
	{
		return l_e_;
	}
	else
		return Rgb(0.f);
}

VolumeRegion *UniformVolume::factory(ParamMap &params, RenderEnvironment &render)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	int att_sc = 5;

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
	params.getParam("attgridScale", att_sc);

	UniformVolume *vol = new UniformVolume(Rgb(sa), Rgb(ss), Rgb(le), g,
										   Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]), att_sc);
	return vol;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("UniformVolume", UniformVolume::factory);
	}
}

END_YAFRAY
