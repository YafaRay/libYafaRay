#include <yafray_constants.h>

#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/surface.h>
#include <core_api/texture.h>
#include <core_api/environment.h>
#include <core_api/params.h>
#include <utilities/mcqmc.h>

BEGIN_YAFRAY

struct RenderState;
struct PSample;

class SkyVolume : public VolumeRegion
{
	public:

		SkyVolume(Rgb sa, Rgb ss, Rgb le, Point3 pmin, Point3 pmax)
		{
			b_box_ = Bound(pmin, pmax);
			s_a_ = Rgb(0.f);
			s_ray_ = sa;
			s_ray_.b_ /= 3.f;
			s_mie_ = ss;
			s_s_ = Rgb(0.f);
			l_e_ = le;
			g_ = 0.f;
			Y_VERBOSE << "SkyVolume: Vol. [" << s_ray_ << ", " << s_mie_ << ", " << l_e_ << "]" << YENDL;
		}

		virtual float p(const Vec3 &w_l, const Vec3 &w_s);

		float phaseRayleigh(const Vec3 &w_l, const Vec3 &w_s);
		float phaseMie(const Vec3 &w_l, const Vec3 &w_s);

		virtual Rgb sigmaA(const Point3 &p, const Vec3 &v);
		virtual Rgb sigmaS(const Point3 &p, const Vec3 &v);
		virtual Rgb emission(const Point3 &p, const Vec3 &v);
		virtual Rgb tau(const Ray &ray, float step, float offset);

		static VolumeRegion *factory(ParamMap &params, RenderEnvironment &render);

	protected:
		Rgb s_ray_;
		Rgb s_mie_;


};

Rgb SkyVolume::sigmaA(const Point3 &p, const Vec3 &v)
{
	return Rgb(0.f);
}

Rgb SkyVolume::sigmaS(const Point3 &p, const Vec3 &v)
{
	//if (bBox.includes(p)) {
	return s_ray_ + s_mie_;
	//}
	//else
	//	return Rgb(0.f);
}

Rgb SkyVolume::tau(const Ray &ray, float step, float offset)
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

	return (s_ray_ + s_mie_) * dist;
}

Rgb SkyVolume::emission(const Point3 &p, const Vec3 &v)
{
	if(b_box_.includes(p))
	{
		return l_e_;
	}
	else
		return Rgb(0.f);
}

float SkyVolume::p(const Vec3 &w_l, const Vec3 &w_s)
{
	return phaseRayleigh(w_l, w_s) + phaseMie(w_l, w_s);
}

float SkyVolume::phaseRayleigh(const Vec3 &w_l, const Vec3 &w_s)
{
	float costheta = (w_l * w_s);
	return 3.f / (16.f * M_PI) * (1.f + costheta * costheta) * s_ray_.energy();
}

float SkyVolume::phaseMie(const Vec3 &w_l, const Vec3 &w_s)
{
	float k = 1.55f * g_ - .55f * g_ * g_ * g_;
	float kcostheta = k * (w_l * w_s);
	return 1.f / (4.f * M_PI) * (1.f - k * k) / ((1.f - kcostheta) * (1.f - kcostheta)) * s_mie_.energy();
}


VolumeRegion *SkyVolume::factory(ParamMap &params, RenderEnvironment &render)
{
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

	SkyVolume *vol = new SkyVolume(Rgb(sa), Rgb(ss), Rgb(le),
								   Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]));
	return vol;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("SkyVolume", SkyVolume::factory);
	}
}

END_YAFRAY
